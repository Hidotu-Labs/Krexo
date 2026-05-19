AS = nasm
CC_BIOS = x86_64-elf-gcc
LD_BIOS = x86_64-elf-ld
CC_UEFI = clang
LD_UEFI = lld-link

EDK2_INC = -Ilib/edk2/MdePkg/Include -Ilib/edk2/MdePkg/Include/X64

CFLAGS_COMMON = -ffreestanding -fno-stack-protector -fno-stack-check -O2 -Iinclude

CFLAGS_BIOS = $(CFLAGS_COMMON) -m32 -march=i386 -fno-pic
LDFLAGS_BIOS = -m elf_i386 -T src/boot/bios/link.ld

CFLAGS_UEFI = -target x86_64-unknown-windows -fshort-wchar -mno-red-zone -mno-stack-arg-probe $(CFLAGS_COMMON) $(EDK2_INC)
LDFLAGS_UEFI = -subsystem:efi_application -entry:efi_main

# Search for OVMF in common locations
OVMF_PATHS = /usr/share/edk2/OVMF.fd \
             /usr/share/edk2-ovmf/x64/OVMF.fd \
             /usr/share/OVMF/OVMF_CODE.fd \
             /usr/share/ovmf/OVMF.fd

OVMF_PATH = $(firstword $(wildcard $(OVMF_PATHS)))

# Check if EDK2 headers exist
EDK2_HEADER = lib/edk2/MdePkg/Include/Uefi.h

all: bios_boot uefi_boot kernel.elf

# Verification before build
check_edk2:
	@if [ ! -f $(EDK2_HEADER) ]; then \
		echo "Error: EDK2 headers not found in lib/edk2."; \
		echo "Please run: git submodule update --init --recursive"; \
		exit 1; \
	fi

# UEFI build targets
UEFI_OBJS = src/boot/uefi/main.o src/boot/uefi/debug.o src/boot/uefi/mmap.o src/boot/uefi/gop.o src/boot/uefi/disk.o \
           src/common/lib/printf_uefi.o src/common/lib/fb_uefi.o src/common/lib/font_data_uefi.o \
           src/common/fs/fat32_uefi.o src/common/lib/config_uefi.o src/common/lib/menu_uefi.o \
           src/common/lib/requests_uefi.o src/common/lib/bmp_uefi.o src/common/lib/png_uefi.o \
           src/common/lib/image_uefi.o src/common/lib/limine_compat_uefi.o

src/boot/uefi/%.o: src/boot/uefi/%.c | check_edk2
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

src/common/lib/%_uefi.o: src/common/lib/%.c | check_edk2
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

src/common/fs/%_uefi.o: src/common/fs/%.c | check_edk2
	$(CC_UEFI) $(CFLAGS_UEFI) -c $< -o $@

uefi_boot: $(UEFI_OBJS)
	$(LD_UEFI) $(LDFLAGS_UEFI) $^ -out:boot.efi

# BIOS build targets
BIOS_OBJS = src/boot/bios/entry.o src/boot/bios/stage2.o src/boot/bios/long_mode.o src/boot/bios/mmap.o \
           src/boot/bios/vbe.o src/boot/bios/disk.o src/common/lib/fb_bios.o src/common/lib/font_data_bios.o \
           src/common/fs/fat32_bios.o src/common/lib/printf_bios.o src/common/lib/config_bios.o \
           src/common/lib/menu_bios.o src/common/lib/requests_bios.o src/common/lib/bmp_bios.o \
           src/common/lib/png_bios.o src/common/lib/image_bios.o src/common/lib/limine_compat_bios.o

src/boot/bios/%.o: src/boot/bios/%.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

src/boot/bios/%.o: src/boot/bios/%.s
	$(AS) -f elf32 $< -o $@

src/boot/bios/stage1.bin: src/boot/bios/stage1.s
	$(AS) -f bin $< -o $@

src/common/lib/%_bios.o: src/common/lib/%.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

src/common/fs/%_bios.o: src/common/fs/%.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

src/boot/bios/stage2.bin: $(BIOS_OBJS)
	$(LD_BIOS) $(LDFLAGS_BIOS) $^ -o $@

bios_boot: src/boot/bios/stage1.bin src/boot/bios/stage2.bin
	cat src/boot/bios/stage1.bin src/boot/bios/stage2.bin > boot.bin
	truncate -s 131072 boot.bin

# Kernel Build
CC_KERNEL = x86_64-elf-gcc
LD_KERNEL = x86_64-elf-ld
CFLAGS_KERNEL = -ffreestanding -O2 -mcmodel=large -mno-red-zone -Iinclude -fno-stack-protector -fno-stack-check
LDFLAGS_KERNEL = -T barebones/kernel/linker-scripts/x86_64.ld -nostdlib

kernel.elf:
	$(MAKE) -C barebones/kernel CC=$(CC_KERNEL) LD=$(LD_KERNEL) ARCH=x86_64
	cp barebones/kernel/bin-x86_64/kernel kernel.elf

# Running targets
run_bios: bios_boot kernel.elf
	bash scripts/mkdisk.sh
	qemu-system-x86_64 -drive format=raw,file=krexo_disk.img -debugcon stdio

run_uefi: uefi_boot bios_boot kernel.elf
	bash scripts/mkdisk.sh
	mkdir -p build/uefi/EFI/BOOT
	cp boot.efi build/uefi/EFI/BOOT/BOOTX64.EFI
	mcopy -o -i krexo_disk.img@@1M boot.efi ::/EFI/BOOT/BOOTX64.EFI || true
	@if [ -z "$(OVMF_PATH)" ] || [ ! -f "$(OVMF_PATH)" ]; then \
		echo "Error: OVMF firmware not found. Please install edk2-ovmf or specify OVMF_PATH."; \
		exit 1; \
	fi
	qemu-system-x86_64 -bios $(OVMF_PATH) -drive format=raw,file=krexo_disk.img -debugcon stdio

clean:
	find src -name "*.o" -delete
	rm -f src/boot/bios/*.bin boot.bin boot.efi krexo_disk.img kernel.elf
	rm -rf build/
