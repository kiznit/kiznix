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

#include <stdio.h>
#include <stdlib.h>

extern "C"
{
    #include <efi.h>
    #include <efilib.h>
}

#include "memory.hpp"
#include "module.hpp"


static MemoryMap g_memoryMap;
static Modules g_modules;


#define STRINGIZE_DELAY(x) #x
#define STRINGIZE(x) STRINGIZE_DELAY(x)


static void InitConsole(SIMPLE_TEXT_OUTPUT_INTERFACE* conout)
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

    conout->SetMode(conout, mode);
}



static EFI_STATUS LoadModule(EFI_HANDLE hDevice, const wchar_t* szPath, const char* name)
{
    EFI_DEVICE_PATH* path = FileDevicePath(hDevice, (CHAR16*)szPath);
    if (!path)
        return EFI_LOAD_ERROR;

    SIMPLE_READ_FILE fp;
    EFI_STATUS status = OpenSimpleReadFile(FALSE, NULL, 0, &path, &hDevice, &fp);
    if (EFI_ERROR(status))
    {
        Print((CHAR16*)L"Could not open module file \"%s\"\n", szPath);
        return status;
    }

    UINTN fileSize = SizeSimpleReadFile(fp);

    // In theory I should be able to call AllocatePages with a custom memory type (0x80000000)
    // to track module data. In practice, doing so crashes my main development system.
    // Motherboard/firmware info: Hero Hero Maximus VI (build 1603 2014/09/19).
    UINTN pageCount = (fileSize + EFI_PAGE_SIZE - 1) & ~EFI_PAGE_MASK;
    EFI_PHYSICAL_ADDRESS address = 0xBFFFFFFF;
    status = BS->AllocatePages(AllocateMaxAddress, EfiBootServicesData, pageCount, &address);
    if (EFI_ERROR(status))
        return status;

    void* fileData = (void*)address;
    if (!fileData)
        return EFI_OUT_OF_RESOURCES;

    UINTN readSize = fileSize;
    status = ReadSimpleReadFile(fp, 0, &readSize, fileData);
    if (EFI_ERROR(status) || readSize != fileSize)
    {
        Print((CHAR16*)L"Could not read module file \"%s\"\n", szPath);
        return EFI_LOAD_ERROR;
    }

    const physaddr_t start = (uintptr_t) fileData;
    const physaddr_t end = start + readSize;

    g_modules.AddModule(name, start, end);

    return EFI_SUCCESS;
}



static EFI_STATUS LoadModules(EFI_HANDLE hDevice)
{
    EFI_STATUS status;

    status = LoadModule(hDevice, L"\\kiznix\\startos", "/kiznix/startos");
    if (EFI_ERROR(status))
        return status;

#if defined(__i386__) || defined(__x86_64__)
    status = LoadModule(hDevice, L"\\kiznix\\kernel_ia32", "/kiznix/kernel_ia32");
    if (EFI_ERROR(status))
        return status;

    status = LoadModule(hDevice, L"\\kiznix\\kernel_x86_64", "/kiznix/kernel_x86_64");
    if (EFI_ERROR(status))
        return status;
#endif

    return EFI_SUCCESS;
}



static EFI_STATUS ExitBootServices(EFI_HANDLE hImage)
{
    UINTN descriptorCount;
    UINTN mapKey;
    UINTN descriptorSize;
    UINT32 descriptorVersion;

    EFI_MEMORY_DESCRIPTOR* memoryMap = LibMemoryMap(&descriptorCount, &mapKey, &descriptorSize, &descriptorVersion);

    if (!memoryMap)
    {
        printf("Failed to retrieve memory map!\n");
        return EFI_OUT_OF_RESOURCES;
    }

    // Map runtime services to this Virtual Memory Address (vma)
    bool mappedAnything = false;
    physaddr_t vma = 0x80000000;

    EFI_MEMORY_DESCRIPTOR* descriptor = memoryMap;
    for (UINTN i = 0; i != descriptorCount; ++i, descriptor = NextMemoryDescriptor(descriptor, descriptorSize))
    {
        if (descriptor->Attribute & EFI_MEMORY_RUNTIME)
        {
            //const physaddr_t start = descriptor->PhysicalStart;
            const physaddr_t size = descriptor->NumberOfPages * EFI_PAGE_SIZE;
            //const physaddr_t end = start + size;

            descriptor->VirtualStart = vma;
            mappedAnything = true;

            vma = vma + size;
        }
    }

//TODO
    (void)hImage;
    // EFI_STATUS status = BS->ExitBootServices(hImage, mapKey);
    // if (EFI_ERROR(status))
    // {
    //     printf("Failed to exit boot services: %08x\n", (int)status);
    //     return status;
    // }

    if (mappedAnything)
    {
        EFI_STATUS status = RT->SetVirtualAddressMap(descriptorCount * descriptorSize, descriptorSize, descriptorVersion, memoryMap);
        if (EFI_ERROR(status))
        {
            printf("Failed to set virtual address map: %08x\n", (int)status);
            return status;
        }
    }

    FreePool(memoryMap);

    return EFI_SUCCESS;
}



