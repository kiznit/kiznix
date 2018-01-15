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

[bits 64]

extern interrupt_dispatch
global interrupt_exit


section .text

%macro INTERRUPT_ENTRY 1
    global interrupt_entry_%+ %1
    interrupt_entry_%+ %1:

        %if !(%1 == 8 || (%1 >= 10 && %1 <= 14) || %1 == 17 || %1 == 30)
            push 0      ; Error code
        %endif

        push %1         ; Interrupt #
        jmp interrupt_entry
%endmacro

%assign interrupt 0
%rep 256
    INTERRUPT_ENTRY interrupt
    %assign interrupt interrupt+1
%endrep


interrupt_entry:

    ; Save interrupt context
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
o16 push gs
o16 push fs
    sub rsp, 4
    mov word [rsp+2], es
    mov word [rsp], ds

    ; Page fault handler needs CR2, others don't...
    mov rax, cr2
    push rax

    ; Sys V ABI requires DF to be clear on function entry
    cld

    mov rbx, rsp        ; Save stack pointer into ebx
    mov rdi, rsp        ; Argument to interrupt_dispatch()
    and rsp, ~15        ; Align stack on 16 bytes as per Sys V ABI
    call interrupt_dispatch
    mov rsp, rbx        ; Restore stack pointer

interrupt_exit:

    ; Restore interrupt context
    add rsp, 8          ; CR2
    mov ds, [rsp]
    mov es, word [rsp+2]
    add rsp, 4
o16 pop fs
o16 pop gs
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; Pop interrupt # and error code
    add rsp, 16

    iretq
