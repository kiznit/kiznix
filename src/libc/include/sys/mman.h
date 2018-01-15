#ifndef _INCLUDED_SYS_MMAN_H
#define _INCLUDED_SYS_MMAN_H

#include <sys/types.h>



#ifdef __cplusplus
extern "C" {
#endif


#define PROT_NONE  0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 4

#define MAP_FAILED ((void*)-1)


void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void* addr, size_t length);


#ifdef __cplusplus
}
#endif



#endif
