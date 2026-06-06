#ifndef KREXO_ACPI_H
#define KREXO_ACPI_H

#include <stdint.h>

typedef struct {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t revision;
  uint32_t rsdt_address;
  uint32_t length;
  uint64_t xsdt_address;
  uint8_t extended_checksum;
  uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

typedef struct {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct {
  acpi_sdt_header_t header;
  uint32_t entry[];
} __attribute__((packed)) acpi_rsdt_t;

typedef struct {
  acpi_sdt_header_t header;
  uint64_t entry[];
} __attribute__((packed)) acpi_xsdt_t;

// MADT (Multiple APIC Description Table)
typedef struct {
  acpi_sdt_header_t header;
  uint32_t lapic_address;
  uint32_t flags;
} __attribute__((packed)) acpi_madt_t;

typedef struct {
  uint8_t type;
  uint8_t length;
} __attribute__((packed)) acpi_madt_entry_t;

// MADT Type 0: Processor Local APIC
typedef struct {
  acpi_madt_entry_t header;
  uint8_t processor_id;
  uint8_t lapic_id;
  uint32_t flags; // Bit 0: Enabled
} __attribute__((packed)) acpi_madt_lapic_t;

void *acpi_find_table(const char *signature);
void acpi_init(void *rsdp);

#endif // KREXO_ACPI_H
