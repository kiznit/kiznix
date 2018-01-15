/*
    Copyright (c) 2015, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/kernel.h>


extern const char kernel_image_start[];
extern const char kernel_image_end[];

extern const char _BootStackBottom[];
extern const char _BootStackTop[];

typedef struct MemoryMapEntry MemoryMapEntry;

struct MemoryMapEntry
{
    uint64_t base;
    uint64_t length;
};

extern int _BootMemoryMapSize;
extern MemoryMapEntry _BootMemoryMap[];


/*
    Physical Memory Map

    x86 / x86_64

    0x00000000 - 0x01000000     Low Memory (ISA IO SPACE, BIOS)
    0x01000000 - 0x01400000     Kiznix Kernel
*/

//todo: move inside pmm.c once vmm.c doesn't depend on PMM_STACK_VMA
#if defined(__i386__)
#define PMM_STACK_SIZE 0x400000
#define PMM_STACK_VMA  0xFF000000
#elif defined(__x86_64__)
#define PMM_STACK_SIZE 0x8000000000ull
#define PMM_STACK_VMA  0xFFFFF00000000000ull
#endif

#define MEM_1_GB 0x100000ull
#define MEM_4_GB 0x100000000ull

static uint64_t pmm_system_memory;         // Detected system memory
static uint64_t pmm_free_memory;           // Free memory
static uint64_t pmm_used_memory;           // Used memory
static uint64_t pmm_unavailable_memory;    // Memory that can't be used

static physaddr_t* const pmm_stack_base = (physaddr_t*)PMM_STACK_VMA;
static physaddr_t* pmm_stack_top = (physaddr_t*)PMM_STACK_VMA;

typedef struct FreeMemory FreeMemory;

struct FreeMemory
{
    physaddr_t start;
    physaddr_t end;
};

static FreeMemory s_free_memory[1000];
static int s_free_memory_count;
static int s_free_memory_current;



void pmm_init()
{
    const uint64_t kernel_start = PAGE_ALIGN_DOWN((uintptr_t)kernel_image_start);
    const uint64_t kernel_end = PAGE_ALIGN_UP((uintptr_t)kernel_image_end);

    s_free_memory_count = 0;
    s_free_memory_current = 0;

    for (int i = 0; i != _BootMemoryMapSize; ++i)
    {
        const MemoryMapEntry* entry = &_BootMemoryMap[i];

        pmm_system_memory += (entry->length);

        uint64_t start = PAGE_ALIGN_UP(entry->base);
        uint64_t end = PAGE_ALIGN_DOWN(entry->base + entry->length);

#if defined(__i386__) && !defined(KIZNIX_PAE)
        // In 32 bits mode (non-PAE), we can't address anything above 4 GB
        if (start >= MEM_4_GB)
        {
            pmm_unavailable_memory += end - start;
            continue;
        }

        if (end > MEM_4_GB)
        {
            pmm_unavailable_memory += end - MEM_4_GB;
            end = MEM_4_GB;
        }
#endif

        // Skip first 1 MB (ISA IO SPACE)
        if (start < MEM_1_GB)
            start = MEM_1_GB;

        // Skip kernel
        if (start >=  kernel_start && start < kernel_end)
            start = kernel_end;

        if (end >= kernel_start && end < kernel_end)
            end = kernel_start;

        // If there is nothing left...
        if (start >= end)
            continue;

        s_free_memory[s_free_memory_count].start = start;
        s_free_memory[s_free_memory_count].end = end;
        ++s_free_memory_count;

        pmm_free_memory += (end - start);

        if (s_free_memory_count == 1000)
            break;
    }

    // Calculate how much of the system memory we used so far
    pmm_used_memory = pmm_system_memory - pmm_free_memory - pmm_unavailable_memory;


    printf("System Memory: %08x %08x (%8.2f MB)\n", (unsigned)(pmm_system_memory >> 32), (unsigned)pmm_system_memory, (double)pmm_system_memory / (1024.0f * 1024.0f));
    printf("Used Memory  : %08x %08x (%8.2f MB)\n", (unsigned)(pmm_used_memory >> 32), (unsigned)pmm_used_memory, (double)pmm_used_memory / (1024.0f * 1024.0f));
    printf("Free Memory  : %08x %08x (%8.2f MB)\n", (unsigned)(pmm_free_memory >> 32), (unsigned)pmm_free_memory, (double)pmm_free_memory / (1024.0f * 1024.0f));
    printf("Unavailable  : %08x %08x (%8.2f MB)\n", (unsigned)(pmm_unavailable_memory >> 32), (unsigned)pmm_unavailable_memory, (double)pmm_unavailable_memory / (1024.0f * 1024.0f));
    printf("\n");

    if (pmm_free_memory == 0)
    {
        fatal("No memory available");
    }
}



physaddr_t pmm_alloc_page()
{
    if (pmm_stack_top != pmm_stack_base)
    {
        // Grab a page off the stack
        --pmm_stack_top;

        physaddr_t page = *pmm_stack_top;

        if (IS_PAGE_ALIGNED(pmm_stack_top))
        {
            // This unmap is not stricly required, but not doing it will
            // trigger an assert in vmm_map_page() where we make sure we don't
            // overwrite existing valid entries.
            vmm_unmap_page(pmm_stack_top);
        }

        pmm_free_memory -= PAGE_SIZE;
        return page;
    }

    // Try from s_free_memory_entries
    while (s_free_memory_current != s_free_memory_count)
    {
        FreeMemory* entry = &s_free_memory[s_free_memory_current];
        if (entry->start != entry->end)
        {
            physaddr_t page = entry->start;
            entry->start += PAGE_SIZE;
            pmm_free_memory -= PAGE_SIZE;
            return page;
        }

        ++s_free_memory_current;
    }

    fatal("Out of physical memory");
}



void pmm_free_page(physaddr_t page)
{
    //printf("pmm_free_page(): %p\n", page);

    if (IS_PAGE_ALIGNED(pmm_stack_top))
    {
        // We need a page of physical memory to extend the stack
        // Might as well use the page that is getting freed!
        if (vmm_map_page(page, pmm_stack_top) != 0)
        {
            fatal("Failed to map page");
        }
    }

    *pmm_stack_top++ = page;

    pmm_free_memory += PAGE_SIZE;
}
