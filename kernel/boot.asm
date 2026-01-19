[bits 16]
[org 0x7c00]

KERNEL_OFFSET equ 0x1000

start:
    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Clear screen (BIOS)
    mov ax, 0x03
    int 0x10

    ; Load kernel from disk
    mov [BOOT_DRIVE], dl
    
    mov bx, KERNEL_OFFSET ; Destination
    mov dh, 32            ; Number of sectors (16KB)
    mov dl, [BOOT_DRIVE]
    call disk_load

    ; Switch to PM
    call switch_to_pm
    jmp $

%include "gdt.asm"

[bits 16]
disk_load:
    pusha
    push dx

    mov ah, 0x02 ; BIOS read sector
    mov al, dh   ; Read DH sectors
    mov ch, 0x00 ; Cylinder 0
    mov dh, 0x00 ; Head 0
    mov cl, 0x02 ; Start from sector 2

    int 0x13
    jc disk_error

    pop dx
    cmp al, dh
    jne disk_error
    popa
    ret

disk_error:
    mov ah, 0x0e
    mov al, 'D'
    int 0x10
    jmp $

[bits 16]
switch_to_pm:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x90000 ; Update stack top

    call KERNEL_OFFSET ; Jump to kernel
    jmp $

BOOT_DRIVE db 0

; Padding and magic number
times 510-($-$$) db 0
dw 0xaa55
