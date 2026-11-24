/* SPDX-License-Identifier: GPL-2.0-or-later */
/* YP3 LCD: 128x160 GC9106 panel on the SL6801 LCD controller. The
 * framebuffer is byte-swapped RGB565 (the controller shifts out the low byte
 * first, the panel wants the high byte first), so updates DMA it unchanged
 * into the controller FIFO. Cold init sends the panel's init command
 * stream; commands use PIO. */
#include "config.h"
#include "lcd.h"
#include "events.h"
#include "system.h"
#include "kernel.h"
#include "irq-yp3.h"
#include "backlight.h"
#include "lcd-yp3.h"
#include "dev-yp3.h"
#include "gpio-yp3.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* ---- LCD controller (0x40095000) ----------------------------------------
 * Register roles from stock firmware's controller init and transfer
 * routines. */

#define LCDC            0x40095000u
/* Used when board config byte 2 is 0: four 4-bit fields from board config
 * bytes 3..6 ([15:12], [11:8], [7:4], [3:0]); the YP3 config has them all
 * zero. Field roles unknown (guessed: bus timing). */
#define LCD_MODE0_CONFIG (LCDC + 0x00u)
/* Bit 0 = board config byte 2, which picks which of two config words is
 * filled in (0: LCD_MODE0_CONFIG, 1: +0x04 and fields of +0x20). The YP3
 * uses 0; what the two modes are is unknown. */
#define LCD_IF_MODE     (LCDC + 0x08u)
#define LCD_IF_MODE_SELECT (1u << 0)
/* DMA routing. For a DMA write, bits 21:20 are cleared and bit 19 set; a
 * DMA read uses bits 5:4 clear and bit 3 set. These look like per-direction
 * DMA request controls; the individual bits are not
 * decoded. */
#define LCD_DMA_CFG     (LCDC + 0x10u)
#define LCD_DMA_CFG_TX_MASK  (3u << 20)
#define LCD_DMA_CFG_TX_SET   (1u << 19)
/* Status. Bits 30:29 must both be clear before a transfer starts (which bit
 * means what is unknown); wait for bit 31 after it; bit 31 is
 * write-1-to-clear. */
#define LCD_STATUS      (LCDC + 0x14u)
#define LCD_STATUS_BUSY 0x60000000u     /* bits 30:29 */
#define LCD_STATUS_DONE 0x80000000u     /* bit 31, W1C */
/* Transfer control. Values per transfer shape: no command and no data
 * 0xe, data only 0xa, command only 0x6, command + data 0x2 (all writes);
 * reads leave bit 1 clear; bits 9:8 = command bytes - 1. Bit 0 starts the
 * transfer. A PIO data transfer clears only bits 3:1. */
#define LCD_CONTROL     (LCDC + 0x20u)
#define LCD_CTRL_START      (1u << 0)
#define LCD_CTRL_WRITE      (1u << 1)   /* clear: read from the panel */
#define LCD_CTRL_NO_DATA    (1u << 2)   /* transfer has no data phase */
#define LCD_CTRL_NO_COMMAND (1u << 3)   /* transfer has no command phase */
#define LCD_CTRL_KIND_MASK  (7u << 1)   /* the three bits above */
#define LCD_CTRL_CMD_BYTES_MASK (3u << 8) /* command bytes - 1; 0 = one byte */
#define LCD_COMMAND     (LCDC + 0x24u)  /* command byte(s) */
#define LCD_COUNT       (LCDC + 0x28u)  /* data bytes - 1 */
/* Transmit FIFO (up to 16 bytes can be preloaded per PIO transfer); a
 * word goes out low byte first. Also the DMA destination. */
#define LCD_DATA        (LCDC + 0x30u)
/* Pin select words: written with these constants after the pad setup;
 * per-field meaning unknown. */
#define LCD_PIN_SEL_LO  (LCDC + 0x40u)
#define LCD_PIN_SEL_HI  (LCDC + 0x44u)
#define LCD_PIN_SEL_LO_VALUE 0xabf1ffffu
#define LCD_PIN_SEL_HI_VALUE 0x58764029u
/* ORed into the GPIO pin-enable mask during LCD bring-up; which bit
 * enables which pad is unknown. */
#define LCD_PIN_ENABLE_BITS 0xffd0u

/* Controller words for the three transfer shapes used here. */
#define LCD_CTRL_PIO_COMMAND \
    (LCD_CTRL_START | LCD_CTRL_WRITE | LCD_CTRL_NO_DATA)
