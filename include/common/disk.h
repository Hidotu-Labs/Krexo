#ifndef KREXO_DISK_H
#define KREXO_DISK_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t drive_id;
    int (*read_sectors)(uint8_t drive, uint64_t lba, uint32_t count, void* buffer);
} krexo_disk_t;

#endif // KREXO_DISK_H
