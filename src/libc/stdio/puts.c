#include <stdio.h>


int puts(const char* string)
{
    const char* p = string;
    for (; *p; ++p)
    {
        putchar(*p);
    }

    putchar('\n');

    return p - string + 1;
}
