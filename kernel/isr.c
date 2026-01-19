#include "types.h"


// this is fucking hell...
// save me..
// someone something
// please i beg.....
struct isr_frame {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 vector;
    u32 error_code;
};

extern void* irq_get_handler(int irq);

void isr_handler(struct isr_frame* frame)
{
    (void)frame;

    volatile u16* vga = (u16*)0xB8000;

    vga[0] = 0x4F45;
    vga[1] = 0x4F52;
    vga[2] = 0x4F52;

    for (;;)
        asm volatile("cli; hlt");
}

void irq_handler(struct isr_frame* frame)
{
    u8 vector = frame->vector;
    int irq_num = vector - 32;

    typedef void (*irq_handler_t)(void);
    irq_handler_t handler = (irq_handler_t)irq_get_handler(irq_num);

    if (handler) {
        handler();
    }

    if (vector >= 40) {
        asm volatile("outb %0, %1" :: "a"((u8)0x20), "Nd"((u16)0xA0));
    }
    asm volatile("outb %0, %1" :: "a"((u8)0x20), "Nd"((u16)0x20));
}