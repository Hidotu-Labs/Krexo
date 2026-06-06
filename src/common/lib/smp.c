#include <common/kprint.h>
#include <common/smp.h>
#include <common/trampoline_data.h>
#include <stddef.h>
#include <stdint.h>

static void lapic_write(uint32_t reg, uint32_t val) {
  volatile uint32_t *lapic = (volatile uint32_t *)(uintptr_t)LAPIC_BASE;
  lapic[reg / 4] = val;
}

static uint32_t lapic_read(uint32_t reg) {
  volatile uint32_t *lapic = (volatile uint32_t *)(uintptr_t)LAPIC_BASE;
  return lapic[reg / 4];
}

// Simple delay function
static void delay(uint32_t count) {
  for (volatile uint32_t i = 0; i < count * 1000; i++)
    __asm__("pause");
}

void smp_wakeup_aps(krexo_smp_info_t *cpus, uint32_t count,
                    uint32_t bsp_lapic_id, uint32_t pml4) {
  kprintf("[SMP] Waking up %d APs...\n", count - 1);

  // 1. Copy trampoline to 0x4000
  uint8_t *trampoline_dst = (uint8_t *)0x4000;
  size_t trampoline_size = src_boot_common_trampoline_bin_len;
  uint8_t *trampoline_src = src_boot_common_trampoline_bin;

  for (size_t i = 0; i < trampoline_size; i++) {
    trampoline_dst[i] = trampoline_src[i];
  }

  // 2. Set the CR3 value for APs at 0x4500 (Expected by trampoline.s)
  *(uint32_t *)0x4500 = pml4;

  // 3. Send INIT and SIPI to each AP
  for (uint32_t i = 0; i < count; i++) {
    if (cpus[i].lapic_id == bsp_lapic_id)
      continue;

    uint32_t apic_id = cpus[i].lapic_id;

    // INIT IPI
    lapic_write(LAPIC_ICR_HIGH, apic_id << 24);
    lapic_write(LAPIC_ICR_LOW, 0x0000C500); // Level-triggered INIT
    delay(10);

    // Startup IPI (SIPI) - Point to 0x4000 (vector 0x04)
    lapic_write(LAPIC_ICR_HIGH, apic_id << 24);
    lapic_write(LAPIC_ICR_LOW, 0x00000604); // Vector 0x04 -> Address 0x4000
    delay(1);

    // Second SIPI
    lapic_write(LAPIC_ICR_HIGH, apic_id << 24);
    lapic_write(LAPIC_ICR_LOW, 0x00000604);
    delay(1);
  }

  kprintf("[SMP] All APs initialized.\n");
}
