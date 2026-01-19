; keyboard.asm
[bits 32]

global keyboard_isr
global keyboard_getchar

extern inb
extern outb

SECTION .data
shift_state db 0
kbd_buffer  db 0
kbd_ready   db 0


keymap:
db 0, 27
db '1','2','3','4','5','6','7','8','9','0','-','='
db 8, 9
db 'q','w','e','r','t','y','u','i','o','p','[',']'
db 13, 0
db 'a','s','d','f','g','h','j','k','l',';','''
db '`', 0
db 'z','x','c','v','b','n','m',',','.','/'
db 0,'*',0,' '

shiftmap:
db 0, 27
db '!','@','#','$','%','^','&','*','(',')','_','+'
db 8, 9
db 'Q','W','E','R','T','Y','U','I','O','P','{','}'
db 13, 0
db 'A','S','D','F','G','H','J','K','L',':','"'
db '~', 0
db 'Z','X','C','V','B','N','M','<','>','?'
db 0,'*',0,' '

SECTION .text

keyboard_isr:
    pusha

    mov al, 0x60
    call inb
    mov bl, al

    ; key release? not sure....
    test bl, 0x80
    jnz .release

    ; shift pressed
    cmp bl, 42
    je .shift_on
    cmp bl, 54
    je .shift_on

    ; normal key
    movzx eax, bl
    cmp eax, 58
    ja .done

    cmp byte [shift_state], 0
    je .normal

    mov al, [shiftmap + eax]
    jmp .store

.normal:
    mov al, [keymap + eax]

.store:
    cmp al, 0
    je .done
    mov [kbd_buffer], al
    mov byte [kbd_ready], 1
    jmp .done

.release:
    and bl, 0x7F
    cmp bl, 42
    je .shift_off
    cmp bl, 54
    je .shift_off
    jmp .done

.shift_on:
    mov byte [shift_state], 1
    jmp .done

.shift_off:
    mov byte [shift_state], 0

.done:
    mov al, 0x20
    call outb        ; PIC EOI

    popa
    iret


keyboard_getchar:
    cmp byte [kbd_ready], 0
    je .noglyph
    mov al, [kbd_buffer]
    mov byte [kbd_ready], 0
    ret

.noglyph:
    mov al, 0
    ret
