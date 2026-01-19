bits 32

global isr_stub_table
extern isr_handler



%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0        ; err
    push dword %1       ; vec
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1       ; vec
    jmp isr_common
%endmacro



isr_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; kernel data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 8          ; vec + err
    iretd



ISR_NOERR 0    ; #DE
ISR_NOERR 1    ; #DB
ISR_NOERR 2    ; NMI
ISR_NOERR 3    ; BP
ISR_NOERR 4    ; OF
ISR_NOERR 5    ; BR
ISR_NOERR 6    ; UD
ISR_NOERR 7    ; NM
ISR_ERR   8    ; DF
ISR_NOERR 9
ISR_ERR   10   ; TS
ISR_ERR   11   ; NP
ISR_ERR   12   ; SS
ISR_ERR   13   ; GP
ISR_ERR   14   ; PF
ISR_NOERR 15
ISR_NOERR 16   ; MF
ISR_ERR   17   ; AC
ISR_NOERR 18   ; MC
ISR_NOERR 19   ; XM



align 4
isr_stub_table:
    dd isr0,  isr1,  isr2,  isr3
    dd isr4,  isr5,  isr6,  isr7
    dd isr8,  isr9,  isr10, isr11
    dd isr12, isr13, isr14, isr15
    dd isr16, isr17, isr18, isr19
