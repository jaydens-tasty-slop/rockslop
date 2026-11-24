/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * SL6801 audio output: audio PLL, codec open/close glue and the PA enable
 * pin (GPIO1.30). All waits use a finite register-read budget; a timeout
 * leaves the codec in its partial state and reports a failure code.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "audiohw.h"
#include "audio.h"
#include "capture-yp3.h"
#include "pcm_sw_volume.h"
#include "dev-yp3.h"
#include "pcm-yp3.h"
#include "gpio-yp3.h"
#include "mmio-sl6801.h"


/* Shared register facts: sl6801.h. */

/* ---- codec analog block (0x40009000) ---- */
/* CODEC_CTRL and its CODEC_CTRL_OPEN_BITS are shared with capture
 * (sl6801.h). */
/* Code 0 is the loudest; each step is ~0.75 dB less (31: -23.4 dB).
 * The PCM runs at -8 dB at most, as VOLUME's range does here
 * (yp3_codec.h): on battery, into headphones, 0.85% THD+N at the top. Into
 * a grounded input with USB in, it clips ~7 dB earlier (measured
 * 2026-10-31). */

/* Stage in the high half of the failure code, result in the low half. */
enum { ST_SHARED = 1, ST_CLOCK, ST_OPEN, ST_CLOSE };
enum { R_OK, R_CLOCK_BUSY, R_PLL_TIMEOUT };

#define STATUS_CLOSED   0xffff0000u     /* never opened, or closed */
static unsigned open_status = STATUS_CLOSED;

unsigned yp3_audiohw_status(void)
{
    return open_status;
}

/* ---- PA amplifier (GPIO1.30) ---- */

static void amp_set(bool on)
{
    yp3_pin_config(PIN_OWNER_AUDIO, on ? PIN_AMP_ON : PIN_AMP_OFF);
}

static int volume_l, volume_r;

/* No speaker volume cap: the jack cuts the speaker in hardware and there is
 * no jack detect, so a cap also limited headphones (user, 2026-10-21). */
static void apply_volume(void)
{
    pcm_set_master_volume(volume_l, volume_r);
}

/* Speaker amplifier (on = PIN_AMP_ON, see gpio-yp3.h; verified by recording
 * the test tone through the speaker with the webcam mic and the player's
 * mic). It is powered only while the DMA plays, with a
 * second's grace after a stop so track skips and key beeps do not switch it
 * each time. */
static bool speaker_enabled, pcm_active;
static struct timeout amp_off_tmo;

static int amp_off_cb(struct timeout *tmo)
{
    (void)tmo;
    if (!pcm_active)
        amp_set(false);
    return 0;
}

/* pcm-yp3.c, at a DMA start or stop (also from its IRQ). */
void yp3_audiohw_pcm_active(bool active)
{
    int irq = disable_irq_save();
    pcm_active = active;
    if (active) {
        timeout_cancel(&amp_off_tmo);
        if (speaker_enabled)
            amp_set(true);
    } else if (speaker_enabled) {
        timeout_register(&amp_off_tmo, amp_off_cb, HZ, 0);
    }
    restore_irq(irq);
}

void audiohw_enable_speaker(bool on)
{
#ifdef YP3_DEV
    /* Dev config speaker=0 (unattended runs, someone asleep nearby): the
     * amplifier stays off whatever the setting, the menus or a .cfg say. */
    static int held_off = -1;
    if (held_off < 0) {
        char v[4];
        held_off = dev_config("speaker", v, sizeof v) && !strcmp(v, "0");
    }
    if (held_off)
        on = false;
#endif
    int irq = disable_irq_save();
    speaker_enabled = on;
    if (!on)
        timeout_cancel(&amp_off_tmo);
    amp_set(on && pcm_active);
    restore_irq(irq);
}

#ifdef YP3_DEV
/* Dev shell "amp". */
void yp3_audiohw_amp(bool on)
{
    audiohw_enable_speaker(on);     /* as the speaker setting */
}
#endif

/* ---- bounded polling ---- */

#define POLL_BUDGET     10000u          /* register reads per call */
#define DAC_CTRL        0x40009100u     /* DAC control (logged at open) */

static uint32_t poll_left;

static void begin_polls(void)
{
    poll_left = POLL_BUDGET;
}

static bool poll_read(uintptr_t addr, uint32_t *v)
{
    if (!poll_left)
        return false;
    --poll_left;
    *v = REG(addr);
    return true;
}

