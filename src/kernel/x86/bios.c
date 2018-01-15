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

#include <kernel/x86/bios.h>
#include <kernel/kernel.h>
#include <kernel/vmm.h>
#include <x86emu.h>
#include <stdio.h>

#define BIOS_RAM_START 0x00000400
#define BIOS_RAM_SIZE  0x00000100

#define BIOS_ROM_START 0x000C0000
#define BIOS_ROM_SIZE  0x00040000

#define VBIOS_ROM      0x000c0000
#define VBIOS_ROM_SIZE 0x00010000

#define VBIOS_MEM      0x000a0000
#define VBIOS_MEM_SIZE 0x00010000

#define VBE_BUF        0x8000

typedef struct vbe_mode_info_s {
  unsigned number;      /**< mode number */
  unsigned attributes;      /**< mode attributes */
  unsigned width, height;   /**< mode size */
  unsigned bytes_p_line;    /**< line length */
  unsigned pixel_size;      /**< bits per pixel */
  unsigned fb_start;        /**< frame buffer start address (if any) */
  unsigned win_A_start;     /**< window A start address */
  unsigned win_A_attr;      /**< window A attributes */
  unsigned win_B_start;     /**< window B start address */
  unsigned win_B_attr;      /**< window B attributes */
  unsigned win_size;        /**< window size in bytes */
  unsigned win_gran;        /**< window granularity in bytes */
  unsigned pixel_clock;     /**< maximum pixel clock */
} vbe_mode_info_t;


typedef struct {
  unsigned ok:1;        /**< data are valid */
  unsigned version;     /**< vbe version */
  unsigned oem_version;     /**< oem version info */
  unsigned memory;      /**< in bytes */
  unsigned fb_start;        /**< != 0 if framebuffer is supported */
  char *oem_name;       /**< oem name */
  char *vendor_name;        /**< vendor name */
  char *product_name;       /**< product name */
  char *product_revision;   /**< product revision */
  unsigned modes;       /**< number of supported video modes */
  vbe_mode_info_t *mode;    /**< video mode list */
  unsigned current_mode;    /**< current video mode */
  unsigned ddc_ports;       /**< max ports to probe */
  unsigned char ddc_port[4][0x80];  /**< ddc monitor info per port */
} vbe_info_t;




void vm_write_byte(x86emu_t *emu, unsigned addr, unsigned val, unsigned perm)
{
    x86emu_write_byte_noperm(emu, addr, val);
    x86emu_set_perm(emu, addr, addr, perm | X86EMU_PERM_VALID);
}



void vm_write_word(x86emu_t *emu, unsigned addr, unsigned val, unsigned perm)
{
    x86emu_write_byte_noperm(emu, addr, val);
    x86emu_write_byte_noperm(emu, addr + 1, val >> 8);
    x86emu_set_perm(emu, addr, addr + 1, perm | X86EMU_PERM_VALID);
}



static void* map_mem(x86emu_t* emu, unsigned start, unsigned size, int rw)
{
    (void) emu;
    (void) rw;

    if (!size)
        return NULL;

    void* p = (void*)(ISA_IO_BASE + start);

    printf("[0x%x, %u]: mmap ok\n", start, size);

    return p;
}



static void copy_to_vm(x86emu_t* emu, unsigned dst, unsigned char* src, unsigned size, unsigned perm)
{
    printf("copy_to_vm(%08x <-- %p, %08x)\n", dst, src, size);

    if (!size)
        return;

    while(size--)
        vm_write_byte(emu, dst++, *src++, perm);
}



static void copy_from_vm(x86emu_t* emu, void* dst, unsigned src, unsigned len)
{
    unsigned char *p = dst;
    unsigned u;

    for(u = 0; u < len; ++u)
    {
        p[u] = x86emu_read_byte_noperm(emu, src + u);
    }
}



static int vm_interrupt(x86emu_t* emu, u8 num, unsigned type)
{
    printf("vm_interrupt(%d, %d)\n", num, type);

    if((type & 0xff) == INTR_TYPE_FAULT) {
        printf("IP: %08x\n", emu->x86.R_IP);
        x86emu_stop(emu);
        return 0;
    }

    // ignore ints != 0x10
    if (num != 0x10)
    {
        return 1;
    }

    return 0;
}


