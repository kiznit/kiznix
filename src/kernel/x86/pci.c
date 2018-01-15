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

#include <kernel/pci.h>
#include <kernel/x86/io.h>

#include <assert.h>


#define PCI_CONFIG_ADDRESSS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_CONFIG_ENABLE 0x80000000


// http://wiki.osdev.org/PCI#Configuration_Space_Access_Mechanism_.231
// http://lxr.free-electrons.com/source/arch/x86/pci/early.c


uint8_t pci_read_config_8(int bus, int device, int function, int offset)
{
    assert(bus >= 0 && bus < 256);
    assert(device >= 0 && device < 32);
    assert(function >= 0 && function < 8);
    assert(offset >= 0 && offset < 256);

    uint32_t address = PCI_CONFIG_ENABLE | (bus << 16) | (device << 11) | (function << 8) | offset;

    io_out_32(PCI_CONFIG_ADDRESSS, address);

    uint8_t result = io_in_8(PCI_CONFIG_DATA + (offset & 3));

    return result;
}



uint16_t pci_read_config_16(int bus, int device, int function, int offset)
{
    assert(bus >= 0 && bus < 256);
    assert(device >= 0 && device < 32);
    assert(function >= 0 && function < 8);
    assert(offset >= 0 && offset < 256);
    assert(!(offset & 1));

    uint32_t address = PCI_CONFIG_ENABLE | (bus << 16) | (device << 11) | (function << 8) | offset;

    io_out_32(PCI_CONFIG_ADDRESSS, address);

    uint8_t result = io_in_16(PCI_CONFIG_DATA + (offset & 2));

    return result;
}



uint32_t pci_read_config_32(int bus, int device, int function, int offset)
{
    assert(bus >= 0 && bus < 256);
    assert(device >= 0 && device < 32);
    assert(function >= 0 && function < 8);
    assert(offset >= 0 && offset < 256);
    assert(!(offset & 3));

    uint32_t address = PCI_CONFIG_ENABLE | (bus << 16) | (device << 11) | (function << 8) | offset;

    io_out_32(PCI_CONFIG_ADDRESSS, address);

    uint8_t result = io_in_32(PCI_CONFIG_DATA);

    return result;
}

