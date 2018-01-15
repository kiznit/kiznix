#include <assert.h>
#include <kernel/kernel.h>



void __assert(const char* expression, const char* file, int line)
{
    fatal("Assertion failed: %s at %s, line %d", expression, file, line);
}
