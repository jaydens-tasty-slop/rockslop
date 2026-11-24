/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Codec ADC capture of the aux (FM tuner) inputs, played back through the
 * PCM mixer (ADC -> DAC).
 * The built-in mic is captured through mic path 2 (mono, no monitor).
 * The ADC runs at 44.1 kHz on the same audio clock as the DAC, so the ring
 * never drifts. */
#include "config.h"
#include "system.h"
#include "audio.h"
#include "pcm_mixer.h"
#include "pcm-internal.h"
#include "gpio-yp3.h"
#include "irq-yp3.h"
#include "capture-yp3.h"
#include "pcm-yp3.h"
#ifdef YP3_DEV
#include "dev-yp3.h"
#endif
#include <string.h>
#include "mmio-sl6801.h"


/* Codec registers whose roles are not established are named by offset/bit
 * and marked unknown. */

/* Bit 1 aux L, bit 17 aux R. Mic paths 0/1 live here too; they do not
 * reach the built-in mic. */
#define CODEC_ANALOG            0x40009008u
#define CODEC_ANALOG_AUX_LR     0x00020002u
/* Mic path 2 = the built-in mic: enable bits 0, 1, 7 and 16, then the
 * 7-bit gain [14:8]. Bit roles unknown. Off clears the enables and the
 * gain. */
#define CODEC_MIC2              0x4000900cu
#define CODEC_MIC2_ENABLE       0x00010083u
#define CODEC_MIC2_GAIN_MASK    0x00007f00u
#define CODEC_MIC2_GAIN_SHIFT   8
#define MIC_GAIN_MAX            127u
/* Aux (FM tuner) inputs: L enable [1:0] = 3, gain code [9:5]; R
 * enable [17:16] = 3, gain code [25:21]. The "off" mask clears the
 * enables and gains. */
#define CODEC_AUX               0x40009010u
#define CODEC_AUX_GAIN_MASK     0x03e003e0u
#define CODEC_AUX_GAIN_L_SHIFT  5
#define CODEC_AUX_GAIN_R_SHIFT  21
#define CODEC_AUX_OFF_KEEP      0xfc1cfc1cu
/* One byte per ADC input 0..2; the clear masks cover bits 0..3 of each
 * used byte. Roles unknown. */
#define CODEC_CH_ROUTE          0x40009020u
#define CODEC_CH_ROUTE_CLEAR01  0x000f0fu
#define CODEC_CH_ROUTE_CLEAR2   0x0f0000u
/* Gain codes 0..31 (5-bit fields). */
#define GAIN_MAX                31u

/* ADC block: bit 1 runs the ADC, [11:8] rate code (8 = 44.1
 * kHz, 3 = 16 kHz). */
#define ADC_CTRL                0x40009200u
#define ADC_CTRL_RUN            2u
#define ADC_CTRL_SETUP          1u              /* set at set-up (F-0030) */
#define ADC_CTRL_RATE_MASK      0xf00u
#define ADC_CTRL_RATE_44K       (8u << 8)
/* F-0030: ADC set-up bits of the shared codec control word. */
#define CODEC_CTRL_INPUTS       ((1u << 24) | (1u << 25))
#define CODEC_CTRL_CAPTURE      (1u << 30)
#define CODEC_REG_FC            0x400090fcu
#define CODEC_REG_FC_CLEAR      0x1fu           /* bits 4:0 */
#define CODEC_REG_7C            0x4000907cu     /* bits 8:6 cleared (mic) */
#define CODEC_AUX_ENABLE        0x00030003u
#define CODEC_MIC2_PREP_CLEAR   0x00000041u     /* bits 0 and 6 */
#define CODEC_REG_7C_MIC_CLEAR  0x000001c0u
#define CODEC_CH_ROUTE_AUX      0x000d0du       /* inputs 0, 1 */
#define CODEC_CH_ROUTE_MIC      0x0d0000u       /* input 2 */
#define ADC_FORMAT_16BIT        (1u << 2)
#define ADC_CH_UNUSED_CLEAR     0xbu            /* bits 0, 1, 3 */
#define ADC_CH_LEVEL(i)         (0x40009228u + 0x30u * (i))
#define ADC_CH_LEVEL_MASK       0x1ffu
#define ADC_CHANNELS            4u
/* Bit 7: role unknown. */
#define ADC_FORMAT              0x40009208u
#define ADC_FORMAT_BIT7         0x80u
/* DMA source: one 32-bit word per stereo frame. */
#define ADC_FIFO                0x4000920cu
/* Per-channel enable words for ADC channels 0..3 (0x30 apart). Value 9
 * (bits 3 and 0) = channel in use. */
