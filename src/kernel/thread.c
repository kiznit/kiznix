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

#include <kernel/thread.h>
#include <kernel/kernel.h>
#include <kernel/spinlock.h>
#include <kernel/timer.h>
#include <kernel/vmm.h>
#include <kernel/x86/pic.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define THREAD_STACK_SIZE 16384

extern void interrupt_exit();


static volatile uint64_t timer_tick;
static thread_t* volatile current_thread;   // todo: need one per CPU
static thread_t* volatile ready_list;       // Threads ready to run
static thread_t* volatile suspended_list;   // Suspended threads

static thread_t thread0;

static DEFINE_SPINLOCK(scheduler_lock);     // Protects the scheduler and thread lists


// List helpers
static inline void list_append(thread_t* volatile* head, thread_t* thread)
{
    thread->next = NULL;

    if (*head == NULL)
    {
        *head = thread;
    }
    else
    {
        thread_t* last = *head;
        while (last->next != NULL)
            last = last->next;
        last->next = thread;
    }
}


static inline thread_t* list_pop(thread_t* volatile* head)
{
    thread_t* thread = *head;

    if (thread != NULL)
    {
        *head = thread->next;
        thread->next = NULL;
        return thread;
    }
    else
    {
        return NULL;
    }
}



static inline void list_remove(thread_t* volatile* head, thread_t* thread)
{
    thread_t** pp = (thread_t**)head;

    while (*pp != NULL)
    {
        if (*pp == thread)
        {
            *pp = thread->next;
            return;
        }

        pp = &((*pp)->next);
    }
}


static inline int list_size(thread_t* head)
{
    int size = 0;
    for (thread_t* p = head; p != NULL; p = p ->next)
        ++size;
    return size;
}



static int timer_callback(interrupt_context_t* context)
{
    (void)context;

    __sync_fetch_and_add(&timer_tick, 1);   //todo: use atomic_increment()

    //printf("TICK: %d\n", (int)timer_tick);

    thread_yield();

    return 1;
}



void thread_init()
{
    thread0.state = THREAD_RUNNING;
    thread0.stack = NULL;               //todo: allocate a proper stack (with guard pages) for thread 0
    thread0.interrupt_frame = NULL;
    thread0.context = NULL;
    thread0.next = NULL;
    thread0.blocker = NULL;

    current_thread = &thread0;

    timer_init(1000, timer_callback);
}



thread_t* thread_current()
{
    return current_thread;
}



// This is the scheduler
static void thread_schedule()
{
//    printf("%p: LIST SIZES: %d, %d\n", thread_current(), list_size(ready_list), list_size(suspended_list));

    if (!scheduler_lock)
    {
        fatal("%p: thread_schedule() - scheduler not locked!", thread_current());
    }

    if (g_spinLockCount != 1)
    {
        fatal("%p: thread_schedule() - spin lock count is not 1", thread_current());
    }

    if (current_thread->state == THREAD_RUNNING)
    {
        fatal("%p: thread_schedule() - Current thread is running!", thread_current());
    }

    if (interrupt_enabled())
    {
        fatal("%p: thread_schedule() - interrupts are enabled!", thread_current());
    }

    if (ready_list == NULL && current_thread->state == THREAD_READY)
    {
        current_thread->state = THREAD_RUNNING;
        return;
    }

    // Append current thread to ready_list or suspended_list
    thread_t* volatile* list = (current_thread->state == THREAD_READY) ? &ready_list : &suspended_list;
    list_append(list, current_thread);

    // Pop the new thread to run
    thread_t* new_thread = list_pop(&ready_list);
    thread_t* old_thread = current_thread;

    //printf("%p: thread_schedule() - Switching to thread %p (%d -> %d)\n", old_thread, new_thread, old_thread->state, new_thread->state);

    new_thread->state = THREAD_RUNNING;
    current_thread = new_thread;

    int interruptsEnabled = g_interruptsEnabled;
    thread_switch(&old_thread->context, new_thread->context);
    g_interruptsEnabled = interruptsEnabled;
}



