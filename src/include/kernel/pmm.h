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

#ifndef KIZNIX_INCLUDED_KERNEL_PMM_H
#define KIZNIX_INCLUDED_KERNEL_PMM_H

#include <stddef.h>
#include <stdint.h>

#if defined(KIZNIX_PAE)
typedef uint64_t physaddr_t;
#elif defined(__i386__)
typedef uint32_t physaddr_t;
#elif defined(__x86_64__)
typedef uint64_t physaddr_t;
#endif

#define PAGE_SIZE 0x1000

// Rounds a memory address down to nearest page.
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))

// Rounds a memory address up to nearest page.
#define PAGE_ALIGN_UP(addr) PAGE_ALIGN_DOWN((addr) + PAGE_SIZE - 1)

// Is the specified pointer aligned on a page boundary?
#define IS_PAGE_ALIGNED(p) (((uintptr_t)(p) & (PAGE_SIZE-1)) == 0)


// Initialize the Physical Memory Manager
void pmm_init();

// Allocate a physical page - will always succeed (or never return)
physaddr_t pmm_alloc_page();

// Free a physical page
void pmm_free_page(physaddr_t page);



#endif