static x86emu_t* vm_create()
{
    x86emu_t* emu = x86emu_new(0, X86EMU_PERM_RW);

    x86emu_set_intr_handler(emu, vm_interrupt);


    unsigned char* p1 = map_mem(emu, 0, 0x1000, 0);
    //copy_to_vm(emu, 0x10*4, p1 + 0x10*4, 4, X86EMU_PERM_RW);
    //copy_to_vm(emu, 0x400, p1 + 0x400, 0x100, X86EMU_PERM_RW);
    copy_to_vm(emu, 0, p1, 0x1000, X86EMU_PERM_RW);

/*
    unsigned char* p2 = map_mem(emu, VBIOS_ROM, VBIOS_ROM_SIZE, 0);

    if(!p2 || p2[0] != 0x55 || p2[1] != 0xaa || p2[2] == 0)
    {
        fatal("error: no video bios\n");
    }

    copy_to_vm(emu, VBIOS_ROM, p2, p2[2] * 0x200, X86EMU_PERM_RX);

    printf("video bios: size 0x%04x\n", x86emu_read_byte(emu, BIOS_ROM_START + 2) * 0x200);
    printf("video bios: entry 0x%04x:0x%04x\n",
            x86emu_read_word(emu, 0x10*4 + 2),
            x86emu_read_word(emu, 0x10*4)
    );

    unsigned char* p3 = map_mem(emu, VBIOS_MEM, VBIOS_MEM_SIZE, 0);
    x86emu_set_perm(emu, VBIOS_MEM, VBIOS_MEM + VBIOS_MEM_SIZE - 1, X86EMU_PERM_RW);
    for (unsigned u = 0; u < VBIOS_MEM_SIZE; u += X86EMU_PAGE_SIZE)
    {
        x86emu_set_page(emu, VBIOS_MEM + u, p3 + u);
    }
*/

    x86emu_set_perm(emu, 0x1000, 0xFFFFF, X86EMU_PERM_RWX);
    for (unsigned page = 0x1000; page != 0x100000; page += PAGE_SIZE)
    {
        x86emu_set_page(emu, page, page + ISA_IO_BASE);
    }


    return emu;
}


static void vm_destroy(x86emu_t* emu)
{
    x86emu_done(emu);
}



static int vm_run(x86emu_t* emu)
{
    int err;

    x86emu_reset_access_stats(emu);

    err = x86emu_run(emu, X86EMU_RUN_LOOP | X86EMU_RUN_NO_CODE);

    //x86emu_dump(emu, X86EMU_DUMP_REGS);

    return err;
}



int vga_set_mode(int mode)
{
    x86emu_t* emu = vm_create();

    // Permissions
    x86emu_set_perm(emu, 0x7000, 0x8000, X86EMU_PERM_RWX);

    // CS:IP
    x86emu_set_seg_register(emu, emu->x86.R_CS_SEL, 0);
    emu->x86.R_EIP = 0x7c00;

    // SS:SP
    x86emu_set_seg_register(emu, emu->x86.R_SS_SEL, 0);
    emu->x86.R_ESP = 0x7c00;

    // int 10h params
    emu->x86.R_AH = 0x0F;
    emu->x86.R_AL = 0x00;

    // Instructions
    vm_write_word(emu, 0x7c00, 0x10cd, X86EMU_PERM_RX);     // int 10h
    vm_write_byte(emu, 0x7c02, 0xf4, X86EMU_PERM_RX);       // hlt

/*
    // Get SVGA Info
    emu->x86.R_EAX = 0x4f00;
    emu->x86.R_EBX = 0;
    emu->x86.R_ECX = 0;
    emu->x86.R_EDX = 0;
    emu->x86.R_EDI = VBE_BUF;
    x86emu_write_dword(emu, VBE_BUF, 0x32454256);   // "VBE2"
    x86emu_set_perm(emu, VBE_BUF, 0xffff, X86EMU_PERM_RW);
*/

    // Set SVGA video mode
    emu->x86.R_EAX = 0x4f02;
    emu->x86.R_EBX = 0x117 | (1 << 14);


    int err = vm_run(emu);

    printf("x86emu_run() returned with %d\n", err);
    printf("IP: %08x\n", emu->x86.R_IP);
    printf("AH: %02x\n", emu->x86.R_AH);
    printf("AL: %02x\n", emu->x86.R_AL);
    printf("BH: %02x\n", emu->x86.R_BH);


/*
    printf("=== vbe get info: %s (eax 0x%x)\n",
        emu->x86.R_AX == 0x4f ? "ok" : "failed",
        emu->x86.R_EAX
    );

    vbe_info_t buffer;
    vbe_info_t* vbe = &buffer;
    vbe->ok = 1;

    vbe->version = x86emu_read_word(emu, VBE_BUF + 0x04);
    vbe->oem_version = x86emu_read_word(emu, VBE_BUF + 0x14);
    vbe->memory = x86emu_read_word(emu, VBE_BUF + 0x12) << 16;

    printf("version = %u.%u, oem version = %u.%u\n",
        vbe->version >> 8, vbe->version & 0xff, vbe->oem_version >> 8, vbe->oem_version & 0xff
    );

    printf("memory = %uk\n", vbe->memory >> 10);
*/
    vm_destroy(emu);

    return 0;
}
