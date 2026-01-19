AS  := nasm
CC  := gcc
LD  := ld

ASFLAGS_BIN := -f bin
ASFLAGS_ELF := -f elf32

CFLAGS := \
	-m32 \
	-ffreestanding \
	-nostdlib -nostdinc \
	-fno-builtin \
	-fno-stack-protector \
	-fno-pie -fno-pic \
	-O2 -Wall -Wextra

LDFLAGS := -m elf_i386 -T linker.ld

BUILD := build

BOOT_BIN   := $(BUILD)/boot.bin
KERNEL_BIN := $(BUILD)/kernel.bin
KERNEL_RAW := $(BUILD)/kernel_flat.bin
IMAGE      := $(BUILD)/uwuos.img

KERNEL_OBJS := \
	$(BUILD)/entry.o \
	$(BUILD)/kernel.o \
	$(BUILD)/idt.o \
	$(BUILD)/isr.o \
	$(BUILD)/isr_stub.o

.PHONY: all clean run

all: $(IMAGE)

$(BUILD):
	@mkdir -p $(BUILD)

$(BOOT_BIN): boot.asm gdt.asm | $(BUILD)
	@$(AS) $(ASFLAGS_BIN) $< -o $@

$(BUILD)/entry.o: entry.asm | $(BUILD)
	@$(AS) $(ASFLAGS_ELF) $< -o $@

$(BUILD)/isr_stub.o: idt.asm | $(BUILD)
	@$(AS) $(ASFLAGS_ELF) $< -o $@

$(BUILD)/kernel.o: kernel.c | $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/idt.o: idt.c idt.h types.h | $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/isr.o: isr.c types.h | $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS) linker.ld
	@$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $@

$(IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@objcopy -O binary $(KERNEL_BIN) $(KERNEL_RAW)
	@cat $(BOOT_BIN) $(KERNEL_RAW) > $@
	@truncate -s 1440K $@

run: $(IMAGE)
	@qemu-system-i386 -drive format=raw,file=$(IMAGE) -m 128M -no-reboot -no-shutdown

clean:
	@rm -rf $(BUILD)
