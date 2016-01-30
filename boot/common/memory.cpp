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

    // Round to page boundaries
    if (type == MemoryType_Available)
    {
        start = MEMORY_ROUND_PAGE_UP(start);
        end = MEMORY_ROUND_PAGE_DOWN(end);

        // Anything left?
        if (start >= end)
            return;
    }
    else
    {
        start = MEMORY_ROUND_PAGE_DOWN(start);
        end = MEMORY_ROUND_PAGE_UP(end);
    }

    // Walk through our existing entries to decide what to do with this new range
    for (int i = 0; i != m_count; ++i)
    {
        MemoryEntry* entry = &m_entries[i];

        // Same type?
        if (type == entry->type)
        {
            // Check for overlaps / adjacency
            if (start <= entry->end && end >= entry->start)
            {
                // Update existing entry in-place
                if (start < entry->start)
                    entry->start = start;

                if (end > entry->end)
                    entry->end = end;

                return;
            }
        }
        else
        {
            // Types are different, check for overlaps
            if (start < entry->end && end > entry->start)
            {
                // Copy the entry as we will delete it
                MemoryEntry other = *entry;

                // Delete existing entry
                --m_count;
                for (int j = i; j != m_count; ++j)
                {
                    m_entries[j] = m_entries[j+1];
                }

                // Handle left piece
                if (start < other.start)
                    AddEntry(type, start, other.start);
                else if (other.start < start)
                    AddEntry(other.type, other.start, start);

                // Handle overlap
                MemoryType overlapType = type < other.type ? other.type : type;
                physaddr_t overlapStart = start < other.start ? other.start : start;
                physaddr_t overlapEnd = end < other.end ? end : other.end;
                AddEntry(overlapType, overlapStart, overlapEnd);

                // Handle right piece
                if (end < other.end)
                    AddEntry(other.type, end, other.end);
                else if (other.end < end)
                    AddEntry(type, other.end, end);

                return;
            }
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

        case MemoryType_AcpiReclaimable:
            type = "ACPI Reclaimable";
            break;

        case MemoryType_AcpiNvs:
            type = "ACPI Non-Volatile Storage";
            break;

        case MemoryType_Bootloader:
            type = "Bootloader";
            break;
        }

        printf("    %08x%08x - %08x%08x : ",
            (unsigned)(entry.start >> 32), (unsigned)(entry.start & 0xFFFFFFFF),
            (unsigned)(entry.end >> 32), (unsigned)(entry.end & 0xFFFFFFFF));
        puts(type);
    }
}



void MemoryMap::Sanitize()
{
    // We will sanitize the memory map by doing an insert-sort of all entries
    // MemoryMap::AddEntry() will take care of merging adjacent blocks.
    MemoryMap sorted;

    while (m_count > 0)
    {
        MemoryEntry* candidate = &m_entries[0];

        for (int i = 1; i != m_count; ++i)
        {
            MemoryEntry* entry = &m_entries[i];
            if (entry->start < candidate->start)
                candidate = entry;
            else if (entry->start == candidate->start && entry->end < candidate->end)
                candidate = entry;
        }

        sorted.AddEntry(candidate->type, candidate->start, candidate->end);

        *candidate = m_entries[--m_count];
    }

    *this = sorted;
}
