/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * PCM sink for the SL6801 DAC: one DMA channel streams 512 byte periods from
 * two private stage buffers to the DAC port. The channel takes one period
 * per start and the DAC's FIFO covers only ~145 us after it ends, so the
 * next period is always copied into the other stage beforehand: the
 * completion IRQ restarts the channel first and refills (calling the PCM
 * core when the caller's buffer is used up) after.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "system.h"
#include "panic.h"
#include "audiohw.h"
#include "pcm_sink.h"
#include "pcm-internal.h"
#include "irq-yp3.h"
#include "pcm-yp3.h"
#include "mmio-sl6801.h"


#define DMA_CHANNEL     2u      /* lowest channel outside the LCD pair 0/1 */
#define DMA_IRQ         DMA_CH_IRQ(DMA_CHANNEL)
#define DMA_CONTROL     DMA_CH_CONTROL(DMA_CHANNEL)
#define DMA_SOURCE      DMA_CH_SOURCE(DMA_CHANNEL)
#define DMA_DEST        DMA_CH_DEST(DMA_CHANNEL)
#define DMA_COUNT       DMA_CH_COUNT(DMA_CHANNEL)
#define DMA_INTMASK     DMA_CH_INTMASK(DMA_CHANNEL)
#define DMA_STATUS      DMA_CH_STATUS(DMA_CHANNEL)
/* Playback config: memory source, DAC request on the destination side,
 * 32-bit both sides (replaces DMA_CONFIG_MASK bits). */
#define DMA_CONFIG      (DMA_CFG_SRC_MEM | DMA_CFG_BIT7 | \
                         DMA_CFG_WIDTH_9_8(DMA_WIDTH_32) | \
                         DMA_CFG_DST_REQ(DMA_REQ_DAC) | DMA_CFG_BIT19 | \
                         DMA_CFG_WIDTH_21_20(DMA_WIDTH_32))

#define PERIOD_BYTES    512u    /* 128 frames, 2.9 ms at 44.1 kHz */
/* DMA moves 32-bit words: buffers and sizes must be word aligned. */
#define WORD_ALIGN_MASK 3u
/* The period interrupt path runs from SRAM: from XIP its register work
 * alone took ~45 us a period (1.5% of the CPU). */
#define PCM_ICODE __attribute__((section(".icode")))

/* The only rate: the audio PLL runs the 44.1 kHz family (audiohw-yp3.c). */
static const unsigned long frequencies[] = { 44100 };
_Static_assert(PCM_DMAST_OK == 0 && PCM_DMAST_ERR_DMA == -1,
               "PCM callback status ABI changed");

enum { IDLE, ACTIVE, STALLED };

static uint32_t stage[2][PERIOD_BYTES / sizeof(uint32_t)];
static size_t stage_size[2];
static unsigned playing;        /* the stage the channel reads */
static bool queued;             /* the other stage holds the next period */
static bool fetching;           /* inside the core's completion callback */
static const uint8_t *source;   /* rest of the caller's buffer */
static size_t remaining;
static unsigned state;          /* ACTIVE: a period plays; STALLED: none was
                                   queued in time (locked core, or late) */
static uint16_t locks;
static bool error_pending;
static bool dma_ready;
static uint32_t completed_periods;
#ifdef YP3_DEV
/* dev shell "load": how late each period started after the previous one
 * should have finished, split by whether the IRQ started it or a refill
 * after a stall did, and who held the lock at the latest stall maximum */
struct yp3_pcm_late yp3_pcm_late;
static unsigned long period_end, lock_t;   /* period_end: sl6801_us() */
static uintptr_t lock_caller;
#endif

PCM_ICODE static void nvic_enable(bool on)
{
    if (on)
        yp3_irq_enable(DMA_IRQ);
    else
        yp3_irq_disable(DMA_IRQ);
    __asm__ volatile("dsb\n\tisb" ::: "memory");
}

static void nvic_clear_pending(void)
{
    yp3_irq_clear(DMA_IRQ);
}

PCM_ICODE static void disable_channel(void)
{
    nvic_enable(false);
    REG(DMA_CONTROL) &= ~DMA_CONTROL_ENABLE;
    __asm__ volatile("dsb" ::: "memory");
}

static void clear_status(void)
{
    uint32_t status = REG(DMA_STATUS);
    if (status)
        REG(DMA_STATUS) = status;   /* write one to clear */
}

PCM_ICODE static bool valid_buffer(const void *addr, size_t size)
{
    return addr && !((uintptr_t)addr & WORD_ALIGN_MASK) && size &&
           !(size & WORD_ALIGN_MASK) &&
           size <= UINTPTR_MAX - (uintptr_t)addr;
}

