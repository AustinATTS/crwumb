#include "types.h"
#include "network.h"

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

#define REG_CMD 0x37
#define REG_IMR 0x3C
#define REG_ISR 0x3E
#define REG_RCR 0x44
#define REG_CONFIG1 0x52

#define CMD_RESET 0x10
#define CMD_RX_ENABLE 0x08
#define CMD_TX_ENABLE 0x04

#define RCR_AAP 0x01
#define RCR_APM 0x02
#define RCR_AM 0x04
#define RCR_AB 0x08
#define RCR_WRAP 0x80

static u16 rtl8139_iobase = 0;
static u8 mac_addr[6];
static u8* rx_buffer;
static u32 rx_buffer_pos = 0;

static u8 tx_buffer[2048];

static inline void outb(u16 port, u8 val) {
    asm volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(u16 port, u16 val) {
    asm volatile("outw %0, %1" :: "a"(val), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(u16 port, u32 val) {
    asm volatile("outl %0, %1" :: "a"(val), "Nd"(port));
}

static inline u32 inl(u16 port) {
    u32 ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static u16 pci_config_read_word(u8 bus, u8 slot, u8 func, u8 offset) {
    u32 address = (u32)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(0xCF8, address);
    return (u16)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
}

static void pci_config_write_word(u8 bus, u8 slot, u8 func, u8 offset, u16 value) {
    u32 address = (u32)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(0xCF8, address);
    outw(0xCFC + (offset & 2), value);
}

static void rtl8139_reset(void) {
    outb(rtl8139_iobase + REG_CMD, CMD_RESET);
    while ((inb(rtl8139_iobase + REG_CMD) & CMD_RESET) != 0);
}

void rtl8139_init(void) {
    for (u16 bus = 0; bus < 256; bus++) {
        for (u8 slot = 0; slot < 32; slot++) {
            u16 vendor = pci_config_read_word(bus, slot, 0, 0);
            u16 device = pci_config_read_word(bus, slot, 0, 2);

            if (vendor == RTL8139_VENDOR_ID && device == RTL8139_DEVICE_ID) {
                u16 cmd = pci_config_read_word(bus, slot, 0, 0x04);
                cmd |= 0x0005;
                pci_config_write_word(bus, slot, 0, 0x04, cmd);

                u16 bar0 = pci_config_read_word(bus, slot, 0, 0x10);
                rtl8139_iobase = bar0 & 0xFFFE;

                rtl8139_reset();

                rx_buffer = (u8*)0x200000;
                rx_buffer_pos = 0;
                outl(rtl8139_iobase + 0x30, (u32)rx_buffer);

                outl(rtl8139_iobase + REG_RCR, RCR_AAP | RCR_APM | RCR_AM | RCR_AB | RCR_WRAP);

                outw(rtl8139_iobase + REG_IMR, 0x0005);

                outb(rtl8139_iobase + REG_CMD, CMD_RX_ENABLE | CMD_TX_ENABLE);

                for (int i = 0; i < 6; i++) {
                    mac_addr[i] = inb(rtl8139_iobase + i);
                }

                return;
            }
        }
    }
}

void rtl8139_send(u8* data, u32 len) {
    if (len > 2048) len = 2048;

    for (u32 i = 0; i < len; i++) {
        tx_buffer[i] = data[i];
    }

    static u8 tx_cur = 0;
    outl(rtl8139_iobase + 0x20 + (tx_cur * 4), (u32)tx_buffer);
    outl(rtl8139_iobase + 0x10 + (tx_cur * 4), len);

    tx_cur = (tx_cur + 1) % 4;
}

void rtl8139_poll(void) {
    u16 status = inw(rtl8139_iobase + REG_ISR);

    if (status) {
        outw(rtl8139_iobase + REG_ISR, status);
    }

    u32 processed = 0;
    while (processed < 16) {
        u8 cmd = inb(rtl8139_iobase + REG_CMD);
        if (cmd & 0x01) break;

        u16 header = *(u16*)(rx_buffer + rx_buffer_pos);
        u16 len = *(u16*)(rx_buffer + rx_buffer_pos + 2);

        if (len < 4 || len > 1600) break;

        if ((header & 0x01) == 0) break;

        net_handle_packet(rx_buffer + rx_buffer_pos + 4, len - 4);

        rx_buffer_pos = (rx_buffer_pos + len + 4 + 3) & ~3;
        rx_buffer_pos %= 8192;

        outw(rtl8139_iobase + 0x38, (u16)((rx_buffer_pos - 16) & 0x1FFF));

        processed++;
    }
}

void rtl8139_get_mac(u8* mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = mac_addr[i];
    }
}