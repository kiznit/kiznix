#include <sys/mman.h>
#include <errno.h>
#include <kernel/kernel.h>
#include <kernel/vmm.h>



void* mmap(void* address, size_t length, int prot, int flags, int fd, off_t offset)
{
    (void) address;
    (void) prot;
    //printf("mmap() called: %p %ld %d %d %d %d\n", address, length, prot, flags, fd, (int)offset);

    if (length == 0)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    if ((flags & MAP_ANONYMOUS) && fd != -1)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    if (fd != -1)
    {
        (void)offset;
        fatal("mmap() doesn't support fd != -1");
    }

    //todo: handle prot

    //todo: handle flags


    void* memory = vmm_alloc(length);

    if (!memory)
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    //printf("mmap() returning %p\n", memory);

    return memory;
}



int munmap(void* address, size_t length)
{
    (void)address;
    (void)length;

    errno = EINVAL;
    return -1;
}
