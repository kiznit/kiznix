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

#ifndef INCLUDED_EFI_MEMORYMAP_HPP
#define INCLUDED_EFI_MEMORYMAP_HPP

#include "efi.hpp"
#include <stddef.h>


class MemoryMap
{
public:

    class iterator
    {
    public:

        iterator(EFI_MEMORY_DESCRIPTOR* ptr, size_t size) : m_ptr(ptr), m_size(size) {}

        EFI_MEMORY_DESCRIPTOR* get()                    { return m_ptr; }
        EFI_MEMORY_DESCRIPTOR& operator*()              { return *get(); }
        EFI_MEMORY_DESCRIPTOR* operator->()             { return get(); }
        iterator& operator++()                          { m_ptr = (EFI_MEMORY_DESCRIPTOR*)((uintptr_t)m_ptr + m_size); return *this; }

    private:
        EFI_MEMORY_DESCRIPTOR* m_ptr;
        size_t m_size;
    };


    MemoryMap();
    ~MemoryMap();

    // STL-like interface
    iterator begin()                                    { return iterator(m_begin, m_descriptorSize); }
    iterator end()                                      { return iterator(m_end, m_descriptorSize); }
    size_t size() const                                 { return m_size; }
    EFI_MEMORY_DESCRIPTOR& operator[](int i)            { return *(EFI_MEMORY_DESCRIPTOR*)((uintptr_t)m_begin + i * m_descriptorSize); }


private:
    EFI_MEMORY_DESCRIPTOR* m_begin; // Start of memory map
    EFI_MEMORY_DESCRIPTOR* m_end;   // One pass the end of the memory map
    size_t m_size;                  // Number of descriptors
    size_t m_key;                   // Memory map key
    size_t m_descriptorSize;        // Descriptor size
    uint32_t m_descriptorVersion;   // Descriptor version
};



inline bool operator==(MemoryMap::iterator a, MemoryMap::iterator b)
{
    return a.get() == b.get();
}


inline bool operator!=(MemoryMap::iterator a, MemoryMap::iterator b)
{
    return !(a == b);
}


#endif
