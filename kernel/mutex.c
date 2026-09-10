#include "mutex.h"
#include "thread.h"

void mutex_init(mutex_t *mutex)
{
    mutex->locked = 0;
    mutex->owner = -1;
}

void mutex_lock(mutex_t *mutex)
{
    int current;

    current = thread_get_current();

    while(1)
    {
        __asm__ __volatile__("cli");
        if(!mutex->locked)
        {
            mutex->locked = 1;
            mutex->owner = current;
            __asm__ __volatile__("sti");
            return;
        }
        thread_set_wait_object(current,mutex);
       thread_block(current);
       __asm__ __volatile__("sti");
       __asm__ __volatile__("hlt");
    
    }
    
}

void mutex_unlock(mutex_t *mutex)
{
    int i;
    int current;
    current = thread_get_current();
    __asm__ __volatile__("cli");

    if(mutex->owner != current)
    {
        __asm__ __volatile__("sti");
        return;
    }

    mutex->locked = 0;
    mutex->owner = -1;

    for(i=0; i < MAX_THREADS; i++)
    {
        if(thread_get_state(i) == THREAD_BLOCKED &&  thread_get_wait_object(i) == mutex)
        {
            thread_wake(i);
            break;
        }
    }
    __asm__ __volatile__("sti");
}