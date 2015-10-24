#include <stdio.h>


int puts(const char* string) __attribute__ ((weak, alias ("__kiznix_puts")));



int __kiznix_puts(const char* string)
{
    int result = __kiznix_print(string);
    __kiznix_putc('\n');
    return result;
}
