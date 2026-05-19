#!/bin/bash
set -e

# Configuration
DISK_IMG="krexo_disk.img"
BOOT_BIN="boot.bin"
STAGE2_BIN="src/boot/bios/stage2.bin"
STAGE1_BIN="src/boot/bios/stage1.bin"

# 1. Create a 64MB empty disk image
dd if=/dev/zero of=$DISK_IMG bs=1M count=64

# 2. Setup partition table (MBR)
(
echo n # Add a new partition
echo p # Primary partition
echo 1 # Partition number
echo 2048 # First sector
echo      # Last sector (default: end of disk)
echo t # Change partition type
echo c # W95 FAT32 (LBA)
echo a # Make it bootable
echo w # Write changes
) | fdisk $DISK_IMG || true

# 3. Create FAT32 filesystem within the partition
mkfs.vfat -F 32 --offset=2048 $DISK_IMG

# 4. Install Stage 1 to the MBR
if [ -f $STAGE1_BIN ]; then
    dd if=$STAGE1_BIN of=$DISK_IMG bs=446 count=1 conv=notrunc
fi

# 5. Install Stage 2 to the reserved sectors
if [ -f $STAGE2_BIN ]; then
    dd if=$STAGE2_BIN of=$DISK_IMG bs=512 seek=1 conv=notrunc
fi

# 6. Setup UEFI Directories in the FAT32 partition
mmd -i $DISK_IMG@@1M ::/EFI || true
mmd -i $DISK_IMG@@1M ::/EFI/BOOT || true

# 7. Copy files
mcopy -o -i $DISK_IMG@@1M krexo.conf ::/krexo.conf || true
mcopy -o -i $DISK_IMG@@1M kernel.elf ::/kernel.elf || true

# Copy background images (both PNG and BMP)
if [ -f a.png ]; then
    mcopy -o -i $DISK_IMG@@1M a.png ::/a.png || true
elif [ -f assets/a.png ]; then
    mcopy -o -i $DISK_IMG@@1M assets/a.png ::/a.png || true
fi

if [ -f background.bmp ]; then
    mcopy -o -i $DISK_IMG@@1M background.bmp ::/background.bmp || true
elif [ -f assets/background.bmp ]; then
    mcopy -o -i $DISK_IMG@@1M assets/background.bmp ::/background.bmp || true
fi

# 8. Copy UEFI executable if it exists
if [ -f boot.efi ]; then
    mcopy -o -i $DISK_IMG@@1M boot.efi ::/EFI/BOOT/BOOTX64.EFI || true
fi

echo "Success! Created $DISK_IMG"
