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

#include <kernel/vmm.h>
#include <kernel/kernel.h>
#include <kernel/interrupt.h>

#include <assert.h>


/*
    Virtual Memory Map

    x86

    0x00000000 - 0xBFFFFFFF     User space
    0xC0000000 - 0xC0100000     Low Memory (ISA IO SPACE, BIOS)
    0xC1000000 - 0xC1400000     Kiznix Kernel

    0xE0000000 - 0xEFFFFFFF     Heap space (vmm_alloc)

    0xFF000000 - 0xFF7FFFFF     Free memory pages stack (8 MB)

    Non-PAE:
    0xFFC00000 - 0xFFFFEFFF     Page Mapping Level 1 (Page Tables)
    0xFFFFF000 - 0xFFFFFFFF     Page Mapping Level 2 (Page Directory)

    PAE:
    0xFF800000 - 0xFFFFFFFF     Page Mappings


    x86_64

    0x00000000 00000000 - 0x00007FFF FFFFFFFF   User space

    0xFFFF8000 00000000 - 0xFFFEFFFF FFFFFFFF   Unused kernel space

    0xFFFFF000 00000000 - 0xFFFFF07F FFFFFFFF   Free memory pages stack (512 GB)

    0xFFFFFF00 00000000 - 0xFFFFFF7F FFFFFFFF   Page Mapping Level 1 (Page Tables)
    0xFFFFFF7F 80000000 - 0xFFFFFF7F BFFFFFFF   Page Mapping Level 2 (Page Directories)
    0xFFFFFF7F BFC00000 - 0xFFFFFF7F BFDFFFFF   Page Mapping Level 3 (PDPTs / Page-Directory-Pointer Tables)
    0xFFFFFF7F BFDFE000 - 0xFFFFFF7F BFDFEFFF   Page Mapping Level 4 (PML4)

    0xFFFFFFFF C0000000 - 0xFFFFFFFF C0100000   Low Memory (ISA IO Space, BIOS, VGA, ...)
    0xFFFFFFFF C0100000 - 0xFFFFFFFF C0140000   Kiznix Kernel
    0xFFFFFFFF E0000000 - 0xFFFFFFFF EFFFFFFF   Heap space (vmm_alloc)
*/

/*
    Intel Page Mapping Overview

    Pages are 4 KB (12 bits per page table entry)

    Page Table Level    x86         x86 PAE     x86_64          Intel Name
    ---------------------------------------------------------------------------------------------------
            4            -             -        9 bits          Page Mapping Level 4
            3            -           2 bits     9 bits          Page Directory Pointer Table
            2           10 bits      9 bits     9 bits          Page Directory
            1           10 bits      9 bits     9 bits          Page Table
         (page)         12 bits     12 bits    12 bits          Page Table Entries
    ---------------------------------------------------------------------------------------------------
                        32 bits     32 bits    48 bits
                         4 GB        64 GB      256 TB          Addressable Physical Memory
*/




/*
 * Volatile isn't enough to prevent the compiler from reordering the
 * read/write functions for the control registers and messing everything up.
 * A memory clobber would solve the problem, but would prevent reordering of
 * all loads stores around it, which can hurt performance. Solution is to
 * use a variable and mimic reads and writes to it to enforce serialization
 */
extern unsigned long __force_order;



static inline uintptr_t x86_get_cr3()
{
    uintptr_t physicalAddress;
    asm ("mov %%cr3, %0" : "=r"(physicalAddress), "=m" (__force_order));
    return physicalAddress;
}



static inline void x86_set_cr3(uintptr_t physicalAddress)
{
    asm volatile ("mov %0, %%cr3" : : "r"(physicalAddress), "m" (__force_order));
}



static inline void vmm_invalidate(void* virtualAddress)
{
    asm volatile ("invlpg (%0)" : : "r"(virtualAddress) : "memory");
}



static int vmm_page_fault_handler(interrupt_context_t* context);



#if defined(KIZNIX_PAE)

/*
    3 levels, 2/9/9 bits

    PML3: Remains at _BootPDPT     - 0x4 entries (2 bits), shift = (32 - 2) = 30
    PML2: 0xFFFFC000 to 0xFFFFFFFF - 0x800 entries (11 bits), shift = (32 - 11) = 21
    PML1: 0xFF800000 to 0xFFFFBFFF - 0x100000 entries (20 bits), shift = (32 - 20) = 12

    long i3 = (address >> 30) & 0x3;
    long i2 = (address >> 21) & 0x7FF;
    long i1 = (address >> 12) & 0xFFFFF;
*/

