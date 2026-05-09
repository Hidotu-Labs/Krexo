AS = nasm
CC_BIOS = x86_64-elf-gcc
LD_BIOS = x86_64-elf-ld
CC_UEFI = clang
LD_UEFI = lld-link

EDK2_INC = -Ilib/edk2/MdePkg/Include -Ilib/edk2/MdePkg/Include/X64

CFLAGS_COMMON = -ffreestanding -fno-stack-protector -fno-stack-check -O2 -Iinclude

CFLAGS_BIOS = $(CFLAGS_COMMON) -m32 -march=i386 -fno-pic
LDFLAGS_BIOS = -m elf_i386 -T bios/link.ld

CFLAGS_UEFI = -target x86_64-unknown-windows -fshort-wchar -mno-red-zone $(CFLAGS_COMMON) $(EDK2_INC)
LDFLAGS_UEFI = -subsystem:efi_application -entry:efi_main

OVMF_PATH = /usr/share/edk2/OVMF.fd

all: bios_boot uefi_boot

# UEFI build
uefi/%.o: uefi/%.c
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

common/lib/printf_uefi.o: common/lib/printf.c
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

common/lib/fb_uefi.o: common/lib/fb.c
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

common/lib/font_data_uefi.o: common/lib/font_data.c
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

common/fs/fat32_uefi.o: common/fs/fat32.c
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

common/lib/config_uefi.o: common/lib/config.c
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

common/lib/menu_uefi.o: common/lib/menu.c
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

common/lib/requests_uefi.o: common/lib/requests.c
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

uefi_boot: uefi/main.o uefi/debug.o uefi/mmap.o uefi/gop.o uefi/disk.o common/lib/printf_uefi.o common/lib/fb_uefi.o common/lib/font_data_uefi.o common/fs/fat32_uefi.o common/lib/config_uefi.o common/lib/menu_uefi.o common/lib/requests_uefi.o
	$(LD_UEFI) $(LDFLAGS_UEFI) $^ -out:boot.efi

# BIOS build
bios/stage1.bin: bios/stage1.s
	$(AS) -f bin bios/stage1.s -o $@

bios/stage2.o: bios/stage2.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

bios/mmap.o: bios/mmap.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

bios/vbe.o: bios/vbe.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

bios/disk.o: bios/disk.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

common/lib/fb_bios.o: common/lib/fb.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

common/lib/font_data_bios.o: common/lib/font_data.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

common/fs/fat32_bios.o: common/fs/fat32.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

bios/entry.o: bios/entry.s
	$(AS) -f elf32 bios/entry.s -o $@

bios/long_mode.o: bios/long_mode.s
	$(AS) -f elf32 bios/long_mode.s -o $@

common/lib/printf_bios.o: common/lib/printf.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

common/lib/config_bios.o: common/lib/config.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

common/lib/menu_bios.o: common/lib/menu.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

common/lib/requests_bios.o: common/lib/requests.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

bios/stage2.bin: bios/entry.o bios/stage2.o bios/long_mode.o bios/mmap.o bios/vbe.o bios/disk.o common/lib/fb_bios.o common/lib/font_data_bios.o common/fs/fat32_bios.o common/lib/printf_bios.o common/lib/config_bios.o common/lib/menu_bios.o common/lib/requests_bios.o
	$(LD_BIOS) $(LDFLAGS_BIOS) $^ -o $@

# Kernel Build
CC_KERNEL = x86_64-elf-gcc
LD_KERNEL = x86_64-elf-ld
CFLAGS_KERNEL = -ffreestanding -O2 -mcmodel=large -mno-red-zone -Iinclude -fno-stack-protector -fno-stack-check
LDFLAGS_KERNEL = -T kernel/linker.ld -nostdlib

kernel/main.o: kernel/main.c
	$(CC_KERNEL) $(CFLAGS_KERNEL) -c $< -o $@

common/lib/font_data_kernel.o: common/lib/font_data.c
	$(CC_KERNEL) $(CFLAGS_KERNEL) -c $< -o $@

kernel.elf: kernel/main.o common/lib/font_data_kernel.o
	$(LD_KERNEL) $(LDFLAGS_KERNEL) $^ -o $@

all: bios_boot uefi_boot kernel.elf

bios_boot: bios/stage1.bin bios/stage2.bin
	cat bios/stage1.bin bios/stage2.bin > boot.bin
	truncate -s 131072 boot.bin

run_bios_full: bios_boot kernel.elf
	bash mkdisk.sh
	qemu-system-x86_64 -drive format=raw,file=krexo_disk.img -debugcon stdio

run_uefi: uefi_boot bios_boot kernel.elf
	bash mkdisk.sh
	mkdir -p build/uefi/EFI/BOOT
	cp boot.efi build/uefi/EFI/BOOT/BOOTX64.EFI
	mcopy -o -i krexo_disk.img@@1M boot.efi ::/EFI/BOOT/BOOTX64.EFI || true
	qemu-system-x86_64 -bios $(OVMF_PATH) -drive format=raw,file=krexo_disk.img

clean:
	rm -rf bios/*.o bios/*.bin uefi/*.o uefi/*.elf common/lib/*.o kernel/*.o boot.bin boot.efi build/ kernel.elf