#define LCD_CTRL_DATA \
    (LCD_CTRL_START | LCD_CTRL_WRITE | LCD_CTRL_NO_COMMAND)

/* ---- GC9106 panel commands ----------------------------------------------
 * Standard MIPI DCS commands. */
#define DCS_SLEEP_IN    0x10u
#define DCS_SLEEP_OUT   0x11u   /* panel needs 120 ms before the next sleep */
#define DCS_INVERT_OFF  0x20u
#define DCS_INVERT_ON   0x21u
#define DCS_DISPLAY_OFF 0x28u
#define DCS_DISPLAY_ON  0x29u
#define DCS_COLUMN_SET  0x2au   /* x start, x end: 16 bits each, high byte
                                   first */
#define DCS_ROW_SET     0x2bu   /* y start, y end */
#define DCS_MEMORY_WRITE 0x2cu  /* pixel data follows */
#define DCS_TEAR_ON     0x35u   /* tearing-effect output on; data = mode */
#define DCS_MADCTL      0x36u   /* memory access control (scan order,
                                   RGB/BGR) */
#define DCS_PIXEL_FORMAT 0x3au  /* data 0x05 = 16 bits/pixel */
#define DCS_TEAR_SCANLINE 0x44u
/* MADCTL 0xc0: row and column order reversed (DCS bits MY|MX), i.e. the
 * image turned 180 degrees. */
#define MADCTL_FINAL    0xc0u
/* GalaxyCore commands. GC9106 datasheet: FEh/EFh = "Inter Register
 * Enable 1/2". `fe fe ef` is sent at the start of the init (standard
 * DCS commands such as 21/36/3a and 10/11 still follow, so it is not a
 * separate command page). 0xff is undocumented: `fe ff` is sent before
 * 35 44 11 and after the sleep commands; role unknown. */
#define GC_INTER_REG_EN1 0xfeu
#define GC_INTER_REG_EN2 0xefu
#define GC_CMD_FF        0xffu

/* ---- general DMA channel for the pixel push -----------------------------
 * Channels 0/1 serve the LCD. Memory to the LCD FIFO (the LCD request on
 * the destination side), 32-bit source reads into 16-bit FIFO writes: width
 * codes 1 in bits 9:8 and 2 in bits 21:20. */
#define DMA_CHANNEL     0u
#define DMA_IRQ         DMA_CH_IRQ(DMA_CHANNEL)
#define DMA_CONTROL     DMA_CH_CONTROL(DMA_CHANNEL)
#define DMA_SOURCE      DMA_CH_SOURCE(DMA_CHANNEL)
#define DMA_DEST        DMA_CH_DEST(DMA_CHANNEL)
#define DMA_COUNT       DMA_CH_COUNT(DMA_CHANNEL)
#define DMA_INTMASK     DMA_CH_INTMASK(DMA_CHANNEL)
#define DMA_STATUS      DMA_CH_STATUS(DMA_CHANNEL)
#define DMA_CONFIG      (DMA_CFG_SRC_MEM | DMA_CFG_BIT7 | DMA_CFG_WIDTH_9_8(1) | \
                         DMA_CFG_DST_REQ(DMA_REQ_LCD) | DMA_CFG_BIT19 | \
                         DMA_CFG_WIDTH_21_20(DMA_WIDTH_32))
/* Count top byte for the LCD (from stock firmware); meaning unknown. */
#define LCD_DMA_COUNT_TOP 0x31u
#define DMA_CHUNK       4096u   /* largest LCD transfer */

/* ---- timeouts and delays (bounded polls) ---- */
#define DMA_POLLS       1000000u /* LCD DONE polls after a DMA chunk */
#define COLD_POLLS      1000000u /* status polls and delay-counter samples */
/* The free-running counter (SL6801_COUNTER_*) runs at 1 MHz; a delay unit is
 * 1000 counts (1 ms). */
#define COUNTER_TICKS_PER_MS 1000u
#define PUSH_POLLS      10000u   /* status polls in the pixel push */

/* The 128x160 image sits at column 2, row 1 of the panel's memory, so
 * window coordinates are shifted by these. */
#define COLUMN_OFFSET   2u
#define ROW_OFFSET      1u

/* Panel timing (Rockbox ticks). */
#define PANEL_SLEEP_SETTLE (HZ * 12 / 100) /* 120 ms around sleep in/out */
#define PANEL_ON_SETTLE    (HZ / 20)       /* ~3 frames after display-on */

enum { LCD_UNINIT = 0, LCD_READY, LCD_FAILED };

extern struct frame_buffer_t lcd_framebuffer_default;

