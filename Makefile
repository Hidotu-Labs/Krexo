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

uefi_boot: uefi/main.o uefi/debug.o uefi/mmap.o common/lib/printf_uefi.o
	$(LD_UEFI) $(LDFLAGS_UEFI) uefi/main.o uefi/debug.o uefi/mmap.o common/lib/printf_uefi.o -out:boot.efi

# BIOS build
bios/stage1.bin: bios/stage1.s
	$(AS) -f bin bios/stage1.s -o $@

bios/stage2.o: bios/stage2.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

bios/mmap.o: bios/mmap.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

common/lib/printf_bios.o: common/lib/printf.c
	$(CC_BIOS) $(CFLAGS_BIOS) -c $< -o $@

bios/stage2.bin: bios/stage2.o bios/mmap.o common/lib/printf_bios.o
	$(LD_BIOS) $(LDFLAGS_BIOS) $^ -o $@

bios_boot: bios/stage1.bin bios/stage2.bin
	cat bios/stage1.bin bios/stage2.bin > boot.bin
	truncate -s 5120 boot.bin

run_bios: bios_boot
	qemu-system-x86_64 -drive format=raw,file=boot.bin

run_uefi: uefi_boot
	mkdir -p build/uefi/EFI/BOOT
	cp boot.efi build/uefi/EFI/BOOT/BOOTX64.EFI
	qemu-system-x86_64 -bios $(OVMF_PATH) -drive format=raw,file=fat:rw:build/uefi

clean:
	rm -rf bios/*.o bios/*.bin uefi/*.o uefi/*.elf common/lib/*.o boot.bin boot.efi build/
