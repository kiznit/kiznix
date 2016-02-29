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
#include <string.h>
#include <cpuid.h>

#include <sys/elf.h>

#include "console.hpp"

#include "multiboot.h"
#include "multiboot2.h"

#include "memory.hpp"
#include "module.hpp"


static MemoryMap g_memoryMap;
static Modules g_modules;

extern const char bootloader_image_start[];
extern const char bootloader_image_end[];


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
static bool VerifyCPU()
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



static void ProcessMultibootInfo(multiboot_info const * const mbi)
{
    if (mbi->flags & MULTIBOOT_MEMORY_INFO)
    {
        const multiboot_mmap_entry* entry = (multiboot_mmap_entry*)mbi->mmap_addr;
        const multiboot_mmap_entry* end = (multiboot_mmap_entry*)(mbi->mmap_addr + mbi->mmap_length);

        while (entry < end)
        {
            MemoryType type = MemoryType_Reserved;

            switch (entry->type)
            {
            case MULTIBOOT_MEMORY_AVAILABLE:
                type = MemoryType_Available;
                break;

            case MULTIBOOT_MEMORY_RESERVED:
                type = MemoryType_Reserved;
                break;

            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                type = MemoryType_AcpiReclaimable;
                break;

            case MULTIBOOT_MEMORY_NVS:
                type = MemoryType_AcpiNvs;
                break;

            case MULTIBOOT_MEMORY_BADRAM:
                type = MemoryType_Unusable;
                break;
            }

            g_memoryMap.AddEntry(type, entry->addr, entry->addr + entry->len);

            // Go to next entry
            entry = (multiboot_mmap_entry*) ((uintptr_t)entry + entry->size + sizeof(entry->size));
        }
    }
    else if (mbi->flags & MULTIBOOT_INFO_MEMORY)
    {
        g_memoryMap.AddEntry(MemoryType_Available, 0, mbi->mem_lower * 1024);
        g_memoryMap.AddEntry(MemoryType_Available, 1024*1024, mbi->mem_upper * 1024);
    }

    if (mbi->flags & MULTIBOOT_INFO_MODS)
    {
        const multiboot_module* modules = (multiboot_module*)mbi->mods_addr;

        for (uint32_t i = 0; i != mbi->mods_count; ++i)
        {
            const multiboot_module* module = &modules[i];
            g_modules.AddModule(module->string, module->mod_start, module->mod_end);
        }
    }
}



static void ProcessMultibootInfo(multiboot2_info const * const mbi)
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
            g_modules.AddModule(module->string, module->mod_start, module->mod_end);
            break;
        }
    }

    if (mmap)
    {
        const multiboot2_mmap_entry* entry = mmap->entries;
        const multiboot2_mmap_entry* end = (multiboot2_mmap_entry*) (((uintptr_t)mmap + mmap->size + MULTIBOOT2_TAG_ALIGN - 1) & ~(MULTIBOOT2_TAG_ALIGN - 1));

        while (entry < end)
        {
            MemoryType type = MemoryType_Reserved;

            switch (entry->type)
            {
            case MULTIBOOT_MEMORY_AVAILABLE:
                type = MemoryType_Available;
                break;

            case MULTIBOOT_MEMORY_RESERVED:
                type = MemoryType_Reserved;
                break;

            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                type = MemoryType_AcpiReclaimable;
                break;

            case MULTIBOOT_MEMORY_NVS:
                type = MemoryType_AcpiNvs;
                break;

            case MULTIBOOT_MEMORY_BADRAM:
                type = MemoryType_Unusable;
                break;
            }

            g_memoryMap.AddEntry(type, entry->addr, entry->addr + entry->len);

            // Go to next entry
            entry = (multiboot2_mmap_entry*) ((uintptr_t)entry + mmap->entry_size);
        }
    }
    else if (meminfo)
    {
        g_memoryMap.AddEntry(MemoryType_Available, 0, meminfo->mem_lower * 1024);
        g_memoryMap.AddEntry(MemoryType_Available, 1024*1024, meminfo->mem_upper * 1024);
    }
}



