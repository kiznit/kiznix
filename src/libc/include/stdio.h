#ifndef _INCLUDED_STDIO_H
#define _INCLUDED_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EOF (-1)

int printf(const char* format, ...);
int snprintf(char* buffer, size_t size, const char* format, ...);
int vsnprintf(char* buffer, size_t size, const char* format, va_list args);

int putchar(int c);
int puts(const char* string);


int __kiznix_print(const char* string);
void __kiznix_putc(unsigned char c);


#ifdef __cplusplus
}
#endif

#endif
