/* SPDX-License-Identifier: GPL-2.0-or-later */
/* USB board bring-up. Applies the difference between Rockbox's running state
 * and the boot ROM's working USB state (measured 2026-10-17): PMU rails 7 and
 * 0xf, clock gate 35, selector 18, 0x40000000 bits 0/6, PHY pins GPIO1.22/23
 * and the controller mode word. The ROM's own sequence also reprograms PLL2
 * and the CPU/bus dividers; USB works at our clocks, so those steps are left
 * out. */
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "power.h"
#include "usb_drv.h"
#include "usb-target.h"
#include "pmu-yp3.h"
#include "gpio-yp3.h"
#include "irq-yp3.h"
#include "dev-yp3.h"
#include "mmio-sl6801.h"


/* Settling after the gate, standing in for the ~2000-NOP wait in the ROM's
 * sequence. */
#define USB_GATE_SETTLE_US 20u

/* The supplies (PMU_USB_RAIL_*) are in pmu-yp3.h. */
#define PMU_POLL_BUDGET         64u     /* bridge busy polls per transfer */
#define PMU_SETTLE_US           50u     /* settle after each write */
/* Retries, one tick apart, to take the PMU bridge (the power thread may
 * hold it) before giving up. */
#define PMU_LOCK_ATTEMPTS       100

/* Bits of SYS_REG_00 (sl6801.h) the ROM's USB mode sets, bit 0 first. */
#define SYS_REG_00_BIT0         (1u << 0)
#define SYS_REG_00_BIT6         (1u << 6)

/* Controller registers used here besides USB_REG_POWER (sl6801.h): the
 * vendor mode word USB_MODE_WORD, whose bit 12 is set before
 * waiting ~50 ms. Bit 12 may select device mode; that is
 * unverified. */
#define USB_MODE_BIT12          (1u << 12)
#define USB_MODE_SETTLE_US      50000u

/* Waits applied by yp3_usb_board_controller_stage(): ~3 ms after SOFTCONN
 * is dropped, ~2 ms after the endpoint flushes. */
#define USB_DISCONNECT_SETTLE_US 3000u
#define USB_FLUSH_SETTLE_US      2000u


static bool prepared;


static bool pmu_rmw(uint8_t index, uint8_t keep, uint8_t value)
{
    uint8_t v;
    if (!yp3_pmu_read(index, PMU_POLL_BUDGET, &v) ||
        !yp3_pmu_write(index, (v & keep) | value, PMU_POLL_BUDGET))
        return false;
    sl6801_udelay(PMU_SETTLE_US);
    return true;
}

static void ICODE_ATTR usb_isr(void)
{
#ifdef YP3_DEV
    uint32_t t0 = REG(SL6801_COUNTER_LO);
    usb_drv_int();
    uint32_t us = REG(SL6801_COUNTER_LO) - t0;
    yp3_usb_stats.isr_us += us;
    if (us > yp3_usb_stats.isr_max_us)
        yp3_usb_stats.isr_max_us = us;
#else
    usb_drv_int();
#endif
}

bool yp3_usb_board_prepare(void)
{
    if (prepared)
        return true;

    /* Supply rails as the ROM's USB mode sets them (see PMU_USB_RAIL_*).
     * Without them the PHY never attaches (2026-10-18). */
    for (int i = 0; !yp3_pmu_try_lock(); i++) {
        if (i > PMU_LOCK_ATTEMPTS)
            return false;
        sleep(1);
    }
    bool ok =
        pmu_rmw(PMU_USB_RAIL_A, PMU_USB_RAIL_A_KEEP, PMU_USB_RAIL_A_STEP1) &&
        pmu_rmw(PMU_USB_RAIL_A, PMU_USB_RAIL_A_KEEP, PMU_USB_RAIL_A_STEP2) &&
        pmu_rmw(PMU_USB_RAIL_A, PMU_USB_RAIL_A_KEEP, PMU_USB_RAIL_A_STEP3) &&
        pmu_rmw(PMU_USB_RAIL_B, PMU_USB_RAIL_B_KEEP, PMU_USB_RAIL_B_VALUE);
    yp3_pmu_unlock();
    if (!ok)
        return false;

    /* Clock gate 35 (request + ack), then selector 18 (0x12). */
    if (!sl6801_clock_gate_enable(CLK_GATE_ID_USB))
        return false;
    sl6801_udelay(USB_GATE_SETTLE_US);
    REG(CLK_SEL_12) |= CLK_SEL_ENABLE;

    /* Then the system bits and the PHY pads, in the ROM's order. */
    REG(SYS_REG_00) |= SYS_REG_00_BIT0;
    REG(SYS_REG_00) |= SYS_REG_00_BIT6;
    yp3_pin_config(PIN_OWNER_USB, PIN_USB_1_23);
    yp3_pin_config(PIN_OWNER_USB, PIN_USB_1_22);

    REG(USB_MODE_WORD) |= USB_MODE_BIT12;
    sl6801_udelay(USB_MODE_SETTLE_US);

    yp3_irq_install(USB_IRQ, usb_isr);
    yp3_irq_priority(USB_IRQ, USB_IRQ_PRIO);
    yp3_irq_install(USB_DMA_IRQ, yp3_usb_drv_dma_int);
    yp3_irq_priority(USB_DMA_IRQ, USB_IRQ_PRIO);
    dev_log("usb board: power=%02x mode=%08lx gate=%08lx",
                YP3_USB_READ8(USB_REG_POWER),
                (unsigned long)REG(USB_MODE_WORD),
                (unsigned long)REG(CLK_GATE_ACK(CLK_GATE_ID_USB)));
    prepared = true;
    return true;
}

/* After the stack has stopped (unplug): the controller's clock selector
 * and gate go off; the next prepare() turns them on again. The PMU values
 * (PMU7 may be a shared supply), the 0x40000000 bits and
 * the pads stay as set: their other users are unknown. */
void yp3_usb_board_release(void)
{
    if (!prepared)
        return;
    yp3_usb_board_irq_enable(false);
    REG(CLK_SEL_12) &= ~CLK_SEL_ENABLE;
    sl6801_clock_gate_disable(CLK_GATE_ID_USB);
    prepared = false;
}

bool yp3_usb_board_controller_stage(enum yp3_usb_controller_stage stage)
{
    sl6801_udelay(stage == YP3_USB_CONTROLLER_DISCONNECTED ?
           USB_DISCONNECT_SETTLE_US : USB_FLUSH_SETTLE_US);
    return true;
}

bool yp3_usb_board_vbus_valid(void)
{
    return power_input_present();
}

void yp3_usb_board_irq_enable(bool enable)
{
    const uint32_t bits = NVIC_BIT(USB_IRQ) | NVIC_BIT(USB_DMA_IRQ);
    if (enable) {
        REG(NVIC_ICPR(USB_IRQ)) = bits;
        REG(NVIC_ISER(USB_IRQ)) = bits;
    } else {
        REG(NVIC_ICER(USB_IRQ)) = bits;
    }
}