static uint8_t lcd_state;
/* Once the panel is up, updates from threads sleep on the DMA completion
 * interrupt instead of polling; the mutex keeps threads (UI, scroll) from
 * interleaving transfers while one sleeps. Polling remains for IRQs off. */
static bool dma_irq_mode;
static struct semaphore dma_sem;
static struct mutex lcd_mtx;

bool yp3_lcd_ready(void)
{
    return lcd_state == LCD_READY;
}

/* ---- pixel push (SRAM resident) ---------------------------------------- */

#define LCD_RAMCODE __attribute__((section(".icode"), noinline))

static LCD_RAMCODE int push_wait(uint32_t mask, uint32_t wanted)
{
    for (uint32_t polls = 0; polls < PUSH_POLLS; ++polls)
        if ((REG(LCD_STATUS) & mask) == wanted)
            return 0;
    return -1;
}

static LCD_RAMCODE int push_command(uint32_t value)
{
    /* Command byte, wait idle, start (one command byte, no data), wait
     * done, ack. */
    REG(LCD_COMMAND) = value;
    if (push_wait(LCD_STATUS_BUSY, 0))
        return -1;
    REG(LCD_CONTROL) = (REG(LCD_CONTROL) &
                        ~(LCD_CTRL_CMD_BYTES_MASK | LCD_CTRL_KIND_MASK))
                       | LCD_CTRL_PIO_COMMAND;
    if (push_wait(LCD_STATUS_DONE, LCD_STATUS_DONE))
        return -1;
    REG(LCD_STATUS) = LCD_STATUS_DONE;
    return 0;
}

static LCD_RAMCODE int push_data(uint32_t value, uint32_t bytes)
{
    /* Stage data and count (count-1) before waiting idle, then start a
     * data-only write. */
    REG(LCD_DATA) = value;
    REG(LCD_COUNT) = bytes - 1u;
    if (push_wait(LCD_STATUS_BUSY, 0))
        return -1;
    REG(LCD_CONTROL) = (REG(LCD_CONTROL) & ~LCD_CTRL_KIND_MASK) | LCD_CTRL_DATA;
    if (push_wait(LCD_STATUS_DONE, LCD_STATUS_DONE))
        return -1;
    REG(LCD_STATUS) = LCD_STATUS_DONE;
    return 0;
}

static LCD_RAMCODE int push_window(uint32_t x, uint32_t y,
                                   uint32_t width, uint32_t height)
{
    /* Coordinates are 16-bit, high byte first; all fit in the low byte, so
     * each high byte is 0. */
    if (push_command(DCS_COLUMN_SET) ||
        push_data(0, 1) || push_data(x + COLUMN_OFFSET, 1) ||
        push_data(0, 1) || push_data(x + width - 1u + COLUMN_OFFSET, 1) ||
        push_command(DCS_ROW_SET) ||
        push_data(0, 1) || push_data(y + ROW_OFFSET, 1) ||
        push_data(0, 1) || push_data(y + height - 1u + ROW_OFFSET, 1) ||
        push_command(DCS_MEMORY_WRITE))
        return -1;
    return 0;
}

/* Send bytes (multiple of 4, at most DMA_CHUNK) from src through DMA as
 * one controller data transfer (no command bytes). */
static LCD_RAMCODE int push_dma(const void *src, uint32_t bytes)
{
    /* Count, wait idle, route the FIFO to DMA, start the DMA channel
     * (status cleared first), then start a data-only write. */
    REG(LCD_COMMAND) = 0;
    REG(LCD_COUNT) = bytes - 1u;
    if (push_wait(LCD_STATUS_BUSY, 0))
        return -1;
    REG(LCD_DMA_CFG) =
        (REG(LCD_DMA_CFG) & ~LCD_DMA_CFG_TX_MASK) | LCD_DMA_CFG_TX_SET;
    REG(DMA_STATUS) = REG(DMA_STATUS);
    REG(DMA_SOURCE) = (uint32_t)(uintptr_t)src;
    REG(DMA_DEST) = LCD_DATA;
    REG(DMA_COUNT) = (REG(DMA_COUNT) & ~DMA_COUNT_BYTES_MASK) | bytes;
    REG(DMA_CONTROL) =
        (REG(DMA_CONTROL) & ~DMA_CONTROL_BIT29) | DMA_CONTROL_ENABLE;
    REG(LCD_CONTROL) = (REG(LCD_CONTROL) &
                        ~(LCD_CTRL_CMD_BYTES_MASK | LCD_CTRL_KIND_MASK))
                       | LCD_CTRL_DATA;
    /* The semaphore only lets the thread sleep; LCD DONE below is what
     * proves the transfer finished (a stale release just shortens a sleep). */
    if (dma_irq_mode && irq_enabled() && get_interrupt_number() == 0)
        semaphore_wait(&dma_sem, HZ / 10);
    uint32_t polls = 0;
    while (!(REG(LCD_STATUS) & LCD_STATUS_DONE))
        if (++polls > DMA_POLLS)
            return -1;
    REG(LCD_STATUS) = LCD_STATUS_DONE;
    REG(DMA_STATUS) = REG(DMA_STATUS);
    return 0;
}

