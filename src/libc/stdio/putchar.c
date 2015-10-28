#include <stdio.h>
#include <libc-internals.h>



int putchar(int i)
{
    unsigned char c = (unsigned char) i;
    __kiznix_putc(c);
    return c;
}