/* mask == 0: OR in value. Otherwise replace the masked field with value
 * shifted to the mask's lowest set bit. */
static void update(uintptr_t addr, uint32_t mask, uint32_t value)
{
    uint32_t cur = REG(addr);

    if (!mask) {
        REG(addr) = cur | value;
        return;
    }
    unsigned shift = 0;
    while (!((mask >> shift) & 1u))
        ++shift;
    REG(addr) = (cur & ~mask) | ((value << shift) & mask);
}

/* ---- audio PLL (CLK_AUDIO_PLL0/1, AUDIO_PLL0_ENABLE: sl6801.h) ----
 * Values for the 22.579 MHz MCLK (44.1 kHz family). The 48 kHz family
 * (24.576 MHz) is 0x3060 and 0x3126 << 14. Kept bits and the other set
 * bits: meaning unknown. */
#define AUDIO_PLL0_KEEP         0xfc1f0000u
#define AUDIO_PLL0_MCLK_44K     0x3040u
#define AUDIO_PLL0_LOCKED       (1u << 28)
#define AUDIO_PLL1_BUSY         (1u << 11)
#define AUDIO_PLL1_KEEP         0x3800u
#define AUDIO_PLL1_SET          0x80000400u     /* bit 31 and bit 10 */
#define AUDIO_PLL1_FIELD_44K    (0x186c2u << 14)

/* Wait for the PLL not busy, program it, enable, poll for lock. */
static unsigned audio_clock_setup(void)
{
    uint32_t v;

    begin_polls();
    do {
        if (!poll_read(CLK_AUDIO_PLL1, &v))
            return R_CLOCK_BUSY;
    } while (v & AUDIO_PLL1_BUSY);
    v = REG(CLK_AUDIO_PLL0);
    REG(CLK_AUDIO_PLL0) = (v & AUDIO_PLL0_KEEP) | AUDIO_PLL0_MCLK_44K;
    v = REG(CLK_AUDIO_PLL1);
    REG(CLK_AUDIO_PLL1) = (v & AUDIO_PLL1_KEEP) | AUDIO_PLL1_SET |
                          AUDIO_PLL1_FIELD_44K;
    update(CLK_AUDIO_PLL0, 0, AUDIO_PLL0_ENABLE);
    while (poll_read(CLK_AUDIO_PLL0, &v))
        if (v & AUDIO_PLL0_LOCKED)
            return R_OK;
    return R_PLL_TIMEOUT;
}

/* ---- codec shared block, DAC digital path, analog output ----
 * Registers and sequences: F-0022..F-0029, F-0047, F-0048. Bit roles are
 * unknown unless named. Orders and waits are as the facts give them; most
 * are not shown to be needed (GUESS there). */
#define CODEC_RAM               0x40009400u
#define CODEC_RAM_END           0x40009600u
#define SYS_REG_20_CODEC_RAM    (1u << 2)
#define DAC_CTRL_RESET          (1u << 0)
#define DAC_CTRL_RATE_SET       (1u << 1)
#define DAC_CTRL_RATE_MASK      0xf00u
#define DAC_CTRL_RATE_44K       (8u << 8)
#define DAC_CTRL_FIELDS_MASK    0xff000u        /* 19:12 */
#define DAC_CTRL_FIELDS         0x55000u        /* 5 and 5 */
#define DAC_FORMAT              0x40009108u
#define DAC_FORMAT_VALUE        0x84u           /* 16-bit + bit 7 */
#define DAC_REG_110             0x40009110u
#define DAC_REG_110_MASK        0x03ffffffu
#define DAC_HANDSHAKE           0x40009114u
#define DAC_HANDSHAKE_BIT       (1u << 1)
#define DAC_CH_MODE(c)          (0x40009130u + 0x20u * (c))
#define DAC_CH_MODE_PLAY        3u
#define DAC_CH_REG4(c)          (0x40009134u + 0x20u * (c))
#define DAC_CH_LEVEL(c)         (0x40009138u + 0x20u * (c))
#define DAC_CH_LEVEL_MASK       0x1ffu
#define DAC_CHANNELS            3u
#define HANDSHAKE_BUDGET        1000000u