// Page Mapping Tables at boot time
extern physaddr_t _BootPDPT[4];
extern physaddr_t _BootPageDirectory[512];
extern physaddr_t _BootPageTables[1024];

// Page Mapping Tables at their final location in virtual memory.
static physaddr_t* const vmm_page_mappings_3 = (physaddr_t*)_BootPDPT;
static physaddr_t* const vmm_page_mappings_2 = (physaddr_t*)0xFFFFC000;
static physaddr_t* const vmm_page_mappings_1 = (physaddr_t*)0xFF800000;

static uintptr_t page_table_start = 0xFF800000;
static uintptr_t page_table_end   = 0xFFFFFFFF;



void vmm_init()
{
    uintptr_t addr_root_page_table = x86_get_cr3();
    uintptr_t addr_page_directory = ((uintptr_t)_BootPageDirectory - KERNEL_VIRTUAL_BASE);

    // Kernel space (high memory) pointers to the page mapping tables
    physaddr_t* pm3 = (physaddr_t*)&_BootPDPT;
    physaddr_t* pm2 = (physaddr_t*)&_BootPageDirectory;

    // Setup recursive mapping
    // For PAE, we do this at PML2 instead of PML3. This is to save virtual memory space.
    // Recursive mapping at PML3 would consume 1 GB of virtual memory.
    // Doing it at PML2 only uses 8 MB of virtual memory.
    // The is accomplished by mapping the 4 page directories (PML2) in the last 4 entries of
    // the last page directory.
    pm2[511] =  addr_page_directory | PAGE_WRITE | PAGE_PRESENT;

    // Undo identity mapping
    pm3[0] = 0;

    // Reload page tables
    x86_set_cr3(addr_root_page_table);

    // We don't need the boot page tables mapped in their old location anymore
    // Can't unmap _BootPDPT since it might contain needed .bss data following the PDPT
    vmm_unmap_page(_BootPageTables);
    vmm_unmap_page(_BootPageDirectory);
    vmm_unmap_page(_BootPageTables + 512);

    // Page fault handler
    interrupt_register(14, vmm_page_fault_handler);
}



int vmm_map_page(physaddr_t physicalAddress, void* virtualAddress)
{
    //printf("vmm_map_page(%p, %p)\n", physicalAddress, virtualAddress);

    uintptr_t addr = (uintptr_t)virtualAddress;

    const long i3 = (addr >> 30) & 0x3;
    const long i2 = (addr >> 21) & 0x7FF;
    const long i1 = (addr >> 12) & 0xFFFFF;

    if (!(vmm_page_mappings_3[i3] & PAGE_PRESENT))
    {
        //todo: must handle out of memory - everywhere we call pmm_alloc_page()!
        physaddr_t page = pmm_alloc_page();
        vmm_page_mappings_3[i3] = page | PAGE_PRESENT;
        void* p = (void*)((uintptr_t)vmm_page_mappings_2) + (i3 << 12);
        vmm_invalidate(p);
        memset(p, 0, PAGE_SIZE);

//TODO: this new page directory needs to be "recurse-mapped" in PD #3 [1FC-1FE]
    }

    if (!(vmm_page_mappings_2[i2] & PAGE_PRESENT))
    {
        physaddr_t page = pmm_alloc_page();
        vmm_page_mappings_2[i2] = page | PAGE_WRITE | PAGE_PRESENT;
        void* p = (void*)((uintptr_t)vmm_page_mappings_1) + (i2 << 12);
        vmm_invalidate(p);
        memset(p, 0, PAGE_SIZE);
    }

    //todo: this should just be an assert
    if (vmm_page_mappings_1[i1] & PAGE_PRESENT)
    {
        fatal("vmm_map_page() - there is already something there!");
    }

    vmm_page_mappings_1[i1] = physicalAddress | PAGE_WRITE | PAGE_PRESENT;
    vmm_invalidate(virtualAddress);

    return 0;
}



void vmm_unmap_page(void* virtualAddress)
{
    uintptr_t addr = (uintptr_t)virtualAddress;
    vmm_page_mappings_1[(addr >> 12) & 0xFFFFF] = 0;
}



#elif defined(__i386__)

/*
    2 levels, 10 bits each

    PML2: 0xFFFFF000 to 0xFFFFFFFF - 0x400 entries (10 bits), shift = (32 - 10) = 22
    PML1: 0xFFC00000 to 0xFFFFEFFF - 0x100000 entries (20 bits), shift = (32 - 20) = 12

    long i2 = (address >> 22) & 0x3FF;
    long i1 = (address >> 12) & 0xFFFFF;
*/

