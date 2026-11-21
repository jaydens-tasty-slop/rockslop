/* SPDX-License-Identifier: GPL-2.0-or-later */
/* YP3 backlight: PWM channel 3 on GPIO1.17 drives the LED. Clock and
 * channel set-up: gate 0x44, selector 0x27, channel profile. */
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "lcd-yp3.h"
#include "gpio-yp3.h"
#include "mmio-sl6801.h"

/* PWM block channel 3 (PWM_BASE, sl6801.h). */
#define PWM3_BASE       PWM_CH_BASE(3)
/* Control word: PWM_CTRL_BIT6 is written alone, then the channel profile
 * is ORed in (bit 4, bit 5, bits 3:0). Bit 28 must be clear before the
 * compare word is written, hence "busy". */
#define PWM3_CONTROL    (PWM3_BASE + 0x00u)
#define PWM_CTRL_ENABLE (1u << 4)   /* clearing it disables the channel */
#define PWM_CTRL_BIT5   (1u << 5)   /* role unknown */
#define PWM_CTRL_BIT6   (1u << 6)   /* written alone first; role unknown */
#define PWM_CTRL_LOW4(n) ((n) & 0xfu) /* role unknown */
#define PWM_CTRL_BUSY   (1u << 28)
/* The backlight setting is 0x3f. */
#define PWM3_SETUP      (PWM_CTRL_ENABLE | PWM_CTRL_BIT5 | \
                         PWM_CTRL_LOW4(0x0fu))
/* Compare word: period in bits 31:16, duty count in bits 15:0 (48000, percent * 480). 48000 counts at the 24 MHz PWM clock = 500 Hz
 * (the mic picks it up as a 500 Hz hum). */
#define PWM3_COMPARE    (PWM3_BASE + 0x04u)
#define PWM3_PERIOD     48000u
#define PWM3_PER_PERCENT (PWM3_PERIOD / 100u)   /* 480 counts = 1 % duty */
/* Selector 0x27 source bit: clear = source 10 (decimal), used here. */
#define SELECTOR27_SOURCE (1u << 4)
#define SELECTOR27_BITS (SELECTOR27_SOURCE | CLK_SEL_ENABLE)
#define POLL_MAX        10000u
#define COLD_DUTY_PERCENT 25u       /* cold-setup duty, before settings load */

static int set_duty(unsigned percent, uint32_t poll_budget)
{
    for (uint32_t i = 0; i < poll_budget; ++i) {
        if (!(REG(PWM3_CONTROL) & PWM_CTRL_BUSY)) {
            REG(PWM3_COMPARE) =
                (PWM3_PERIOD << 16) | (percent * PWM3_PER_PERCENT);
            return 0;
        }
    }
    return -1;
}

int yp3_backlight_cold_setup(void)
{
    /* Only the source/enable bits of selector 0x27 may change below; the
     * other bits are checked again after the gate is up. */
    const uint32_t other_bits = REG(CLK_SEL_27) & ~SELECTOR27_BITS;

    if (REG(PWM3_CONTROL) & PWM_CTRL_BUSY)
        return -1;

    yp3_pin_config(PIN_OWNER_BACKLIGHT, PIN_BACKLIGHT_PWM);

    if (!sl6801_clock_gate_enable(CLK_GATE_ID_PWM))
        return -1;

    uint32_t selector = REG(CLK_SEL_27);
    if ((selector & ~SELECTOR27_BITS) != other_bits)
        return -1;
    if (selector & SELECTOR27_SOURCE) {
        selector &= ~SELECTOR27_SOURCE; /* source 10 */
        REG(CLK_SEL_27) = selector;
    }
    if (!(selector & CLK_SEL_ENABLE)) {
        selector |= CLK_SEL_ENABLE;
        REG(CLK_SEL_27) = selector;
    }

    /* Write bit 6 alone, then OR the setting. */
    REG(PWM3_CONTROL) = PWM_CTRL_BIT6;
    REG(PWM3_CONTROL) = REG(PWM3_CONTROL) | PWM3_SETUP;

    return set_duty(COLD_DUTY_PERCENT, POLL_MAX);
}

static int backlight_set_checked(int brightness)
{
    if (brightness < 0 || brightness > 100 || !yp3_lcd_ready())
        return -1;
    if (!(REG(PWM3_CONTROL) & PWM_CTRL_ENABLE))
        return -1; /* channel disabled */
    return set_duty((unsigned)brightness, POLL_MAX);
}

/* Rockbox brightness levels 1..10 -> PWM duty 10..100 %. */
static int duty_pct = DEFAULT_BRIGHTNESS_SETTING * 10;
static bool lit = true;

bool backlight_hw_init(void)
{
    return yp3_lcd_ready();     /* PWM3 is set up by lcd_init_device() */
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true);   /* wakes the panel if lcd_sleep() put it to sleep */
#endif
    lit = true;
    (void)backlight_set_checked(duty_pct);
}

void backlight_hw_off(void)
{
    lit = false;
    (void)backlight_set_checked(0);
}

void backlight_hw_brightness(int brightness)
{
    duty_pct = brightness * 10;
    if (lit)
        (void)backlight_set_checked(duty_pct);
}

void lcd_backlight(bool on)
{
    (void)backlight_set_checked(on ? 100 : 0);
}