#define OUT_GAIN                0x40009030u
#define OUT_GAIN_EN0            (1u << 0)
#define OUT_GAIN_EN1            (1u << 4)
#define OUT_GAIN_CODES          0x1f1f0000u     /* 20:16 and 28:24 */
#define OUT_GAIN_RAMP           0xe0000000u     /* 31:29 */
#define OUT_PATH2               0x4000902cu
#define OUT_PATH2_GAIN          0x001f0000u
#define OUT_BIAS                0x40009040u
#define OUT_BIAS_BIT9           (1u << 9)
#define OUT_BIAS_FIELD          0x30u           /* 5:4 = 2 */
#define OUT_BIAS_BIT11          (1u << 11)
#define OUT_CTRL                0x4000904cu
#define OUT_CTRL_SEL0           (1u << 17)
#define OUT_CTRL_SEL1           (1u << 16)
#define OUT_CTRL_BIT20          (1u << 20)
#define OUT_CTRL_ACTIVE         0x30u           /* bits 4, 5 */
#define OUT_CTRL_BRACKET        0x3u            /* bits 0, 1 */
#define OUT_CTRL_BIT10          (1u << 10)
#define OUT_PATH(p)             (0x40009050u + 4u * (p))
#define OUT_PATH_BIT15          (1u << 15)
#define OUT_PATH_BIT28          (1u << 28)
#define OUT_PATH_OPEN           (1u << 29)
#define OUT_PATH_LEVEL          0xfffu
#define OUT_TRIM(p)             (0x40009064u + 4u * (p))
#define OUT_PATHS               3u
#define OUT_WAIT_US             10000u

static void codec_wait_us(uint32_t us)
{
    sl6801_udelay(us);
}

/* F-0029 step 3: the shared reset, also the last step of a close. */
static void shared_reset(void)
{
    update(DAC_CTRL, 0, DAC_CTRL_RESET);
    codec_wait_us(20000);
    for (unsigned c = 0; c < DAC_CHANNELS; c++)
        update(DAC_CH_LEVEL(c), DAC_CH_LEVEL_MASK, DAC_CH_LEVEL_MASK);
    sl6801_clock_gate_enable(CLK_GATE_ID_CODEC_RAM);
    REG(SYS_REG_20) |= SYS_REG_20_CODEC_RAM;
    for (uintptr_t a = CODEC_RAM; a < CODEC_RAM_END; a += 4)
        REG(a) = 0u;
    REG(SYS_REG_20) &= ~SYS_REG_20_CODEC_RAM;
    sl6801_clock_gate_disable(CLK_GATE_ID_CODEC_RAM);
}

static bool output_active(void)
{
    for (unsigned p = 0; p < OUT_PATHS; p++)
        if (REG(OUT_PATH(p)) & OUT_PATH_OPEN)
            return true;
    return false;
}

/* Up: clocks and the shared reset (F-0029), unless an output path is open
 * (then the block is in use: leave it). Down: the shared reset (F-0026
 * step 5). */
static unsigned codec_shared_power(bool up)
{
    if (output_active())
        return R_OK;
    if (up) {
        if (!sl6801_clock_gate_enable(CLK_GATE_ID_CODEC))
            return R_CLOCK_BUSY;
        REG(CLK_SEL_13) |= CLK_SEL_ENABLE;
    }
    shared_reset();
    if (up) {
        update(DAC_CTRL, DAC_CTRL_FIELDS_MASK, DAC_CTRL_FIELDS >> 12);
        for (unsigned c = 0; c < DAC_CHANNELS; c++)
            REG(DAC_CH_REG4(c)) &= ~(1u << 16);
    }
    return R_OK;
}

/* F-0028: the DAC digital path, after the audio PLL has locked. */
static unsigned dac_open(void)
{
    uint32_t v;

    update(CODEC_CTRL, 0, CODEC_CTRL_OPEN_BITS);
    REG(DAC_REG_110) &= ~DAC_REG_110_MASK;
    REG(DAC_CTRL) = (REG(DAC_CTRL) & ~DAC_CTRL_RATE_MASK) |
                    DAC_CTRL_RATE_44K | DAC_CTRL_RATE_SET;
    update(DAC_FORMAT, 0, DAC_FORMAT_VALUE);
    update(DAC_HANDSHAKE, 0, DAC_HANDSHAKE_BIT);
    for (unsigned n = 0; ; n++) {
        if (n == HANDSHAKE_BUDGET)
            return R_CLOCK_BUSY;
        v = REG(DAC_HANDSHAKE);
        if (!(v & DAC_HANDSHAKE_BIT))
            break;
    }
    REG(DAC_CH_MODE(0)) = DAC_CH_MODE_PLAY;
    REG(DAC_CH_MODE(1)) = DAC_CH_MODE_PLAY;
    REG(DAC_CH_MODE(2)) = 0u;
    for (unsigned c = 0; c < DAC_CHANNELS; c++)
        REG(DAC_CH_LEVEL(c)) &= ~DAC_CH_LEVEL_MASK;
    return R_OK;
}

