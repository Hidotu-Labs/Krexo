#!/bin/bash
set -e

# Configuration
DISK_IMG="krexo_disk.img"
BOOT_BIN="boot.bin"
STAGE2_BIN="bios/stage2.bin"

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
dd if=bios/stage1.bin of=$DISK_IMG bs=446 count=1 conv=notrunc

# 5. Install Stage 2 to the reserved sectors
dd if=$STAGE2_BIN of=$DISK_IMG bs=512 seek=1 conv=notrunc

# 6. Setup UEFI Directories in the FAT32 partition
mmd -i $DISK_IMG@@1M ::/EFI || true
mmd -i $DISK_IMG@@1M ::/EFI/BOOT || true

# 7. Copy files
mcopy -o -i $DISK_IMG@@1M krexo.conf ::/krexo.conf || true
mcopy -o -i $DISK_IMG@@1M kernel.elf ::/kernel.elf || true

echo "This is a test kernel file content!" > kernel.txt
mcopy -o -i $DISK_IMG@@1M kernel.txt ::/kernel.txt || true
rm kernel.txt

# 8. Copy UEFI executable if it exists
if [ -f boot.efi ]; then
    mcopy -o -i $DISK_IMG@@1M boot.efi ::/EFI/BOOT/BOOTX64.EFI || true
fi

echo "Success! Created $DISK_IMG"
