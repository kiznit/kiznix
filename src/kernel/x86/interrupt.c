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

#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/x86/pic.h>

#include <assert.h>

#define PIC_IRQ_OFFSET 32

/*
    x86 CPU exceptions

    0   Divide Error                        16  Floating-Point Error
    1   Debug                               17  Alignment Check
    2   NMI                                 18  Machine Check
    3   Breakpoint                          19  SIMD Floating-Point Error
    4   Overflow                            20  Virtualization Exception
    5   BOUND Range Exceeded                21  - Reserved -
    6   Invalid Opcode                      22  - Reserved -
    7   Device Not Available                23  - Reserved -
    8   Double Fault                        24  - Reserved -
    9   - Reserved -                        25  - Reserved -
    10  Invalid TSS                         26  - Reserved -
    11  Segment Not Present                 27  - Reserved -
    12  Stack Fault                         28  - Reserved -
    13  General Protection                  29  - Reserved -
    14  Page Fault                          30  Security Exception (AMD)
    15  - Reserved -                        31   - Reserved -

    The following CPU exceptions will push an error code: 8, 10-14, 17, 30.
*/

#define INTERRUPT_TABLE \
    INTERRUPT(0) INTERRUPT(1) INTERRUPT(2) INTERRUPT(3) INTERRUPT(4) INTERRUPT(5) INTERRUPT(6) INTERRUPT(7)\
    INTERRUPT(8) INTERRUPT_NULL(9) INTERRUPT(10) INTERRUPT(11) INTERRUPT(12) INTERRUPT(13) INTERRUPT(14) INTERRUPT_NULL(15)\
    INTERRUPT(16) INTERRUPT(17) INTERRUPT(18) INTERRUPT(19) INTERRUPT(20) INTERRUPT_NULL(21) INTERRUPT_NULL(22) INTERRUPT_NULL(23)\
    INTERRUPT_NULL(24) INTERRUPT_NULL(25) INTERRUPT_NULL(26) INTERRUPT_NULL(27) INTERRUPT_NULL(28) INTERRUPT_NULL(29) INTERRUPT(30) INTERRUPT_NULL(31)\
    INTERRUPT(32) INTERRUPT(33) INTERRUPT(34) INTERRUPT(35) INTERRUPT(36) INTERRUPT(37) INTERRUPT(38) INTERRUPT(39)\
    INTERRUPT(40) INTERRUPT(41) INTERRUPT(42) INTERRUPT(43) INTERRUPT(44) INTERRUPT(45) INTERRUPT(46) INTERRUPT(47)\
    INTERRUPT(48) INTERRUPT(49) INTERRUPT(50) INTERRUPT(51) INTERRUPT(52) INTERRUPT(53) INTERRUPT(54) INTERRUPT(55)\
    INTERRUPT(56) INTERRUPT(57) INTERRUPT(58) INTERRUPT(59) INTERRUPT(60) INTERRUPT(61) INTERRUPT(62) INTERRUPT(63)\
    INTERRUPT(64) INTERRUPT(65) INTERRUPT(66) INTERRUPT(67) INTERRUPT(68) INTERRUPT(69) INTERRUPT(70) INTERRUPT(71)\
    INTERRUPT(72) INTERRUPT(73) INTERRUPT(74) INTERRUPT(75) INTERRUPT(76) INTERRUPT(77) INTERRUPT(78) INTERRUPT(79)\
    INTERRUPT(80) INTERRUPT(81) INTERRUPT(82) INTERRUPT(83) INTERRUPT(84) INTERRUPT(85) INTERRUPT(86) INTERRUPT(87)\
    INTERRUPT(88) INTERRUPT(89) INTERRUPT(90) INTERRUPT(91) INTERRUPT(92) INTERRUPT(93) INTERRUPT(94) INTERRUPT(95)\
    INTERRUPT(96) INTERRUPT(97) INTERRUPT(98) INTERRUPT(99) INTERRUPT(100) INTERRUPT(101) INTERRUPT(102) INTERRUPT(103)\
    INTERRUPT(104) INTERRUPT(105) INTERRUPT(106) INTERRUPT(107) INTERRUPT(108) INTERRUPT(109) INTERRUPT(110) INTERRUPT(111)\
    INTERRUPT(112) INTERRUPT(113) INTERRUPT(114) INTERRUPT(115) INTERRUPT(116) INTERRUPT(117) INTERRUPT(118) INTERRUPT(119)\
    INTERRUPT(120) INTERRUPT(121) INTERRUPT(122) INTERRUPT(123) INTERRUPT(124) INTERRUPT(125) INTERRUPT(126) INTERRUPT(127)\
    INTERRUPT(128) INTERRUPT(129) INTERRUPT(130) INTERRUPT(131) INTERRUPT(132) INTERRUPT(133) INTERRUPT(134) INTERRUPT(135)\
    INTERRUPT(136) INTERRUPT(137) INTERRUPT(138) INTERRUPT(139) INTERRUPT(140) INTERRUPT(141) INTERRUPT(142) INTERRUPT(143)\
    INTERRUPT(144) INTERRUPT(145) INTERRUPT(146) INTERRUPT(147) INTERRUPT(148) INTERRUPT(149) INTERRUPT(150) INTERRUPT(151)\
    INTERRUPT(152) INTERRUPT(153) INTERRUPT(154) INTERRUPT(155) INTERRUPT(156) INTERRUPT(157) INTERRUPT(158) INTERRUPT(159)\
    INTERRUPT(160) INTERRUPT(161) INTERRUPT(162) INTERRUPT(163) INTERRUPT(164) INTERRUPT(165) INTERRUPT(166) INTERRUPT(167)\
    INTERRUPT(168) INTERRUPT(169) INTERRUPT(170) INTERRUPT(171) INTERRUPT(172) INTERRUPT(173) INTERRUPT(174) INTERRUPT(175)\
    INTERRUPT(176) INTERRUPT(177) INTERRUPT(178) INTERRUPT(179) INTERRUPT(180) INTERRUPT(181) INTERRUPT(182) INTERRUPT(183)\
    INTERRUPT(184) INTERRUPT(185) INTERRUPT(186) INTERRUPT(187) INTERRUPT(188) INTERRUPT(189) INTERRUPT(190) INTERRUPT(191)\
    INTERRUPT(192) INTERRUPT(193) INTERRUPT(194) INTERRUPT(195) INTERRUPT(196) INTERRUPT(197) INTERRUPT(198) INTERRUPT(199)\
    INTERRUPT(200) INTERRUPT(201) INTERRUPT(202) INTERRUPT(203) INTERRUPT(204) INTERRUPT(205) INTERRUPT(206) INTERRUPT(207)\
    INTERRUPT(208) INTERRUPT(209) INTERRUPT(210) INTERRUPT(211) INTERRUPT(212) INTERRUPT(213) INTERRUPT(214) INTERRUPT(215)\
    INTERRUPT(216) INTERRUPT(217) INTERRUPT(218) INTERRUPT(219) INTERRUPT(220) INTERRUPT(221) INTERRUPT(222) INTERRUPT(223)\
    INTERRUPT(224) INTERRUPT(225) INTERRUPT(226) INTERRUPT(227) INTERRUPT(228) INTERRUPT(229) INTERRUPT(230) INTERRUPT(231)\
    INTERRUPT(232) INTERRUPT(233) INTERRUPT(234) INTERRUPT(235) INTERRUPT(236) INTERRUPT(237) INTERRUPT(238) INTERRUPT(239)\
    INTERRUPT(240) INTERRUPT(241) INTERRUPT(242) INTERRUPT(243) INTERRUPT(244) INTERRUPT(245) INTERRUPT(246) INTERRUPT(247)\
    INTERRUPT(248) INTERRUPT(249) INTERRUPT(250) INTERRUPT(251) INTERRUPT(252) INTERRUPT(253) INTERRUPT(254) INTERRUPT(255)