static void dma_irq(void)
{
    uint32_t status = REG(DMA_STATUS);
    REG(DMA_STATUS) = status;
    if (status & DMA_STATUS_COMPLETE)
        semaphore_release(&dma_sem);
}

/* Channel setup: disable, clear status, load the config, the
 * count top byte and the interrupt mask. Each transfer then sets the
 * addresses and length (push_dma). */
static int dma_setup(void)
{
    if (!sl6801_clock_gate_enable(CLK_GATE_ID_DMA))
        return -1;
    REG(DMA_CONTROL) = REG(DMA_CONTROL) & ~DMA_CONTROL_ENABLE;
    REG(DMA_STATUS) = REG(DMA_STATUS);
    REG(DMA_CONTROL) = (REG(DMA_CONTROL) & ~DMA_CONFIG_MASK) | DMA_CONFIG;
    REG(DMA_COUNT) = (REG(DMA_COUNT) & ~DMA_COUNT_TOP(0xffu)) |
                     DMA_COUNT_TOP(LCD_DMA_COUNT_TOP);
    REG(DMA_INTMASK) = (REG(DMA_INTMASK) & DMA_INTMASK_KEEP) | DMA_INTMASK_SET;
    return 0;
}

/* Push rows [top, bottom) at full width: the rows are contiguous in the
 * framebuffer, so the whole band goes out in DMA_CHUNK transfers. */
static LCD_RAMCODE int push_rows(const fb_data *fb, int32_t top,
                                 int32_t bottom)
{
    if (top < 0) top = 0;
    if (bottom > LCD_HEIGHT) bottom = LCD_HEIGHT;
    if (top >= bottom)
        return 0;
    if (push_window(0, (uint32_t)top, LCD_WIDTH, (uint32_t)(bottom - top)))
        return -1;
    const uint8_t *p = (const uint8_t *)(fb + LCD_WIDTH * top);
    uint32_t left = (uint32_t)(bottom - top) * LCD_WIDTH * sizeof(fb_data);
    while (left) {
        uint32_t n = left < DMA_CHUNK ? left : DMA_CHUNK;
        if (push_dma(p, n))
            return -1;
        p += n;
        left -= n;
    }
    return 0;
}

/* ---- cold init --------------------------------------------------------- */

/* Board pad setup for the LCD. */
static const uint32_t lcd_pins[] = {
    PIN_LCD_BUS_1_18, PIN_LCD_BUS_3_1, PIN_LCD_BUS_3_2, PIN_LCD_BUS_3_3,
    PIN_LCD_1_5, PIN_LCD_1_4, PIN_LCD_1_3, PIN_LCD_1_2, PIN_LCD_1_1,
    PIN_LCD_1_0,
    PIN_LCD_BUS_3_10, PIN_LCD_RESET,
};

struct command_data {
    uint8_t command;
    uint8_t length;
    uint8_t bytes[14];
};

/* GC9106 init table, sent after `fe fe ef` (panel_init); values from stock
 * firmware. lcd_command() inserts INVON before MADCTL and reorders the tail
 * (see there). The a3..b6 and f0/f1 entries
 * are GalaxyCore vendor registers; their roles are not documented here
 * (f0/f1 have the shape of gamma curves). */
static const struct command_data init_data[] = {
    {0xaf, 1, {0x80}},
    {0xb3, 1, {0x03}},
    {0xb6, 1, {0x11}},
    {0xac, 1, {0x1a}},
    {0xa3, 1, {0x11}},
    {DCS_MADCTL, 1, {0xd0}},        /* MY|MX|ML; replaced by MADCTL_FINAL */
    {DCS_PIXEL_FORMAT, 1, {0x05}},  /* 16 bits/pixel (RGB565) */
    {0xb4, 1, {0x21}},
    {0xf0, 14, {0x31,0x4c,0x24,0x58,0xa8,0x26,0x28,
                 0x00,0x2c,0x0c,0x0c,0x15,0x15,0x0f}},
    {0xf1, 14, {0x0e,0x2d,0x24,0x3e,0x99,0x12,0x13,
                 0x00,0x0a,0x0d,0x0d,0x14,0x13,0x0f}},
    {DCS_TEAR_ON, 1, {0x00}},       /* mode 0 */
    {DCS_TEAR_SCANLINE, 1, {0x00}},
};

