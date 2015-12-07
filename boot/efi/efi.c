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

#include <efi.h>
#include <efilib.h>
#include <elf.h>

#define STRINGIZE_DELAY(x) #x
#define STRINGIZE(x) STRINGIZE_DELAY(x)

#define EfiMemoryKiznixKernel 0x80000000



static void console_init(SIMPLE_TEXT_OUTPUT_INTERFACE* conout)
{
    // Mode 0 is 80x25 text mode and always supported
    // Mode 1 is 80x50 text mode and not always supported
    // Mode 2+ are differents on every device
    UINTN mode = 0;
    UINTN width = 80;
    UINTN height = 25;

    for (UINTN m = 0; ; ++m)
    {
        UINTN w, h;
        EFI_STATUS status = conout->QueryMode(conout, m, &w, &h);
        if (EFI_ERROR(status))
        {
            // Mode 1 might return EFI_UNSUPPORTED, we still want to check modes 2+
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
        conout->SetMode(conout, mode);
    }
}



static EFI_STATUS boot(EFI_HANDLE hImage)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE* bootLoaderImage = NULL;

    // Get bootloader image information to get at the boot device
    status = BS->OpenProtocol(hImage, &LoadedImageProtocol, (void**)&bootLoaderImage,
                              hImage, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status))
    {
        Print(L"Could not open loaded image protocol");
        return status;
    }

    Print(L"Boot device     : %s\n", DevicePathToStr(DevicePathFromHandle(bootLoaderImage->DeviceHandle)));
    Print(L"Bootloader      : %s\n", DevicePathToStr(bootLoaderImage->FilePath));

    // Load the right kernel for the current architecture
    CHAR16 szPath[] = L"\\kiznix\\kernel_" STRINGIZE(ARCH);

    EFI_DEVICE_PATH* path = FileDevicePath(bootLoaderImage->DeviceHandle, szPath);
    EFI_HANDLE hDevice;
    SIMPLE_READ_FILE fp;
    status = OpenSimpleReadFile(FALSE, NULL, 0, &path, &hDevice, &fp);
    if (EFI_ERROR(status))
    {
        Print(L"Could not open kernel image file \"%s\"", szPath);
        return status;
    }

    Print(L"Kernel image    : %s\n", szPath);

    UINTN elfSize = SizeSimpleReadFile(fp);
    void* elfImage = AllocatePool(elfSize);
    UINTN readSize = elfSize;
    status = ReadSimpleReadFile(fp, 0, &readSize, elfImage);
    if (EFI_ERROR(status) || readSize != elfSize)
    {
        Print(L"Could not load kernel \"%s\"", szPath);
        return status;
    }

    Print(L"Kernel size     : %d\n", elfSize);

    elf_context elf;
    if (!elf_init(&elf, elfImage, elfSize))
    {
        Print(L"Unrecognized file format for \"%s\"", szPath);
        return EFI_LOAD_ERROR;
    }

    for (int i = 0; ; ++i)
    {
        elf_segment segment;
        if (!elf_read_segment(&elf, i, &segment))
            break;

        if (segment.type != ELF_TYPE_LOAD)
        {
            Print(L"Don't know how to handle program header of type %d, skipping\n", segment.type);
            continue;
        }

        uint64_t pageCount = EFI_SIZE_TO_PAGES(segment.size);
        uint64_t physicalAddress;
        status = BS->AllocatePages(AllocateAnyPages, EfiMemoryKiznixKernel, pageCount, &physicalAddress);
        if (EFI_ERROR(status))
        {
            Print(L"Failed to allocate memory loading \"%s\"", szPath);
            return status;
        }

        Print(L"Allocated %ld pages at %lx for segment %d:\n", pageCount, physicalAddress, i);

        Print(L"    type: %x\n", segment.type);
        Print(L"    flags: %x\n", segment.flags);
        Print(L"    data: %lx\n", segment.data);
        Print(L"    lenData: %lx\n", segment.lenData);
        Print(L"    address: %lx\n", segment.address);
        Print(L"    size: %lx\n", segment.size);
        Print(L"    alignment: %lx\n", segment.alignment);

        // Copy data
        char* p = (char*)(uintptr_t)physicalAddress;
        CopyMem(p, segment.data, segment.lenData);

        // Clear remaining memory
        if (segment.size > segment.lenData)
        {
            ZeroMem(p + segment.lenData, segment.size - segment.lenData);
        }
    }

    Print(L"Entry point: %lx\n", elf.entry);

    return EFI_SUCCESS;
}



EFI_STATUS efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    InitializeLib(hImage, systemTable);

    console_init(ST->ConOut);

    Print(L"Kiznix EFI Bootloader (" STRINGIZE(ARCH) ")\n\n", (int)sizeof(void*)*8);

    EFI_STATUS status = boot(hImage);

    if (EFI_ERROR(status))
    {
        CHAR16 buffer[64];
        StatusToString(buffer, status);
        Print(L": %s\n", buffer);
    }

    // Wait for a key press
    UINTN index;
    EFI_EVENT event = ST->ConIn->WaitForKey;
    ST->BootServices->WaitForEvent(1, &event, &index);

    return status;
}
