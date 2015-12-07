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

#ifndef INCLUDED_BOOT_COMMON_ELF_H
#define INCLUDED_BOOT_COMMON_ELF_H

#include <stddef.h>
#include <stdint.h>

#define ELF_MAGIC 0x464C457F

#define ELF_CLASS_32 1
#define ELF_CLASS_64 2

#define ELF_DATA_LITTLE_ENDIAN 1
#define ELF_DATA_BIG_ENDIAN 2

#define ELF_VERSION_CURRENT 1


typedef struct elf_ident
{
    uint32_t    magic;      // File identification
    uint8_t     class;      // File class
    uint8_t     data;       // Data encoding
    uint8_t     version;    // File version
    uint8_t     pad[9];     // Padding
} elf_ident;



#define ELF_ET_EXEC 2

#define ELF_EM_386 3
#define ELF_EM_EM_X86_64 62

typedef struct elf_header32
{
    elf_ident   e_ident;    // Magic number and other info
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
} elf_header32;

typedef struct elf_header64
{
    elf_ident   e_ident;    // Magic number and other info
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
} elf_header64;




typedef struct elf_section32
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
} elf_section32;

typedef struct elf_section64
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
} elf_section64;



typedef struct elf_segment32
{
    uint32_t    p_type;         // Segment type
    uint32_t    p_offset;       // Segment file offset
    uint32_t    p_vaddr;        // Segment virtual address
    uint32_t    p_paddr;        // Segment physical address
    uint32_t    p_filesz;       // Segment size in file
    uint32_t    p_memsz;        // Segment size in memory
    uint32_t    p_flags;        // Segment flags
    uint32_t    p_align;        // Segment alignment
} elf_segment32;

typedef struct elf_segment64
{
    uint32_t    p_type;         // Segment type
    uint32_t    p_flags;        // Segment flags
    uint64_t    p_offset;       // Segment file offset
    uint64_t    p_vaddr;        // Segment virtual address
    uint64_t    p_paddr;        // Segment physical address
    uint64_t    p_filesz;       // Segment size in file
    uint64_t    p_memsz;        // Segment size in memory
    uint64_t    p_align;        // Segment alignment
} elf_segment64;



typedef struct elf_context
{
    const char*     image;      // Start of elf image in memory
    size_t          size;       // Size of elf image in memory
    elf_header32*   header32;   // Header for 32 bits elf files
    elf_header64*   header64;   // Header for 64 bits elf files

} elf_context;


typedef struct elf_segment
{
    uint32_t    type;           // Segment type
    uint32_t    flags;          // Segment flags
    const char* data;           // Binary image
    uint64_t    lenData;        // Length of image in file
    uint64_t    address;        // Virtual address
    uint64_t    size;           // Length of segment in memory
    uint64_t    alignment;      // Alignment

} elf_segment;



int elf_init(elf_context* context, const char* image, size_t size);
int elf_read_segment(elf_context* context, int index, elf_segment* segment);

#endif
