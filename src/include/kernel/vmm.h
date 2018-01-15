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

#ifndef KIZNIX_INCLUDED_KERNEL_VMM_H
#define KIZNIX_INCLUDED_KERNEL_VMM_H

#include <kernel/pmm.h>

#if defined(__i386__)
#define KERNEL_VIRTUAL_BASE 0xC0000000
#define KERNEL_SPACE        0xC0000000
#elif defined(__x86_64__)
#define KERNEL_VIRTUAL_BASE 0xFFFFFFFFC0000000ull
#define KERNEL_SPACE        0x8000000000000000ull
#endif

#define ISA_IO_BASE KERNEL_VIRTUAL_BASE


// Page mapping flags (12 bits)
#define PAGE_PRESENT        0x001
#define PAGE_WRITE          0x002
#define PAGE_USER           0x004
#define PAGE_WRITE_THROUGH  0x008
#define PAGE_CACHE_DISABLE  0x010
#define PAGE_ACCESSED       0x020
#define PAGE_DIRTY          0x040
#define PAGE_LARGE          0x080
#define PAGE_GLOBAL         0x100
#define PAGE_ALLOCATED      0x200   // Page was allocated (vmm_alloc)
#define PAGE_RESERVED_1     0x400
#define PAGE_RESERVED_2     0x800


// Initialize the Virtual Memory Manager
void vmm_init();

// Map the specified physical page to the specified virtual page
int vmm_map_page(physaddr_t physicalAddress, void* virtualAddress);

// Unmap the specified virtual memory page
void vmm_unmap_page(void* virtualAddress);

// Map pages
void* vmm_map(physaddr_t physicalAddress, size_t length);

// Unmap pages
int vmm_unmap(void* virtualAddress, size_t length);


// Alloc virtual memory
// Returns NULL on failure (or if length is 0)
void* vmm_alloc(size_t length);


#endif
