[BITS 32]

EXTERN scheduler_irq0_handler


GLOBAL switch_context
GLOBAL irq0_stub

;-------------------------------------------------
; Context-switch 
;-------------------------------------------------

switch_context:
     mov eax, [esp + 4]     ;get new process stack pointer
     mov esp, eax           ;switch to new process stack 

    popad
    iretd


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

   
     