// Defined in interrupt_xx.asm
#define INTERRUPT(    x) extern void interrupt_entry_##x();
#define INTERRUPT_NULL(x)
    INTERRUPT_TABLE
#undef INTERRUPT
#undef INTERRUPT_NULL

#define INTERRUPT(x) interrupt_entry_##x,
#define INTERRUPT_NULL(x) NULL,
static void* interrupt_init_table[256] =
{
    INTERRUPT_TABLE
};
#undef INTERRUPT_NULL
#undef INTERRUPT


typedef struct idt_descriptor idt_descriptor;

struct idt_descriptor
{
    uint16_t offset_low;
    uint16_t selector;
    uint16_t flags;
    uint16_t offset_mid;
#if defined(__x86_64__)
    uint32_t offset_high;
    uint32_t reserved;
#endif
};


typedef struct idt_ptr idt_ptr;

struct idt_ptr
{
    uint16_t size;
    void* address;
} __attribute__((packed));



static idt_descriptor IDT[256] __attribute__((aligned(16)));


static idt_ptr IDT_PTR =
{
    sizeof(IDT)-1,
    IDT
};


static interrupt_handler_t interrupt_handlers[256];



static void idt_set_null(idt_descriptor* descriptor)
{
    memset(descriptor, 0, sizeof(*descriptor));
}



