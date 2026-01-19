#include "types.h"

/**
 * @brief Exception handler entry point
 */
void isr_handler(u32 vector, u32 error_code) {
    (void)vector;
    (void)error_code;

    /* VGA diagnostic output */
    *(volatile u16*)0xB8000 = 0x4F45; /* 'E' */
    *(volatile u16*)0xB8002 = 0x4F52; /* 'R' */
    *(volatile u16*)0xB8004 = 0x4F52; /* 'R' */

    for (;;) {
        asm volatile("cli; hlt");
    }
}
