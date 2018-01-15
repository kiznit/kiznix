#include <stdio.h>

#include <kernel/console.h>



int puts(const char* string)
{
    console_writestring(string);
    console_putchar('\n');
    return 1;
}
