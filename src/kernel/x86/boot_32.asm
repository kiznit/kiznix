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

global _start_kernel
global _BootPageDirectory
global _BootPageTable
global cpu_halt

extern kernel_early
extern kernel_main
extern _BootStackTop

PAGE_WRITE      equ 0x02
PAGE_PRESENT    equ 0x01



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Start the kernel
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[bits 32]
section .boot

_start_kernel:

    ; Initialize the page directory (4 MB per entry)
    mov edi, _BootPageDirectory - KERNEL_VIRTUAL_BASE
    mov eax, (_BootPageTable - KERNEL_VIRTUAL_BASE) + PAGE_WRITE + PAGE_PRESENT
    mov [edi], eax                                      ; Identity map the first 4 MB
    mov [edi + (KERNEL_VIRTUAL_BASE >> 22) * 4], eax    ; Map the same 4 MB to high memory

    ; Initialize the page table
    mov edi, _BootPageTable - KERNEL_VIRTUAL_BASE
    mov eax, 0x00000000 | PAGE_WRITE | PAGE_PRESENT     ; Start at physical address 0
    mov ecx, 1024                                       ; 1024 entries
.fill_page_table:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .fill_page_table

    ; Load Page Directory Base Register.
    mov eax, _BootPageDirectory - KERNEL_VIRTUAL_BASE
    mov cr3, eax

    ; Enable paging
    mov eax, cr0
    bts eax, 31
    mov cr0, eax

    ; Jump to high memory using an absolute jump
    mov ecx, enter_higher_half
    jmp ecx


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; High Memory Entry Point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .text
align 4

enter_higher_half:
    ; Initialize stack
    mov esp, _BootStackTop

    ; Kernel early initialization
    call kernel_early
    or eax,eax
    jnz cpu_halt

    ; Transfer control to kernel
    call kernel_main

    ; Hang if kernel unexpectedly returns.
cpu_halt:
    cli
.hangloop:
    hlt
    jmp .hangloop



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Page Mapping Tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .bss nobits
align 0x1000

_BootPageDirectory:
    resb 0x1000

_BootPageTable:
    resb 0x1000