/* Deliver a deferred DMA error to the PCM core, unless locked. */
static void report_error(void)
{
    if (!error_pending || locks)
        return;
    error_pending = false;
    const void *addr = NULL;
    size_t size = 0;
    if (pcm_play_dma_complete_callback(PCM_DMAST_ERR_DMA, &addr, &size))
        pcm_play_stop_int();
}

static void fail(void)
{
    state = IDLE;
    queued = false;
    error_pending = true;
    disable_channel();
    nvic_clear_pending();
    yp3_audiohw_pcm_active(false);
    report_error();
}

/* Start the channel on stage i (source, destination, count, then control
 * bit 29 clear / 30 set). */
PCM_ICODE static bool start(unsigned i)
{
    if (REG(DMA_STATUS)) {
        fail();
        return false;
    }
#ifdef YP3_DEV
    unsigned long now = sl6801_us();
    if (state != IDLE) {
        long late = (long)(now - period_end);
        int k = state == STALLED;
        if (late > 100)
            yp3_pcm_late.over100[k]++;
        if (late > 300)
            yp3_pcm_late.over300[k]++;
        if (late > (long)yp3_pcm_late.max_us[k]) {
            yp3_pcm_late.max_us[k] = late;
            if (k) {
                yp3_pcm_late.lock_caller = lock_caller;
                yp3_pcm_late.lock_us = now - lock_t;
            }
        }
    }
    period_end = now + stage_size[i] * 1000000ul / (44100 * 4);
#endif
    playing = i;
    state = ACTIVE;
    __asm__ volatile("dsb" ::: "memory");
    REG(DMA_SOURCE) = (uint32_t)(uintptr_t)stage[i];
    REG(DMA_DEST) = CODEC_DAC_PORT;
    REG(DMA_COUNT) = (REG(DMA_COUNT) & ~DMA_COUNT_BYTES_MASK) |
                     (uint32_t)stage_size[i];
    REG(DMA_CONTROL) = (REG(DMA_CONTROL) & ~DMA_CONTROL_BIT29) |
                       DMA_CONTROL_ENABLE;
    __asm__ volatile("dsb" ::: "memory");
    nvic_enable(true);
    return true;
}

/* Copy the next period of the caller's buffer (word aligned, not empty:
 * valid_buffer) into stage i. */
PCM_ICODE static void fill(unsigned i)
{
    size_t size = remaining > PERIOD_BYTES ? PERIOD_BYTES : remaining;
    memcpy(stage[i], source, size);
    stage_size[i] = size;
    source += size;
    remaining -= size;
}

static void stop_dma(void)
{
    int old = disable_irq_save();
    state = IDLE;
    queued = false;
    error_pending = false;
    source = NULL;
    remaining = 0;
    disable_channel();
    clear_status();
    nvic_clear_pending();
    yp3_audiohw_pcm_active(false);
    restore_irq(old);
}

/* Queue the period after the one playing. A used-up caller buffer is
 * handed back to the core for the next one (all of it is in the stages by
 * then), which waits while the core is locked; the core's STARTED status
 * follows at once, as the next buffer is taken. Runs from the IRQ after
 * the restart, from unlock, and from play. */
PCM_ICODE static void prepare(void)
{
    if (state == IDLE || queued || fetching)
        return;
    if (!remaining) {
        if (locks)
            return;
        const void *addr = NULL;
        size_t size = 0;
        fetching = true;
        bool more = pcm_play_dma_complete_callback(PCM_DMAST_OK, &addr, &size);
        fetching = false;
        if (state == IDLE)      /* stopped or failed inside the callback */
            return;
        if (!more) {            /* the core stops itself first */
            stop_dma();
            return;
        }
        if (!valid_buffer(addr, size)) {
            fail();
            return;
        }
        source = addr;
        remaining = size;
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);
        if (state == IDLE)
            return;
    }
    fill(playing ^ 1);
    queued = true;
    if (state == STALLED) {
        queued = false;
        start(playing ^ 1);
    }
}

#ifdef YP3_DEV
extern unsigned long yp3_dma_us, yp3_dma_irqs;     /* dev shell "load" */
#endif
PCM_ICODE static void dma_irq(void)
{
#ifdef YP3_DEV
    unsigned long t0 = sl6801_us();
#endif
    int old = disable_irq_save();
    uint32_t status = REG(DMA_STATUS);

    if (status)
        REG(DMA_STATUS) = status;   /* acknowledge the sampled flags */
    if (state == ACTIVE && status) {
        if (status & ~(DMA_STATUS_COMPLETE | DMA_STATUS_HALF)) {
            fail();
        } else if (status & DMA_STATUS_COMPLETE) {
            /* The half flag can stay latched; it never completes a period. */
            disable_channel();
            if ((REG(DMA_CONTROL) & DMA_CONTROL_ENABLE) || REG(DMA_STATUS)) {
                fail();
            } else {
                ++completed_periods;
                if (queued) {
                    queued = false;
                    start(playing ^ 1);
                } else {
                    state = STALLED;
                }
                prepare();
            }
        }
    }
    restore_irq(old);
#ifdef YP3_DEV
    yp3_dma_us += sl6801_us() - t0;
    yp3_dma_irqs++;
#endif
}

