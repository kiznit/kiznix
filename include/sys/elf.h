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

#ifndef KIZNIX_INCLUDED_SYS_ELF_H
#define KIZNIX_INCLUDED_SYS_ELF_H

#include <stdint.h>



/*
    Basic types
*/

typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Half;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;



/*
    Elf header
*/

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9
#define EI_NIDENT 16

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define EV_NONE 0
#define EV_CURRENT 1

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define ELFOSABI_SYSV 0
#define ELFOSABI_HPUX 1
#define ELFOSABI_STANDALONE 255

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOOS 0xfe00
#define ET_HIOS 0xfeff
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define EM_NONE 0
#define EM_386 3
#define EM_X86_64 62

typedef struct
{
    uint8_t     e_ident[16];// File identification
    uint16_t    e_type;     // Object file type
    uint16_t    e_machine;  // Architecture
    uint32_t    e_version;  // Object file version
    uint32_t    e_entry;    // Entry point virtual address
    uint32_t    e_phoff;    // Program header table file offset
    uint32_t    e_shoff;    // Section header table file offset
    uint32_t    e_flags;    // Processor-specific flags
    uint16_t    e_ehsize;   // ELF header size in bytes
    uint16_t    e_phentsize;// Program header table entry size
    uint16_t    e_phnum;    // Program header table entry count
    uint16_t    e_shentsize;// Section header table entry size
    uint16_t    e_shnum;    // Section header table entry count
    uint16_t    e_shstrndx; // Section header string table index
} Elf32_Ehdr;

typedef struct
{
    uint8_t     e_ident[16];// File identification
    uint16_t    e_type;     // Object file type
    uint16_t    e_machine;  // Architecture
    uint32_t    e_version;  // Object file version
    uint64_t    e_entry;    // Entry point virtual address
    uint64_t    e_phoff;    // Program header table file offset
    uint64_t    e_shoff;    // Section header table file offset
    uint32_t    e_flags;    // Processor-specific flags
    uint16_t    e_ehsize;   // ELF header size in bytes
    uint16_t    e_phentsize;// Program header table entry size
    uint16_t    e_phnum;    // Program header table entry count
    uint16_t    e_shentsize;// Section header table entry size
    uint16_t    e_shnum;    // Section header table entry count
    uint16_t    e_shstrndx; // Section header string table index
} Elf64_Ehdr;



/*
    Section header
*/

#define SHN_UNDEF 0
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_LOOS 0xff20
#define SHN_HIOS 0xff3f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2

typedef struct
{
    uint32_t    sh_name;        // Section name (string tbl index)
    uint32_t    sh_type;        // Section type
    uint32_t    sh_flags;       // Section flags
    uint32_t    sh_addr;        // Section virtual addr at execution
    uint32_t    sh_offset;      // Section file offset
    uint32_t    sh_size;        // Section size in bytes
    uint32_t    sh_link;        // Link to another section
    uint32_t    sh_info;        // Additional section information
    uint32_t    sh_addralign;   // Section alignment
    uint32_t    sh_entsize;     // Entry size if section holds table
} Elf32_Shdr;

typedef struct
{
    uint32_t    sh_name;        // Section name (string tbl index)
    uint32_t    sh_type;        // Section type
    uint64_t    sh_flags;       // Section flags
    uint64_t    sh_addr;        // Section virtual addr at execution
    uint64_t    sh_offset;      // Section file offset
    uint64_t    sh_size;        // Section size in bytes
    uint32_t    sh_link;        // Link to another section
    uint32_t    sh_info;        // Additional section information
    uint64_t    sh_addralign;   // Section alignment
    uint64_t    sh_entsize;     // Entry size if section holds table
} Elf64_Shdr;



/*
    Program header
*/

#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6
#define PT_LOOS     0x60000000
#define PT_HIOS     0x6fffffff
#define PT_LOPROC   0x70000000
#define PT_HIPROC   0x7fffffff

#define PF_X        0x1
#define PF_W        0x2
#define PF_R        0x4
#define PF_MASKOS   0x0ff00000
#define PF_MASKPROC 0xf0000000

typedef struct
{
    uint32_t    p_type;         // Segment type
    uint32_t    p_offset;       // Segment file offset
    uint32_t    p_vaddr;        // Segment virtual address
    uint32_t    p_paddr;        // Segment physical address
    uint32_t    p_filesz;       // Segment size in file
    uint32_t    p_memsz;        // Segment size in memory
    uint32_t    p_flags;        // Segment flags
    uint32_t    p_align;        // Segment alignment
} Elf32_Phdr;

typedef struct
{
    uint32_t    p_type;         // Segment type
    uint32_t    p_flags;        // Segment flags
    uint64_t    p_offset;       // Segment file offset
    uint64_t    p_vaddr;        // Segment virtual address
    uint64_t    p_paddr;        // Segment physical address
    uint64_t    p_filesz;       // Segment size in file
    uint64_t    p_memsz;        // Segment size in memory
    uint64_t    p_align;        // Segment alignment
} Elf64_Phdr;


/*
    Dynamic structure
*/

#define DT_NULL         0
#define DT_NEEDED       1
#define DT_PLTRELSZ     2
#define DT_PLTGOT       3
#define DT_HASH         4
#define DT_STRTAB       5
#define DT_SYMTAB       6
#define DT_RELA         7
#define DT_RELASZ       8
#define DT_RELAENT      9
#define DT_STRSZ        10
#define DT_SYMENT       11
#define DT_INIT         12
#define DT_FINI         13
#define DT_SONAME       14
#define DT_RPATH        15
#define DT_SYMBOLIC     16
#define DT_REL          17
#define DT_RELSZ        18
#define DT_RELENT       19
#define DT_PLTREL       20
#define DT_DEBUG        21
#define DT_TEXTREL      22
#define DT_JMPREL       23
#define DT_BIND_NOW     24
#define DT_INIT_ARRAY   25
#define DT_FINI_ARRAY   26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_LOOS         0x6000000d
#define DT_HIOS         0x6ffff000

typedef struct
{
    int32_t d_tag;
    union
    {
        uint32_t d_val;
        uint32_t d_ptr;
    } d_un;

} Elf32_Dyn;

typedef struct
{
    int64_t d_tag;
    union
    {
        uint64_t d_val;
        uint64_t d_ptr;
    } d_un;

} Elf64_Dyn;



#endif