// Page Mapping Tables at boot time
extern physaddr_t _BootPageDirectory[1024];
extern physaddr_t _BootPageTable[1024];

// Page Mapping Tables at their final location in virtual memory.
static physaddr_t* const vmm_page_mappings_2 = (physaddr_t*)0xFFFFF000;
static physaddr_t* const vmm_page_mappings_1 = (physaddr_t*)0xFFC00000;

static uintptr_t page_table_start = 0xFFC00000;
static uintptr_t page_table_end   = 0xFFFFFFFF;



void vmm_init()
{
    uintptr_t addr_root_page_table = x86_get_cr3();

    // Kernel space (high memory) pointers to the page mapping tables
    physaddr_t* pm2 = (physaddr_t*)&_BootPageDirectory;

    // Setup recursive mapping
    pm2[1023] =  addr_root_page_table | PAGE_WRITE | PAGE_PRESENT;

    // Undo identity mapping
    pm2[0] = 0;

    // Reload page tables
    x86_set_cr3(addr_root_page_table);

    // We don't need the boot page tables mapped in their old location anymore
    vmm_unmap_page(_BootPageDirectory);
    vmm_unmap_page(_BootPageTable);

    // Page fault handler
    interrupt_register(14, vmm_page_fault_handler);
}



int vmm_map_page(physaddr_t physicalAddress, void* virtualAddress)
{
    //printf("vmm_map_page(%p, %p)\n", physicalAddress, virtualAddress);

    uintptr_t addr = (uintptr_t)virtualAddress;

    const int i2 = (addr >> 22) & 0x3FF;
    const int i1 = (addr >> 12) & 0xFFFFF;

    if (!(vmm_page_mappings_2[i2] & PAGE_PRESENT))
    {
        physaddr_t page = pmm_alloc_page();
        vmm_page_mappings_2[i2] = page | PAGE_WRITE | PAGE_PRESENT;
        void* p = (void*)((uintptr_t)vmm_page_mappings_1) + (i2 << 12);
        vmm_invalidate(p);
        memset(p, 0, PAGE_SIZE);
    }

    //todo: this should just be an assert
    if (vmm_page_mappings_1[i1] & PAGE_PRESENT)
    {
        fatal("vmm_map_page() - there is already something there!");
    }

    vmm_page_mappings_1[i1] = physicalAddress | PAGE_WRITE | PAGE_PRESENT;
    vmm_invalidate(virtualAddress);

    return 0;
}



void vmm_unmap_page(void* virtualAddress)
{
    uintptr_t addr = (uintptr_t)virtualAddress;
    const int i1 = (addr >> 12) & 0xFFFFF;
    vmm_page_mappings_1[i1] = 0;
    vmm_invalidate(virtualAddress);
}


#elif defined(__x86_64__)

/*
    4 levels, 9 bits each

    PML4: 0xFFFFFF7F BFDFE000 to 0xFFFFFF7F BFDFEFFF - 0x200 entries (9 bits), shift = (48 - 9) = 39
    PML3: 0xFFFFFF7F BFC00000 to 0xFFFFFF7F BFDFFFFF - 0x40000 entries (18 bits), shift = (48 - 18) = 30
    PML2: 0xFFFFFF7F 80000000 to 0xFFFFFF7F BFFFFFFF - 0x8000000 entries (27 bits), shift = (48 - 27) = 21
    PML1: 0xFFFFFF00 00000000 to 0xFFFFFF7F FFFFFFFF - 0x1000000000 entries (36 bits), shift = (48 - 36) = 12

    long i4 = (address >> 39) & 1FF;
    long i3 = (address >> 30) & 0x3FFFF;
    long i2 = (address >> 21) & 0x7FFFFFF;
    long i1 = (address >> 12) & 0xFFFFFFFFF;
*/

// Page Mapping Tables at boot time
extern physaddr_t _BootPML4[512];
extern physaddr_t _BootPDPT[512];
extern physaddr_t _BootPageDirectory[512];
extern physaddr_t _BootPageTables[1024];

// Page Mapping Tables at their final location in virtual memory.
static physaddr_t* const vmm_page_mappings_4 = (physaddr_t*)0xFFFFFF7FBFDFE000ull;
static physaddr_t* const vmm_page_mappings_3 = (physaddr_t*)0xFFFFFF7FBFC00000ull;
static physaddr_t* const vmm_page_mappings_2 = (physaddr_t*)0xFFFFFF7F80000000ull;
static physaddr_t* const vmm_page_mappings_1 = (physaddr_t*)0xFFFFFF0000000000ull;

