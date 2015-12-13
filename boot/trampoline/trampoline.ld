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

OUTPUT_FORMAT(elf32-i386)
ENTRY(trampoline_main)


PHDRS
{
    segment_text PT_LOAD;
    segment_rodata PT_LOAD;
    segment_data PT_LOAD;
    segment_dynamic PT_DYNAMIC;
}


SECTIONS
{
    . = 0;

    .text BLOCK(4K) :
    {
        *(.text)
    } : segment_text

    .rodata BLOCK(4K) :
    {
        *(.rodata*)
    } : segment_rodata

    .data BLOCK(4K) :
    {
        *(.data*)
    } : segment_data

    .bss :
    {
        *(.bss)
    } : segment_data

    .dynamic :
    {
      *(.dynamic)
    } : segment_dynamic

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}

/*

architecture: i386, flags 0x00000150:
HAS_SYMS, DYNAMIC, D_PAGED
start address 0x00000000


Contents of section .text:
 0000 e81f0000 0081c173 01000031 c08b91fc  .......s...1....
 0010 ffffff90 88041040 83f86475 f78d81b0  .......@..du....
 0020 feffffc3                             ....
Contents of section .text.__x86.get_pc_thunk.cx:
 0024 8b0c24c3                             ..$.
Contents of section .rodata.str1.1:
 0028 61626364 656600                      abcdef.
Contents of section .eh_frame:
 0030 14000000 00000000 017a5200 017c0801  .........zR..|..
 0040 1b0c0404 88010000 10000000 1c000000  ................
 0050 b0ffffff 24000000 00000000 10000000  ....$...........
 0060 30000000 c0ffffff 04000000 00000000  0...............
Contents of section .dynsym:
 0070 00000000 00000000 00000000 00000000  ................
 0080 16000000 f8000000 0a000000 11000900  ................
 0090 01000000 00000000 24000000 12000100  ........$.......
 00a0 11000000 a0010000 64000000 11000d00  ........d.......
Contents of section .dynstr:
 00b0 00747261 6d706f6c 696e655f 6d61696e  .trampoline_main
 00c0 00646174 61006461 74613200           .data.data2.
Contents of section .hash:
 00cc 03000000 04000000 00000000 01000000  ................
 00dc 03000000 00000000 00000000 00000000  ................
 00ec 02000000                             ....
Contents of section .rel.dyn:
 00f0 74010000 06030000                    t.......
Contents of section .data:
 00f8 01020304 05060708 090a               ..........
Contents of section .dynamic:
 0104 04000000 cc000000 05000000 b0000000  ................
 0114 06000000 70000000 0a000000 1c000000  ....p...........
 0124 0b000000 10000000 11000000 f0000000  ................
 0134 12000000 08000000 13000000 08000000  ................
 0144 00000000 00000000 00000000 00000000  ................
 0154 00000000 00000000 00000000 00000000  ................
 0164 00000000 00000000 00000000 00000000  ................
Contents of section .got:
 0174 00000000                             ....
Contents of section .got.plt:
 0178 04010000 00000000 00000000           ............
Contents of section .comment:
 0000 4743433a 2028474e 55292034 2e382e33  GCC: (GNU) 4.8.3
 0010 00


Program Header:
    LOAD off    0x00001000 vaddr 0x00000000 paddr 0x00000000 align 2**12
         filesz 0x00000184 memsz 0x00000204 flags rwx
 DYNAMIC off    0x00001104 vaddr 0x00000104 paddr 0x00000104 align 2**2
         filesz 0x00000070 memsz 0x00000070 flags rw-

Dynamic Section:
  HASH                 0x000000cc
  STRTAB               0x000000b0
  SYMTAB               0x00000070
  STRSZ                0x0000001c
  SYMENT               0x00000010
  REL                  0x000000f0
  RELSZ                0x00000008
  RELENT               0x00000008
*/
