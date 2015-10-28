#ifndef _INCLUDED_LIBC_INTERNALS_H
#define _INCLUDED_LIBC_INTERNALS_H


#define WEAK_FUNCTION __attribute__ ((weak))

#ifdef __cplusplus
extern "C" {
#endif

void __kiznix_putc(unsigned char c);
WEAK_FUNCTION int __kiznix_print(const char* string);


#ifdef __cplusplus
}
#endif

#endif