static int LoadElf32(const void* file)
{
    const char* file_base = (const char*)file;
    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)file_base;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        return -1;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
        ehdr->e_machine != EM_386 ||
        ehdr->e_version  != EV_CURRENT)
    {
        return -2;
    }

    const char* phdr_base = file_base + ehdr->e_phoff;

    // Calculate how much memory we need to load this ELF
    uint32_t start = 0xFFFFFFFF;
    uint32_t end = 0;
    uint32_t align = 1;

    for (int i = 0; i != ehdr->e_phnum; ++i)
    {
        const Elf32_Phdr* phdr = (const Elf32_Phdr*)(phdr_base + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_paddr < start)
            start = phdr->p_paddr;

        if (phdr->p_paddr + phdr->p_memsz > end)
            end = phdr->p_paddr + phdr->p_memsz;

        if (phdr->p_align > align)
            align = phdr->p_align;
    }

    if (start > end)
    {
        return -3;
    }

    printf("ELF: %08lx - %08lx, align %08lx\n", (long)start, (long)end, (long)align);

    // Allocate memory
    char* memory = (char*) g_memoryMap.Alloc(MemoryZone_Normal, MemoryType_Unusable, end - start);

    printf("Memory allocated at %p\n", memory);

    // Load ELF
    for (int i = 0; i != ehdr->e_phnum; ++i)
    {
        const Elf32_Phdr* phdr = (const Elf32_Phdr*)(phdr_base + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_filesz != 0)
        {
            const char* src = file_base + phdr->p_offset;
            void* dst = memory + (phdr->p_paddr - start);
            memcpy(dst, src, phdr->p_filesz);
        }

        if (phdr->p_memsz > phdr->p_filesz)
        {
            void* dst = memory + (phdr->p_paddr - start) + phdr->p_filesz;
            const size_t count = phdr->p_memsz - phdr->p_filesz;
            memset(dst, 0, count);
        }
    }

    // Relocations
    const char* shdr_base = file_base + ehdr->e_shoff;

    for (int i = 0; i != ehdr->e_shnum; ++i)
    {
        const Elf32_Shdr* shdr = (const Elf32_Shdr*)(shdr_base + i * ehdr->e_shentsize);

        if (shdr->sh_type != SHT_REL)
            continue;

        printf("Section %2d: type: %5d, addr: %08x, offset: %08x, size: %05x, entsize: %08x\n", i,
            (int)shdr->sh_type, (int)shdr->sh_addr, (int)shdr->sh_offset, (int)shdr->sh_size, (int)shdr->sh_entsize);

        const Elf32_Shdr* symbols_section = (const Elf32_Shdr*)(shdr_base + shdr->sh_link * ehdr->e_shentsize);
        const char* symbols_base = file_base + symbols_section->sh_offset;

        const char* rel_base = file_base + shdr->sh_offset;

        for (int j = 0; j != (int)(shdr->sh_size / shdr->sh_entsize); ++j)
        {
            const Elf32_Rel* rel = (const Elf32_Rel*)(rel_base + j * shdr->sh_entsize);

            const int sym = ELF32_R_SYM(rel->r_info);
            const int type = ELF32_R_TYPE(rel->r_info);

            const Elf32_Sym* symbol = (const Elf32_Sym*)(symbols_base + sym * symbols_section->sh_entsize);

            printf("    %d: %08x %08x, type: %d, sym: %d, symbol.value: %08x\n", j, (int)rel->r_offset, (int)rel->r_info, type, sym, (int)symbol->st_value);

            switch (type)
            {
            case R_386_32:
                // Symbol value + addend
                *(uint32_t*)(memory + rel->r_offset) += symbol->st_value + (uint32_t)memory;
                break;

            case R_386_GLOB_DAT:
                // Symbol value
                *(uint32_t*)(memory + rel->r_offset) = symbol->st_value + (uint32_t)memory;
                break;

            case R_386_RELATIVE:
                // Base address + addend
                *(uint32_t*)(memory + rel->r_offset) += (uint32_t)memory;
                break;

            default:
                printf("Unknown relocation type %d!\n", type);
                break;
            }
        }
    }

    // TEMP: execute Launcher to see that it works properly
    const char* (*entry)(char**) = (const char* (*)(char**))(memory + ehdr->e_entry);

    printf("ENTRY AT %p\n", entry);
    char* out;
    const char* result = entry(&out);

    printf("RESULT: %p, out: %p\n", result, out);
    printf("Which is: '%s', [%d, %d, %d, ..., %d]\n", result, out[0], out[1], out[2], out[99]);

    return 0;
}



