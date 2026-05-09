#include <common/fs/fat32.h>
#include <common/disk.h>
#include <common/kprint.h>
#include <stdint.h>

static krexo_disk_t *g_disk = NULL;
static uint32_t g_partition_lba = 0;

typedef struct {
    uint8_t  boot_jmp[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_sectors_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t info_cluster;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_num;
    uint8_t  reserved1;
    uint8_t  boot_sig;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} __attribute__((packed)) fat32_vbr_t;

static fat32_vbr_t g_vbr;
extern void debug_putc(char c);

static void debug_print(const char* s) {
    while (*s) debug_putc(*s++);
}

int fat32_init(krexo_disk_t *disk) {
    g_disk = disk;
    uint8_t buf[512];
    if (disk->read_sectors(disk->drive_id, 0, 1, buf) != 0) return -1;
    if (buf[82] == 'F' && buf[83] == 'A' && buf[84] == 'T') {
        g_partition_lba = 0;
    } else {
        for (int i = 0; i < 4; i++) {
            uint32_t lba = *(uint32_t*)&buf[446 + (i * 16) + 8];
            uint8_t type = buf[446 + (i * 16) + 4];
            if (type == 0x0C || type == 0x0B) { g_partition_lba = lba; break; }
        }
        if (disk->read_sectors(disk->drive_id, g_partition_lba, 1, buf) != 0) return -1;
    }
    for (int i = 0; i < sizeof(fat32_vbr_t); i++) ((uint8_t*)&g_vbr)[i] = buf[i];
    return 0;
}

static uint32_t get_cluster_lba(uint32_t cluster) {
    uint32_t fat_lba = g_partition_lba + g_vbr.reserved_sectors;
    uint32_t data_lba = fat_lba + (g_vbr.fat_count * g_vbr.sectors_per_fat);
    return data_lba + (cluster - 2) * g_vbr.sectors_per_cluster;
}

static uint32_t get_next_cluster(uint32_t cluster) {
    uint32_t fat_lba = g_partition_lba + g_vbr.reserved_sectors;
    uint32_t ent_per_sec = 512 / 4;
    uint32_t sec = cluster / ent_per_sec;
    uint32_t off = cluster % ent_per_sec;
    
    uint32_t buf[128];
    if (g_disk->read_sectors(g_disk->drive_id, fat_lba + sec, 1, buf) != 0) return 0xFFFFFFFF;
    return buf[off] & 0x0FFFFFFF;
}

int fat32_find_file(const char* filename, uint32_t* out_cluster, uint32_t* out_size) {
    uint8_t buf[512];
    uint32_t cluster = g_vbr.root_cluster;

    while (cluster < 0x0FFFFFF8) {
        uint32_t lba = get_cluster_lba(cluster);
        for (int s = 0; s < g_vbr.sectors_per_cluster; s++) {
            if (g_disk->read_sectors(g_disk->drive_id, lba + s, 1, buf) != 0) return -1;
            fat32_dir_entry_t* entries = (fat32_dir_entry_t*)buf;
            for (int i = 0; i < 512 / sizeof(fat32_dir_entry_t); i++) {
                if (entries[i].name[0] == 0) continue; // Don't stop early
                if (entries[i].name[0] == 0xE5 || entries[i].attr == 0x0F) continue;
                
                // Match KREXO or KERNEL
                int match = 1;
                const char* target = (filename[0] == 'k' && filename[1] == 'r') ? "KREXO" : "KERNEL";
                for (int j = 0; j < 5; j++) if (entries[i].name[j] != target[j]) match = 0;
                
                if (match) {
                    *out_cluster = ((uint32_t)entries[i].cluster_high << 16) | entries[i].cluster_low;
                    *out_size = entries[i].size;
                    return 0;
                }
            }
        }
        cluster = get_next_cluster(cluster);
    }
    return -1;
}

int fat32_read_file(uint32_t cluster, void* buffer, uint32_t size) {
    uint8_t* out = (uint8_t*)buffer;
    uint32_t remaining = size;
    uint8_t sector_buf[512];

    while (remaining > 0 && cluster < 0x0FFFFFF8) {
        uint32_t lba = get_cluster_lba(cluster);
        for (int s = 0; s < g_vbr.sectors_per_cluster && remaining > 0; s++) {
            if (remaining >= 512) {
                if (g_disk->read_sectors(g_disk->drive_id, lba + s, 1, out) != 0) return -1;
                out += 512;
                remaining -= 512;
            } else {
                if (g_disk->read_sectors(g_disk->drive_id, lba + s, 1, sector_buf) != 0) return -1;
                for (uint32_t k = 0; k < remaining; k++) out[k] = sector_buf[k];
                remaining = 0;
            }
        }
        cluster = get_next_cluster(cluster);
    }
    return 0;
}