/* F-0024, F-0047: the per-device output trims from the trim bank, 7 bits
 * each. Path 0: word 0x2c bits 3:0 above word 0x28 bits 31:29. Path 1:
 * word 0x2c bits 10:4. */
#define TRIM_PATH0_HIGH_MASK    0xfu
#define TRIM_PATH0_LOW_SHIFT    29
#define TRIM_PATH0_LOW_BITS     3
#define TRIM_PATH1_SHIFT        4
#define TRIM_PATH1_MASK         0x7fu

/* Leaves the bank's clock selector alone if something else configured
 * it. */
static void read_trims(uint32_t *t0, uint32_t *t1)
{
    sl6801_clock_gate_enable(CLK_GATE_ID_TRIM);
    if ((REG(CLK_SEL_1B) & ~CLK_SEL_ENABLE) == 0u)
        REG(CLK_SEL_1B) |= CLK_SEL_ENABLE;
    uint32_t w28 = REG(TRIM_WORD_28), w2c = REG(TRIM_WORD_2C);
    *t0 = ((w2c & TRIM_PATH0_HIGH_MASK) << TRIM_PATH0_LOW_BITS) |
          (w28 >> TRIM_PATH0_LOW_SHIFT);
    *t1 = (w2c >> TRIM_PATH1_SHIFT) & TRIM_PATH1_MASK;
}

/* Paths 0 and 1 (headphones L, R): F-0025 open, F-0026 close. The open
 * makes a DC step, so the driver opens once and keeps it open. */
static unsigned analog_path_set(bool open)
{
    if (open) {
        uint32_t t0, t1;
        unsigned rc = dac_open();
        if (rc != R_OK)
            return rc;
        read_trims(&t0, &t1);
        update(OUT_GAIN, 0, OUT_GAIN_EN0);
        update(OUT_GAIN, 0, OUT_GAIN_EN1);
        update(OUT_BIAS, 0, OUT_BIAS_BIT9);
        update(OUT_CTRL, 0, OUT_CTRL_SEL0);
        update(OUT_CTRL, 0, OUT_CTRL_SEL1);
        update(OUT_BIAS, OUT_BIAS_FIELD, 2u);
        update(OUT_CTRL, 0, OUT_CTRL_BIT20);
        update(OUT_BIAS, 0, OUT_BIAS_BIT11);
        update(OUT_PATH(0), 0, OUT_PATH_BIT15);
        update(OUT_PATH(1), 0, OUT_PATH_BIT15);
        REG(OUT_TRIM(0)) = t0;
        REG(OUT_TRIM(1)) = t1;
        codec_wait_us(OUT_WAIT_US);
        for (unsigned p = 0; p < 2; p++) {
            update(OUT_PATH(p), 0, OUT_PATH_BIT28);
            update(OUT_PATH(p), 0, OUT_PATH_OPEN);
        }
        update(OUT_CTRL, 0, OUT_CTRL_ACTIVE);
        update(OUT_CTRL, 0, OUT_CTRL_BRACKET);
        codec_wait_us(OUT_WAIT_US);
        for (unsigned p = 0; p < 2; p++)
            update(OUT_PATH(p), OUT_PATH_LEVEL, OUT_PATH_LEVEL);
        codec_wait_us(OUT_WAIT_US);
        REG(OUT_CTRL) &= ~OUT_CTRL_BRACKET;
        /* Gain codes stay 0 (loudest); the level is set in software. */
        REG(OUT_GAIN) &= ~OUT_GAIN_CODES;
        return R_OK;
    }
    REG(OUT_GAIN) &= ~OUT_GAIN_CODES;
    for (unsigned n = 0; n < 10; n++) {         /* ramp, up to 10 ms */
        if (!(REG(OUT_GAIN) & OUT_GAIN_RAMP))
            break;
        codec_wait_us(1000);
    }
    codec_wait_us(OUT_WAIT_US);
    for (unsigned p = 0; p < OUT_PATHS; p++)
        REG(OUT_PATH(p)) &= ~OUT_PATH_LEVEL;
    REG(OUT_CTRL) &= ~(OUT_CTRL_ACTIVE | OUT_CTRL_BIT10);
    for (unsigned p = 0; p < OUT_PATHS; p++)
        REG(OUT_PATH(p)) &= ~OUT_PATH_OPEN;
    for (unsigned p = 0; p < OUT_PATHS; p++)
        REG(OUT_PATH(p)) &= ~OUT_PATH_BIT28;
    REG(OUT_BIAS) &= ~OUT_BIAS_BIT9;
    REG(OUT_PATH2) &= ~OUT_PATH2_GAIN;
    return R_OK;
}

