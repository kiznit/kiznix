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

#ifndef KIZNIX_INCLUDED_KERNEL_THREAD_H
#define KIZNIX_INCLUDED_KERNEL_THREAD_H

#include <kernel/defs.h>
#include <kernel/interrupt.h>

typedef void (*thread_function_t)();


typedef struct thread_registers thread_registers_t;
typedef enum thread_state thread_state_t;


enum thread_state
{
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_SUSPENDED,
};


#if defined(__i386__)

struct thread_registers
{
    uint32_t ebx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eip;
};

#elif defined(__x86_64__)

struct thread_registers
{
    uint64_t ebx;
    uint64_t ebp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
};

#endif



struct thread
{
    thread_state_t          state;              // Scheduling state
    char*                   stack;              // Kernel stack
    interrupt_context_t*    interrupt_frame;    // Interrupt frame
    thread_registers_t*     context;            // Saved context (on the thread's stack)

    thread_t*               next;               // Next thread in list
    semaphore_t*            blocker;            // What's blocking this thread
};


// Initialize scheduler
void thread_init();

// Create a new thread
thread_t* thread_create(thread_function_t user_thread_function);

// Retrieve the currently running thread
thread_t* thread_current();

// Thread context switch
void thread_switch(thread_registers_t** old, thread_registers_t* new);

// Suspend a thread
void thread_suspend(semaphore_t* semaphore);

// Wake up a thread
void thread_wakeup(thread_t* thread);

// Yield the CPU to another thread
void thread_yield();


#endif

