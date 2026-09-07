#include "../include/types.h"
#define MAX_PROCESSES 8
#define PROCESS_STACK_SIZE 4096

typedef enum {
    PROCESS_UNUSED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED
}process_state_t;

typedef void (*entry_fn_t)(void);

typedef struct{
    uint32_t pid;
    process_state_t state;
    uint32_t *stack_pointer;
    entry_fn_t enter_pointer;
} pcb_t;

pcb_t process_table[MAX_PROCESSES];
uint8_t process_stacks[MAX_PROCESSES][PROCESS_STACK_SIZE];
static uint32_t next_pid = 1;

void process_init(void)
{
    uint32_t i;
    for(i=0 ; i < MAX_PROCESSES; i++)
    {
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_UNUSED;
        process_table[i].stack_pointer = 0;
        process_table[i].enter_pointer = 0;
    }

    next_pid = 1;
}

int create_process(entry_fn_t entry_fn)
{
    uint32_t i;
    for( i=0 ; i<MAX_PROCESSES; i++)
    {
        if(process_table[i].state == PROCESS_UNUSED)
        {
            break;
        }
    }
    if( i == MAX_PROCESSES)
    {
        return -1;
    }

    process_table[i].pid = next_pid++;
    process_table[i].state = PROCESS_READY;
    process_table[i].enter_pointer = entry_fn;

    uint32_t *stack = (uint32_t *)&process_stacks[i][PROCESS_STACK_SIZE];
    *(--stack) = 0x202;
    *(--stack) = 0x08;
    *(--stack) = (uint32_t)entry_fn;

    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;
    *(--stack) =0;

    process_table[i].stack_pointer = stack ;
    return (int)process_table[i].pid;
}

int process_get_info(int index, uint32_t *pid , int *state)
{
    if(index < 0 || index >= MAX_PROCESSES)
    { return -1;}

    *pid = process_table[index].pid;
    *state = (int)process_table[index].state;

    return 0;
}

