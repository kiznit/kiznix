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

#include <kernel/spinlock.h>
#include <kernel/interrupt.h>
#include <kernel/thread.h>
#include <assert.h>
#include <stdio.h>
#include <xmmintrin.h>


//todo: move to a CPU object
int g_spinLockCount;        // todo: temp hack, does not belong here
int g_interruptsEnabled;    // todo: temp hack, does not belong here



void spin_lock(volatile spinlock_t* lock)
{
    int interruptsEnabled = interrupt_enabled();

    interrupt_disable();

    //printf("spin_lock: %p\n", lock);

    if (g_spinLockCount++ == 0)
    {
        g_interruptsEnabled = interruptsEnabled;
    }

    while (__sync_lock_test_and_set(lock, 1))
    {
        while (*lock)
        {
            _mm_pause();
        }
    }
}


void spin_unlock(volatile spinlock_t* lock)
{
    //printf("spin_unlock: %p\n", lock);

    assert(!interrupt_enabled());

    assert(g_spinLockCount > 0);

    __sync_lock_release(lock);

    if (--g_spinLockCount == 0 && g_interruptsEnabled)
    {
        interrupt_enable();
    }
}