#define ADC_CH(i)               (0x40009220u + 0x30u * (i))
#define ADC_CH_MODE_MASK        0xfu            /* bits 3:0 */
#define ADC_CH_MODE_USED        9u              /* bits 3 and 0 */

#define DMA_CHANNEL     3u
#define DMA_IRQ         DMA_CH_IRQ(DMA_CHANNEL)
#define DMA_CONTROL     DMA_CH_CONTROL(DMA_CHANNEL)
#define DMA_SOURCE      DMA_CH_SOURCE(DMA_CHANNEL)
#define DMA_DEST        DMA_CH_DEST(DMA_CHANNEL)
#define DMA_COUNT       DMA_CH_COUNT(DMA_CHANNEL)
#define DMA_INTMASK     DMA_CH_INTMASK(DMA_CHANNEL)
#define DMA_STATUS      DMA_CH_STATUS(DMA_CHANNEL)
/* Capture config: ADC request on the source side, memory destination,
 * 32-bit both sides (replaces DMA_CONFIG_MASK bits). */
#define DMA_CONFIG      (DMA_CFG_SRC_REQ(DMA_REQ_ADC) | DMA_CFG_BIT7 | \
                         DMA_CFG_WIDTH_9_8(DMA_WIDTH_32) | DMA_CFG_DST_MEM | \
                         DMA_CFG_BIT19 | DMA_CFG_WIDTH_21_20(DMA_WIDTH_32))

#define FRAME_BYTES     4u              /* 16-bit stereo */
#define PERIOD_BYTES    1024u           /* 256 stereo frames, 5.8 ms */
#define PERIODS         4u              /* 23 ms of slack */
#define PERIOD_FRAMES   (PERIOD_BYTES / FRAME_BYTES)
/* Mic: channel 2 alone gives two mono 16-bit samples per FIFO word, so a
 * period's DMA fills half as many bytes; the IRQ widens it to stereo. */
#define MONO_PERIOD_BYTES (PERIOD_FRAMES * 2u)

static uint32_t ring[PERIODS][PERIOD_BYTES / 4];
static uint32_t mono[MONO_PERIOD_BYTES / 4];
static bool use_mic;
static uint32_t silence[64];      /* returned on underrun */
static volatile unsigned produced, consumed;    /* period counters */
/* After an underrun the mixer gets silence until the ring holds this many
 * periods again. The DAC takes one period at a time; the A2DP media tick
 * takes up to three in one 10 ms tick, so with a bare restart it found the
 * ring empty again at the next tick (~17 underruns/s, 2026-11-01). */
#define REFILL_DAC      1u
#define REFILL_BT       2u
static bool refilling;
static bool running;
struct yp3_capture_stats yp3_capture_stats;

#ifdef YP3_DEV
/* Who stops draining the ring (torture fm overruns): time the mixer's
 * calls, and log the longest stalls where they happen in the log */
#define PERIOD_US       (PERIOD_FRAMES * 1000000u / 44100u)
#define LATE_US         (2 * PERIOD_US)
#define LOG_GAP_US      50000u
#define LOG_GAPS        8               /* lines per start */
static unsigned gaps_logged;

