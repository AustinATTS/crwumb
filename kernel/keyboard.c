#include "types.h"

// save me

static inline u8 inb(u16 p) {
    u8 r;
    asm volatile ("inb %1, %0" : "=a"(r) : "Nd"(p));
    return r;
}

static inline void outb(u16 p, u8 v) {
    asm volatile ("outb %0, %1" :: "a"(v), "Nd"(p));
}

#define KBD_BUFFER_SIZE 256

static u8 kbd_buffer[KBD_BUFFER_SIZE];
static volatile u32 kbd_read_pos = 0;
static volatile u32 kbd_write_pos = 0;

static u8 key_map[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

void keyboard_handler(void) {
    u8 sc = inb(0x60);

    if (sc < 0x80 && key_map[sc]) {
        u32 next_pos = (kbd_write_pos + 1) % KBD_BUFFER_SIZE;
        if (next_pos != kbd_read_pos) {
            kbd_buffer[kbd_write_pos] = key_map[sc];
            kbd_write_pos = next_pos;
        }
    }
}

u8 keyboard_getchar(void) {
    while (kbd_read_pos == kbd_write_pos) {
        asm volatile("hlt");
    }
    u8 c = kbd_buffer[kbd_read_pos];
    kbd_read_pos = (kbd_read_pos + 1) % KBD_BUFFER_SIZE;
    return c;
}

int keyboard_available(void) {
    return kbd_read_pos != kbd_write_pos;
}