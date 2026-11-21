/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "timer.h"
#include "dev-yp3.h"
#include "pmu-yp3.h"
#include "recovery-yp3.h"
#include "fmradio_i2c.h"
#include "mmio-sl6801.h"

#define STR_(x) #x
#define STR(x) STR_(x)

/* MUSB POWER register (sl6801.h); SOFTCONN switches the D+ pull-up. */
#define USB_POWER          (USB_BASE + USB_REG_POWER)

/* Our bound on the ack poll after enabling a clock gate (an arbitrary
 * safety limit). */
#define CLK_GATE_POLLS     10000u

/* Clocks, XIP and the recovery watchdog are set up before C runs (copied
 * early entry + crt0.S); SysTick is started by the kernel
 * (kernel-sl6801.c). */
void system_init(void)
{
    yp3_dev_init();
#ifdef YP3_WATCHDOG_POWEROFF
    /* before anything touches storage or the LCD */
    yp3_recovery_check_watchdog_reset();
#endif
#ifdef YP3_RECOVERY_WATCHDOG
    yp3_recovery_watchdog_start();
#endif
#if CONFIG_TUNER
    fmradio_i2c_init();
#endif
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    set_cpu_frequency(CPUFREQ_NORMAL);
#endif
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
/* CPU clock = PLL2 (384 MHz) / divider, field CLK_CPU[5:2] = divider - 1
 * (write, short wait, poll bit 31 clear). Only the
 * divisors 2 (192 MHz) and 6 (64 MHz) are verified. AHB, NOR, SysTick
 * and peripheral clocks have their own dividers and do not change. */
#define CPU_DIV_SETTLE_US 20        /* "short wait" before polling bit 31 */
#define CPU_DIV_POLLS 100000        /* our bound on the bit 31 poll */
/* Cold PLL2; the CPU source. Revisit if USB
 * bring-up retunes PLL2. */
#define PLL2_HZ 384000000
#define CPU_DIV_MAX_FREQ 2          /* 384 / 2 = 192 MHz (CPUFREQ_MAX) */
#define CPU_DIV_NORMAL   6          /* 384 / 6 = 64 MHz */

/* Runs from SRAM: switching the divider while the core fetches from XIP
 * stopped the core now and then (silent watchdog returns under torture,
 * none with the clock pinned).
 * IRQs are masked. */
static int ICODE_ATTR NO_INLINE cpu_div_switch(uint32_t div)
{
    REG(CLK_CPU) = (REG(CLK_CPU) & ~CLK_CPU_DIV_MASK) |
                   (div - 1) << CLK_CPU_DIV_SHIFT;
    for (uint32_t t = REG(SL6801_COUNTER_LO);
         REG(SL6801_COUNTER_LO) - t < CPU_DIV_SETTLE_US; )
        ;
    /* bit 31 set (negative) while the new divider is applied */
    int polls = 0;
    while ((int32_t)REG(CLK_CPU) < 0 && ++polls < CPU_DIV_POLLS)
        ;
    return polls;
}

void set_cpu_frequency(long frequency)
{
    bool max = frequency >= CPUFREQ_MAX;
    uint32_t div = max ? CPU_DIV_MAX_FREQ : CPU_DIV_NORMAL;
    if (!yp3_core_voltage(max, false)) {
        dev_log("cpu freq: core voltage not raised, clock kept");
        return;
    }
    int irq = disable_irq_save();
    int polls = cpu_div_switch(div);
    /* Report what the register holds, not what we asked for, in case the
     * switch did not finish. */
    uint32_t cur = ((REG(CLK_CPU) & CLK_CPU_DIV_MASK) >> CLK_CPU_DIV_SHIFT) + 1;
    cpu_frequency = PLL2_HZ / cur;
    restore_irq(irq);
    yp3_core_voltage(cpu_frequency >= CPUFREQ_MAX, true);
    if (polls >= CPU_DIV_POLLS)
        dev_log("cpu freq: divider %lu still busy, CLK_CPU=%08lx",
                    (unsigned long)div, (unsigned long)REG(CLK_CPU));
}
#endif

/* Set by the USB driver while the D+ pull-up (MUSB POWER.SOFTCONN) is on.
 * A watchdog reset keeps the pull-up, so the host never sees the device
 * leave and ROM recovery is not re-enumerated: drop it before giving up. */
volatile uint8_t yp3_usb_softconn;

__attribute__((section(".data.yp3_usb_drop_softconn")))
void yp3_usb_drop_softconn(void)
{
    if (yp3_usb_softconn) {
        *(volatile uint8_t *)USB_POWER &= ~USB_POWER_SOFTCONN;
        yp3_usb_softconn = 0;
    }
}

/* Park with IRQs masked, executing from SRAM and keeping bus traffic going,
 * so an armed recovery watchdog expires even if XIP is unusable. */
__attribute__((section(".data.yp3_recovery_park"), naked, noreturn,
               aligned(4)))
void yp3_recovery_park(void)
{
    __asm__ volatile("cpsid i\n"
                     "bl yp3_usb_drop_softconn\n"
                     "ldr r1, 2f\n"
                     "1: ldr r2, [r1]\n"
                     "b 1b\n"
                     ".balign 4\n"
                     "2: .word " STR(SL6801_COUNTER_LO));
}

#define UDELAY_POLL_LIMIT 10000000u

void sl6801_udelay(uint32_t us)
{
    uint32_t start = sl6801_us();
    bool can_yield = irq_enabled() && get_interrupt_number() == 0;
    for (uint32_t i = 0;
         i < UDELAY_POLL_LIMIT && sl6801_us() - start < us; i++)
        if (can_yield)
            yield();
}

/* No caches on this Cortex-M4: barriers are enough for DMA/code loading. */
void commit_dcache(void) { __asm__ volatile("dsb" ::: "memory"); }
void commit_discard_dcache(void) { commit_dcache(); }
void commit_discard_idcache(void)
{
    commit_dcache();
    __asm__ volatile("isb" ::: "memory");
}
void commit_discard_dcache_range(const void *p, unsigned int n)
{ (void)p; (void)n; commit_dcache(); }
void commit_dcache_range(const void *p, unsigned int n)
{ (void)p; (void)n; commit_dcache(); }
void discard_dcache_range(const void *p, unsigned int n)
{ (void)p; (void)n; commit_dcache(); }

bool dbg_ports(void) { return false; }
void i2c_init(void) {}

/* No user timer: timer_set() returns false, so plugins that use the user
 * timer do not get it. */
bool timer_set(long cycles, bool start)
{
    (void)cycles;
    (void)start;
    return false;
}
bool timer_start(void) { return false; }
void timer_stop(void) {}

/* Turn on clock gate `id` (CLK_GATE_ID_* / the ids in sl6801.h) unless it
 * is already on: query the request and ack bits; if not both set, set the
 * request bit, then the ack bit, and poll the ack bit. False if the ack
 * never comes. */
bool sl6801_clock_gate_enable(unsigned id)
{
    const uint32_t req = CLK_GATE_REQ(id);
    const uint32_t ack = CLK_GATE_ACK(id);
    const uint32_t bit = CLK_GATE_BIT(id);
    /* IRQs off only for the read-modify-writes of the shared words; the
     * ack poll runs with them on. */
    int old = disable_irq_save();
    bool ready = (REG(req) & bit) && (REG(ack) & bit);
    if (!ready) {
        REG(req) = REG(req) | bit;
        REG(ack) = REG(ack) | bit;
        __asm__ volatile("dsb" ::: "memory");
    }
    restore_irq(old);
    for (unsigned polls = 0; !ready && polls < CLK_GATE_POLLS; ++polls)
        ready = REG(ack) & bit;
    if (!ready)
        dev_log("clock gate %u: no ack", id);
    return ready;
}

/* Turn off clock gate `id`: clear the ack bit, then the request bit; no
 * poll. */
void sl6801_clock_gate_disable(unsigned id)
{
    const uint32_t bit = CLK_GATE_BIT(id);
    int old = disable_irq_save();
    REG(CLK_GATE_ACK(id)) = REG(CLK_GATE_ACK(id)) & ~bit;
    REG(CLK_GATE_REQ(id)) = REG(CLK_GATE_REQ(id)) & ~bit;
    restore_irq(old);
}
