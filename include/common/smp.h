#ifndef KREXO_SMP_H
#define KREXO_SMP_H

#include <common/requests.h>
#include <stdint.h>

#define LAPIC_BASE 0xFEE00000
#define LAPIC_ICR_LOW 0x300
#define LAPIC_ICR_HIGH 0x310

void smp_wakeup_aps(krexo_smp_info_t *cpus, uint32_t count,
                    uint32_t bsp_lapic_id, uint32_t pml4);

#endif // KREXO_SMP_H
