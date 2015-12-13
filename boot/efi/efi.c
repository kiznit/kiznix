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
#include <memory.h>
#include <stdio.h>

#define STRINGIZE_DELAY(x) #x
#define STRINGIZE(x) STRINGIZE_DELAY(x)


typedef struct elf_info
{
    physaddr_t code_start;
    physaddr_t code_end;
    physaddr_t rodata_start;
    physaddr_t rodata_end;
    physaddr_t data_start;
    physaddr_t data_end;
    physaddr_t entry;
} elf_info;



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



static EFI_STATUS load_elf(EFI_HANDLE hDevice, CHAR16* szPath, elf_info* info)
{
    ZeroMem(info, sizeof(*info));

    SIMPLE_READ_FILE fp;
    EFI_STATUS status;
    EFI_DEVICE_PATH* path = FileDevicePath(hDevice, szPath);

    status = OpenSimpleReadFile(FALSE, NULL, 0, &path, &hDevice, &fp);
    if (EFI_ERROR(status))
    {
        printf("Could not open elf binary: \"%s\"", szPath);
        return status;
    }

    printf("Elf image       : %s\n", szPath);

    UINTN elfSize = SizeSimpleReadFile(fp);
    void* elfImage = AllocatePool(elfSize);
    UINTN readSize = elfSize;
    status = ReadSimpleReadFile(fp, 0, &readSize, elfImage);
    if (EFI_ERROR(status) || readSize != elfSize)
    {
        printf("Could not load elf binary: \"%s\"", szPath);
        return status;
    }

    printf("Elf size        : %d\n", elfSize);

    elf_context elf;
    if (!elf_init(&elf, elfImage, elfSize))
    {
        printf("Unrecognized file format: \"%s\"", szPath);
        return EFI_LOAD_ERROR;
    }

    for (int i = 0; ; ++i)
    {
        elf_segment segment;
        if (!elf_read_segment(&elf, i, &segment))
            break;

/*
        printf("Segment %d:\n", i);
        printf("    type: %x\n", segment.type);
        printf("    flags: %x\n", segment.flags);
        printf("    data: %lx\n", segment.data);
        printf("    lenData: %lx\n", segment.lenData);
        printf("    address: %lx\n", segment.address);
        printf("    size: %lx\n", segment.size);
        printf("    alignment: %lx\n", segment.alignment);
*/
        if (segment.type == ELF_PT_LOAD)
        {
            uint64_t pageCount = EFI_SIZE_TO_PAGES(segment.size);
            uint64_t physicalAddress;

            /*
                Using a memory type of 0x80000000 (reserved to OS loaders) causes GetMemoryMap()
                to hang on my computer (Asus Maximums VI Hero with BIOS version 1603 x64 08/15/2014).
                For this reason, we use memory type EfiLoaderData which works well enough for us.
            */
            status = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pageCount, &physicalAddress);
            if (EFI_ERROR(status))
            {
                printf("Failed to allocate memory loading \"%s\"", szPath);
                return status;
            }

            // Copy data
            char* p = (char*)(uintptr_t)physicalAddress;
            CopyMem(p, segment.data, segment.lenData);

            // Clear remaining memory
            if (segment.size > segment.lenData)
            {
                ZeroMem(p + segment.lenData, segment.size - segment.lenData);
            }

            // Remember this segment
            if (segment.flags == ELF_PF_READ + ELF_PF_EXEC)
            {
                info->code_start = physicalAddress;
                info->code_end = physicalAddress + segment.size;
            }
            else if (segment.flags == ELF_PF_READ)
            {
                info->rodata_start = physicalAddress;
                info->rodata_end = physicalAddress + segment.size;
            }
            else if (segment.flags == ELF_PF_READ + ELF_PF_WRITE)
            {
                info->data_start = physicalAddress;
                info->data_end = physicalAddress + segment.size;
            }
        }
        else if (segment.type == ELF_PT_DYNAMIC && elf.header32 != NULL)
        {
            elf_dynamic32* dyns = (elf_dynamic32*)segment.data;
            void* relocs = NULL;
            int relocsSize = 0;

            for (int i = 0; i != (int)(segment.size / sizeof(elf_dynamic32)); ++i)
            {
                switch (dyns[i].d_tag)
                {
                case DT_REL:
                    relocs = (void*)(intptr_t)dyns[i].d_un.d_ptr;
                    break;

                case DT_RELSZ:
                case DT_RELENT:
                    relocsSize = dyns[i].d_un.d_val;
                    break;
                }
            }

            printf("Relocation table at %x, size %d\n", relocs, relocsSize);
        }
        else
        {
            printf("Don't know how to handle program header of type %d, skipping\n", segment.type);
            continue;
        }
    }

    info->entry = elf.entry;

    printf("Elf code segment  : %lx - %lx\n", info->code_start, info->code_end);
    printf("Elf rodata segment: %lx - %lx\n", info->rodata_start, info->rodata_end);
    printf("Elf data segment  : %lx - %lx\n", info->data_start, info->data_end);
    printf("Elf entry point   : %lx\n", info->entry);

    return EFI_SUCCESS;
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
        printf("Could not open loaded image protocol");
        return status;
    }

    printf("Boot device     : %s\n", DevicePathToStr(DevicePathFromHandle(bootLoaderImage->DeviceHandle)));
    printf("Bootloader      : %s\n", DevicePathToStr(bootLoaderImage->FilePath));


    // Load trampoline
    CHAR16 szTrampolinePath[] = L"\\kiznix\\trampoline";
    elf_info trampolineInfo;
    status = load_elf(bootLoaderImage->DeviceHandle, szTrampolinePath, &trampolineInfo);
    if (EFI_ERROR(status))
    {
        return status;
    }

