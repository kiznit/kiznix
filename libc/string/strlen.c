#include <string.h>


size_t strlen(const char* string)
{
    size_t result = 0;

    for (const char* p = string; *p; ++p)
    {
        ++result;
    }

    return result;
}
