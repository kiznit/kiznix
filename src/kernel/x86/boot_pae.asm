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
global _BootPDPT:
global _BootPageDirectory
global _BootPageTables
global cpu_halt

extern _print_error
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

    ; Check if cpuid is supported
    pushfd                                  ; Push EFLAGS
    pop eax                                 ; Get EFLAGS in EAX
    mov ecx,eax                             ; Store in ECX
    xor eax, 1<<21                          ; Toggle CPUID bit
    push eax                                ; Push new value
    popfd                                   ; Load into EFLAGS
    pushfd                                  ; Push new EFLAGS
    pop eax                                 ; EAX = Post toggled EFLAGS
    push ecx                                ; Save original EFLAGS
    popfd                                   ; Restore original EFLAGS
    xor eax,ecx                             ; XOR original EFLAGS with new value
    jz no_cpuid                             ; If zero flag set then CPUID is not supported

    ; Check if PAE is supported
    xor eax, eax                            ; Get number of CPUID commands
    cpuid                                   ; Query CPU for CPUID commands
    cmp eax, 1                              ; Command 1 supported?
    jb no_pae                               ; PAE not supported
    mov eax, 1                              ; EAX = Processor Info and Feature Bits
    cpuid                                   ; Get CPU features
    test edx, 1 << 6                        ; Test PAE bit
    jz no_pae                               ; Clear, kernel can not load


    ; Initialize the PDPT
    mov edi, _BootPDPT - KERNEL_VIRTUAL_BASE
    mov eax, (_BootPageDirectory - KERNEL_VIRTUAL_BASE) + PAGE_PRESENT
    mov [edi], eax                                      ; Identity map the first 4 MB
    mov [edi + 3 * 8], eax                              ; Map the same 4 MB to high memory

    ; Initialize the page directory (2 MB per entry)
    mov edi, _BootPageDirectory - KERNEL_VIRTUAL_BASE
    mov eax, (_BootPageTables - KERNEL_VIRTUAL_BASE) + PAGE_WRITE + PAGE_PRESENT
    mov [edi], eax                                      ; First page table (First 2 MB)
    add eax, 0x1000
    mov [edi + 8], eax                                  ; Second page table (Next 2 MB)

    ; Initialize the page tables
    mov edi, _BootPageTables - KERNEL_VIRTUAL_BASE
    mov eax, 0x00000000 | PAGE_WRITE | PAGE_PRESENT     ; Start at physical address 0
    mov ecx, 1024                                       ; 1024 entries
.fill_page_table:
    mov [edi], eax
    add eax, 0x1000
    add edi, 8
    loop .fill_page_table

    ; Enable PAE
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    ; Set PDPT
    mov eax, _BootPDPT - KERNEL_VIRTUAL_BASE
    mov cr3, eax

    ; Enable paging
    mov eax, cr0
    bts eax, 31
    mov cr0, eax


    ; Jump to high memory using an absolute jump
    mov ecx, enter_higher_half
    jmp ecx


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Error handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

no_cpuid:
    mov esi, error_no_cpuid - KERNEL_VIRTUAL_BASE
    jmp _print_error

no_pae:
    mov esi, error_no_pae - KERNEL_VIRTUAL_BASE
    jmp _print_error


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
; Constants
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .rodata

error_no_cpuid db "Fatal error: CPUID not supported!",0
error_no_pae db "Fatal error: CPU does not support Physical Address Extension (PAE)!", 0



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Page Mapping Tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .bss nobits
align 0x1000

_BootPageDirectory:
    resb 0x1000

_BootPageTables:
    resb 0x2000

; Page-Directory-Pointer Table
_BootPDPT:
    resb 0x20
