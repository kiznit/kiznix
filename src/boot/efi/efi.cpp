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
#include "memorymap.hpp"
#include <stdio.h>
#include <string.h>


EFI_HANDLE efi_image;
EFI_SYSTEM_TABLE* efi;



void fatal(const char* format, ...)
{
    printf("Fatal error: ");

    char buffer[500];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    puts(buffer);

    if (efi && efi->BootServices)
    {
        CHAR16* exitData = NULL;
        UINTN exitDataSize = 0;

        size_t len = strlen(buffer) + 1;
        EFI_STATUS status = efi->BootServices->AllocatePool(EfiLoaderData, len*2, (void**)&exitData);

        if (!EFI_ERROR(status))
        {
            for (size_t i = 0; i != len; ++i)
            {
                exitData[i] = buffer[i];
            }
        }

        efi->BootServices->Exit(efi_image, 1, exitDataSize, exitData);
    }

    for (;;) ;
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




static void console_init()
{
    SIMPLE_TEXT_OUTPUT_INTERFACE* self = efi->ConOut;

    UINTN mode = 0;
    UINTN width = 80;
    UINTN height = 25;

    for (UINTN m = 0; ; ++m)
    {
        UINTN w, h;
        EFI_STATUS status = self->QueryMode(self, m, &w, &h);
        if (EFI_ERROR(status))
        {
            if (m > 1)
                break;
        }
        else
        {
            if (w * h > width * height)
            {
                mode = m;
                width = w;
                height = h;
            }
        }
    }

    if (mode > 0)
    {
        self->SetMode(self, mode);
    }
}



static void find_memory()
{
    MemoryMap memmap;

    for (size_t i = 0; i != memmap.size(); ++i)
    {
        const EFI_MEMORY_DESCRIPTOR& entry = memmap[i];
        add_memory(entry.Type, entry.PhysicalStart, entry.NumberOfPages * 4096ull, entry.Attribute);
    }
}



extern "C" EFIAPI EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* systemTable)
{
    efi_image = image;
    efi = systemTable;

    console_init();

    printf("Kiznix EFI Application (efi_main) - %d bits.\n", (int)sizeof(void*)*8);

    find_memory();

    // Wait for a key press
    UINTN index;
    EFI_EVENT event = efi->ConIn->WaitForKey;
    efi->BootServices->WaitForEvent(1, &event, &index);

    return EFI_SUCCESS;
}
