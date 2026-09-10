#ifndef THREAD_H
#define THREAD_H

#include"../include/types.h"

#define MAX_THREADS 8
#define THREAD_STACK_SIZE 4096

typedef enum 
{
    THREAD_UNUSED,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_TERMINATED,
    THREAD_BLOCKED
} thread_state_t ;

typedef void (*thread_entry_t)(void *);

typedef struct
{
    uint32_t tid;
    thread_state_t state;
    uint32_t *stack_pointer;
    thread_entry_t entry;
    void *arg;
    void *wait_object;
} thread_t;

void thread_init(void);
int thread_create(thread_entry_t entry , void *arg);
int thread_has_ready(void);
uint32_t *thread_get_stack(int index);
void thread_set_running(int index);
void thread_set_ready(int index);
void thread_save_stack(int index, uint32_t *sp);
int thread_get_state(int index);
int thread_get_current(void);
void thread_set_current(int index);
int thread_next_ready(void);
void thread_exit(void);
void thread_block(int index);
void thread_wake(int index);
void thread_set_wait_object(int index, void *object);
void *thread_get_wait_object(int index);

#endif