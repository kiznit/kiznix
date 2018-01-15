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

#include <kernel/semaphore.h>
#include <kernel/thread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>



void semaphore_init(semaphore_t* semaphore, int initialCount)
{
    assert(initialCount >= 0);

    semaphore->lock = SPINLOCK_UNLOCKED;
    semaphore->count = initialCount;
    semaphore->head = NULL;
    semaphore->tail = NULL;
}



void semaphore_lock(semaphore_t* semaphore)
{
    spin_lock(&semaphore->lock);

    if (semaphore->count > 0)
    {
        // Lock acquired
        --semaphore->count;
        //printf("%p: semaphore_lock() - got lock, count = %d\n", thread_current(), semaphore->count);
        spin_unlock(&semaphore->lock);
    }
    else
    {
        // Blocked - queue current thread and yield
        //printf("%p: semaphore_lock() - blocked\n", thread_current());

        // Append a waiter
        thread_t* thread = thread_current();

        waiter_t waiter;
        waiter.thread = thread;
        waiter.next = NULL;

        if (semaphore->tail == NULL)
        {
            semaphore->head = semaphore->tail = &waiter;
        }
        else
        {
            semaphore->tail->next = &waiter;
            semaphore->tail = &waiter;
        }

        thread->state = THREAD_SUSPENDED;
        thread->blocker = semaphore;

        spin_unlock(&semaphore->lock);

//todo: it's possible we get pre-empted here, and that would mean inefficient scheduling right now

        thread_yield();
    }
}



int semaphore_try_lock(semaphore_t* semaphore)
{
    spin_lock(&semaphore->lock);

    if (semaphore->count > 0)
    {
        // Lock acquired
        --semaphore->count;
        spin_unlock(&semaphore->lock);
        return 1;
    }
    else
    {
        spin_unlock(&semaphore->lock);
        return 0;
    }
}



void semaphore_unlock(semaphore_t* semaphore)
{
    spin_lock(&semaphore->lock);

    if (semaphore->head == NULL)
    {
        // No thread to wakup, increment counter
        ++semaphore->count;
        //printf("%p: semaphore_unlock() - no blocked thread, count = %d\n", thread_current(), semaphore->count);
        spin_unlock(&semaphore->lock);
    }
    else
    {
        // Wake up a waiting thread
        //printf("%p: semaphore_unlock() - waking up thread %p\n", thread_current(), semaphore->head->thread);

        // Pop a waiter
        waiter_t* waiter = semaphore->head;

        semaphore->head = waiter->next;
        if (semaphore->head == NULL)
            semaphore->tail = NULL;

        thread_wakeup(waiter->thread);

        spin_unlock(&semaphore->lock);
    }
}
