#include "idt.h"
#include "types.h"

extern void* isr_stub_table[];  // 0–31
extern void* irq_stub_table[];  // 32–47

static struct idt_entry idt[256];
static struct idt_ptr   idtr;


static inline void outb(u16 port, u8 val) {
    asm volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}


void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags) {
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].selector    = sel;
    idt[num].reserved    = 0;
    idt[num].flags       = flags;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
}


static void pic_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    outb(0x21, 0x20); // master offset = 32
    outb(0xA1, 0x28); // slave offset  = 40

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);
}


static void pic_set_mask(void) {


    outb(0x21, 0xFC); // 11111100 → IRQ0 + IRQ1 enabled
    outb(0xA1, 0xFF); // disable all slave IRQs mayb im also a slave to asm....
}

void idt_init(void) {
    asm volatile ("cli");  // hahah im sooo silylyylylylylylylyylylylyllylyllylylyly


    for (int i = 0; i < 256; i++) {
        idt[i] = (struct idt_entry){0};
    }


    for (u8 i = 0; i < 32; i++) {
        idt_set_gate(i, (u32)isr_stub_table[i], 0x08, 0x8E);
    }


    for (u8 i = 0; i < 16; i++) {
        idt_set_gate(32 + i, (u32)irq_stub_table[i], 0x08, 0x8E);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (u32)&idt;

    pic_remap();
    pic_set_mask();

    asm volatile ("lidt %0" :: "m"(idtr));
    asm volatile ("sti");
}