/* Progress of the tail reordering in lcd_command()/lcd_data(). */
enum {
    TAIL_PENDING = 0,   /* TEAR_ON not seen yet */
    TAIL_TEAR_ON,       /* TEAR_ON held back; its data byte is captured */
    TAIL_SCANLINE,      /* TEAR_SCANLINE held back; data byte captured */
    TAIL_SENT,          /* reordered tail sent; drop the table's ff/11 */
    TAIL_DONE,          /* table's own sleep-out dropped */
};

struct cold_ctx {
    uint32_t inversion_command_sent;    /* INVON already inserted */
    uint32_t captured_tail_stage;       /* TAIL_* */
    uint8_t captured_35_data;           /* TEAR_ON data byte */
    uint8_t captured_44_data;           /* TEAR_SCANLINE data byte */
};

static uint32_t rmw(uintptr_t a, uint32_t clear, uint32_t set)
{
    uint32_t value = (REG(a) & ~clear) | set;
    REG(a) = value;
    return value;
}

/* Coherent 64-bit read of the 1 MHz counter: high, low, high again; a
 * changed high word means the low word wrapped in between, so retry once. */
static int counter_read(uint64_t *value)
{
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        uint32_t hi0 = REG(SL6801_COUNTER_HI);
        uint32_t lo = REG(SL6801_COUNTER_LO);
        uint32_t hi1 = REG(SL6801_COUNTER_HI);
        if (hi0 == hi1) {
            *value = ((uint64_t)hi1 << 32) | lo;
            return 1;
        }
    }
    return 0;
}

/* Wait `units` ms. Busy-waits on the 1 MHz counter;
 * returns nonzero if the counter never advances far enough within
 * COLD_POLLS samples. Once the panel is up and a thread is calling, it
 * sleeps instead. */
static int delay_units(uint32_t units)
{
    uint64_t start, now;
    uint64_t ticks = (uint64_t)COUNTER_TICKS_PER_MS * units;

    if (lcd_state == LCD_READY && irq_enabled() &&
        get_interrupt_number() == 0) {
        /* panel restart on wake: let audio run */
        sleep(units * HZ / 1000 + 1);
        return 0;
    }

    if (!counter_read(&start))
        return 1;
    for (uint32_t polls = 0; polls < COLD_POLLS; ++polls)
        if (counter_read(&now) && now - start >= ticks)
            return 0;
    return 1;
}

static int wait_lcd(uint32_t mask, uint32_t wanted)
{
    for (uint32_t i = 0; i < COLD_POLLS; ++i)
        if ((REG(LCD_STATUS) & mask) == wanted)
            return 0;
    return 1;
}

static int lcd_command_raw(uint8_t value)
{
    /* FIFO, idle, START, DONE, final ACK (as push_command). */
    REG(LCD_COMMAND) = value;
    if (wait_lcd(LCD_STATUS_BUSY, 0))
        return 1;
    rmw(LCD_CONTROL, LCD_CTRL_CMD_BYTES_MASK | LCD_CTRL_KIND_MASK,
        LCD_CTRL_PIO_COMMAND);
    if (wait_lcd(LCD_STATUS_DONE, LCD_STATUS_DONE))
        return 1;
    REG(LCD_STATUS) = LCD_STATUS_DONE;
    return 0;
}

static int lcd_data_raw(uint8_t value)
{
    /* Stage both fields before waiting idle; no leading ACK. One byte:
     * count register 0. */
    REG(LCD_DATA) = value;
    REG(LCD_COUNT) = 0;
    if (wait_lcd(LCD_STATUS_BUSY, 0))
        return 1;
    rmw(LCD_CONTROL, LCD_CTRL_KIND_MASK, LCD_CTRL_DATA);
    if (wait_lcd(LCD_STATUS_DONE, LCD_STATUS_DONE))
        return 1;
    REG(LCD_STATUS) = LCD_STATUS_DONE;
    return 0;
}

