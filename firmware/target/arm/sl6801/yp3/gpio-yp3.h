/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SL6801 pads and the YP3 board pin map.
 *
 * Every pad configured through yp3_pin_config()/yp3_pin_write() is listed
 * in yp3_pins (gpio-yp3.c) with the one driver that owns it; a pad owned by
 * another driver is refused (and logged in dev builds). Add new pads to
 * that table rather than writing the pad registers directly. The
 * centre-key pad is in the table, but its IRQ rearm (button-yp3.c) writes
 * the pad's mux field directly; the bank-0 NOR pads set up by the boot code
 * are not covered.
 *
 * Not from a datasheet (there is no documentation): these definitions were
 * worked out on the hardware and from the SoC's behaviour; the names are
 * ours. */
#ifndef GPIO_YP3_H
#define GPIO_YP3_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "cpu.h"

/* ---- Pad registers: four banks of 32 pads ------------------------------ */
#define GPIO_BANK(bank)        (GPIO_BASE + 0x40u * (bank))  /* sl6801.h */
/* 4 bits/pad */
#define GPIO_MUX(bank, pin)    (GPIO_BANK(bank) + 0x00u + ((pin) >> 3) * 4u)
/* pad levels, 1 bit/pad */
#define GPIO_INPUT(bank)       (GPIO_BANK(bank) + 0x10u)
/* ROM "dir" field, 2 bits/pad; exact semantics (direction vs drive)
 * unclear. */
#define GPIO_DIR(bank, pin)    (GPIO_BANK(bank) + 0x14u + ((pin) >> 4) * 4u)
/* ROM "mode" fields, 2 bits/pad; called pull here from the key probe's
 * behaviour (they may also set drive). */
#define GPIO_PULL_A(bank, pin) (GPIO_BANK(bank) + 0x24u + ((pin) >> 4) * 4u)
#define GPIO_PULL_B(bank, pin) (GPIO_BANK(bank) + 0x2cu + ((pin) >> 4) * 4u)
/* write 1s: output latch high / low */
#define GPIO_SET(bank)         (GPIO_BANK(bank) + 0x34u)
#define GPIO_CLR(bank)         (GPIO_BANK(bank) + 0x38u)
#define GPIO_MUX_SHIFT(pin)    (((pin) & 7u) * 4u)
#define GPIO_FIELD_SHIFT(pin)  (((pin) & 15u) * 2u)
/* Pad interrupt registers, 0x200 above the bank's pad registers, one bit
 * per pad in the mask and pending words (bank_base + 0x210 and + 0x214).
 * The word at + 0x200 holds a 4-bit field
 * per pad; pad 2 (the only one used) is bits 11:8, so pads 0-7 presumably
 * follow at 4n (unconfirmed; higher pads unknown); GPIO2.2's field is
 * restored before each rearm (probably the trigger type, unconfirmed). */
#define GPIO_IRQ_PAD(bank)     (GPIO_BANK(bank) + 0x200u)
/* 1 = interrupt enabled */
#define GPIO_IRQ_MASK(bank)    (GPIO_BANK(bank) + 0x210u)
/* write 1 to clear */
#define GPIO_IRQ_PENDING(bank) (GPIO_BANK(bank) + 0x214u)

/* ---- Pad descriptor ----------------------------------------------------
 * Packing: bank [19:16], pin [15:11], mux [10:7], output level [6],
 * direction code [5:4], pull code [3:0]. */
#define YP3_PIN(bank, pin, mux, level, dir, pull) \
    (((uint32_t)(bank) << 16) | ((uint32_t)(pin) << 11) | \
     ((uint32_t)(mux) << 7) | ((uint32_t)(level) << 6) | \
     ((uint32_t)(dir) << 4) | (uint32_t)(pull))
#define YP3_PIN_BANK(desc)     (((desc) >> 16) & 0xfu)
#define YP3_PIN_NUM(desc)      (((desc) >> 11) & 0x1fu)
#define YP3_PIN_MUX(desc)      (((desc) >> 7) & 0xfu)
#define YP3_PIN_LEVEL(desc)    (((desc) >> 6) & 1u)
#define YP3_PIN_DIR(desc)      (((desc) >> 4) & 3u)
#define YP3_PIN_PULL(desc)     ((desc) & 0xfu)
/* The pad's level: its bank's GPIO_INPUT word and its bit there */
#define YP3_PIN_INPUT(desc)    GPIO_INPUT(YP3_PIN_BANK(desc))
#define YP3_PIN_BIT(desc)      (1u << YP3_PIN_NUM(desc))

