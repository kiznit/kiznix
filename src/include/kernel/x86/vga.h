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

#ifndef KIZNIX_INCLUDED_KERNEL_X86_VGA_H
#define KIZNIX_INCLUDED_KERNEL_X86_VGA_H

//todo: vga shouldn't be under 'x86'
#include <kernel/vmm.h>
#include <kernel/x86/io.h>


static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

static uint16_t* const VGA_MEMORY = (uint16_t*)(ISA_IO_BASE + 0x000B8000);


enum vga_color
{
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};


static inline uint8_t vga_make_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}


static inline uint16_t vga_make_entry(char c, uint8_t color)
{
    uint16_t c16 = c;
    uint16_t color16 = color;
    return c16 | color16 << 8;
}


static inline vga_move_cursor(int x, int y)
{
   uint16_t cursorLocation = y * VGA_WIDTH + x;
   io_out_8(0x3D4, 14);
   io_out_8(0x3D5, cursorLocation >> 8);
   io_out_8(0x3D4, 15);
   io_out_8(0x3D5, cursorLocation);
}


/* Make the cursor a solid block */
static inline vga_solid_cursor()
{
    io_out_8(0x3d4, 0x0a);
    io_out_8(0x3d5, 0x00);
}


static inline vga_hide_cursor()
{
    io_out_8(0x3d4, 0x200a);
    io_out_8(0x3d4, 0x000b);
}


#endif