static int lcd_command(struct cold_ctx *c, uint8_t value)
{
    /* The init table puts 35/44 (TEAR_ON/TEAR_SCANLINE) before panel_init's
     * fe ff 11; the byte stream sent is fe, ff, 35, 44, 11. Hold back the
     * table's 35/44 and their data, send that stream when the table's fe
     * arrives, then drop the table's own ff and 11. */
    if (c->captured_tail_stage == TAIL_TEAR_ON && value == DCS_TEAR_SCANLINE) {
        c->captured_tail_stage = TAIL_SCANLINE;
        return 0;
    }
    if (c->captured_tail_stage == TAIL_SCANLINE && value == GC_INTER_REG_EN1) {
        if (lcd_command_raw(GC_INTER_REG_EN1) || lcd_command_raw(GC_CMD_FF) ||
            lcd_command_raw(DCS_TEAR_ON) ||
            lcd_data_raw(c->captured_35_data) ||
            lcd_command_raw(DCS_TEAR_SCANLINE) ||
            lcd_data_raw(c->captured_44_data) ||
            lcd_command_raw(DCS_SLEEP_OUT)) return 1;
        c->captured_tail_stage = TAIL_SENT;
        return 0;
    }
    if (c->captured_tail_stage == TAIL_SENT &&
        (value == GC_CMD_FF || value == DCS_SLEEP_OUT)) {
        if (value == DCS_SLEEP_OUT) c->captured_tail_stage = TAIL_DONE;
        return 0;
    }
    if (value == DCS_TEAR_ON && c->inversion_command_sent) {
        c->captured_tail_stage = TAIL_TEAR_ON;
        return 0;
    }
    /* A standalone 0x21 goes out between A3 and 36 (this panel needs
     * inversion on to show RGB565 values as themselves). */
    if (value == DCS_MADCTL && !c->inversion_command_sent) {
        if (lcd_command_raw(DCS_INVERT_ON)) return 1;
        c->inversion_command_sent = 1;
    }
    return lcd_command_raw(value);
}

static int lcd_data(struct cold_ctx *c, uint8_t value)
{
    if (c->captured_tail_stage == TAIL_TEAR_ON) {
        c->captured_35_data = value;
        return 0;
    }
    if (c->captured_tail_stage == TAIL_SCANLINE) {
        c->captured_44_data = value;
        return 0;
    }
    return lcd_data_raw(value);
}

static int panel_init(struct cold_ctx *c)
{
    /* fe fe ef (inter register enable) goes out before the table. */
    if (lcd_command(c, GC_INTER_REG_EN1) || lcd_command(c, GC_INTER_REG_EN1) ||
        lcd_command(c, GC_INTER_REG_EN2))
        return 1;
    for (unsigned i = 0; i < sizeof(init_data) / sizeof(init_data[0]); ++i) {
        if (lcd_command(c, init_data[i].command)) return 1;
        for (unsigned j = 0; j < init_data[i].length; ++j)
            if (lcd_data(c, init_data[i].bytes[j])) return 1;
    }
    if (lcd_command(c, GC_INTER_REG_EN1) || lcd_command(c, GC_CMD_FF) ||
        lcd_command(c, DCS_SLEEP_OUT))
        return 1;
    /* The panel needs 120 ms after sleep-out. */
    if (delay_units(120)) return 1;
    if (lcd_command(c, DCS_DISPLAY_ON) || lcd_command(c, DCS_MEMORY_WRITE))
        return 1;
    return 0;
}

/* Reset pulse and the full init table: from any panel state. */
static int panel_restart(struct cold_ctx *c)
{
    /* Panel reset pulse (low = reset): high 10, low 10, high 50 ms, then
     * the init. */
    yp3_pin_write(PIN_OWNER_LCD, PIN_LCD_RESET, true);
    if (delay_units(10)) return 1;
    yp3_pin_write(PIN_OWNER_LCD, PIN_LCD_RESET, false);
    if (delay_units(10)) return 1;
    yp3_pin_write(PIN_OWNER_LCD, PIN_LCD_RESET, true);
    if (delay_units(50)) return 1;

    if (panel_init(c)) return 1;
    /* Final standalone 36=c0 orientation. */
    if (lcd_command_raw(DCS_MADCTL) || lcd_data_raw(MADCTL_FINAL)) return 1;
    return 0;
}

/* Controller config with the YP3 board values: mode 0, all mode-0 fields
 * zero; then the pin enables and pin selects. */
static void controller_config(void)
{
    rmw(LCD_IF_MODE, LCD_IF_MODE_SELECT, 0);
    REG(LCD_MODE0_CONFIG) = 0;
    rmw(GPIO_PIN_ENABLE, 0, LCD_PIN_ENABLE_BITS);
    REG(LCD_PIN_SEL_LO) = LCD_PIN_SEL_LO_VALUE;
    REG(LCD_PIN_SEL_HI) = LCD_PIN_SEL_HI_VALUE;
}

