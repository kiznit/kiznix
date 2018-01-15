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

%include "multiboot.inc"

global _start
global _print_error
global _BootMemoryMapSize
global _BootMemoryMap
global _BootStackBottom
global _BootStackTop

extern _start_kernel


MAX_MEMORY_MAP_ENTRIES equ 256



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Boot Entry Point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[bits 32]
section .boot

align 4
MODULEALIGN equ  1<<0                   ; align loaded modules on page boundaries
MEMINFO     equ  1<<1                   ; provide memory map
FLAGS       equ  MODULEALIGN | MEMINFO  ; this is the Multiboot 'flag' field
MAGIC       equ  0x1BADB002             ; 'magic number' lets bootloader find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum of above, to prove we are multiboot

multiboot_header:
    dd MAGIC
    dd FLAGS
    dd CHECKSUM


align 4

_start:
    ; Initialize stack
    mov esp, _BootStackTop - KERNEL_VIRTUAL_BASE

    ; Copy the memory info before we enable paging
    cmp eax, MULTIBOOT_BOOTLOADER_MAGIC
    jne no_multiboot
    test dword [ebx + multiboot_info.flags], MULTIBOOT_INFO_MEM_MAP
    jz no_memory_map

    mov edx, [ebx + multiboot_info.mmap_length]     ; edx = length of multiboot memory map
    mov esi, [ebx + multiboot_info.mmap_addr]       ; esi = start of multiboot memory map
    add edx, esi                                    ; edx = end of multiboot memory map
    mov edi, _BootMemoryMap - KERNEL_VIRTUAL_BASE   ; edi = _BootMemoryMap
    xor ecx, ecx                                    ; ecx = _BootMemoryMapSize

.memory_map_loop:
    cmp ecx, MAX_MEMORY_MAP_ENTRIES                 ; Max entries reached?
    je .memory_map_done

    cmp esi, edx                                    ; Reached end of memory map?
    jae .memory_map_done

    mov eax, [esi + multiboot_mmap_entry.type]      ; eax = memory type
    cmp eax, MULTIBOOT_MEMORY_AVAILABLE             ; Available memory?
    jne .next_entry                                 ; No...

    ; Copy 'addr' from multiboot memory map to _BootMemoryMap
    mov eax, [esi + multiboot_mmap_entry.addr]
    mov ebx, [esi +  multiboot_mmap_entry.addr + 4]
    mov [edi], eax
    mov [edi + 4], ebx

    ; Copy 'len' from multiboot memory map to _BootMemoryMap
    mov eax, [esi +  multiboot_mmap_entry.len]
    mov ebx, [esi +  multiboot_mmap_entry.len + 4]
    mov [edi + 8], eax
    mov [edi + 12], ebx

    lea edi, [edi + 16]                             ; Next _BootMemoryMap entry
    inc ecx                                         ; _BootMemoryMapSize += 1

.next_entry:
    add esi, dword [esi + multiboot_mmap_entry.size]; Next multiboot memory map entry
    add esi, 4
    jmp .memory_map_loop

.memory_map_done:
    mov [_BootMemoryMapSize - KERNEL_VIRTUAL_BASE], ecx


    ; Initialize FPU
    mov eax, cr0
    btr eax, 1
    bts eax, 4
    mov cr0, eax
    fninit

    jmp _start_kernel



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Error handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

no_multiboot:
    mov esi, error_no_multiboot - KERNEL_VIRTUAL_BASE
    jmp _print_error

no_memory_map:
    mov esi, error_no_memory_map - KERNEL_VIRTUAL_BASE

_print_error:
    lea edi, [0xB8000]
.loop:
    lodsb
    or al,al
    jz .hang
    stosb
    mov al, 0x07
    stosb
    jmp .loop
.hang:
    cli
.hangloop:
    hlt
    jmp .hangloop




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Constants
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .rodata

error_no_multiboot db "Fatal error: no multiboot information!",0
error_no_memory_map db "Fatal error: no memory map in multiboot data!", 0



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Boot Stack
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .bss nobits
align 0x1000

_BootStackBottom:
    resb 0x4000
_BootStackTop:



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Memory Map
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .bss nobits

_BootMemoryMapSize:
    resd 1

_BootMemoryMap:
    resq MAX_MEMORY_MAP_ENTRIES * 2
