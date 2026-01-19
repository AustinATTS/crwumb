[bits 32]
global _start
extern kernel_main

_start:
    cli
    call kernel_main

.hang:
    hlt
    jmp .hang
