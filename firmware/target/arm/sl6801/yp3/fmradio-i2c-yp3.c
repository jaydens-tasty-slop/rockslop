/* SPDX-License-Identifier: GPL-2.0-or-later */
/* I2C to the RDA58xx FM tuner controller 0
 * at 0x40089000, SCL/SDA on GPIO1.13/1.11, 12 MHz reference from clock 0x1c
 * on GPIO3.11. Every poll is bounded.
 *   +0 control (bit31 reset/busy, [3:0] divider: 24 MHz >> (n+1))
 *   +4 command (bit31 go, bit0 START, bit1 READ, bit2 NACK, bit3 STOP)
 *   +8 tx data, +0xc rx data, +0x10 status (bit15 NACK, write 1 to clear)
 */
#include "config.h"
#include "system.h"
#include "fmradio_i2c.h"
#include "gpio-yp3.h"
#include "power.h"
#include "mmio-sl6801.h"

#define I2C(o)      REG(I2C0_BASE + (o))
#define I2C_CTRL    0x00u
#define I2C_CMD     0x04u
#define I2C_TX      0x08u
#define I2C_RX      0x0cu
#define I2C_STATUS  0x10u
#define CMD_GO      0x80000000u
#define CMD_START   0x01u
#define CMD_READ    0x02u
#define CMD_NACK    0x04u
#define CMD_STOP    0x08u
#define STATUS_NACK 0x8000u
/* Controller reset writes this to the status register: bit 15 clears a stale NACK; bit 14's role is unknown. */
#define STATUS_RESET_CLEAR 0xc000u
#define CTRL_RESET  0x80000000u   /* set to reset; reads 1 until done */
/* Bus clock = 24 MHz >> (n + 1); n = 8 gives 46.9 kHz. */
#define CTRL_DIV_MASK 0xfu
/* 24 MHz / 2^(n+1): 8 is ~47 kHz; 6 (~190 kHz, the tuner takes
 * 400) cut the RDS poll from 6.4% to 2.5% CPU */
#define CTRL_DIV_190KHZ 6u
#define POLLS       100000u       /* bound on every busy poll */
/* Delay between the I2C setup and the reference clock: 100 ms (purpose not
 * established). */
#define SETUP_DELAY_US 100000u
#define REFCLK_SOURCE 2u        /* 12 MHz: CLK_SEL_1C in sl6801.h */

/* GO, START and STOP self-clear when done; READ and NACK stay set. */
static bool i2c_cmd(uint32_t cmd)
{
    uint32_t busy = CMD_GO | (cmd & (CMD_START | CMD_STOP));
    I2C(I2C_CMD) = CMD_GO | cmd;
    for (unsigned i = 0; i < POLLS; i++)
        if (!(I2C(I2C_CMD) & busy))
            return true;
    return false;
}

/* Address phase; on NACK clear the flag and send STOP. */
static bool i2c_start(unsigned char address)
{
    I2C(I2C_TX) = address;
    if (!i2c_cmd(CMD_START))
        return false;
    if (I2C(I2C_STATUS) & STATUS_NACK) {
        I2C(I2C_STATUS) = STATUS_NACK;
        i2c_cmd(CMD_STOP);
        return false;
    }
    return true;
}

/* False if init found the controller's clock gate dead: no bus access. */
static bool i2c_ready;

int fmradio_i2c_write(unsigned char address, const unsigned char *buf,
                      int count)
{
    if (!i2c_ready || !i2c_start(address & ~1))
        return -1;
    for (int i = 0; i < count; i++) {
        I2C(I2C_TX) = buf[i];
        if (!i2c_cmd(i == count - 1 ? CMD_STOP : 0))
            return -1;
        if (I2C(I2C_STATUS) & STATUS_NACK) {
            I2C(I2C_STATUS) = STATUS_NACK;
            i2c_cmd(CMD_STOP);
            return -1;
        }
    }
    return 0;
}

int fmradio_i2c_read(unsigned char address, unsigned char *buf, int count)
{
    if (!i2c_ready || !i2c_start(address | 1))
        return -1;
    for (int i = 0; i < count; i++) {
        uint32_t cmd = i == count - 1 ? CMD_READ | CMD_NACK | CMD_STOP
                                      : CMD_READ;
        if (!i2c_cmd(cmd))
            return -1;
        buf[i] = I2C(I2C_RX);
    }
    return 0;
}

void fmradio_i2c_init(void)
{
    /* Clock gate 0x50 (request + ack), then clock 0x3a. */
    i2c_ready = sl6801_clock_gate_enable(CLK_GATE_ID_I2C0);
    if (!i2c_ready)
        return;
    REG(CLK_SEL_3A) |= CLK_SEL_ENABLE;
    yp3_pin_config(PIN_OWNER_TUNER, PIN_TUNER_SCL);
    yp3_pin_config(PIN_OWNER_TUNER, PIN_TUNER_SDA);
    I2C(I2C_CTRL) |= CTRL_RESET;
    for (unsigned i = 0; i < POLLS && (I2C(I2C_CTRL) & CTRL_RESET); i++)
        ;
    I2C(I2C_STATUS) = STATUS_RESET_CLEAR;
    I2C(I2C_CTRL) = (I2C(I2C_CTRL) & ~CTRL_DIV_MASK) | CTRL_DIV_190KHZ;
    sl6801_udelay(SETUP_DELAY_US);
    /* 12 MHz tuner reference: clock 0x1c, enable, out on
     * GPIO3.11; the tuner runs in 12 MHz mode (RDA5802_POWERCFG_INIT). */
    yp3_pin_config(PIN_OWNER_TUNER, PIN_TUNER_REFCLK);
    REG(CLK_SEL_1C) = (REG(CLK_SEL_1C) & ~CLK_SEL_1C_SOURCE_MASK) |
                      CLK_SEL_1C_SOURCE(REFCLK_SOURCE);
    tuner_power(true);
}

/* The reference clock runs only while the radio is in use. */
bool tuner_power(bool status)
{
    bool old = tuner_powered();
    if (status)
        REG(CLK_SEL_1C) |= CLK_SEL_ENABLE;
    else
        REG(CLK_SEL_1C) &= ~CLK_SEL_ENABLE;
    return old;
}

bool tuner_powered(void)
{
    return REG(CLK_SEL_1C) & CLK_SEL_ENABLE;
}
