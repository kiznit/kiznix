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

#include "elf.h"


int elf_init(elf_context* context, const char* image, size_t size)
{
    context->image = image;
    context->size = size;
    context->header32 = NULL;
    context->header64 = NULL;

    // File identification
    if (size < sizeof(elf_ident))
        return 0;

    elf_ident* ident = (elf_ident*)image;

    if (ident->magic != ELF_MAGIC)
        return 0;

    if (ident->class != ELF_CLASS_32 && ident->class != ELF_CLASS_64)
        return 0;

    if (ident->data != ELF_DATA_LITTLE_ENDIAN)
        return 0;

    if (ident->version != ELF_VERSION_CURRENT)
        return 0;

    // Header
    if (ident->class == ELF_CLASS_32)
    {
        if (size < sizeof(elf_header32))
            return 0;

        elf_header32* header = (elf_header32*)image;

        if (header->e_type != ELF_ET_EXEC && header->e_type != ELF_ET_DYN)
            return 0;

        if (header->e_machine != ELF_EM_386)
            return 0;

        if (header->e_version != ELF_VERSION_CURRENT)
            return 0;

        context->header32 = header;
        context->entry = header->e_entry;
    }
    else if (ident->class == ELF_CLASS_64)
    {
        if (size < sizeof(elf_header64))
            return 0;

        elf_header64* header = (elf_header64*)image;

        if (header->e_type != ELF_ET_EXEC && header->e_type != ELF_ET_DYN)
            return 0;

        if (header->e_machine != ELF_EM_EM_X86_64)
            return 0;

        if (header->e_version != ELF_VERSION_CURRENT)
            return 0;

        context->header64 = header;
        context->entry = header->e_entry;
    }

    // This elf is looking good...

    return 1;
}


int elf_read_segment(elf_context* context, int index, elf_segment* segment)
{
    if (context->header32)
    {
        elf_header32* header = context->header32;

        if (index < 0 || index >= context->header32->e_phnum)
            return 0;

        uintptr_t offset = header->e_phoff + index * header->e_phentsize;
        if (offset > context->size - header->e_phentsize)
            return 0;

        elf_segment32* s = (elf_segment32*)(context->image + offset);

        segment->type = s->p_type;
        segment->flags = s->p_flags;
        segment->data = context->image + s->p_offset;
        segment->lenData = s->p_filesz;
        segment->address = s->p_vaddr;
        segment->size = s->p_memsz;
        segment->alignment = s->p_align;
    }
    else if (context->header64)
    {
       elf_header64* header = context->header64;

        if (index < 0 || index >= context->header64->e_phnum)
            return 0;

        uintptr_t offset = header->e_phoff + index * header->e_phentsize;
        if (offset > context->size - header->e_phentsize)
            return 0;

        elf_segment64* s = (elf_segment64*)(context->image + offset);

        segment->type = s->p_type;
        segment->flags = s->p_flags;
        segment->data = context->image + s->p_offset;
        segment->lenData = s->p_filesz;
        segment->address = s->p_vaddr;
        segment->size = s->p_memsz;
        segment->alignment = s->p_align;\
    }
    else
    {
        return 0;
    }

    // Validation
    if (segment->data < context->image)
        return 0;

    if (segment->data + segment->lenData > context->image + context->size)
        return 0;

    return 1;
}
