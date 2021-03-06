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

#include <kernel/kernel.h>

typedef struct gdt_descriptor gdt_descriptor;

struct gdt_descriptor
{
    uint16_t limit;
    uint16_t base;
    uint16_t flags1;
    uint16_t flags2;
};



typedef struct gdt_ptr gdt_ptr;

struct gdt_ptr
{
    uint16_t size;
    void* address;
} __attribute__((packed));




static gdt_descriptor GDT[] __attribute__((aligned(16))) =
{
    // 0x00 - Null Descriptor
    { 0, 0, 0, 0 },

#if defined(__i386__)

    // 0x08 - Kernel Code Descriptor
    {
        0xFFFF,     // Limit = 0x100000 * 4 KB = 4 GB
        0x0000,     // Base = 0
        0x9A00,     // P + DPL 0 + S + Code + Execute + Read
        0x00CF,     // G + D (32 bits)
    },

    // 0x10 - Kernel Data Descriptor
    {
        0xFFFF,     // Limit = 0x100000 * 4 KB = 4 GB
        0x0000,     // Base = 0
        0x9200,     // P + DPL 0 + S + Data + Read + Write
        0x00CF,     // G + B (32 bits)
    },

#elif defined(__x86_64__)

    // 0x08 - Kernel Code Descriptor
    {
        0x0000,     // Limit ignored in 64 bits mode
        0x0000,     // Base ignored in 64 bits mode
        0x9A00,     // P + DPL 0 + S + Code + Execute + Read
        0x0020,     // L (64 bits)
    },

    // 0x10 - Kernel Data Descriptor
    {
        0x0000,     // Limit ignored in 64 bits mode
        0x0000,     // Base ignored in 64 bits mode
        0x9200,     // P + DPL 0 + S + Data + Read + Write
        0x0000,     // Nothing
    },

#endif
};

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10


static gdt_ptr GDT_PTR =
{
    sizeof(GDT)-1,
    GDT
};



void cpu_init()
{
    // Load GDT
#if defined(__i386__)
    asm volatile ("lgdt %0" : : "m" (GDT_PTR) );
#elif defined(__x86_64__)
    asm volatile ("lgdtq %0" : : "m" (GDT_PTR) );
#endif


    // Load code segment
#if defined(__i386__)
    asm volatile (
        "push %0\n"
        "push $1f\n"
        "retf\n"
        "1:\n"
        : : "i"(GDT_KERNEL_CODE) : "memory"
    );
#elif defined(__x86_64__)
    asm volatile (
        "push %0\n"
        "push $1f\n"
        "retfq\n"
        "1:\n"
        : : "i"(GDT_KERNEL_CODE) : "memory"
    );
#endif

    // Load data segments
    asm volatile (
        "movl %0, %%ds\n"
        "movl %0, %%es\n"
        "movl %0, %%fs\n"
        "movl %0, %%gs\n"
        "movl %0, %%ss\n"
        : : "r" (GDT_KERNEL_DATA) : "memory"
    );
}