static EFI_STATUS BuildMemoryMap()
{
    UINTN descriptorCount;
    UINTN mapKey;
    UINTN descriptorSize;
    UINT32 descriptorVersion;

    EFI_MEMORY_DESCRIPTOR* memoryMap = LibMemoryMap(&descriptorCount, &mapKey, &descriptorSize, &descriptorVersion);

    if (!memoryMap)
    {
        printf("Failed to retrieve memory map!\n");
        return EFI_OUT_OF_RESOURCES;
    }

    EFI_MEMORY_DESCRIPTOR* descriptor = memoryMap;
    for (UINTN i = 0; i != descriptorCount; ++i, descriptor = NextMemoryDescriptor(descriptor, descriptorSize))
    {
        MemoryType type = MemoryType_Reserved;

        switch (descriptor->Type)
        {
        case EfiUnusableMemory:
            type = MemoryType_Unusable;
            break;

        case EfiLoaderCode:
        case EfiLoaderData:
        case EfiConventionalMemory:
            if (descriptor->Attribute & EFI_MEMORY_WB)
                type = MemoryType_Available;
            else
                type = MemoryType_Reserved;
            break;

        case EfiBootServicesCode:
        case EfiBootServicesData:
            // Work around buggy firmware that call boot services after we exited them.
            if (descriptor->Attribute & EFI_MEMORY_WB)
                type = MemoryType_Bootloader;
            else
                type = MemoryType_Reserved;
            break;

        case EfiRuntimeServicesCode:
        case EfiRuntimeServicesData:
            type = MemoryType_FirmwareRuntime;
            break;

        case EfiACPIReclaimMemory:
            type = MemoryType_AcpiReclaimable;
            break;

        case EfiACPIMemoryNVS:
            type = MemoryType_AcpiNvs;
            break;

        case EfiReservedMemoryType:
        case EfiMemoryMappedIO:
        case EfiMemoryMappedIOPortSpace:
        case EfiPalCode:
            type = MemoryType_Reserved;
            break;
        }

        const physaddr_t start = descriptor->PhysicalStart;
        const physaddr_t end = start + descriptor->NumberOfPages * EFI_PAGE_SIZE;

        g_memoryMap.AddEntry(type, start, end);
    }

    FreePool(memoryMap);

    // Now account for the bootloader modules
    for (Modules::const_iterator module = g_modules.begin(); module != g_modules.end(); ++module)
    {
        g_memoryMap.AddEntry(MemoryType_Bootloader, module->start, module->end);
    }

    g_memoryMap.Sanitize();

    return EFI_SUCCESS;
}



static EFI_STATUS Boot(EFI_HANDLE hImage)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE* bootLoaderImage = NULL;

    // Get bootloader image information to get at the boot device
    status = BS->OpenProtocol(hImage, &LoadedImageProtocol, (void**)&bootLoaderImage,
                              hImage, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status))
    {
        printf("Could not open loaded image protocol");
        return status;
    }

    printf("Boot device     : %s\n", DevicePathToStr(DevicePathFromHandle(bootLoaderImage->DeviceHandle)));
    printf("Bootloader      : %s\n", DevicePathToStr(bootLoaderImage->FilePath));

    status = LoadModules(bootLoaderImage->DeviceHandle);
    if (EFI_ERROR(status))
    {
        printf("Could not load modules");
        return status;
    }

    status = BuildMemoryMap();
    if (EFI_ERROR(status))
    {
        printf("Could not retrieve memory map");
        return status;
    }

    ExitBootServices(hImage);

    return EFI_SUCCESS;
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



extern "C" EFI_STATUS efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    InitializeLib(hImage, systemTable);

    InitConsole(ST->ConOut);

    printf("Kiznix EFI Bootloader (" STRINGIZE(ARCH) ")\n\n", (int)sizeof(void*)*8);

    call_global_constructors();

    EFI_STATUS status = Boot(hImage);

    if (EFI_ERROR(status))
    {
        CHAR16 buffer[64];
        StatusToString(buffer, status);
        printf(": %s\n", buffer);
    }
    else
    {
        putchar('\n');
        g_memoryMap.Print();
        putchar('\n');
        g_modules.Print();
    }

    getchar();

    call_global_destructors();

    return status;
}
