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

#include "efi.hpp"
#include <stdio.h>
#include <malloc.h>


static EFI_SYSTEM_TABLE* efi;



extern "C" void __kiznix_putc(unsigned char c)
{
    CHAR16 s[3] = { c, 0, 0 };

    if (c == '\n')
    {
        s[1] = '\r';
    }

    SIMPLE_TEXT_OUTPUT_INTERFACE* out = efi->ConOut;
    out->OutputString(out, s);
}



static void add_memory(int type, uint64_t address, uint64_t length, uint64_t attributes)
{
    unsigned int s0 = address >> 32;
    unsigned int s1 = address & 0xFFFFFFFF;
    unsigned int e0 = (address + length) >> 32;
    unsigned int e1 = (address + length) & 0xFFFFFFFF;

    double size = length / (1024.0 * 1024.0);

    unsigned int a0 = attributes >> 32;
    unsigned int a1 = attributes & 0xFFFFFFFF;

    printf("    %d: %08x%08x - %08x%08x (%.2f MB) - %08x%08x\n", type, s0, s1, e0, e1, size, a0, a1);
}



static void find_memory()
{
    UINTN mapSize = 0;
    UINTN mapKey;
    UINTN descriptorSize;
    UINT32 descriptorVersion;

    EFI_STATUS status = efi->BootServices->GetMemoryMap(&mapSize, 0, &mapKey, &descriptorSize, &descriptorVersion);
    if (status != EFI_BUFFER_TOO_SMALL)
    {
        printf("Error: First call to GetMemoryMap() returned %d\n", (int)status);
        return;
    }

    EFI_MEMORY_DESCRIPTOR* map = (EFI_MEMORY_DESCRIPTOR*)alloca(mapSize);

    status = efi->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &descriptorSize, &descriptorVersion);
    if (EFI_ERROR(status))
    {
        printf("Error: Second call to GetMemoryMap() returned %d\n", (int)status);
        return;
    }

    const EFI_MEMORY_DESCRIPTOR* entry = map;
    const EFI_MEMORY_DESCRIPTOR* end = (EFI_MEMORY_DESCRIPTOR*)((uintptr_t)map + mapSize);

    printf("\nMemory map:\n");

    while (entry < end)
    {
        add_memory(entry->Type, entry->PhysicalStart, entry->NumberOfPages * 4096ull, entry->Attribute);
        entry = (EFI_MEMORY_DESCRIPTOR*)((uintptr_t)entry + descriptorSize);
    }
}



extern "C" EFIAPI EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* systemTable)
{
    (void)image;

    efi = systemTable;

    printf("Kiznix EFI Application (efi_main) - %d bits.\n", (int)sizeof(void*)*8);

    find_memory();

    // Wait for a key press
    UINTN index;
    EFI_EVENT event = systemTable->ConIn->WaitForKey;
    systemTable->BootServices->WaitForEvent(1, &event, &index);

    return EFI_SUCCESS;
}