static unsigned long counter_now(void)
{
    unsigned long now = *(volatile unsigned long *)SL6801_COUNTER_LO;
    return now ? now : 1;               /* 0 means "no call yet" */
}

/* from the monitor's start, so a slow DAC start counts as a gap */
static void note_consumer(void)
{
    unsigned long now = counter_now();
    unsigned long gap = now - yp3_capture_stats.last_us;
    yp3_capture_stats.last_us = now;
    if (gap > yp3_capture_stats.gap_max_us)
        yp3_capture_stats.gap_max_us = gap;
    if (gap > LATE_US)
        yp3_capture_stats.late++;
    if (gap > LOG_GAP_US && gaps_logged < LOG_GAPS) {
        gaps_logged++;
        dev_log("capture: %lu ms without a mixer call", gap / 1000);
    }
}
#endif

static void arm_period(void)
{
    REG(DMA_SOURCE) = ADC_FIFO;
    REG(DMA_DEST) = use_mic ? (uint32_t)mono
                            : (uint32_t)ring[produced % PERIODS];
    REG(DMA_COUNT) = (REG(DMA_COUNT) & ~DMA_COUNT_BYTES_MASK) |
                     (use_mic ? MONO_PERIOD_BYTES : PERIOD_BYTES);
    REG(DMA_CONTROL) = (REG(DMA_CONTROL) & ~DMA_CONTROL_BIT29) |
                       DMA_CONTROL_ENABLE;
}

#ifdef HAVE_RECORDING
/* Recording: completed periods are copied into the buffers pcm_record
 * hands out (the same stream also feeds the monitor through the mixer). */
static uint8_t *rec_ptr;
static size_t rec_left;
static bool recording;
static unsigned rec_locks;

static void record_period(const uint8_t *src, size_t size)
{
    while (recording && size) {
        size_t n = MIN(size, rec_left);
        memcpy(rec_ptr, src, n);
        rec_ptr += n, rec_left -= n, src += n, size -= n;
        if (!rec_left) {
            void *addr = NULL;
            size_t next = 0;
            if (pcm_rec_dma_complete_callback(PCM_DMAST_OK, &addr, &next) &&
                addr && next) {
                rec_ptr = (uint8_t *)addr;
                rec_left = next;
            } else {
                recording = false;
            }
        }
    }
}
#endif

/* Mono period -> stereo frames (L = R); the first sample of each FIFO
 * word is in its low half. */
static void widen_mono(uint32_t *dst)
{
    const uint16_t *src = (const uint16_t *)mono;
    for (unsigned i = 0; i < PERIOD_FRAMES; i++)
        dst[i] = src[i] | (uint32_t)src[i] << 16;
}

static void dma_irq(void)
{
    uint32_t status = REG(DMA_STATUS);
    REG(DMA_STATUS) = status;
    if (!running || !(status & DMA_STATUS_COMPLETE))
        return;
    /* Keep at most PERIODS - 1 periods ahead of the mixer. */
    if (produced - consumed >= PERIODS - 1)
        consumed++, yp3_capture_stats.overruns++;
    if (use_mic)
        widen_mono(ring[produced % PERIODS]);
#ifdef HAVE_RECORDING
    record_period((const uint8_t *)ring[produced % PERIODS], PERIOD_BYTES);
#endif
    produced++;
    yp3_capture_stats.periods++;
    arm_period();
}

