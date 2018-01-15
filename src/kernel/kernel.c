/*
    Copyright (c) 2015, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <kernel/kernel.h>
#include <kernel/console.h>
#include <kernel/interrupt.h>
#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <kernel/pmm.h>
#include <kernel/thread.h>
#include <kernel/vmm.h>
#include <kernel/x86/bios.h>

#include <stdlib.h>


#if defined(KIZNIX_PAE)
#define KIZNIX_ARCH "x86 PAE"
#elif defined(__i386__)
#define KIZNIX_ARCH "x86"
#elif defined(__x86_64__)
#define KIZNIX_ARCH "x86_64"
#else
#error "Unknown architecture"
#endif


void fatal(const char* format, ...)
{
    interrupt_disable();

    printf("Fatal error: ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    cpu_halt();
}



// Very early call to initialize the console and memory
int kernel_early()
{
    console_init();

    printf("Booting Kiznix (" KIZNIX_ARCH ")...\n\n");

    cpu_init();
    pmm_init();
    vmm_init();

    return 0;
}




DEFINE_SPINLOCK(magic);

semaphore_t sem;


void thread1()
{
    printf("This is thread 1 (%p)!\n", thread_current());
    for (;;)
    {
        semaphore_lock(&sem);
        printf("1");
        semaphore_unlock(&sem);
    }
}

void thread2()
{
    printf("This is thread 2 (%p)!\n", thread_current());
    for (;;)
    {
        semaphore_lock(&sem);
        printf("2");
        semaphore_unlock(&sem);
    }
}

void thread3()
{
    printf("This is thread 3 (%p)!\n", thread_current());
    for (;;)
    {
        semaphore_lock(&sem);
        printf("3");
        semaphore_unlock(&sem);
    }
}



// Kernel entry point
void kernel_main()
{
    printf("Hello from kernel_main! (%p)\n\n", kernel_main);

    interrupt_init();

    thread_init();

    //*(int*)KERNEL_HEAP_START = 0;

    //acpi_init();

    //semaphore_init(&sem, 1);
    //thread_create(thread2);
    //thread_create(thread3);
    //thread1();

    vga_set_mode(4);

    printf("kiznix running\n");

    for(;;)
    {
        thread_yield();
    }
}