void thread_wakeup(thread_t* thread)
{
    spin_lock(&scheduler_lock);

    //printf("%p: thread_wakeup(%p)\n", thread_current(), thread);

    if (thread->state != THREAD_SUSPENDED)
    {
        fatal("%p: thread_wakeup() - Thread isn't suspended! (%d)\n", thread, thread->state);
    }

    thread->state = THREAD_READY;
    thread->blocker = NULL;

    list_remove(&suspended_list, thread);
    list_append(&ready_list, thread);

    spin_unlock(&scheduler_lock);
}



// This code assumes interrupts are disabled, which is the case if we are coming from the timer interrupt
//todo: make sure interrupts are disabled!
void thread_yield()
{
    spin_lock(&scheduler_lock);

    if (current_thread->state != THREAD_RUNNING && current_thread->state != THREAD_SUSPENDED)
    {
        fatal("%p: thread_yield() - Current thread isn't running! (%d)\n", current_thread, current_thread->state);
    }

    if (current_thread->state == THREAD_RUNNING)
    {
        current_thread->state = THREAD_READY;
    }

    thread_schedule();

    spin_unlock(&scheduler_lock);
}



// Entry point for all threads.
static void thread_entry()
{
    printf("%p: thread_entry()\n", thread_current());

    // We got here immediately after a call to switch_context(). This means we
    // still have the scheduler lock and we must release it.
    spin_unlock(&scheduler_lock);

    // IRQ 0 (PIT) is disabled at this point, re-enable it
    pic_enable_irq(0);
}



// Exit point for threads that exit normally (returning from their thread function).
static void thread_exit()
{
    printf("%p: thread_exit()\n", thread_current());

    //todo: kill current thread (i.e. zombify it)
    //todo: yield() / schedule()

    cpu_halt();
}



thread_t* thread_create(thread_function_t user_thread_function)
{
    //todo: don't like this malloc
    thread_t* thread = malloc(sizeof(*thread));

    //todo: proper stack allocation with guard pages
    thread->state = THREAD_READY;
    thread->stack = vmm_alloc(THREAD_STACK_SIZE);


    /*
        We are going to build multiple frames on the stack
    */

    char* stack = thread->stack + THREAD_STACK_SIZE;


    /*
        Setup the last frame to execute thread_exit().
    */

    stack -= sizeof(void*);
    *(void**)stack = thread_exit;


    /*
        Setup an interrupt_context_t frame that returns to the user's thread function.
    */

    stack = stack - sizeof(interrupt_context_t);

    thread->interrupt_frame = (interrupt_context_t*)stack;
    thread->interrupt_frame->cr2 = 0;
    thread->interrupt_frame->cs = 0x08;   //todo: no hard coded constant! use GDT_KERNEL_CODE
    thread->interrupt_frame->ss = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA
    thread->interrupt_frame->ds = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA
    thread->interrupt_frame->es = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA
    thread->interrupt_frame->fs = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA
    thread->interrupt_frame->gs = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA

#if defined(__i386__)
    thread->interrupt_frame->eflags = X86_EFLAGS_IF; // IF = Interrupt Enable
    thread->interrupt_frame->eip = (uintptr_t)user_thread_function;
    thread->interrupt_frame->esp = (uintptr_t)(stack + sizeof(interrupt_context_t));
#elif defined(__x86_64__)
    thread->interrupt_frame->rflags = X86_EFLAGS_IF; // IF = Interrupt Enable
    thread->interrupt_frame->rip = (uintptr_t)user_thread_function;
    thread->interrupt_frame->rsp = (uintptr_t)(stack + sizeof(interrupt_context_t));
#endif


    /*
        Setup a frame so that thread_entry() returns from the scheduler's interrupt.
    */

    stack -= sizeof(void*);
    *(void**)stack = interrupt_exit;


    /*
        Setup a thread_registers_t frame to start execution at thread_entry().
    */

    stack = stack - sizeof(thread_registers_t);
    thread_registers_t* context = (thread_registers_t*)stack;
#if defined(__i386__)
    context->eip = (uintptr_t)thread_entry;
#elif defined(__x86_64__)
    context->rip = (uintptr_t)thread_entry;
#endif
    thread->context = context;


    /*
        Queue this new thread
    */

    assert(interrupt_enabled());
    spin_lock(&scheduler_lock);

    thread->blocker = NULL;

    list_append(&ready_list, thread);

    spin_unlock(&scheduler_lock);

    return thread;
}
