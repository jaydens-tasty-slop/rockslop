/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SL6801_SYSTEM_TARGET_H
#define SL6801_SYSTEM_TARGET_H
#include "system-arm.h"
#include "cpucache-arm.h"
#include <stdbool.h>

/* Clock gates by id (CLK_GATE_ID_* and driver ids, sl6801.h). Enable
 * returns true once the gate acknowledges. */
bool sl6801_clock_gate_enable(unsigned id);
void sl6801_clock_gate_disable(unsigned id);

#ifdef HAVE_THREAD_CPU_ACCOUNTING
/* core_sleep() that also counts the microseconds spent in WFI, for the dev
 * shell's "load" (system-arm-micro.h leaves it to the target) */
extern unsigned long yp3_idle_us;
static inline void core_sleep(void)
{
    unsigned long t0 = *(volatile unsigned long *)SL6801_COUNTER_LO;
    asm volatile ("wfi");
    yp3_idle_us += *(volatile unsigned long *)SL6801_COUNTER_LO - t0;
    enable_irq();
}
#endif
#endif
