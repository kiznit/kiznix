#ifndef _INCLUDED_STDIO_H
#define _INCLUDED_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char*, ...);
int vprintf(const char*, va_list);

int putchar(int);
int puts(const char*);

#ifdef __cplusplus
}
#endif

#endif
