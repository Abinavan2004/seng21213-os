#include "semaphore.h"
#include "thread.h"

void semaphore_init(semaphore_t *sem , int value)
{
    sem->value = value;
}

void semaphore_wait(semaphore_t *sem)
{
    int current;

    current = thread_get_current();

    while(1)
    {
        __asm__ __volatile__("cli");

        if(sem->value > 0)
        {
            sem->value--;
            __asm__ __volatile__("sti");
            return;
        }
        thread_set_wait_object(current, sem);
        thread_block(current);

        __asm__ __volatile__("sti");
       __asm__ __volatile__("hlt");
    }
    
}

void semaphore_signal(semaphore_t *sem)
{
    int i;
    __asm__ __volatile__("cli");
    sem->value++;
    for(i = 0; i < MAX_THREADS ; i++)
    {
        if(thread_get_state(i) == THREAD_BLOCKED && thread_get_wait_object(i) == sem)
        {
            thread_wake(i);
            break;
        }
    }
    __asm__ __volatile__("sti");
}