/* Mux values as the board uses them (alternate functions differ per pad). */
#define MUX_GPIO     0x0u   /* plain pad: keys, amplifier enable */
#define MUX_1        0x1u   /* LCD reset pad
                               (presumably latch-driven output; unverified) */
#define MUX_I2C      0x3u   /* tuner I2C SCL/SDA */
#define MUX_ALT5     0x5u   /* backlight PWM3, tuner 12 MHz reference out */
#define MUX_SD       0xbu
#define MUX_LCD      0xcu   /* LCD bus */
#define MUX_ANALOG   0xfu   /* analog: tuner audio, key ladder, USB strap
                               pads */

/* Pull codes: pairs of 2-bit fields at GPIO_PULL_A/B. Up and
 * down are as the Left/Right key probe reads them; code 8 is used for
 * SD pad 3.4 and the tuner I2C (meaning unknown). */
#define PULL_NONE    0x0u   /* A=0 B=0 */
#define PULL_DOWN    0x5u   /* A=1 B=3 */
#define PULL_8       0x8u   /* A=2 B=0 */
#define PULL_UP      0xbu   /* A=2 B=2 */

/* GPIO_DIR codes as used here: 0 for the keys, LCD, tuner I2C and
 * audio pads and the amplifier enable, 1 for SD and the USB pads, 3 for
 * the tuner reference clock output. Meaning (direction vs drive) unknown. */
#define DIR_0        0x0u
#define DIR_1        0x1u
#define DIR_3        0x3u

/* ---- YP3 board pins ---------------------------------------------------- */
/* LCD bus pads (signal roles unknown; the controller's pin select words
 * map signals to pads). */
#define PIN_LCD_1_0         YP3_PIN(1, 0, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_1_1         YP3_PIN(1, 1, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_1_2         YP3_PIN(1, 2, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_1_3         YP3_PIN(1, 3, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_1_4         YP3_PIN(1, 4, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_1_5         YP3_PIN(1, 5, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_BUS_1_18    YP3_PIN(1, 18, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_BUS_3_1     YP3_PIN(3, 1, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_BUS_3_2     YP3_PIN(3, 2, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_BUS_3_3     YP3_PIN(3, 3, MUX_LCD, 0, DIR_0, PULL_NONE)
#define PIN_LCD_BUS_3_10    YP3_PIN(3, 10, MUX_LCD, 0, DIR_0, PULL_NONE)
/* Panel reset (pulsed high/low/high at bring-up). */
#define PIN_LCD_RESET       YP3_PIN(1, 10, MUX_1, 1, DIR_0, PULL_UP)

/* SD bus on GPIO3.4-3.9; which pad is CLK/CMD/DAT0-3 is not known (applied in
 * the order 4, 6, 7, 8, 9, 5). */
#define PIN_SD_3_4          YP3_PIN(3, 4, MUX_SD, 0, DIR_1, PULL_8)
#define PIN_SD_3_5          YP3_PIN(3, 5, MUX_SD, 0, DIR_1, PULL_NONE)
#define PIN_SD_3_6          YP3_PIN(3, 6, MUX_SD, 0, DIR_1, PULL_UP)
#define PIN_SD_3_7          YP3_PIN(3, 7, MUX_SD, 0, DIR_1, PULL_UP)
#define PIN_SD_3_8          YP3_PIN(3, 8, MUX_SD, 0, DIR_1, PULL_UP)
#define PIN_SD_3_9          YP3_PIN(3, 9, MUX_SD, 0, DIR_1, PULL_UP)

#define PIN_TUNER_SDA       YP3_PIN(1, 11, MUX_I2C, 0, DIR_0, PULL_8)
#define PIN_TUNER_SCL       YP3_PIN(1, 13, MUX_I2C, 0, DIR_0, PULL_8)
/* 12 MHz reference out (fmradio-i2c-yp3.c) */
#define PIN_TUNER_REFCLK    YP3_PIN(3, 11, MUX_ALT5, 0, DIR_3, PULL_NONE)
/* Tuner audio reaches the codec ADC through GPIO1.20/1.21 (which is left
 * is not known). Owned by the capture code, as are 1.7 and 1.9 (roles
 * unknown; the owner table reserves them, nothing configures them). */
