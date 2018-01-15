#include <stdio.h>

#include <kernel/console.h>



int putchar(int c)
{
    console_putchar(c);
    return (unsigned char) c;
}
