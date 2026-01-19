#include "idt.h"

extern void* isr_stub_table[];

static struct idt_entry idt[256];
static struct idt_ptr   idtr;

/**
 * @brief Configure IDT gate descriptor
 */
static inline void idt_set_gate(u8 n, u32 handler) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x08; /* Kernel CS */
    idt[n].reserved    = 0;
    idt[n].flags       = 0x8E; /* P=1, DPL=0, Type=InterruptGate */
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

/**
 * @brief Initialize Interrupt Descriptor Table
 */
void idt_init(void) {
    for (u16 i = 0; i < 32; i++) {
        idt_set_gate((u8)i, (u32)isr_stub_table[i]);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (u32)&idt;

    asm volatile("lidt %0" : : "m"(idtr));
}
