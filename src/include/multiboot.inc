; Copyright (c) 2015, Thierry Tremblay
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;
; * Redistributions of source code must retain the above copyright notice, this
;   list of conditions and the following disclaimer.
;
; * Redistributions in binary form must reproduce the above copyright notice,
;   this list of conditions and the following disclaimer in the documentation
;   and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

%ifndef KIZNIX_INCLUDED_MULTIBOOT_INC
%define KIZNIX_INCLUDED_MULTIBOOT_INC



MULTIBOOT_BOOTLOADER_MAGIC equ 0x2BADB002
MULTIBOOT_INFO_MEM_MAP equ 0x00000040

struc multiboot_info

    ; Multiboot info flags
    .flags:             resd 1

    ; Available memory from BIOS
    .mem_lower:         resd 1
    .mem_upper:         resd 1

    ; "root" partition
    .boot_device:       resd 1

    ; Kernel command line
    .cmdline:           resd 1

    ; Boot-Module list
    .mods_count:        resd 1
    .mods_addr:         resd 1

    ; The symbol table for a.out. / The section header table for ELF.
    .aout_tabsize:
    .elf_num:           resd 1
    .aout_strsize:
    .elf_size:          resd 1
    .aout_addr:
    .elf_addr:          resd 1
    .aout_reserved:
    .elf_shndx:         resd 1

    ; Memory Mapping buffer
    .mmap_length:       resd 1
    .mmap_addr:         resd 1

    ; Drive Info buffer
    .drives_length:     resd 1
    .drives_addr:       resd 1

    ; ROM configuration table
    .config_table:      resd 1

    ; Boot Loader Name
    .boot_loader_name:  resd 1

    ; APM table
    .apm_table:         resd 1

    ; Video
    .vbe_control_info:  resd 1
    .vbe_mode_info:     resd 1
    .vbe_mode:          resw 1
    .vbe_interface_seg: resw 1
    .vbe_interface_off: resw 1
    .vbe_interface_len: resw 1

endstruc


MULTIBOOT_MEMORY_AVAILABLE equ 1

struc multiboot_mmap_entry
    .size:  resd 1
    .addr:  resq 1
    .len:   resq 1
    .type:  resd 1
endstruc



%endif
