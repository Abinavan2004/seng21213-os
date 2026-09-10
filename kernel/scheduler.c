#include "../include/types.h"
#include "thread.h"
#define IDT_ENTRIES 256

typedef struct{

    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
}__attribute__((packed)) idt_entry_t;

typedef struct {

    uint16_t limit;
    uint32_t base;
}__attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

extern void irq0_stub(void);

static void outb(uint16_t port , uint8_t value)
{
   __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}
static void idt_set_gate(int num, uint32_t handler)
{
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = 0x08;
    idt[num].zero = 0;
    idt[num].type_attr = 0x8E;
    idt[num].offset_high = (handler >> 16 ) & 0xFFFF;
     
}

static void idt_init(void)
{
    uint32_t i;

    for(i=0 ; i< IDT_ENTRIES ; i++)
    {
        idt_set_gate(i,0);
    }

    idt_set_gate(32,(uint32_t)irq0_stub);

    idt_ptr.limit = sizeof(idt)-1;
    idt_ptr.base = (uint32_t)&idt;

    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}    
static void pic_remap(void)
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);


    outb(0x21, 0x20);
    outb(0xA1, 0x28);

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);


}
static void pit_init(void)
{
    uint32_t divisor = 1193180 / 100;

    outb(0x43 , 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void scheduler_timer_init(void)
{
    idt_init();
    pic_remap();
    pit_init();
    __asm__ __volatile__("sti");
}
#define MAX_PROCESSES 8

typedef enum{

    PROCESS_UNUSED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED
} process_state_t;

typedef void (*entry_fn_t)(void);

typedef struct {
    uint32_t pid;
    process_state_t state;
    uint32_t *stack_pointer;
    entry_fn_t enter_pointer;
}pcb_t;

extern pcb_t process_table[MAX_PROCESSES];

static int current_process = -1;
typedef enum
{
    SCHED_PROCESS,
    SCHED_THREAD

}sched_entity_type_t;

static sched_entity_type_t current_entity_type = SCHED_PROCESS;

void scheduler_init(void)
{
    current_process=-1;
    thread_init();
}
int scheduler_next(void)
{
int i;
for(i=1; i <= MAX_PROCESSES ; i++)
{
    int index = (current_process+i)% MAX_PROCESSES;
    if(process_table[index].state == PROCESS_READY)
    {
        return index;
    }
}
return -1;
}
static int scheduler_next_thread(void)
{
    return thread_next_ready();

}

int scheduler_schedule(void)
{
    int next_process;
    int next_thread;

    next_process= scheduler_next();
    next_thread= scheduler_next_thread();
    if(current_entity_type == SCHED_PROCESS)
    {
        if(next_thread != -1)
        {
            if(current_process >= 0 && process_table[current_process].state == PROCESS_RUNNING)
            {
                process_table[current_process].state = PROCESS_READY;
            }

            
            thread_set_running(next_thread);
            current_entity_type = SCHED_THREAD;
            return next_thread;
        }
        if(next_process != -1)
        {
            if(current_process >= 0 && process_table[current_process].state == PROCESS_RUNNING)
            {
                process_table[current_process].state = PROCESS_READY;
            }
            current_process = next_process;
            process_table[current_process].state = PROCESS_RUNNING;

            return current_process;
        }
    }
    else 
    {
        
            int old_thread = thread_get_current();

            if(old_thread >= 0 && thread_get_state(old_thread) == THREAD_RUNNING)
            {
                thread_set_ready(old_thread);
            }
        
        if(next_process != -1)
        {
            current_process = next_process;
            process_table[current_process].state = PROCESS_RUNNING;
            current_entity_type = SCHED_PROCESS;

            return current_process;
        }
        if(next_thread != -1)
        {
                thread_set_running(next_thread);
                current_entity_type = SCHED_THREAD;

                return next_thread;
         }
    }
     
    return -1;
}

uint32_t *scheduler_irq0_handler(uint32_t *current_sp)

{
    if(current_entity_type == SCHED_PROCESS)
    {
    if(current_process >= 0 && process_table[current_process].state == PROCESS_RUNNING)
    {
        process_table[current_process].stack_pointer = current_sp;
    }

    }
    else
    {
        int thread_index = thread_get_current();

        if(thread_index >= 0 && thread_get_state(thread_index)!= THREAD_TERMINATED)
        {
            thread_save_stack(thread_index , current_sp);
        }
    }

    scheduler_schedule();

    outb(0x20 , 0x20);

    if(current_entity_type == SCHED_PROCESS)
    {

    if(current_process >= 0)
    {
        return process_table[current_process].stack_pointer;
    }
}
else 
{
    int thread_index = thread_get_current();

    if(thread_index >= 0)
    {
        return thread_get_stack(thread_index);
    }
}
    return current_sp;

    
}