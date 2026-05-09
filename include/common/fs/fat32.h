#pragma once
#include <stdint.h>
#include <common/disk.h>

typedef struct {
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed)) fat32_dir_entry_t;

int fat32_init(krexo_disk_t *disk);
int fat32_find_file(const char* filename, uint32_t* out_cluster, uint32_t* out_size);
int fat32_read_file(uint32_t cluster, void* buffer, uint32_t size);