/* Mixer callback (playback DMA completion): next captured period. */
static void get_more(const void **start, size_t *size)
{
#ifdef YP3_DEV
    note_consumer();
#endif
#ifdef HAVE_BLUETOOTH
    unsigned refill = pcm_current_sink() == PCM_SINK_BT ? REFILL_BT
                                                       : REFILL_DAC;
#else
    unsigned refill = REFILL_DAC;
#endif
    /* The capture IRQ's overrun path also advances consumed, and for the
     * Bluetooth sink this runs from the tick, which that IRQ preempts. */
    int irq = disable_irq_save();
    bool ready = produced - consumed >= (refilling ? refill : 1u);
    if (ready)
        *start = ring[consumed++ % PERIODS];
    restore_irq(irq);
    if (ready) {
        refilling = false;
        *size = PERIOD_BYTES;
        yp3_capture_stats.consumed++;
    } else {
        if (!refilling)
            yp3_capture_stats.underruns++;
        refilling = true;
        *start = silence;
        *size = sizeof silence;
    }
}

static const struct mixer_play_cbs mixer_cbs = { .get_more = get_more };

/* Aux gain codes 0..31 (~1.8 dB steps; the tuner at full volume clips
 * above ~24; 0 is very quiet). */
static unsigned gain_l = 22, gain_r = 22;

static void apply_gain(void)
{
    REG(CODEC_AUX) = (REG(CODEC_AUX) & ~CODEC_AUX_GAIN_MASK) |
                     gain_l << CODEC_AUX_GAIN_L_SHIFT |
                     gain_r << CODEC_AUX_GAIN_R_SHIFT;
}

void yp3_capture_set_gain(unsigned left, unsigned right)
{
    gain_l = MIN(left, GAIN_MAX);
    gain_r = MIN(right, GAIN_MAX);
    if (running)
        apply_gain();
}

/* Monitor the input through the DAC: the mixer playback channel. */
static bool monitoring;              /* the mixer playback channel is ours */

static void monitor_start(void)
{
    monitoring = true;
    refilling = true;
#ifdef YP3_DEV
    yp3_capture_stats.last_us = counter_now();
    gaps_logged = 0;
#endif
    /* Playback leaves this channel faded out; the monitor plays at unity. */
    mixer_channel_set_amplitude(PCM_MIXER_CHAN_PLAYBACK, MIX_AMP_UNITY);
    mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, &mixer_cbs, silence,
                            sizeof silence);
}

/* Mic gain code 0..127 (default 0x54, AMIC2). */
static unsigned mic_gain = 0x54;

static void apply_mic_gain(void)
{
    REG(CODEC_MIC2) = (REG(CODEC_MIC2) & ~CODEC_MIC2_GAIN_MASK) |
                      mic_gain << CODEC_MIC2_GAIN_SHIFT;
}

void yp3_capture_set_mic_gain(unsigned gain)
{
    mic_gain = MIN(gain, MIC_GAIN_MAX);
    if (running && use_mic)
        apply_mic_gain();
}

/* Record ADC start on the aux (FM) or mic input (F-0030, F-0032): pads,
 * shared control bits, input, ADC rate and format, routing, channels, and
 * run last. */
