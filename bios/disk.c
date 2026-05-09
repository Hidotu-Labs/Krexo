#include <common/disk.h>
#include <common/kprint.h>

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_SEL    0x1F6
#define ATA_PRIMARY_COMM_STAT    0x1F7

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void insw(uint16_t port, void *addr, uint32_t count) {
    asm volatile ("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

static void ata_io_wait() {
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
}

int ata_read_sectors(uint8_t drive, uint64_t lba, uint32_t count, void* buffer) {
    // Drive selection (0xA0 for primary master, 0xB0 for slave)
    // We assume the boot drive (0x80) is the primary master
    uint8_t drive_select = 0xE0;
    if (drive == 0x81) drive_select = 0xF0; 

    outb(ATA_PRIMARY_DRIVE_SEL, drive_select | ((lba >> 24) & 0x0F));
    ata_io_wait();
    
    outb(ATA_PRIMARY_SECCOUNT, (uint8_t)count);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMM_STAT, 0x20); // READ SECTORS

    uint8_t *ptr = (uint8_t *)buffer;
    for (uint32_t i = 0; i < count; i++) {
        // Wait for BSY to clear
        int timeout = 100000;
        while ((inb(ATA_PRIMARY_COMM_STAT) & 0x80) && --timeout);
        if (timeout == 0) return -1;

        // Wait for DRQ to set
        timeout = 100000;
        while (!(inb(ATA_PRIMARY_COMM_STAT) & 0x08) && --timeout);
        if (timeout == 0) return -1;

        insw(ATA_PRIMARY_DATA, ptr, 256);
        ptr += 512;
        ata_io_wait();
    }

    return 0;
}

int bios_disk_init(krexo_disk_t *disk, uint8_t drive_id) {
    disk->drive_id = drive_id;
    disk->read_sectors = ata_read_sectors;
    
    // Check if controller is alive
    uint8_t status = inb(ATA_PRIMARY_COMM_STAT);
    if (status == 0xFF) return -1; // Floating bus / No controller
    
    return 0;
}
