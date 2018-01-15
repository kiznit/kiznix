#ifndef _INCLUDED_SYS_IO_H
#define _INCLUDED_SYS_IO_H

#include <kernel/x86/io.h>



#ifdef __cplusplus
extern "C" {
#endif

#define inb io_in_8
#define inw io_in_16
#define inl io_in_32

#define outb io_out_8
#define outw io_out_16
#define outl io_out_32

#ifdef __cplusplus
}
#endif



#endif