static void idt_set_interrupt_gate(idt_descriptor* descriptor, void* entry)
{
    uintptr_t address = (uintptr_t)entry;

    descriptor->offset_low = address & 0xFFFF;
    descriptor->selector = 0x08;    // GDT_KERNEL_CODE - todo: use a constant
    descriptor->flags = 0x8E00;     // Present + 32 bits + interrupt gate
    descriptor->offset_mid = (address >> 16) & 0xFFFF;

#if defined(__x86_64__)
    descriptor->offset_high = (address >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
#endif
}



void interrupt_init()
{
    printf("interrupt_init()\n");

    // Initialize the interrupt table
    for (int i = 0; i != 256; ++i)
    {
        void* entry = interrupt_init_table[i];
        if (entry)
            idt_set_interrupt_gate(IDT + i, entry);
        else
            idt_set_null(IDT + i);
    }

    // Load IDT
    asm volatile ("lidt %0"::"m" (IDT_PTR));

    // First 32 interrupts are reserved by the CPU, remap PIC
    pic_init(PIC_IRQ_OFFSET);

    // Enable interrupts
    interrupt_enable();
}



int interrupt_register(int interrupt, interrupt_handler_t handler)
{
    assert(interrupt >= 0 && interrupt <= 255);

    if (interrupt_handlers[interrupt])
    {
        printf("interrupt_register(): already have a handler for interrupt %d", interrupt);
        return 0;
    }

    interrupt_handlers[interrupt] = handler;
    return 1;
}



void interrupt_dispatch(interrupt_context_t* context)
{
    // PIC handling
    int irq = context->interrupt - PIC_IRQ_OFFSET;

    if (irq >= 0 && irq <= 15)
    {
        if (!pic_irq_real(irq))
        {
            //printf("Ignoring spurious IRQ %d\n", irq);
            return;
        }

        // Disable this IRQ: we don't want handlers to deal with nested interrupts
        pic_disable_irq(irq);
        //printf("interrupt_dispatch - disabled interrupts\n");

        // Notify the PICs that we handled the interrupt, this unblocks other interrupts
        pic_eoi(irq);
        //printf("interrupt_dispatch - eoi sent\n");
    }

    // Dispatch to interrupt handler
    interrupt_handler_t handler = interrupt_handlers[context->interrupt];

    if (handler)
    {
        handler(context);
    }
    else
    {
        #if defined(__i386__)
        fatal("Unhandled interrupt: %ld, error: %ld, eip: %p", context->interrupt, context->error, (void*)context->eip);
        #elif defined(__x86_64__)
        fatal("Unhandled interrupt: %ld, error: %ld, rip: %p", context->interrupt, context->error, (void*)context->rip);
        #endif
    }


    if (irq >= 0 && irq <= 15)
    {
        // Done handling the interrupt, re-enable it
        pic_enable_irq(irq);
        //printf("interrupt_dispatch - enabled interrupts\n");
    }
}
