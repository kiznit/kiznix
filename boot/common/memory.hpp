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

#ifndef INCLUDED_BOOT_COMMON_MEMORY_HPP
#define INCLUDED_BOOT_COMMON_MEMORY_HPP

#include <sys/types.h>


#define MEMORY_MAX_ENTRIES 1024



enum MemoryType
{
    MemoryType_Available,       // Available memory (RAM)
    MemoryType_Reserved,        // Reserved / unknown / do not use
    MemoryType_Unusable,        // Memory in which errors have been detected
    MemoryType_FirmwareRuntime, // Firmware Runtime Memory (e.g. EFI runtime services)
    MemoryType_ACPIReclaimable, // ACPI Tables (can be reclaimed once parsed)
    MemoryType_ACPIRuntime,     // ACPI Runtime Memory (e.g. Non-Volatile Storage)
};



struct MemoryEntry
{
    physaddr_t start;   // Start of memory range
    physaddr_t end;     // End of memory range
    MemoryType type;    // Type of memory
};



class MemoryMap
{
public:

    MemoryMap();

    void AddEntry(MemoryType type, physaddr_t start, physaddr_t end);

    void Print();


private:

    MemoryEntry  m_entries[MEMORY_MAX_ENTRIES]; // Memory entries
    int          m_count;                       // Memory entry count
};

/*
void memory_sanitize(MemoryTable* table);
*/



#endif