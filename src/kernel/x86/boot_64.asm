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
global _BootPML4:
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

    ; Check if long mode is supported
    mov eax, 0x80000000                     ; Get number of extended CPUID commands
    cpuid                                   ; Query CPU for CPUID extensions
    cmp eax, 0x80000001                     ; No CPUID extensions
    jb no_long_mode                         ; No long mode support
    mov eax, 0x80000001                     ; EAX = Get extended features
    cpuid                                   ; Get CPU features
    test edx, 1 << 29                       ; Test long mode bit
    jz no_long_mode                         ; Clear, kernel can not load


    ; Initialize the PML4
    mov edi, _BootPML4 - KERNEL_VIRTUAL_BASE
    mov eax, (_BootPDPT - KERNEL_VIRTUAL_BASE) + PAGE_WRITE + PAGE_PRESENT
    mov [edi], eax                                      ; Identity map the first 4 MB
    mov [edi + 511 * 8], eax                            ; Map the same 4 MB to high memory

    ; Initialize the PDPT
    mov edi, _BootPDPT - KERNEL_VIRTUAL_BASE
    mov eax, (_BootPageDirectory - KERNEL_VIRTUAL_BASE) + PAGE_WRITE + PAGE_PRESENT
    mov [edi], eax                                      ; Identity map the first 4 MB
    mov [edi + 511 * 8], eax                            ; Map the same 4 MB to high memory

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

    ; Set PML4
    mov eax, _BootPML4 - KERNEL_VIRTUAL_BASE
    mov cr3, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    bts eax, 8
    wrmsr

    ; Enable paging and enter long mode
    mov eax, cr0
    bts eax, 31
    mov cr0, eax

    ; Enable SSE
    mov eax, cr0
    bts eax, 1
    btr eax, 2
    mov cr0, eax
    mov eax, cr4
    or eax, 0x600
    mov cr4, eax

    ; Load temporary GDT
    lgdt [GDTR - KERNEL_VIRTUAL_BASE]

    ; Load segments
    mov eax, GDT_BOOT_DATA
    mov ds, eax
    mov ss, eax

    ; Far jump into long mode, note that it is impossible to jump to a 64-bit RIP,
    ; so jump to identity mapped RIP in lower mem
    jmp GDT_BOOT_CODE:(enter_long_mode - KERNEL_VIRTUAL_BASE)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Error handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

no_cpuid:
    mov esi, error_no_cpuid - KERNEL_VIRTUAL_BASE
    jmp _print_error

no_long_mode:
    mov esi, error_no_long_mode - KERNEL_VIRTUAL_BASE
    jmp _print_error


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; High Memory Entry Point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[bits 64]
section .text
align 4

enter_long_mode:
    mov rcx, enter_higher_half              ; ECX => Next instruction in virtual memory
    jmp rcx                                 ; Jump to higher half in virtual memory

enter_higher_half:
    ; Initialize stack
    mov rsp, _BootStackTop

    ; Kernel early initialization
    call kernel_early
    or rax, rax
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
; GDT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .rodata
align 16

GDT_BOOT_NULL equ gdt_boot_null - GDT
GDT_BOOT_CODE equ gdt_boot_code - GDT
GDT_BOOT_DATA equ gdt_boot_data - GDT

GDT:
gdt_boot_null:
    dq 0

gdt_boot_code:
    dw 0
    dw 0
    db 0
    db 10011010b    ; P + DPL 0 + S + Code + Execute + Read
    db 00100000b    ; Long mode
    db 0

gdt_boot_data:
    dw 0
    dw 0
    db 0
    db 10010010b    ; P + DPL 0 + S + Data + Read + Write
    db 00000000b
    db 0

GDTR:
    dw GDTR - GDT - 1
    dq GDT - KERNEL_VIRTUAL_BASE



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Constants
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .rodata

error_no_cpuid db "Fatal error: CPUID not supported!",0
error_no_long_mode db "Fatal error: CPU does not support 64 bits mode!", 0



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Page Mapping Tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .bss
align 0x1000

; Page Map Level 4
_BootPML4:
    resb 0x1000

; Page-Directory-Pointer Table
_BootPDPT:
    resb 0x1000

_BootPageDirectory:
    resb 0x1000

_BootPageTables:
    resb 0x2000