static int cold_start(void)
{
    struct cold_ctx c = {0};

    /* Cold boot leaves clock source 8 disabled (CLK_ROOT == 0x51); it is
     * enabled below after the 0x5c gate is acknowledged. Refuse unless
     * CLK_ROOT is in its cold state (or a transfer is in flight). */
    if (REG(CLK_ROOT) != CLK_ROOT_COLD)
        return 1;
    if (REG(LCD_STATUS) & LCD_STATUS_BUSY) return 1;

    /* The key ladder pad must be analog before the LCD bus works (else
     * coloured noise; only its mux is set here, the full setup is what the
     * key ADC applies later anyway). */
    yp3_pin_config(PIN_OWNER_KEYS, PIN_KEY_LADDER);
    for (unsigned i = 0; i < sizeof lcd_pins / sizeof lcd_pins[0]; ++i)
        yp3_pin_config(PIN_OWNER_LCD, lcd_pins[i]);

    if (!sl6801_clock_gate_enable(CLK_GATE_ID_LCD)) return 1;
    /* Enable source 8 (96 MHz), select it for selector 0x3f, divide by 6
     * (16 MHz LCD clock), then enable the selector. */
    rmw(CLK_ROOT, 0, CLK_ROOT_SOURCE8_ENABLE);
    rmw(CLK_SEL_3F, 0, CLK_SEL_3F_SOURCE8);
    rmw(CLK_SEL_3F, CLK_SEL_3F_DIV_MASK, CLK_SEL_3F_DIV(6u));
    rmw(CLK_SEL_3F, 0, CLK_SEL_ENABLE);

    controller_config();
    return panel_restart(&c);
}

/* Clear to black after reset and init, then wait 100 ms (from stock
 * firmware; need not established). */
static __attribute__((noinline)) int open_tail(void)
{
    fb_data *fb = (fb_data *)lcd_framebuffer_default.fb_ptr;
    if (!fb)
        return 1;
    memset(fb, 0, LCD_WIDTH * LCD_HEIGHT * sizeof(fb_data));
    if (dma_setup() || push_rows(fb, 0, LCD_HEIGHT))
        return 1;
    return delay_units(100);
}

void lcd_init_device(void)
{
    if (lcd_state != LCD_UNINIT)
        return;

    int err = cold_start();
    if (!err)
        err = open_tail();
    /* PWM3 backlight setup (25 % duty) happens with the panel bring-up. */
    if (!err)
        err = yp3_backlight_cold_setup();
    lcd_state = err ? LCD_FAILED : LCD_READY;
    if (!err) {
        semaphore_init(&dma_sem, 1, 0);
        mutex_init(&lcd_mtx);
        yp3_irq_install(DMA_IRQ, dma_irq);
        yp3_irq_clear(DMA_IRQ);
        yp3_irq_enable(DMA_IRQ);
        dma_irq_mode = true;
    }
}

/* ---- Rockbox API ------------------------------------------------------- */


static bool lcd_inverted;       /* lcd_set_invert_display, kept for wakes */

#ifdef HAVE_LCD_ENABLE
/* Off (lcd_sleep, a while after the backlight went off): display off and
 * asleep; on (backlight on): awake with the current frame, then the
 * backlight (the caller) comes on.
 *
 * Sleep and wake send `fe ef 10` / `fe ef 11`, then `fe ff`, as the init
 * table does before its own sleep-out: without the `fe ff`,
 * pixel pushes that raced the sleep
 * (lcd_on was checked outside the lock) could land in configuration
 * registers and leave the panel white until a reset. The panel needs
 * 120 ms between sleep-in and sleep-out, and 120 ms after sleep-out. */
static bool lcd_on = true;
static long lcd_slept_tick;

/* One command after `fe ef` (inter register enable 1/2), then `fe ff`. */
static int panel_page_command(uint8_t command)
{
    return push_command(GC_INTER_REG_EN1) || push_command(GC_INTER_REG_EN2) ||
           push_command(command) ||
           push_command(GC_INTER_REG_EN1) || push_command(GC_CMD_FF);
}

