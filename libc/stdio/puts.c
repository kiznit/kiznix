#include <stdio.h>
#include <libc-internals.h>



int puts(const char* string)
{
    int result = __kiznix_print(string);
    __kiznix_putc('\n');
    return result;
}