/*
    // Load the right kernel for the current architecture
    CHAR16 szKernelPath[] = L"\\kiznix\\kernel_" STRINGIZE(ARCH);
    elf_info kernelInfo;
    status = load_elf(bootLoaderImage->DeviceHandle, szKernelPath, &kernelInfo);
    if (EFI_ERROR(status))
    {
        return status;
    }
*/

/*
    UINTN descriptorCount;
    UINTN mapKey;
    UINTN descriptorSize;
    UINT32 descriptorVersion;
    printf("\nMemoryMap:\n");

    EFI_MEMORY_DESCRIPTOR* memoryMap = LibMemoryMap(&descriptorCount, &mapKey, &descriptorSize, &descriptorVersion);

    if (!memoryMap)
    {
        printf("Failed to retrieve memory map!\n");
        return EFI_LOAD_ERROR;
    }


    MemoryTable memoryTable;
    memory_init(&memoryTable);

    EFI_MEMORY_DESCRIPTOR* descriptor = memoryMap;
    for (UINTN i = 0; i != descriptorCount; ++i, descriptor = NextMemoryDescriptor(descriptor, descriptorSize))
    {
        MemoryType type;

        switch (descriptor->Type)
        {
            case EfiUnusableMemory:
                type = MemoryType_Unusable;
                break;

            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                if (descriptor->Attribute & EFI_MEMORY_WB)
                    type = MemoryType_Available;
                else
                    type = MemoryType_Reserved;
                break;

            case EfiRuntimeServicesCode:
            case EfiRuntimeServicesData:
                type = MemoryType_FirmwareRuntime;
                break;

            case EfiACPIReclaimMemory:
                type = MemoryType_ACPIReclaim;
                break;

            case EfiACPIMemoryNVS:
                type = MemoryType_ACPIRuntime;
                break;

            case EfiReservedMemoryType:
            case EfiMemoryMappedIO:
            case EfiMemoryMappedIOPortSpace:
            case EfiPalCode:
            default:
                type = MemoryType_Reserved;
                break;
        }


        uint64_t start = descriptor->PhysicalStart;
        uint64_t end = start + descriptor->NumberOfPages * EFI_PAGE_SIZE;

        memory_add_entry(&memoryTable, start, end, type);
    }

    memory_sanitize(&memoryTable);

    printf("    Type      Start             End\n");
    for (int i = 0; i != memoryTable.count; ++i)
    {
        MemoryEntry* entry = &memoryTable.entries[i];
        printf("%2d:  %8x  %16lx  %16lx\n", i, entry->type, entry->start, entry->end);
    }
*/

    return EFI_SUCCESS;
}



EFI_STATUS efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    InitializeLib(hImage, systemTable);

    console_init(ST->ConOut);

    printf("Kiznix EFI Bootloader (" STRINGIZE(ARCH) ")\n\n", (int)sizeof(void*)*8);

    EFI_STATUS status = boot(hImage);

    if (EFI_ERROR(status))
    {
        CHAR16 buffer[64];
        StatusToString(buffer, status);
        printf(": %s\n", buffer);
    }

    // Wait for a key press
    UINTN index;
    EFI_EVENT event = ST->ConIn->WaitForKey;
    ST->BootServices->WaitForEvent(1, &event, &index);

    return status;
}