#define PIN_TUNER_AUDIO_1_20 YP3_PIN(1, 20, MUX_ANALOG, 0, DIR_0, PULL_NONE)
#define PIN_TUNER_AUDIO_1_21 YP3_PIN(1, 21, MUX_ANALOG, 0, DIR_0, PULL_NONE)
/* Mic path 2 pads, set analog in some
 * configurations; the built-in mic was verified with them analog. */
#define PIN_MIC_1_14        YP3_PIN(1, 14, MUX_ANALOG, 0, DIR_0, PULL_NONE)
#define PIN_MIC_1_15        YP3_PIN(1, 15, MUX_ANALOG, 0, DIR_0, PULL_NONE)

#define PIN_BACKLIGHT_PWM   YP3_PIN(1, 17, MUX_ALT5, 0, DIR_0, PULL_NONE)
/* Speaker amplifier enable (observed: pull code 0xb = speaker on, no pull
 * = off; mux 0, latch low). */
#define PIN_AMP_ON          YP3_PIN(1, 30, MUX_GPIO, 0, DIR_0, PULL_UP)
#define PIN_AMP_OFF         YP3_PIN(1, 30, MUX_GPIO, 0, DIR_0, PULL_NONE)

/* Up/Down resistor ladder into the key ADC (it also shows an SD-detect
 * level); it must also be analog during LCD bring-up (without it the
 * LCD shows coloured noise), so the LCD driver configures it too. */
#define PIN_KEY_LADDER      YP3_PIN(3, 0, MUX_ANALOG, 0, DIR_0, PULL_NONE)
/* Left/Right: one pad, probed with a pull-up (Right reads low) and a
 * pull-down (Left reads high). */
#define PIN_KEY_LR_PULLUP   YP3_PIN(3, 12, MUX_GPIO, 0, DIR_0, PULL_UP)
#define PIN_KEY_LR_PULLDOWN YP3_PIN(3, 12, MUX_GPIO, 0, DIR_0, PULL_DOWN)
#define PIN_KEY_LR_INPUT    YP3_PIN_INPUT(PIN_KEY_LR_PULLUP)
#define PIN_KEY_LR_BIT      YP3_PIN_BIT(PIN_KEY_LR_PULLUP)
/* Centre key: PMU event line on GPIO2.2 (mux 0xe, the pad's IRQ input). */
#define GPIO_CENTER_KEY_BANK 2u
#define GPIO_CENTER_KEY_PIN  2u
#define MUX_CENTER_KEY_IRQ   0xeu
/* Pad setup: mux 0xe, latch low, dir 0, pull-down. */
#define PIN_CENTER_KEY \
    YP3_PIN(GPIO_CENTER_KEY_BANK, GPIO_CENTER_KEY_PIN, MUX_CENTER_KEY_IRQ, \
            0, DIR_0, PULL_DOWN)

/* Pads set to mux 0xf before USB bring-up; the ROM uses the same
 * pads as USB strap signals. */
#define PIN_USB_1_22        YP3_PIN(1, 22, MUX_ANALOG, 0, DIR_1, PULL_NONE)
#define PIN_USB_1_23        YP3_PIN(1, 23, MUX_ANALOG, 0, DIR_1, PULL_NONE)

/* ---- Owners and access ---------------------------------------------- */
enum yp3_pin_owner {
    PIN_OWNER_LCD, PIN_OWNER_SD, PIN_OWNER_TUNER, PIN_OWNER_AUDIO,
    PIN_OWNER_BACKLIGHT, PIN_OWNER_KEYS, PIN_OWNER_USB,
    PIN_OWNER_DEV,          /* dev shell experiments: any pad, logged */
};

/* Apply a pad descriptor: mux, output latch, direction and pull fields.
 * Refused (false) for a pad another owner holds. */
bool yp3_pin_config(enum yp3_pin_owner owner, uint32_t desc);
/* Drive the output latch only. Refused for another owner's pad. */
bool yp3_pin_write(enum yp3_pin_owner owner, uint32_t desc, bool high);

#endif
