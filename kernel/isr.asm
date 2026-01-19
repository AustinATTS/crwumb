bits 32
global isr_stub_table
extern isr_handler

%macro ISR_NOERR 1
global isr%1
isr%1:
    push 0          ; fake error code
    push %1         ; vector
    jmp isr_common
%endmacro

isr_common:
    pusha
    call isr_handler
    popa
    add esp, 8
    iretd

isr_stub_table:
%assign i 0
%rep 32
    dd isr%+i
    ISR_NOERR i
%assign i i+1
%endrep
