#include <stdlib.h>

#include <kernel/kernel.h>


void abort()
{
    fatal("LIBC - abort() called");
}
