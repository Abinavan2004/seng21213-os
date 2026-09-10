[BITS 32]

EXTERN scheduler_irq0_handler
EXTERN thread_exit


GLOBAL switch_context
GLOBAL irq0_stub
GLOBAL thread_bootstrap


;-------------------------------------------------
; Context-switch 
;-------------------------------------------------

switch_context:
     mov eax, [esp + 4]     ;get new process stack pointer
     mov esp, eax           ;switch to new process stack 

    popad
    iretd
;---------------------------------------------------
; kernel thread bootstrap
; After iretd:
;    [ESP + 0] = thread entry function
;    [ESP + 4] = thread argument
;----------------------------------------------------

thread_bootstrap:
    mov eax, [esp]
    mov edx, [esp + 4]

    push edx
    call eax
    add esp, 4

    call thread_exit

    .hang:
        cli
        hlt
        jmp  .hang

 ; -------------------------------------------------
 ; IRQ0 timer interrupt stub 
 ; -------------------------------------------------
 
 irq0_stub:
     pushad

    ; pass current stack pointer to scheduler
    push esp
    call scheduler_irq0_handler
    add esp,4

    ; EAX contains the next process stack pointer
    push eax
    call switch_context

   
     