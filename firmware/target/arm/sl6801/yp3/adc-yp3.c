/* SPDX-License-Identifier: GPL-2.0-or-later */
/* ADC controller 0x40096000: only channel 1 (button ladder) is used
 * (F-0018). */
#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "adc.h"
#include "adc-target.h"
#include "adc-yp3.h"
#include "gpio-yp3.h"
#include "irq-yp3.h"
#include "mmio-sl6801.h"
#ifdef YP3_DEV
#include "devlog.h"
#endif

/* Registers (F-0019). Field meanings are unknown; the values are those
 * shown to convert the key channel. */
#define ADC_CTRL        (ADC_BASE + 0x00u)  /* bit n: channel n enabled */
#define ADC_CTRL_INIT   0x80180000u
#define ADC_RUN         (ADC_BASE + 0x04u)  /* 0 while set up */
#define ADC_RUN_VALUE   0xa800u             /* then converts continuously */
#define ADC_REG_0C      (ADC_BASE + 0x0cu)
#define ADC_MODE0       (ADC_BASE + 0x10u)  /* 4-bit mode, channels 0..7 */
#define ADC_MODE1       (ADC_BASE + 0x18u)  /* channels 8..9 */
#define ADC_CH_CONFIG(ch) (ADC_BASE + 0x20u + (ch) * 0x10u)
#define ADC_KEY_CONFIG  0x0fff0030u         /* channel 1 (F-0036) */
#define ADC_CHANNELS    10u
#define ADC_GATE_SETTLE_US 50u              /* F-0020 step 1 */
#define ADC_SETTLE_US   5000u               /* F-0020 step 4 */

/* Interrupt status words; writing back what was read retires the
 * interrupt (F-0019, measured). */
#define ADC_STATUS0     (ADC_BASE + 0x14u)
#define ADC_STATUS1     (ADC_BASE + 0x1cu)

static bool adc_ok;

/* The key ladder pad (GPIO3.0) as analog input to the ADC. */
static void configure_pin(void)
{
    yp3_pin_config(PIN_OWNER_KEYS, PIN_KEY_LADDER);
}

/* F-0020: clock, reset values, channel 1 only, settle, then run. The
 * waits are not shown to be needed (F-0020: GUESS); they cost 5 ms once.
 * Battery voltage comes from the PMU, not this block (F-0018). */
static bool adc_controller_start(void)
{
    if (!sl6801_clock_gate_enable(CLK_GATE_ID_ADC))
        return false;
    sl6801_udelay(ADC_GATE_SETTLE_US);
    REG(ADC_CTRL) = ADC_CTRL_INIT;
    REG(ADC_RUN) = 0u;
    REG(ADC_REG_0C) = 0u;
    REG(ADC_MODE0) = 0u;
    REG(ADC_MODE1) = 0u;
    REG(ADC_CTRL) |= 1u << ADC_KEY_CHANNEL;
    for (unsigned ch = 0; ch < ADC_CHANNELS; ch++)
        REG(ADC_CH_CONFIG(ch)) = ch == ADC_KEY_CHANNEL ? ADC_KEY_CONFIG : 0u;
    sl6801_udelay(ADC_SETTLE_US);
    REG(ADC_RUN) = ADC_RUN_VALUE;
    return true;
}

static bool controller_init(void)
{
    int oldlevel;

    if (!adc_controller_start())
        return false;
    yp3_irq_install(ADC_IRQ, yp3_adc_irq);
    yp3_irq_enable(ADC_IRQ);
    __asm__ volatile("dsb\n\tisb" ::: "memory");
    oldlevel = disable_irq_save();
    configure_pin();
    restore_irq(oldlevel);
    return true;
}

void adc_init(void)
{
    static bool attempted;
    int oldlevel;
    if (attempted)
        return;
    attempted = true;
    adc_ok = controller_init();
#ifdef YP3_DEV
    if (adc_ok) {
        sl6801_udelay(1000);
        dev_log("adc: key ladder idle %03lx", (unsigned long)REG(ADC_KEY_SAMPLE));
    }
#endif
    if (!adc_ok) {
        oldlevel = disable_irq_save();
        yp3_irq_disable(ADC_IRQ);
        __asm__ volatile("dsb" ::: "memory");
        restore_irq(oldlevel);
    }
}

__attribute__((section(".icode"))) bool yp3_adc_ready(void)
{
    return adc_ok;
}

unsigned short adc_read(int channel)
{
    if (channel != ADC_BUTTONS || !adc_ok)
        return 0xffffu;             /* no such channel / ADC not running */
    return (unsigned short)REG(ADC_KEY_SAMPLE);
}

/* No conversion consumer: write the captured status words back to retire
 * the interrupt. */
void yp3_adc_irq(void)
{
    uint32_t status0 = REG(ADC_STATUS0);
    uint32_t status1 = REG(ADC_STATUS1);
    REG(ADC_STATUS0) = status0;
    REG(ADC_STATUS1) = status1;
}
