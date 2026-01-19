#include "types.h"
#include "idt.h"

#define VIDEO_MEM 0xB8000
#define SCREEN_W  80
#define SCREEN_H  25

extern void keyboard_handler(void);
extern u8 keyboard_getchar(void);
extern void irq_init(void);
extern void irq_install_handler(int irq, void (*handler)(void));

static u16 *vga_buffer;
static u32  cursor_y, cursor_x;
static u8   terminal_color;
static char current_path[256];

static inline void outb(u16 port, u8 val)
{
    asm volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port)
{
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void terminal_clear(void)
{
    for (u32 i = 0; i < SCREEN_W * SCREEN_H; ++i)
        vga_buffer[i] = (terminal_color << 8) | ' ';
    cursor_x = cursor_y = 0;
}

void terminal_scroll(void)
{
    for (u32 i = 0; i < SCREEN_W * (SCREEN_H - 1); ++i)
        vga_buffer[i] = vga_buffer[i + SCREEN_W];

    for (u32 i = 0; i < SCREEN_W; ++i)
        vga_buffer[SCREEN_W * (SCREEN_H - 1) + i] = (terminal_color << 8) | ' ';

    cursor_y = SCREEN_H - 1;
}

void terminal_putc(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        if (++cursor_y >= SCREEN_H) terminal_scroll();
        return;
    }

    vga_buffer[cursor_y * SCREEN_W + cursor_x] = (terminal_color << 8) | c;
    if (++cursor_x >= SCREEN_W) {
        cursor_x = 0;
        if (++cursor_y >= SCREEN_H) terminal_scroll();
    }
}

void terminal_write(const char *s)
{
    while (*s) terminal_putc(*s++);
}

void terminal_writeln(const char *s)
{
    terminal_write(s);
    terminal_putc('\n');
}

int str_compare(const char *a, const char *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return *(const u8 *)a - *(const u8 *)b;
}

void str_copy(char *dst, const char *src)
{
    while ((*dst++ = *src++));
}

void mem_set(void *dst, u8 val, u32 len)
{
    u8 *p = dst;
    while (len--) *p++ = val;
}

void terminal_readline(char *buf, u32 max)
{
    u32 i = 0;
    while (i < max - 1) {
        u8 c = keyboard_getchar();

        if (c == '\n') {
            buf[i] = 0;
            terminal_putc('\n');
            return;
        }
        if (c == '\b' && i) {
            --i;
            if (cursor_x) --cursor_x;
            else { cursor_x = SCREEN_W - 1; --cursor_y; }
            vga_buffer[cursor_y * SCREEN_W + cursor_x] = (terminal_color << 8) | ' ';
            continue;
        }
        if (c >= 32 && c < 127) {
            buf[i++] = c;
            terminal_putc(c);
        }
    }
    buf[max - 1] = 0;
}

void shell_execute(char *cmd)
{
    char *args = cmd;
    while (*args && *args != ' ') ++args;
    if (*args) *args++ = 0;

    if (!str_compare(cmd, "help")) {
        terminal_writeln("Commands: help clear echo pwd ver halt");
    } else if (!str_compare(cmd, "clear")) {
        terminal_clear();
    } else if (!str_compare(cmd, "echo")) {
        if (*args) terminal_writeln(args);
    } else if (!str_compare(cmd, "pwd")) {
        terminal_writeln(current_path);
    } else if (!str_compare(cmd, "ver")) {
        terminal_writeln("kill me");
    } else if (!str_compare(cmd, "halt")) {
        asm volatile("cli; hlt");
    } else {
        terminal_write("Unknown: ");
        terminal_writeln(cmd);
    }
}

void shell_loop(void)
{
    char buf[256];
    for (;;) {
        terminal_color = 0x0A;
        terminal_write("# ");
        terminal_color = 0x0F;
        terminal_readline(buf, sizeof buf);
        shell_execute(buf);
    }
}

void kernel_main(void)
{
    vga_buffer = (u16 *)VIDEO_MEM;
    terminal_color = 0x0F;
    terminal_clear();

    idt_init();
    irq_init();

    irq_install_handler(1, keyboard_handler);

    str_copy(current_path, "/");

    asm volatile("sti");

    terminal_writeln("Kernel ready.");
    terminal_writeln("Starting shell...");
    shell_loop();

    for (;;) asm volatile("hlt");
}