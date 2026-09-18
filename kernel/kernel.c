/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stage 0 – Foundations)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   This is the heart of your operating system. Right now it:
 *     1. Initialises VGA text-mode display
 *     2. Initialises the keyboard driver
 *     3. Prints a splash screen
 *     4. Runs a minimal interactive shell ("ksh")
 *
 * ASSIGNMENT MILESTONES  (what YOU will add in later lectures)
 *   Lecture  9  – Process Management  →  process.h / process.c / scheduler.c
 *   Lecture 10  – Threads             →  thread.h  / thread.c
 *   Lecture 11  – Memory Management   →  pmm.h     / pmm.c / vmm.c
 *   Lecture 12  – File System         →  fs.h      / fs.c
 *
 * CODING CONVENTION
 *   - Prefix kernel-internal functions with k_ (e.g. k_strcmp)
 *   - All driver APIs live in their own .h/.c pair
 *   - NEVER call malloc – use the PMM you build in Lecture 11
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "pmm.h"
#include "fs.h"
#include "string.h"
void process_init(void);
void scheduler_init(void);
int create_process(void (*entry_fn)(void));
void scheduler_timer_init(void);



/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_ps(void);
int process_get_info(int index, uint32_t *pid, int *state );

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

/* Skip leading spaces */
static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    /* Top banner box */
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 0: Kernel Foundations", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Built by students, for students.  Type 'help' to begin.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Welcome! This kernel was compiled from source and booted entirely\n");
    vga_puts("  from bare metal. There is no Linux or Windows underneath – only\n");
    vga_puts("  the code you and your team write.\n");
    vga_puts("\n");
    vga_puts("  Assignment milestones to implement:\n");
    vga_puts_color("    [L09] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Process Management  – PCB, ready queue, round-robin scheduler\n");
    vga_puts_color("    [L10] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Threads & Sync      – kernel threads, mutex, semaphore\n");
    vga_puts_color("    [L11] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Memory Management   – physical page allocator, virtual memory\n");
    vga_puts_color("    [L12] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("File System         – RAM disk, FAT-like directory structure\n");
    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  help    – Show this help message\n");
    vga_puts("  clear   – Clear the screen\n");
    vga_puts("  about   – About this OS and course\n");
    vga_puts("  echo    – Echo text to screen\n");
    vga_puts("  mem     – Memory map (stub)\n");
    vga_puts_color("\n  Milestones (to implement):\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps      – [L09] List processes\n");
    vga_puts("  kill    – [L09] Terminate a process\n");
    vga_puts("  threads – [L10] List kernel threads\n");
    vga_puts("  free    – [L11] Show free memory\n");
    vga_puts("  ls      – [L12] List files\n");
    vga_puts("  cat     – [L12] Print file contents\n\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void) {
    // /* Stage 0 stub – students implement the real PMM in Lecture 11 */
    // vga_puts_color("\n  Memory Map (stub – implement PMM in Lecture 11)\n",
    //                VGA_LIGHT_CYAN, VGA_BLACK);
    // vga_puts("  ─────────────────────────────────────────────\n");
    // vga_puts("  0x00000000 – 0x000FFFFF  :  First 1 MB (reserved/BIOS)\n");
    // vga_puts("  0x00100000 – 0x00EFFFFF  :  Extended memory (usable ~14 MB)\n");
    // vga_puts("  0x00F00000 – 0x00FFFFFF  :  BIOS / ROM area\n");
    // vga_puts("  0xB8000    – 0xBFFFF     :  VGA frame buffer\n");
    // vga_puts_color("\n  TODO: Use BIOS int 0x15, EAX=0xE820 to get real memory map\n\n",
    //                VGA_YELLOW, VGA_BLACK);

    uint32_t total = pmm_get_total_frames();
    uint32_t free = pmm_get_free_frames();
    uint32_t used = total - free;
   
    uint32_t total_mb = (total * 4) / 1024;
    uint32_t used_mb = (used * 4) / 1024;
    uint32_t free_mb = (free * 4) / 1024;
  
    vga_puts_color("\n memory information \n" ,VGA_LIGHT_CYAN , VGA_BLACK);
    
    vga_printf("total memory : %u MB \n", total_mb);
     vga_printf("used memory : %u MB \n", used_mb);
     vga_printf("free memory : %u MB \n", free_mb);
    

}

static void print_number(uint32_t number)
{
    char buffer[11];
    int i =0;

    if(number == 0)
    {
        vga_puts("0");
        return;
    }

    while(number > 0)
    {
        buffer[i++] = '0'+ (number%10);
        number /=10;
    }

    while(i > 0)
    {
        i--;
        char text[2];
        text[0] = buffer[i];
        text[1]= '\0';
        vga_puts(text);
    }
}

static void cmd_ps(void)
{

    uint32_t pid;
    int state;
    int i;
    vga_puts_color("\n PID STATE\n", VGA_YELLOW, VGA_BLACK);
    vga_puts(" --------------------\n");




    for(i=0; i<8 ; i++)
    {
        if(process_get_info(i,&pid,&state)==0 && pid != 0)
        {
            vga_puts(" ");
            print_number(pid);
            vga_puts("      ");

            if(state == 1)
               vga_puts("READY \n");
            else if(state == 2)
            vga_puts("RUNNING \n");
            else if(state == 3)
            vga_puts("TERMINATED \n");
        }
    }
    vga_puts("\n");
}


static void cmd_pmm_test(void)
{
    uint32_t frames[100];
    uint32_t before;
    uint32_t after;
    uint32_t i;

    before = pmm_get_free_frames();

    for(i=0 ; i <100 ; i++)
    {
        frames[i] = pmm_alloc_frame();

        if(frames[i] == 0xFFFFFFFF)
        {
            vga_puts_color("\n pmm test failed : allocation \n ", VGA_LIGHT_RED, VGA_BLACK);
            return ;
        }
    }
     for(i=0 ; i <100 ; i++)
    {
        pmm_free_frame(frames[i]);
    }

    after = pmm_get_free_frames();
    if(after == before)
    {
        vga_puts_color("\n pmm test passed : 100 frames allocated and free \n ", VGA_LIGHT_GREEN, VGA_BLACK);

    }
    else
    {
       vga_puts_color("\n pmm test failed : memory leak detected \n ", VGA_LIGHT_RED, VGA_BLACK);
       vga_printf("before: %u \n ", before); 
       vga_printf("after: %u \n ", after); 
    }
}
/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts(" ksh> ");
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "meminfo")   == 0) { cmd_mem();   continue; }
         if (k_strcmp(cmd, "pmmtest")   == 0) { cmd_pmm_test();   continue; }
        if(k_strcmp(cmd,"ps")      == 0) { cmd_ps();    continue; }
        
        
        if(strcmp(cmd,"ls")==0){fs_list();}
        if(strcmp(cmd,"touch", 6) == 0)
        {fs_touch(cmd + 6);}
        if(strcmp(cmd,"cat", 4) == 0)
         {fs_cat(cmd + 4);}
         if(strcmp(cmd,"rm", 3)==0)
         {fs_rm(cmd + 3);}
      /*  if(k_strcmp(cmd,"ls") == 0)
        {
            fs_list();
            continue;
        }*/
        /* Milestone stubs */
        if (k_strcmp(cmd, "ps")      == 0 ||
            k_strcmp(cmd, "kill")    == 0 ||
            k_strcmp(cmd, "threads") == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "cat")     == 0) {
            vga_puts_color("  [TODO] This command is not yet implemented.\n",
                           VGA_YELLOW, VGA_BLACK);
            vga_puts("  Implement it as part of your lecture assignment.\n");
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

static void process1(void)
{
    while(true)
    {
        vga_set_cursor(0,0);
        vga_puts("process 1 running     ");

    }
}
static void process2(void)
{
    while(true)
    {
          vga_set_cursor(1,0);
        vga_puts("process 2 running     ");
    }
}

void test_thread(void *arg)
{
    volatile uint32_t *counter = (volatile uint32_t *)arg;
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    while(1)
    {
        (*counter)++;
        vga[23 * 80 + 70] = (uint16_t)(0x0F00 | 'T');
    }
}

static mutex_t test_mutex;
static uint32_t shared_counter = 0;

void mutex_test_thread(void *arg)
{
    volatile uint32_t *counter = (volatile uint32_t *)arg;
    uint32_t i;

    for(i=0; i < 1000 ; i++)
    {
        mutex_lock(&test_mutex);

        (*counter)++;
        
        mutex_unlock(&test_mutex);
    }
    thread_exit();
}

static mutex_t race_mutex;
static uint32_t myglobal = 0;
static uint32_t protected_finished = 0;

void race_thread(void *arg)
{
    volatile uint32_t *value = (volatile uint32_t *)arg;
    uint32_t i;
    for(i=0 ; i< 10000; i++)
    {
      mutex_lock(&race_mutex);
        (*value)++;

        mutex_unlock(&race_mutex);
    }
    mutex_lock(&race_mutex);
    protected_finished ++;

    if(protected_finished == 2)
    {
        vga_set_cursor(10,0);
        vga_printf("protected result : %u", *value);
    }

    mutex_unlock(&race_mutex);
    while(1)
    {
        __asm__ __volatile__("hlt");
    }
}

static uint32_t race_without_mutex = 0;
void race_no_mutex_thread(void *arg)
{
    volatile uint32_t *value = (volatile uint32_t *)arg;
    uint32_t temp;
    uint32_t i;
    for(i=0 ; i< 10000; i++)
    {
        temp = *value;
        __asm__ __volatile__("sti");
        

        *value = temp +1;
    }

    vga_set_cursor(11,0);
    vga_printf("unprotected result : %u", *value);

    while(1)
    {
        __asm__ __volatile__("hlt");
    }
}

#define BUFFER_SIZE 4

static int buffer[BUFFER_SIZE];
static int buffer_in = 0;
static int  buffer_out = 0;

static semaphore_t empty;
static semaphore_t full;
static semaphore_t buffer_mutex;

void producer(void *arg)
{
    int item = 0;

    (void)arg;

    while(1)
    {
        semaphore_wait(&empty);
        semaphore_wait(&buffer_mutex);

        buffer[buffer_in] = item++;
        buffer_in = (buffer_in +1)% BUFFER_SIZE;

        semaphore_signal(&buffer_mutex);
        semaphore_signal(&full);
    }
}

void consumer(void *arg)
{
    int item;
    (void)arg;

    while(1)
    {
        semaphore_wait(&full);
        semaphore_wait(&buffer_mutex);

        item = buffer[buffer_out];
        buffer_out = (buffer_out+1)% BUFFER_SIZE;

        semaphore_signal(&buffer_mutex);
        semaphore_signal(&empty);

        (void)item;

    }
}
/* ---------------------------------------------------------------------------
 * Kernel entry point – called from kernel_entry.asm
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    vga_init();
    kb_init();

   pmm_init();
   fs_init();

    process_init();
    scheduler_init();
    create_process(process1);
    create_process(process2);
    mutex_init(&test_mutex);

    thread_create(mutex_test_thread, &shared_counter);
     thread_create(mutex_test_thread, &shared_counter);

   mutex_init(&race_mutex);
   thread_create(race_thread , &myglobal);
   thread_create(race_thread , &myglobal);

   thread_create(race_no_mutex_thread , &race_without_mutex);
   thread_create(race_no_mutex_thread , &race_without_mutex);

   semaphore_init(&empty , BUFFER_SIZE);
   semaphore_init(&full , 0);
   semaphore_init(&buffer_mutex, 1);

   thread_create(producer , NULL);
   thread_create(consumer , NULL);
    
   print_splash();
   shell_run();

    scheduler_timer_init();

        

    /* Should never reach here */
    __asm__ __volatile__("hlt");
}
