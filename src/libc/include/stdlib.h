#ifndef _INCLUDED_STDLIB_H
#define _INCLUDED_STDLIB_H

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


void abort();

void* malloc(size_t);
void free(void*);
void* calloc(size_t, size_t);
void* realloc(void*, size_t);


#ifdef __cplusplus
}
#endif

#endif