static void adc_start(void)
{
    if (use_mic) {
        yp3_pin_config(PIN_OWNER_AUDIO, PIN_MIC_1_14);
        yp3_pin_config(PIN_OWNER_AUDIO, PIN_MIC_1_15);
    } else {
        yp3_pin_config(PIN_OWNER_AUDIO, PIN_TUNER_AUDIO_1_20);
        yp3_pin_config(PIN_OWNER_AUDIO, PIN_TUNER_AUDIO_1_21);
    }
    REG(CODEC_CTRL) |= CODEC_CTRL_INPUTS;
    REG(CODEC_CTRL) |= CODEC_CTRL_OPEN_BITS;
    REG(CODEC_CTRL) |= CODEC_CTRL_CAPTURE;
    REG(CODEC_REG_FC) &= ~CODEC_REG_FC_CLEAR;
    if (use_mic) {
        REG(CODEC_REG_7C) &= ~CODEC_REG_7C_MIC_CLEAR;
        REG(CODEC_MIC2) &= ~CODEC_MIC2_PREP_CLEAR;
        REG(CODEC_MIC2) |= CODEC_MIC2_ENABLE;
        apply_mic_gain();
    } else {
        REG(CODEC_AUX) |= CODEC_AUX_ENABLE;
        apply_gain();
    }
    REG(CODEC_ANALOG) |= CODEC_ANALOG_AUX_LR;
    REG(ADC_CTRL) &= ~ADC_CTRL_RUN;
    REG(ADC_CTRL) |= ADC_CTRL_SETUP;
    for (unsigned i = 0; i < ADC_CHANNELS; i++)
        REG(ADC_CH_LEVEL(i)) &= ~ADC_CH_LEVEL_MASK;
    REG(ADC_CTRL) = (REG(ADC_CTRL) & ~ADC_CTRL_RATE_MASK) | ADC_CTRL_RATE_44K;
    /* Read-modify-write: hardware-driven bits must not be written back
     * (F-0123). */
    REG(ADC_FORMAT) |= ADC_FORMAT_16BIT | ADC_FORMAT_BIT7;
    REG(CODEC_CH_ROUTE) |= use_mic ? CODEC_CH_ROUTE_MIC : CODEC_CH_ROUTE_AUX;
    for (unsigned i = 0; i < ADC_CHANNELS; i++) {
        bool used = use_mic ? i == 2 : i < 2;
        uint32_t v = REG(ADC_CH(i));
        REG(ADC_CH(i)) = used ? (v & ~ADC_CH_MODE_MASK) | ADC_CH_MODE_USED
                              : v & ~ADC_CH_UNUSED_CLEAR;
    }
    REG(ADC_CTRL) |= ADC_CTRL_RUN;
}

static void adc_stop(void)
{
    REG(CODEC_AUX) &= CODEC_AUX_OFF_KEEP;
    REG(CODEC_ANALOG) &= ~CODEC_ANALOG_AUX_LR;
    if (use_mic)
        REG(CODEC_MIC2) &= ~(CODEC_MIC2_ENABLE | CODEC_MIC2_GAIN_MASK);
    for (unsigned i = 0; i < 3; i++)
        REG(ADC_CH(i)) &= ~ADC_CH_MODE_USED;
    REG(CODEC_CH_ROUTE) &= ~(CODEC_CH_ROUTE_CLEAR01 | CODEC_CH_ROUTE_CLEAR2);
    REG(ADC_FORMAT) &= ~ADC_FORMAT_BIT7;
    REG(ADC_CTRL) &= ~ADC_CTRL_RUN;
}

/* Capture without the DAC monitor (the mixer playback channel stays free). */
static bool monitor_wanted = true;

void yp3_capture_start_nomonitor(void)
{
    monitor_wanted = false;
    yp3_capture_start();
    monitor_wanted = true;
}

void yp3_capture_start(void)
{
    if (running)
        return;
    yp3_audiohw_use(YP3_CODEC_CAPTURE, true);
    produced = consumed = 0;
    int irq = disable_irq_save();
    yp3_irq_disable(DMA_IRQ);
    REG(DMA_CONTROL) &= ~DMA_CONTROL_ENABLE;
    REG(DMA_STATUS) = REG(DMA_STATUS);
    yp3_irq_install(DMA_IRQ, dma_irq);
    yp3_irq_priority(DMA_IRQ, YP3_AUDIO_DMA_PRIO);
    REG(DMA_CONTROL) = (REG(DMA_CONTROL) & ~DMA_CONFIG_MASK) | DMA_CONFIG;
    REG(DMA_COUNT) = (REG(DMA_COUNT) & ~DMA_COUNT_TOP(DMA_COUNT_TOP_AUDIO)) |
                     DMA_COUNT_TOP(DMA_COUNT_TOP_AUDIO);
    REG(DMA_INTMASK) = (REG(DMA_INTMASK) & DMA_INTMASK_KEEP) | DMA_INTMASK_SET;
    yp3_irq_clear(DMA_IRQ);
    running = true;
    adc_start();
    arm_period();
#ifdef HAVE_RECORDING
    if (!rec_locks)
#endif
    yp3_irq_enable(DMA_IRQ);
    restore_irq(irq);
    if (monitor_wanted)
        monitor_start();
}