/* ---- Rockbox audiohw interface ---- */

/* Codec and PLL bring-up. */
static void codec_open(void)
{
    unsigned stage = ST_SHARED, rc;

    open_status = STATUS_CLOSED;
    amp_set(false);
    rc = codec_shared_power(true);
    if (rc == R_OK) {
        stage = ST_CLOCK;
        rc = audio_clock_setup();
    }
    if (rc == R_OK) {
        stage = ST_OPEN;
        rc = analog_path_set(true);
    }
    dev_log("audio open: stage %u result %u; 80000=%08lx 80010=%08lx "
                "80014=%08lx 80040=%08lx 9100=%08lx", stage, rc,
                (unsigned long)REG(CLK_PLL2),
                (unsigned long)REG(CLK_AUDIO_PLL0),
                (unsigned long)REG(CLK_AUDIO_PLL1),
                (unsigned long)REG(CLK_CPU),
                (unsigned long)REG(DAC_CTRL));
    dev_log("audio out: 9030=%08lx 9040=%08lx 904c=%08lx 9050=%08lx "
                "9054=%08lx trims %lu %lu", (unsigned long)REG(OUT_GAIN),
                (unsigned long)REG(OUT_BIAS), (unsigned long)REG(OUT_CTRL),
                (unsigned long)REG(OUT_PATH(0)),
                (unsigned long)REG(OUT_PATH(1)),
                (unsigned long)REG(OUT_TRIM(0)),
                (unsigned long)REG(OUT_TRIM(1)));
    open_status = rc == R_OK ? 0 : (stage << 16) | rc;
}

static struct mutex power_mtx;
static unsigned users = YP3_CODEC_OUTPUT;

/* Called by the PCM sink at boot, before it configures DMA. */
void audiohw_preinit(void)
{
    mutex_init(&power_mtx);
    codec_open();
}

/* PA pin setup, after the PCM DMA channel is configured: off until the
 * speaker setting is applied. */
void audiohw_postinit(void)
{
    amp_set(false);
}

/* PA off, DMA stopped, then analog close. */
void audiohw_close(void)
{
    if (open_status)
        return;
    amp_set(false);
    yp3_pcm_halt();
    unsigned rc = analog_path_set(false);
    if (rc == R_OK)
        rc = codec_shared_power(false);
    open_status = rc == R_OK ? STATUS_CLOSED : (ST_CLOSE << 16) | rc;
}

/* The analog open pops (DC
 * steps, -12..-23 dBFS on the headphone out), so the codec stays open:
 * while Bluetooth is the sink and nothing captures, only its audio PLL is
 * off. Stopping and restarting MCLK clicks at ~-35 dBFS each way. */
void yp3_audiohw_use(unsigned user, bool use)
{
    mutex_lock(&power_mtx);
    unsigned was = users;
    users = use ? users | user : users & ~user;
    if (users && !was) {
        unsigned rc = audio_clock_setup();
        if (rc != R_OK)
            open_status = (ST_CLOCK << 16) | rc;
    } else if (!users && was) {
        REG(CLK_AUDIO_PLL0) &= ~AUDIO_PLL0_ENABLE;
    }
    mutex_unlock(&power_mtx);
}

void audiohw_set_volume(int left, int right)
{
    volume_l = left;
    volume_r = right;
    apply_volume();
}


#ifdef HAVE_RECORDING
void audiohw_set_recvol(int left, int right, int type)
{
    if (type == AUDIO_GAIN_LINEIN)
        yp3_capture_set_gain(left, right);
    else if (type == AUDIO_GAIN_MIC)
        yp3_capture_set_mic_gain(left);
}
#endif

void audio_set_output_source(int source)
{
    (void)source;
}
