
#include "../include/types.h"
#include "thread.h"
extern void thread_bootstrap(void);
#define MAX_THREADS 8
#define THREAD_STACK_SIZE 4096

static thread_t thread_table[MAX_THREADS];
static uint32_t thread_stacks[MAX_THREADS][THREAD_STACK_SIZE / sizeof(uint32_t)];

static uint32_t next_tid = 1;
static int current_thread = -1;

void thread_init(void)
{
    int i;

    for(i=0 ; i< MAX_THREADS ; i++)
    {
        thread_table[i].tid = 0;
        thread_table[i].state= THREAD_UNUSED;
        thread_table[i].stack_pointer = NULL;
        thread_table[i].entry = NULL;
        thread_table[i].arg = NULL;
        thread_table[i].wait_object = NULL;

    }
    next_tid = 1;
}

int thread_create(thread_entry_t entry, void *arg)
{
    int i;
    if(entry == NULL)
    {
        return -1;
    }
    for(i=0 ; i< MAX_THREADS; i++)
    {
        if(thread_table[i].state == THREAD_UNUSED)
        break;
    }

    if(i== MAX_THREADS)
    return -1;
  
     thread_table[i].tid = next_tid++;
        thread_table[i].state= THREAD_READY;
        
        thread_table[i].entry = entry;
        thread_table[i].arg = arg;



uint32_t *stack = &thread_stacks[i][THREAD_STACK_SIZE/sizeof(uint32_t)];

    *(--stack) =  (uint32_t)arg;
        *(--stack) = (uint32_t)entry;

 *(--stack) = 0x202;
    *(--stack) = 0x08;
    *(--stack) = (uint32_t)entry;

    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;

    thread_table[i].stack_pointer = stack;
    return (int)thread_table[i].tid;
}
int thread_has_ready(void)
{
    int i;
    for(i=0 ; i < MAX_THREADS ; i++)
    {
        if(thread_table[i].state == THREAD_READY)
        {
           return 1;
        }
    }
    return 0;
}

uint32_t *thread_get_stack(int index)
{
    if(index < 0 || index >= MAX_THREADS)
    {
        return NULL;
    }
    return thread_table[index].stack_pointer;
}

void thread_set_running(int index)
{
    if(index >=0 && index < MAX_THREADS)
    {
        thread_table[index].state = THREAD_RUNNING;
        current_thread = index;
    }
}
 void thread_set_ready(int index)
{
    if(index >=0 && index < MAX_THREADS)
    {
        if(thread_table[index].state == THREAD_RUNNING)
          {
            thread_table[index].state =THREAD_READY;
          }
    }
}
void thread_save_stack(int index , uint32_t *sp)
{
    if(index >= 0 && index < MAX_THREADS)
    {
        thread_table[index].stack_pointer = sp;
    }
}
int thread_get_state(int index)
{
    if(index < 0 || index >= MAX_THREADS)
    {
        return THREAD_UNUSED;
    }
     return (int)thread_table[index].state;
}

int thread_get_current(void)
{
    return current_thread;
}
 

void thread_set_current(int index)
{
    if(index >=0 && index < MAX_THREADS)
    {
        current_thread = index;
    }
}

int thread_next_ready(void)
{
    int i;
    int current;
    int index;

    current = thread_get_current();
    for(i=0 ; i< MAX_THREADS ; i++)
    {
        index = (current + i)%MAX_THREADS;
        if(thread_table[index].state == THREAD_READY)
        {
            return index;
        }
    }
    return -1;
}

void thread_exit(void)
{
    if(current_thread >= 0 && current_thread < MAX_THREADS)
    {
        thread_table[current_thread].state = THREAD_TERMINATED;
    }
}

void thread_block(int index)
{
    if(index >= 0 && index < MAX_THREADS)
    {
        thread_table[index].state = THREAD_BLOCKED;
    }
}
 void thread_wake(int index)
 {
    if(index >= 0 && index < MAX_THREADS)
    {
        if(thread_table[index].state == THREAD_BLOCKED)
        {
            thread_table[index].state = THREAD_READY;
            thread_table[index].wait_object = NULL;
        }
    }
 }

 void thread_set_wait_object(int index, void *object)
 {
    if(index >= 0 && index < MAX_THREADS)
    {
        thread_table[index].wait_object = object;
    }
 }

 void *thread_get_wait_object(int index)
 {
    if(index < 0 || index >= MAX_THREADS)
    {
        return NULL;
    }
    return thread_table[index].wait_object;
 }