static uintptr_t page_table_start = 0xFFFFFF0000000000ull;
static uintptr_t page_table_end   = 0xFFFFFF7FFFFFFFFFull;


static inline void vmm_set_page_entry(uintptr_t address, uintptr_t flags)
{
    assert(IS_PAGE_ALIGNED(address));

#if defined(__i386__)
    long index = (address >> 12) & 0xFFFFF;
#elif defined(__x86_64__)
    long index = (address >> 12) & 0xFFFFFFFFFull;
#endif

    vmm_page_mappings_1[index] = address | flags;
}


void vmm_init()
{
    uintptr_t addr_root_page_table = x86_get_cr3();

    // Kernel space (high memory) pointers to the page mapping tables
    physaddr_t* pm4 = (physaddr_t*)&_BootPML4;
    physaddr_t* pm3 = (physaddr_t*)&_BootPDPT;

    // Setup recursive mapping
    pm4[510] =  addr_root_page_table | PAGE_WRITE | PAGE_PRESENT;

    // Undo identity mapping
    pm3[0] = 0;
    pm4[0] = 0;

    // Reload page tables
    x86_set_cr3(addr_root_page_table);

    // We don't need the boot page tables mapped in their old location anymore
    vmm_unmap_page(_BootPML4);
    vmm_unmap_page(_BootPDPT);
    vmm_unmap_page(_BootPageDirectory);
    vmm_unmap_page(_BootPageTables);
    vmm_unmap_page(_BootPageTables + 512);

    // Page fault handler
    interrupt_register(14, vmm_page_fault_handler);
}



int vmm_map_page(physaddr_t physicalAddress, void* virtualAddress)
{
    //printf("vmm_map_page(%p, %p)\n", physicalAddress, virtualAddress);

    uintptr_t addr = (uintptr_t)virtualAddress;

    const long i4 = (addr >> 39) & 0x1FF;
    const long i3 = (addr >> 30) & 0x3FFFF;
    const long i2 = (addr >> 21) & 0x7FFFFFF;
    const long i1 = (addr >> 12) & 0xFFFFFFFFFul;

    if (!(vmm_page_mappings_4[i4] & PAGE_PRESENT))
    {
        physaddr_t page = pmm_alloc_page();
        vmm_page_mappings_4[i4] = page | PAGE_WRITE | PAGE_PRESENT;
        void* p = (void*)((uintptr_t)vmm_page_mappings_3) + (i4 << 12);
        vmm_invalidate(p);
        memset(p, 0, PAGE_SIZE);
    }

    if (!(vmm_page_mappings_3[i3] & PAGE_PRESENT))
    {
        physaddr_t page = pmm_alloc_page();
        vmm_page_mappings_3[i3] = page | PAGE_WRITE | PAGE_PRESENT;
        void* p = (void*)((uintptr_t)vmm_page_mappings_2) + (i3 << 12);
        vmm_invalidate(p);
        memset(p, 0, PAGE_SIZE);
    }

    if (!(vmm_page_mappings_2[i2] & PAGE_PRESENT))
    {
        physaddr_t page = pmm_alloc_page();
        vmm_page_mappings_2[i2] = page | PAGE_WRITE | PAGE_PRESENT;
        void* p = (void*)((uintptr_t)vmm_page_mappings_1) + (i2 << 12);
        vmm_invalidate(p);
        memset(p, 0, PAGE_SIZE);
    }

    //todo: this should just be an assert
    if (vmm_page_mappings_1[i1] & PAGE_PRESENT)
    {
        fatal("vmm_map_page() - there is already something there!");
    }

    vmm_page_mappings_1[i1] = physicalAddress | PAGE_WRITE | PAGE_PRESENT;
    vmm_invalidate(virtualAddress);

    return 0;
}



void vmm_unmap_page(void* virtualAddress)
{
    uintptr_t addr = (uintptr_t)virtualAddress;
    vmm_page_mappings_1[(addr >> 12) & 0xFFFFFFFFFull] = 0;
}


#endif



//todo: total hack for now, implement proper virtual space allocation
#if defined(__i386__)
#define KERNEL_HEAP_BEGIN   0xE0000000
#define KERNEL_HEAP_END     0xF0000000
#elif defined(__x86_64__)
#define KERNEL_HEAP_BEGIN   0xFFFFFFFFE0000000ull
#define KERNEL_HEAP_END     0xFFFFFFFFF0000000ull
#endif

