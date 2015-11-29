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
#include <string.h>


EFI_HANDLE efi_image;
EFI_SYSTEM_TABLE* efi;
EFI_LOADED_IMAGE gBootImage;

//EFI_GUID gEfiSimpleFileSystemProtocolGuid = SIMPLE_FILE_SYSTEM_PROTOCOL;
EFI_GUID gEfiLoadedImageProtocol = LOADED_IMAGE_PROTOCOL;


/*
static void add_memory(int type, uint64_t address, uint64_t length, uint64_t attributes)
{
    unsigned int s0 = address >> 32;
    unsigned int s1 = address & 0xFFFFFFFF;
    unsigned int e0 = (address + length) >> 32;
    unsigned int e1 = (address + length) & 0xFFFFFFFF;

    double size = length / (1024.0 * 1024.0);

    unsigned int a0 = attributes >> 32;
    unsigned int a1 = attributes & 0xFFFFFFFF;

    printf("    %2d: %08x%08x - %08x%08x (%.2f MB) - %08x%08x\n", type, s0, s1, e0, e1, size, a0, a1);
}
*/



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



/*
static void find_memory()
{
    MemoryMap map;

    for (MemoryMap::iterator it = map.begin(); it != map.end(); ++it)
    {
        add_memory(it->Type, it->PhysicalStart, it->NumberOfPages * 4096ull, it->Attribute);
    }
}
*/


static void load_kernel()
{
    printf("\nload_kernel():\n");

    EFI_STATUS status;
    EFI_LOADED_IMAGE* bootImageInfo;

    status = efi->BootServices->OpenProtocol(efi_image, &gEfiLoadedImageProtocol, (void**)&bootImageInfo, efi_image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status))
    {
        fatal("OpenProtocol() returned %d", (int)status);
    }


    printf("DeviceHandle = %p\n", bootImageInfo->DeviceHandle);
    printf("FilePath = { type: %d, subtype: %d, length: %04x, path: ",
        bootImageInfo->FilePath->Type,
        bootImageInfo->FilePath->SubType,
        bootImageInfo->FilePath->Length[1] * 256 + bootImageInfo->FilePath->Length[0]);

    if (bootImageInfo->FilePath->Type == 4 && bootImageInfo->FilePath->SubType == 4)
    {
        FILEPATH_DEVICE_PATH* fp =(FILEPATH_DEVICE_PATH*)bootImageInfo->FilePath;
        SIMPLE_TEXT_OUTPUT_INTERFACE* self = efi->ConOut;
        self->OutputString(self, fp->PathName);
    }

    printf(" }\n");

/*
    UINTN handleCount;
    EFI_HANDLE* handles;

    status = efi->BootServices->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &handleCount, &handles);
    if (EFI_ERROR(status))
    {
        fatal("Unable to locate simple file system protocol");
    }

    printf("Got %d handles\n", (int)handleCount);
*/
}



extern "C" EFIAPI EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* systemTable)
{
    efi_image = image;
    efi = systemTable;

    console_init();

    printf("Kiznix EFI Bootloader (%d bits)\n\n", (int)sizeof(void*)*8);

    //find_memory();

    load_kernel();

    // Wait for a key press
    UINTN index;
    EFI_EVENT event = efi->ConIn->WaitForKey;
    efi->BootServices->WaitForEvent(1, &event, &index);

    return EFI_SUCCESS;
}
