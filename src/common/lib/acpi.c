#include <common/acpi.h>
#include <common/kprint.h>
#include <stddef.h>
#include <stdint.h>

static acpi_rsdp_t *g_rsdp = NULL;

static int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *p1 = s1, *p2 = s2;
  while (n--) {
    if (*p1 != *p2)
      return *p1 - *p2;
    p1++;
    p2++;
  }
  return 0;
}

void acpi_init(void *rsdp) { g_rsdp = (acpi_rsdp_t *)rsdp; }

void *acpi_find_table(const char *signature) {
  if (!g_rsdp) {
    // Try to find RSDP in BIOS if not initialized (fallback)
    for (uint8_t *p = (uint8_t *)0xE0000; p < (uint8_t *)0x100000; p += 16) {
      if (memcmp(p, "RSD PTR ", 8) == 0) {
        g_rsdp = (acpi_rsdp_t *)p;
        break;
      }
    }
  }

  if (!g_rsdp)
    return NULL;

  if (g_rsdp->revision >= 2 && g_rsdp->xsdt_address != 0) {
    // Search in XSDT
    acpi_xsdt_t *xsdt = (acpi_xsdt_t *)(uintptr_t)g_rsdp->xsdt_address;
    size_t entries = (xsdt->header.length - sizeof(acpi_sdt_header_t)) / 8;
    for (size_t i = 0; i < entries; i++) {
      acpi_sdt_header_t *header = (acpi_sdt_header_t *)(uintptr_t)xsdt->entry[i];
      if (memcmp(header->signature, signature, 4) == 0) {
        return header;
      }
    }
  } else if (g_rsdp->rsdt_address != 0) {
    // Search in RSDT
    acpi_rsdt_t *rsdt = (acpi_rsdt_t *)(uintptr_t)g_rsdp->rsdt_address;
    size_t entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4;
    for (size_t i = 0; i < entries; i++) {
      acpi_sdt_header_t *header = (acpi_sdt_header_t *)(uintptr_t)rsdt->entry[i];
      if (memcmp(header->signature, signature, 4) == 0) {
        return header;
      }
    }
  }

  return NULL;
}
