#include <string.h>



/*
int memcmp(const void* aptr, const void* bptr, size_t size)
{
    const unsigned char* a = (const unsigned char*) aptr;
    const unsigned char* b = (const unsigned char*) bptr;
    for ( size_t i = 0; i < size; i++ )
        if ( a[i] < b[i] )
            return -1;
        else if ( b[i] < a[i] )
            return 1;
    return 0;
}
*/



void* memcpy(void* restrict dest_, const void* src_, size_t length)
{
    char* src = (char*)src_;
    char* dest = (char*)dest_;

    for (size_t i = length; i != 0; --i)
    {
        *dest++ = *src++;
    }

    return dest;
}


/*
void* memmove(void* dstptr, const void* srcptr, size_t size)
{
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;
    if ( dst < src )
        for ( size_t i = 0; i < size; i++ )
            dst[i] = src[i];
    else
        for ( size_t i = size; i != 0; i-- )
            dst[i-1] = src[i-1];
    return dstptr;
}
*/



void* memset(void* dest_, int value, size_t length)
{
    char* dest = (char*)dest_;

    for (size_t i = length; i != 0; --i)
    {
        *dest++ = (char)value;
    }

    return dest;
}


/*
void* memchr(const void* memory, int character, size_t size)
{
    const char c = character;
    for (const char* p = memory; size != 0; ++p, --size)
    {
        if (*p == c)
            return (void*)p;
    }
    return NULL;
}
*/


char* strchr(const char* string, int character)
{
    const char c = character;
    for (const char* p = string; *p; ++p)
    {
        if (*p == c)
            return (char*)p;
    }
    return NULL;
}


size_t strlen(const char* string)
{
    size_t result = 0;
    while ( string[result] )
        result++;
    return result;
}