static void dma_configure(void)
{
    int old = disable_irq_save();

    uint32_t control = REG(DMA_CONTROL);
    /* A stale transfer is stopped, not trusted. */
    if ((control & DMA_CONTROL_ENABLE) || REG(DMA_STATUS)) {
        disable_channel();
        clear_status();
        control = REG(DMA_CONTROL);
    }
    nvic_enable(false);
    REG(DMA_CONTROL) = (control & ~DMA_CONFIG_MASK) | DMA_CONFIG;
    REG(DMA_COUNT) = (REG(DMA_COUNT) & ~DMA_COUNT_TOP(DMA_COUNT_TOP_AUDIO)) |
                      DMA_COUNT_TOP(DMA_COUNT_TOP_AUDIO);
    REG(DMA_INTMASK) = (REG(DMA_INTMASK) & DMA_INTMASK_KEEP) | DMA_INTMASK_SET;
    nvic_clear_pending();
    __asm__ volatile("dsb" ::: "memory");
    restore_irq(old);
}

#ifdef YP3_DEV
/* How late the period in flight is now (0 if none): the profiler's filter */
long yp3_pcm_late_now(void)
{
    if (state != ACTIVE)
        return 0;
    return (long)(sl6801_us() - period_end);
}
#endif

uint32_t yp3_pcm_completed_periods(void)
{
    return completed_periods;
}

void yp3_pcm_halt(void)
{
    stop_dma();
}

static void sink_init(void)
{
    audiohw_preinit();
    if (yp3_audiohw_status() || !sl6801_clock_gate_enable(CLK_GATE_ID_DMA))
        return;                 /* play() reports the failure */
    yp3_irq_install(DMA_IRQ, dma_irq);
    yp3_irq_priority(DMA_IRQ, YP3_AUDIO_DMA_PRIO);
    dma_configure();
    audiohw_postinit();
    dma_ready = true;
}

static void sink_postinit(void)
{
}

#ifdef HAVE_PCM_SINK_POWER
void pcm_sink_power(enum pcm_sink_ids sink, bool on)
{
    if (sink == PCM_SINK_BUILTIN)
        yp3_audiohw_use(YP3_CODEC_OUTPUT, on);
}
#endif

static void sink_set_freq(uint16_t frequency)
{
    if (!dma_ready)
        panicf("YP3 PCM: audio not open (%x)", yp3_audiohw_status());
    if (frequency != 0)
        panicf("YP3 PCM: unsupported sample-rate index");
}

static void sink_lock(void)
{
    int old = disable_irq_save();
    if (locks == UINT16_MAX)
        panicf("YP3 PCM: lock overflow");
#ifdef YP3_DEV
    if (!locks) {
        lock_t = sl6801_us();
        lock_caller = (uintptr_t)__builtin_return_address(0);
    }
#endif
    ++locks;
    restore_irq(old);
}

static void sink_unlock(void)
{
    int old = disable_irq_save();
    if (!locks)
        panicf("YP3 PCM: unlock underflow");
    if (--locks == 0) {
        prepare();
        report_error();
    }
    restore_irq(old);
}

static void sink_play(const void *addr, size_t size)
{
    if (!dma_ready || yp3_audiohw_status())
        panicf("YP3 PCM: audio not open (%x)", yp3_audiohw_status());
    if (!valid_buffer(addr, size))
        panicf("YP3 PCM: bad buffer %p/%u", addr, (unsigned)size);

    int old = disable_irq_save();
    bool ok = false;
    if (state == IDLE) {
        source = addr;
        remaining = size;
        fill(0);
        ok = start(0);
        prepare();
    }
    if (ok)
        yp3_audiohw_pcm_active(true);
    restore_irq(old);
    if (!ok)
        panicf("YP3 PCM: submit refused");
}

static void sink_stop(void)
{
    if (dma_ready)
        stop_dma();
}

struct pcm_sink builtin_pcm_sink = {
    .caps = {
        .samprs = frequencies,
        .num_samprs = 1,
        .default_freq = 0,
#ifdef WANT_SWVOL
        .volume_type = PCM_SINK_SWVOL,
#else
        .volume_type = PCM_SINK_HWVOL,
#endif
    },
    .ops = {
        .init = sink_init,
        .postinit = sink_postinit,
        .set_freq = sink_set_freq,
        .lock = sink_lock,
        .unlock = sink_unlock,
        .play = sink_play,
        .stop = sink_stop,
    },
};
