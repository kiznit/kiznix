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

#ifndef KIZNIX_INCLUDED_KERNEL_SEMAPHORE_H
#define KIZNIX_INCLUDED_KERNEL_SEMAPHORE_H

#include <kernel/spinlock.h>


typedef struct waiter waiter_t;

struct waiter
{
    thread_t* thread;
    waiter_t* next;
};


struct semaphore
{
    spinlock_t  lock;   // Lock protecting the semaphore structure
    int         count;  // Semaphore count
    waiter_t*   head;   // Head of waiters list
    waiter_t*   tail;   // Tail of waiters list
};


void semaphore_init(semaphore_t* semaphore, int initialCount);
void semaphore_lock(semaphore_t* semaphore);
int semaphore_try_lock(semaphore_t* semaphore);
void semaphore_unlock(semaphore_t* semaphore);


#endif