static int LoadLauncher()
{
    const ModuleInfo* launcher = NULL;

    for (Modules::const_iterator module = g_modules.begin(); module != g_modules.end(); ++module)
    {
        //todo: use case insensitive strcmp
        if (strcmp(module->name, "/kiznix/launcher") == 0)
        {
            launcher = module;
            break;
        }
    }

    if (!launcher)
    {
        printf("Module not found: launcher");
        return -1;
    }

    if (launcher->end > 0x100000000)
    {
        printf("Module launcher is in high memory (>4 GB) and can't be loaded");
        return -1;
    }

    int result = LoadElf32((void*)launcher->start);
    if (result < 0)
    {
        printf("Failed to load launcher");
        return result;
    }

    return 0;
}



static void Boot(int multibootVersion)
{
    printf("Bootloader      : Multiboot %d\n", multibootVersion);

    // Add bootloader to memory map
    const physaddr_t start = (physaddr_t)&bootloader_image_start;
    const physaddr_t end = (physaddr_t)&bootloader_image_end;
    g_memoryMap.AddEntry(MemoryType_Bootloader, start, end);

    // Add modules to memory map
    for (Modules::const_iterator module = g_modules.begin(); module != g_modules.end(); ++module)
    {
        g_memoryMap.AddEntry(MemoryType_Bootloader, module->start, module->end);
    }

    g_memoryMap.Sanitize();

    putchar('\n');
    g_memoryMap.Print();
    putchar('\n');
    g_modules.Print();

    if (LoadLauncher() !=0)
        return;
}



static void call_global_constructors()
{
    extern void (*__CTOR_LIST__[])();

    uintptr_t count = (uintptr_t) __CTOR_LIST__[0];

    if (count == (uintptr_t)-1)
    {
        count = 0;
        while (__CTOR_LIST__[count + 1])
            ++count;
    }

    for (uintptr_t i = count; i >= 1; --i)
    {
        __CTOR_LIST__[i]();
    }
}



static void call_global_destructors()
{
    extern void (*__DTOR_LIST__[])();

    for (void (**p)() = __DTOR_LIST__ + 1; *p; ++p)
    {
        (*p)();
    }
}



extern "C" void multiboot_main(unsigned int magic, void* mbi)
{
    console_init();

    call_global_constructors();

    printf("Kiznix Multiboot Bootloader\n\n");

    if (!VerifyCPU())
    {
        printf("CPU doesn't support required features, aborting\n");
    }
    else if (magic == MULTIBOOT_BOOTLOADER_MAGIC && mbi)
    {
        ProcessMultibootInfo(static_cast<multiboot_info*>(mbi));
        Boot(1);
    }
    else if (magic== MULTIBOOT2_BOOTLOADER_MAGIC && mbi)
    {
        ProcessMultibootInfo(static_cast<multiboot2_info*>(mbi));
        Boot(2);
    }
    else
    {
        printf("No multiboot information!\n");
    }

    call_global_destructors();
}