static uintptr_t next_alloc = KERNEL_HEAP_BEGIN;


void* vmm_alloc(size_t length)
{
    if (length == 0)
    {
        return NULL;
    }

    uintptr_t begin = (uintptr_t)next_alloc;
    uintptr_t end = begin + PAGE_ALIGN_UP(length);

    for (uintptr_t p = begin; p != end; p += PAGE_SIZE)
    {
#if defined(__i386__)
        long i1 = (next_alloc >> 12) & 0xFFFFF;
#elif defined(__x86_64__)
        long i1 = (next_alloc >> 12) & 0xFFFFFFFFF;
#endif

        vmm_page_mappings_1[i1] = PAGE_ALLOCATED;
        vmm_invalidate((void*)next_alloc);

        next_alloc += PAGE_SIZE;
    }

    //todo: handle offset when 'address' isn't on a page boundary

    return (void*)begin;
}



void* vmm_map(physaddr_t physicalAddress, size_t length)
{
    physaddr_t begin = PAGE_ALIGN_DOWN(physicalAddress);
    physaddr_t end = PAGE_ALIGN_UP(physicalAddress + length);

    void* virtualAddress = vmm_alloc(end - begin);

    if ((uintptr_t)virtualAddress + end - begin > KERNEL_HEAP_END)
    {
        fatal("vmm_map() - out of virtual space");
    }

    //printf("vmm_map(%p, %p)\n", (void*)physicalAddress, (void*)length);

    for (uintptr_t page = begin; page != end; page += PAGE_SIZE)
    {
        //printf("    %p --> %p\n", (void*)(uintptr_t)virtualAddress + (page - begin), (void*)page);
        vmm_map_page(page, (void*)(uintptr_t)virtualAddress + (page - begin));
    }

    physaddr_t offset = physicalAddress - begin;

    return (void*)(uintptr_t)virtualAddress + offset;
}



int vmm_unmap(void* address, size_t length)
{
    uintptr_t begin = PAGE_ALIGN_DOWN((uintptr_t)address);
    uintptr_t end = PAGE_ALIGN_UP((uintptr_t)address + length);

    //printf("vmm_unmap(%p, %x) --> %p, %p\n", address, length, (void*)begin, (void*)(end-1));

    for (uintptr_t p = begin; p != end; p += PAGE_SIZE)
    {
        //printf("    %p\n", (void*)p);
        vmm_unmap_page((void*)p);
    }

    return 0;
}



#define PAGE_FAULT_PRESENT 1
#define PAGE_FAULT_WRITE 2
#define PAGE_FAULT_USER 4
#define PAGE_FAULT_RESERVED 8
#define PAGE_FAULT_INSTRUCTION 16

/*
    US RW  P - Description
    0  0  0 - Supervisory process tried to read a non-present page entry
    0  0  1 - Supervisory process tried to read a page and caused a protection fault
    0  1  0 - Supervisory process tried to write to a non-present page entry
    0  1  1 - Supervisory process tried to write a page and caused a protection fault
    1  0  0 - User process tried to read a non-present page entry
    1  0  1 - User process tried to read a page and caused a protection fault
    1  1  0 - User process tried to write to a non-present page entry
    1  1  1 - User process tried to write a page and caused a protection fault
*/


static int vmm_page_fault_handler(interrupt_context_t* context)
{
    uintptr_t address = context->cr2;
    uintptr_t error = context->error;

    //printf("PAGE FAULT: %p (error: %p)\n", (void*)address, (void*)error);

    address = PAGE_ALIGN_DOWN(address);

#if defined(__i386__)
    long index = (address >> 12) & 0xFFFFF;
#elif defined(__x86_64__)
    long index = (address >> 12) & 0xFFFFFFFFF;
#endif

    physaddr_t* pPageEntry = &vmm_page_mappings_1[index];


    if (error == PAGE_FAULT_WRITE)
    {
        if ((*pPageEntry & PAGE_ALLOCATED) || (address >= page_table_start && address <= page_table_end))
        {
            physaddr_t page = pmm_alloc_page();

            //printf("Creating entry for %p at %p\n", (void*)address, pPageEntry);

            physaddr_t entry = page | PAGE_WRITE | PAGE_PRESENT;

            if (address >= KERNEL_SPACE)
            {
                entry |= PAGE_GLOBAL;
            }

            *pPageEntry = entry;
            vmm_invalidate((void*)address);
            memset((void*)address, 0, PAGE_SIZE);

            return 1;
        }
    }

    fatal("UNHANDLED PAGE FAULT: %p", (void*)address);
}
