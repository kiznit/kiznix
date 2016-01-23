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

#include "memory.hpp"
#include <stdio.h>


MemoryMap::MemoryMap()
{
    m_count = 0;
}



void MemoryMap::AddEntry(MemoryType type, physaddr_t start, physaddr_t end)
{
    // Ignore invalid entries (including zero-sized ones)
    if (start >= end)
        return;

    // Check if we can merge this new range with an existing entry
    for (int i = 0; i != m_count; ++i)
    {
        MemoryEntry* entry = &m_entries[i];

        if (type != entry->type)
            continue;

        if (start == entry->end)
        {
            entry->end = end;
            return;
        }

        if (end == entry->start)
        {
            entry->start = start;
            return;
        }
    }

    // If the table is full, we can't add more entries
    if (m_count == MEMORY_MAX_ENTRIES)
        return;

    // Insert this new entry
    MemoryEntry* entry = &m_entries[m_count];
    entry->start = start;
    entry->end = end;
    entry->type = type;
    ++m_count;
}



void MemoryMap::Print()
{
    printf("Memory map:\n");
    for (int i = 0; i != m_count; ++i)
    {
        const MemoryEntry& entry = m_entries[i];

        const char* type = "Unknown";

        switch (entry.type)
        {
        case MemoryType_Available:
            type = "Available";
            break;

        case MemoryType_Reserved:
            type = "Reserved";
            break;

        case MemoryType_Unusable:
            type = "Unusable";
            break;

        case MemoryType_FirmwareRuntime:
            type = "Firmware Runtime";
            break;

        case MemoryType_ACPIReclaimable:
            type = "ACPI Reclaimable";
            break;

        case MemoryType_ACPIRuntime:
            type = "ACPI Runtime";
            break;
        }

        printf("    %08x%08x - %08x%08x : ",
            (unsigned)(entry.start >> 32), (unsigned)(entry.start & 0xFFFFFFFF),
            (unsigned)(entry.end >> 32), (unsigned)(entry.end & 0xFFFFFFFF));
        puts(type);
    }
}




/*
void memory_sanitize(MemoryTable* table)
{
    (void)table;

    // First step: insert-sort all entries into a temporary table
    MemoryTable sorted;
    memory_init(&sorted);

    while (table->count > 0)
    {
        MemoryEntry* candidate = &table->entries[0];

        for (int i = 1; i != table->count; ++i)
        {
            MemoryEntry* entry = &table->entries[i];
            if (entry->start < candidate->start)
                candidate = entry;
            else if (entry->start == candidate->start && entry->end < candidate->end)
                candidate = entry;
        }

        sorted.entries[sorted.count++] = *candidate;

        *candidate = table->entries[--table->count];
    }


    // Now insert the entries back into the table, handling overlaps
    for (int i = 0; i != sorted.count; ++i)
    {
        MemoryEntry* entry = &sorted.entries[i];
        physaddr_t start = entry->start;
        physaddr_t end = entry->end;
        uint32_t type = entry->type;

        for (int j = i; j != sorted.count; ++j)
        {
            MemoryEntry* entry = &sorted.entries[j];
            if (entry->start >= end)
                break;

//todo: handle overlaps
        }

        memory_add_entry(table, start, end, type);
    }
}
*/