void lcd_enable(bool on)
{
    if (on == lcd_on || lcd_state != LCD_READY)
        return;
    mutex_lock(&lcd_mtx);
    int err;
    if (on) {
        long since = current_tick - lcd_slept_tick;
        if (since < PANEL_SLEEP_SETTLE)
            sleep(PANEL_SLEEP_SETTLE - since + 1);
        err = !sl6801_clock_gate_enable(CLK_GATE_ID_LCD);
        if (!err) {
            /* The gate loses the controller's setup (white panel without) */
            controller_config();
            err = dma_setup();
        }
        err = err || panel_page_command(DCS_SLEEP_OUT) ||
              push_command(lcd_inverted ? DCS_INVERT_OFF : DCS_INVERT_ON);
        if (!err) {
            sleep(PANEL_SLEEP_SETTLE + 1);
            err = push_rows((const fb_data *)lcd_framebuffer_default.fb_ptr,
                            0, LCD_HEIGHT) ||
                  push_command(DCS_DISPLAY_ON);
            /* display-on shows white until the panel has refreshed a few
             * times: keep the backlight off for ~3 frames */
            if (!err)
                sleep(PANEL_ON_SETTLE + 1);
        }
        lcd_on = true;
    } else {
        lcd_on = false;         /* no pushes from here (checked in the lock) */
        err = push_command(DCS_DISPLAY_OFF) ||
              panel_page_command(DCS_SLEEP_IN);
        lcd_slept_tick = current_tick;
        /* Nothing reaches the controller until the wake above re-enables
         * its clock and re-applies its setup (lost across the gate). */
        if (!err)
            sl6801_clock_gate_disable(CLK_GATE_ID_LCD);
    }
    if (err)
        lcd_state = LCD_FAILED;
    mutex_unlock(&lcd_mtx);
    if (on)
        send_event(LCD_EVENT_ACTIVATION, NULL);
}

#ifdef HAVE_LCD_SLEEP
/* Called by the backlight thread a while after the backlight went off. */
void lcd_sleep(void)
{
    lcd_enable(false);
}

/* At shutdown, so the shutdown screen shows. */
void lcd_awake(void)
{
    lcd_enable(true);
}
#endif

bool lcd_active(void)
{
    return lcd_on;
}
#endif

static void update_rows(int top, int bottom)
{
    bool lock = dma_irq_mode && irq_enabled() && get_interrupt_number() == 0;
    if (lock)
        mutex_lock(&lcd_mtx);
    if (lcd_state == LCD_READY &&
#ifdef HAVE_LCD_ENABLE
        lcd_on &&               /* inside the lock: never into a sleeping
                                   panel */
#endif
        push_rows((const fb_data *)lcd_framebuffer_default.fb_ptr, top,
                  bottom))
        lcd_state = LCD_FAILED;
    if (lock)
        mutex_unlock(&lcd_mtx);
}

void lcd_update_rect(int x, int y, int width, int height)
{
    if (lcd_state != LCD_READY || width <= 0 || height <= 0 ||
        x >= LCD_WIDTH || x + width <= 0)
        return;

    update_rows(y, y + height);
}

void lcd_update(void)
{
    if (lcd_state != LCD_READY)
        return;
#ifdef YP3_DEV
    static unsigned frames;
    uint32_t t0 = REG(SL6801_COUNTER_LO);
#endif
    update_rows(0, LCD_HEIGHT);
#ifdef YP3_DEV
    if (++frames <= 3)
        dev_log("lcd_update: full frame %lu us",
                    (unsigned long)(REG(SL6801_COUNTER_LO) - t0));
#endif
}

/* The panel inverts in hardware: INVON (0x21) is normal here.
 * While the panel sleeps (controller clock gated) the setting waits for
 * the wake. */
void lcd_set_invert_display(bool yesno)
{
    bool lock = dma_irq_mode && irq_enabled() && get_interrupt_number() == 0;
    if (lock)
        mutex_lock(&lcd_mtx);
    lcd_inverted = yesno;
    if (lcd_state == LCD_READY &&
#ifdef HAVE_LCD_ENABLE
        lcd_on &&
#endif
        push_command(yesno ? DCS_INVERT_OFF : DCS_INVERT_ON))
        lcd_state = LCD_FAILED;
    if (lock)
        mutex_unlock(&lcd_mtx);
}

void lcd_set_contrast(int contrast) { (void)contrast; }
/* Greyscale/mono blitting (pluginlib) is not supported on this panel. */
void lcd_blit_mono(const unsigned char *p, int x, int y, int w, int h,
                   int stride)
{ (void)p; (void)x; (void)y; (void)w; (void)h; (void)stride; }
void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                         int x, int y, int w, int h, int stride)
{
    (void)values; (void)phases; (void)x; (void)y; (void)w; (void)h;
    (void)stride;
}
