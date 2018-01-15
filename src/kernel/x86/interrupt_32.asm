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

[bits 32]

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
    push ebp
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax
o16 push gs
o16 push fs
o16 push es
o16 push ds

    ; Page fault handler needs CR2, others don't...
    mov eax, cr2
    push eax

    ; Sys V ABI requires DF to be clear on function entry
    cld

    push esp            ; Argument to interrupt_dispatch()
    call interrupt_dispatch
    add  esp, 4         ; Pop arguments

interrupt_exit:

    ; Restore interrupt context
    add esp, 4          ; CR2
o16 pop ds
o16 pop es
o16 pop fs
o16 pop gs
    pop eax
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi
    pop ebp

    ; Pop interrupt # and error code
    add esp, 8

    iret
