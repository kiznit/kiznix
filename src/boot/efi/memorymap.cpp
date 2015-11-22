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

#include "memorymap.hpp"
#include <stdio.h>


extern EFI_SYSTEM_TABLE* efi;


MemoryMap::MemoryMap()
{
    EFI_MEMORY_DESCRIPTOR* map = NULL;
    UINTN mapSize = 0;
    UINTN mapKey;
    UINTN descriptorSize;
    UINT32 descriptorVersion;

    EFI_STATUS status;

    for (;;)
    {
        status = efi->BootServices->GetMemoryMap(&mapSize, map, &mapKey, &descriptorSize, &descriptorVersion);

        if (status != EFI_BUFFER_TOO_SMALL)
            break;

        // Allocating memory could add another descriptor, so compensate for this
        mapSize += descriptorSize;

        status = efi->BootServices->AllocatePool(EfiLoaderData, mapSize, (void**)&map);
        if (EFI_ERROR(status))
        {
            fatal("Failed to allocate memory for memory map (status = %d)", (int)status);
        }
    }

    if (EFI_ERROR(status))
    {
        fatal("Failed to get memory map (status = %d)", (int)status);
    }

    m_map = map;
    m_size = mapSize;
    m_key = mapKey;
    m_descriptorSize = descriptorSize;
    m_descriptorVersion = descriptorVersion;
}


MemoryMap::~MemoryMap()
{
}
