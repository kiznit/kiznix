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

#include <stdint.h>
#include <stdio.h>
#include <cpuid.h>

#include "console.hpp"

#include "multiboot.h"
#include "multiboot2.h"


extern "C" void __kiznix_putc(unsigned char c)
{
    console_putchar(c);
}


// Multiboot don't define all the structures we need. We do.

struct multiboot_module
{
    uint32_t mod_start;
    uint32_t mod_end;
    const char* string;
    uint32_t reserved;
};


struct multiboot2_info
{
    uint32_t total_size;
    uint32_t reserved;
};



struct multiboot2_module
{
    multiboot2_header_tag tag;
    uint32_t mod_start;
    uint32_t mod_end;
    char     string[];
};


// i686 = P6 (Pentium Pro)
// Reference:
//  https://en.m.wikipedia.org/wiki/P6_(microarchitecture)
static bool verify_cpu()
{
    unsigned int maxLevel = __get_cpuid_max(0, NULL);
    if (maxLevel == 0)
    {
        printf("Unable to identify CPU (CPUID instruction not supported)\n");
        return false;
    }

    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx) || !(edx & bit_SSE2))
    {
        printf("Streaming SIMD Extensions 2 (SSE2) not detected\n");
        return false;
    }

    return true;
}




static void add_memory(int type, uint64_t address, uint64_t length)
{
    unsigned int s0 = address >> 32;
    unsigned int s1 = address & 0xFFFFFFFF;
    unsigned int e0 = (address + length) >> 32;
    unsigned int e1 = (address + length) & 0xFFFFFFFF;

    double size = length / (1024.0 * 1024.0);

    printf("    %d: %08x%08x - %08x%08x (%.2f MB)\n", type, s0, s1, e0, e1, size);
}



static void process_multiboot_info(multiboot_info const * const mbi)
{
    if (mbi->flags & MULTIBOOT_MEMORY_INFO)
    {
        printf("\nMemory map:\n");

        const multiboot_mmap_entry* entry = (multiboot_mmap_entry*)mbi->mmap_addr;
        const multiboot_mmap_entry* end = (multiboot_mmap_entry*)(mbi->mmap_addr + mbi->mmap_length);

        while (entry < end)
        {
            add_memory(entry->type, entry->addr, entry->len);
            entry = (multiboot_mmap_entry*) ((uintptr_t)entry + entry->size + sizeof(entry->size));
        }
    }

    if (mbi->flags & MULTIBOOT_INFO_MEMORY)
    {
        printf("\nBasic memory info:\n");
        add_memory(MULTIBOOT_MEMORY_AVAILABLE, 0, mbi->mem_lower * 1024);
        add_memory(MULTIBOOT_MEMORY_AVAILABLE, 1024*1024, mbi->mem_upper * 1024);
    }

    if (mbi->flags & MULTIBOOT_INFO_MODS)
    {
        printf("\nModules:\n");

        const multiboot_module* modules = (multiboot_module*)mbi->mods_addr;

        for (uint32_t i = 0; i != mbi->mods_count; ++i)
        {
            const multiboot_module* module = &modules[i];
            printf("    %lu: %08lx - %08lx (%ld bytes) - \"%s\"\n", i, module->mod_start, module->mod_end, module->mod_end - module->mod_start, module->string);
        }
    }
}



static void process_multiboot_info(multiboot2_info const * const mbi)
{
    const multiboot2_tag_basic_meminfo* meminfo = NULL;
    const multiboot2_tag_mmap* mmap = NULL;

    for (multiboot2_tag* tag = (multiboot2_tag*)(mbi + 1);
         tag->type != MULTIBOOT2_TAG_TYPE_END;
         tag = (multiboot2_tag*) (((uintptr_t)tag + tag->size + MULTIBOOT2_TAG_ALIGN - 1) & ~(MULTIBOOT2_TAG_ALIGN - 1)))
    {
        switch (tag->type)
        {
        case MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO:
            meminfo = (multiboot2_tag_basic_meminfo*)tag;
            break;

        case MULTIBOOT2_TAG_TYPE_MMAP:
            mmap = (multiboot2_tag_mmap*)tag;
            break;

        case MULTIBOOT2_TAG_TYPE_MODULE:
            const multiboot2_module* module = (multiboot2_module*)tag;
            printf("    module: %08lx - %08lx (%ld bytes) - \"%s\"\n", module->mod_start, module->mod_end, module->mod_end - module->mod_start, module->string);
            break;
        }
    }

    if (mmap)
    {
        printf("\nMemory map:\n");

        const multiboot2_mmap_entry* entry = mmap->entries;
        const multiboot2_mmap_entry* end = (multiboot2_mmap_entry*) (((uintptr_t)mmap + mmap->size + MULTIBOOT2_TAG_ALIGN - 1) & ~(MULTIBOOT2_TAG_ALIGN - 1));

        while (entry < end)
        {
            add_memory(entry->type, entry->addr, entry->len);
            entry = (multiboot2_mmap_entry*) ((uintptr_t)entry + mmap->entry_size);
        }
    }


    if (meminfo)
    {
        printf("\nBasic memory info:\n");

        add_memory(MULTIBOOT_MEMORY_AVAILABLE, 0, meminfo->mem_lower * 1024);
        add_memory(MULTIBOOT_MEMORY_AVAILABLE, 1024*1024, meminfo->mem_upper * 1024);
    }
}



extern "C" void multiboot_main(unsigned int magic, void* mbi)
{
    console_init();

    printf("Kiznix Multiboot Bootloader\n\n");

    if (!verify_cpu())
    {
        printf("CPU doesn't support required features, aborting\n");
    }
    else if (magic == MULTIBOOT_BOOTLOADER_MAGIC && mbi)
    {
        printf("This is multiboot 1\n");
        process_multiboot_info((multiboot_info*)mbi);
    }
    else if (magic == MULTIBOOT2_BOOTLOADER_MAGIC && mbi)
    {
        printf("This is multiboot 2\n");
        process_multiboot_info((multiboot2_info*)mbi);
    }
    else
    {
        printf("Kiznix boot error: no multiboot information!\n");
    }
}