void yp3_capture_stop(void)
{
    if (!running)
        return;
    if (monitoring)
        mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);
    monitoring = false;
    int irq = disable_irq_save();
    running = false;
    adc_stop();
    yp3_irq_disable(DMA_IRQ);
    REG(DMA_CONTROL) &= ~DMA_CONTROL_ENABLE;
    REG(DMA_STATUS) = REG(DMA_STATUS);
    yp3_irq_clear(DMA_IRQ);
    restore_irq(irq);
    yp3_audiohw_use(YP3_CODEC_CAPTURE, false);
}

/* Peak and mean absolute level of the newest captured period (dev use). */
void yp3_capture_levels(int *peak_l, int *peak_r, int *avg_l, int *avg_r)
{
    const int16_t *s = (const int16_t *)ring[(produced - 1) % PERIODS];
    int pl = 0, pr = 0;
    long al = 0, ar = 0;
    for (unsigned i = 0; i < PERIOD_BYTES / 2; i += 2) {
        int l = s[i] < 0 ? -s[i] : s[i];
        int r = s[i + 1] < 0 ? -s[i + 1] : s[i + 1];
        pl = MAX(pl, l), pr = MAX(pr, r), al += l, ar += r;
    }
    *peak_l = pl, *peak_r = pr;
    *avg_l = al / (PERIOD_BYTES / FRAME_BYTES),
        *avg_r = ar / (PERIOD_BYTES / FRAME_BYTES);
}

#if INPUT_SRC_CAPS != 0
/* The radio selects the FM input while playing and deselects it on pause
 * or exit (apps/radio/radio.c). */
void audio_input_mux(int source, unsigned flags)
{
    bool mic = source == AUDIO_SRC_MIC;
    bool fm = source == AUDIO_SRC_FMRADIO && !(flags & SRCF_FMRADIO_PAUSED);
    if (running && use_mic != mic)
        yp3_capture_stop();             /* switch inputs */
    use_mic = mic;
    if (fm)
        yp3_capture_start();
    else if (mic)
        yp3_capture_start_nomonitor();  /* no speaker -> mic feedback */
    else
        yp3_capture_stop();
}
#endif

#ifdef HAVE_RECORDING
/* pcm_record interface: the ADC runs while the FM input is selected
 * (audio_input_mux); recording only taps the stream. */
/* Block the capture IRQ (which feeds pcm_record) while pcm_record works on
 * its buffers. */
void pcm_rec_lock(void)
{
    int irq = disable_irq_save();
    if (rec_locks++ == 0)
        yp3_irq_disable(DMA_IRQ);
    restore_irq(irq);
}

void pcm_rec_unlock(void)
{
    int irq = disable_irq_save();
    if (rec_locks && --rec_locks == 0 && running)
        yp3_irq_enable(DMA_IRQ);
    restore_irq(irq);
}

/* pcm_init_recording() resets the mixer first (Rockbox expects hardware
 * monitoring): restart the monitor before recording is flagged active.
 * Never for the mic, which would feed back through the speaker. */
void pcm_rec_dma_init(void)
{
    if (running && !use_mic)
        monitor_start();
}

void pcm_rec_dma_close(void)
{
    pcm_rec_dma_stop();
    if (running && !use_mic &&
        mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK) != CHANNEL_PLAYING)
        monitor_start();
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    int irq = disable_irq_save();
    rec_ptr = addr;
    rec_left = size;
    recording = true;
    restore_irq(irq);
}

void pcm_rec_dma_stop(void)
{
    int irq = disable_irq_save();
    recording = false;
    restore_irq(irq);
}

const void *pcm_rec_dma_get_peak_buffer(void)
{
    return ring[(produced - 1) % PERIODS];
}
#endif
