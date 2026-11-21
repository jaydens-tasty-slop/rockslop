/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "mmio-sl6801.h"
#include "dev-yp3.h"
#include "recovery-yp3.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

unsigned yp3_dev_return_seconds;
unsigned long yp3_idle_us;

/* Retained log layout, read by yp3dev/yp3.py log. */
#define LOG_MAGIC 0x474f4c59u /* bytes "YLOG" */
#define TRACE_MAGIC 0x43525459u /* bytes "YTRC" */
#define TRACE_PCS 30
#define LOG_HEADER_BYTES 16     /* magic, head, boots, reserved */
#define TRACE_HEADER_BYTES 8    /* trace_magic, trace_pos */
#define SNAP_MAGIC 0x52485459u  /* bytes "YTHR" */
#define LOG_DATA (YP3_DEV_LOG_BYTES - LOG_HEADER_BYTES - TRACE_HEADER_BYTES - \
                  4 * TRACE_PCS - YP3_DEV_SNAP_BYTES)
/* A head above this is garbage (not a count we wrote): reset the ring. */
#define LOG_HEAD_MAX 0x7fffffffu

#define TRACE_PC_MASK    0x00ffffffu    /* SRAM and XIP fit in 24 bits */
#define TRACE_SLOT_SHIFT 24
#define TRACE_SLOT_MASK  0xffu

#define CONFIG_BYTES 4096       /* dev config area size (dev-yp3.h) */

/* Survives watchdog resets (not PMU resets); see app.lds .yp3_retained. */
static struct {
    uint32_t magic;
    uint32_t head;      /* total bytes ever written; data[head % LOG_DATA] */
    uint32_t boots;
    uint32_t reserved;
    /* PC interrupted by each of the last TRACE_PCS ticks: after a hang and
     * watchdog return, `yp3.py log` shows where the CPU was. */
    uint32_t trace_magic;
    uint32_t trace_pos;
    uint32_t trace[TRACE_PCS];
    /* scheduler snapshot: SNAP_MAGIC, its size, then kernel-sl6801.c's */
    uint32_t snap[YP3_DEV_SNAP_BYTES / 4];
    char data[LOG_DATA];
} retained __attribute__((section(".noinit.dev_log"), used, aligned(8)));

void *yp3_dev_snap_area(void)
{
    return retained.snap;
}

/* Copy of the trace ring, newest first (n <= TRACE_PCS). */
unsigned dev_trace_copy(uint32_t *out, unsigned n)
{
    uint32_t pos = retained.trace_pos;
    if (n > TRACE_PCS)
        n = TRACE_PCS;
    for (unsigned i = 0; i < n; i++)
        out[i] = retained.trace[(pos - 1 - i) % TRACE_PCS];
    return n;
}

/* PC in the low 24 bits (SRAM/XIP), interrupted thread's slot + 1 above
 * (`mem` lists slots) */
void yp3_dev_trace_tick(uint32_t pc)
{
    pc = (pc & TRACE_PC_MASK) |
         ((thread_self() & TRACE_SLOT_MASK) + 1u) << TRACE_SLOT_SHIFT;
    retained.trace[retained.trace_pos++ % TRACE_PCS] = pc;
    retained.trace_magic = TRACE_MAGIC;
}

bool dev_config(const char *key, char *buf, size_t size)
{
    return devlog_config_find(YP3_DEV_XIP(YP3_DEV_CONFIG_OFFSET), CONFIG_BYTES,
                              key, buf, size);
}

void dev_feed(void)
{
#ifdef YP3_RECOVERY_WATCHDOG
    yp3_recovery_watchdog_refresh();
#endif
}

/* Stop the watchdog feeds: ROM USB in ~16 s, the log ring kept */
void dev_return(void)
{
#ifdef USB_ENABLE_SERIAL
    char usb[120];      /* TODO(cleanup): lost shell replies */
    yp3_usb_dev_in_state(usb, sizeof usb);
    dev_log("return: %s", usb);
#endif
    yp3_dev_return_seconds = 1;
}

/* Parked: the recovery watchdog resets into ROM USB */
void dev_return_now(void)
{
    yp3_recovery_park();
}

bool dev_code_address(uint32_t addr)
{
    return addr >= YP3_DEV_CODE_BEGIN && addr < YP3_DEV_CODE_END;
}

uint32_t dev_usec(void)
{
    return sl6801_us();
}

void yp3_dev_init(void)
{
    if (retained.magic != LOG_MAGIC || retained.head > LOG_HEAD_MAX) {
        retained.magic = LOG_MAGIC;
        retained.head = 0;
        retained.boots = 0;
    }
    devlog_init(&retained.head, retained.data, LOG_DATA);
    retained.boots++;
    memset(retained.snap, 0, sizeof retained.snap);
    retained.snap[0] = SNAP_MAGIC;       /* marks the layout for yp3.py */
    retained.snap[1] = sizeof retained.snap;
    /* the counter runs from reset: time spent in ROM and the bootloader */
    dev_log("--- boot %lu --- %lu ms after reset",
            (unsigned long)retained.boots,
            (unsigned long)*(volatile uint32_t *)SL6801_COUNTER_LO / 1000);
    char v[12];
#ifdef YP3_DEV_RETURN_SECONDS
    yp3_dev_return_seconds = YP3_DEV_RETURN_SECONDS;
#endif
    if (dev_config("return", v, sizeof v))
        yp3_dev_return_seconds = atoi(v);
}

#ifdef YP3_DEV
/* Data watchpoint (DWT comparator 0, DebugMonitor): finds who writes a word,
 * e.g. a heap block header that something overruns. Hits are recorded here
 * (no logging in exception context) and printed by the dev shell. */
#define DEMCR_TRCENA    (1u << 24)
#define DEMCR_MON_EN    (1u << 16)
#define DFSR_DWTTRAP    (1u << 2)
#define DWT_FN_WRITE    6u          /* watchpoint on data write */
#define WATCH_HITS      16

struct yp3_watch_hit yp3_watch_hits[WATCH_HITS];
volatile unsigned yp3_watch_count;
static volatile uint32_t *watch_addr;
static uint32_t watch_lo, watch_hi;     /* values in [lo, hi] are no hit */

void yp3_dev_debugmon_c(uint32_t *frame)
{
    REG(SCB_DFSR) = DFSR_DWTTRAP;
    if (watch_hi && *watch_addr >= watch_lo && *watch_addr <= watch_hi)
        return;
    unsigned n = yp3_watch_count;
    if (n < WATCH_HITS) {
        yp3_watch_hits[n].pc = frame[6];
        yp3_watch_hits[n].lr = frame[5];
        yp3_watch_hits[n].value = *watch_addr;
        yp3_watch_hits[n].thread = thread_self();
    }
    if (n < WATCH_HITS) {               /* into the retained ring too */
        dev_log_raw("watch hit %u: pc %08lx lr %08lx value %08lx thread %x "
                    "sp %08lx\n",
                    n, (unsigned long)frame[6], (unsigned long)frame[5],
                    (unsigned long)*watch_addr, thread_self(),
                    (unsigned long)(uintptr_t)frame);
        /* return addresses into XIP code above the frame: the call chain */
        if (n < 2) {
            dev_log_raw("  stack:");
            for (int i = 8, k = 0; i < 256 && k < 12; i++)
                if ((frame[i] & 1) && dev_code_address(frame[i] & ~1u)) {
                    dev_log_raw(" %08lx", (unsigned long)frame[i]);
                    k++;
                }
            dev_log_raw("\n");
        }
    }
    if (++yp3_watch_count >= WATCH_HITS)
        REG(DWT_FUNCTION0) = 0;
}

__attribute__((naked)) static void debugmon_entry(void)
{
    __asm__ volatile("tst lr, #4\n"
                     "ite eq\n"
                     "mrseq r0, msp\n"
                     "mrsne r0, psp\n"
                     "b yp3_dev_debugmon_c\n");
}

void yp3_dev_watch(uintptr_t addr)
{
    yp3_dev_watch_range(addr, 0, 0);
}

/* A watch that ignores writes of values in [LO, HI] (e.g. a saved stack
 * pointer, written at each switch, that must stay on its stack) */
void yp3_dev_watch_range(uintptr_t addr, uint32_t lo, uint32_t hi)
{
    extern uint32_t __vectors_arm[];
    REG(DWT_FUNCTION0) = 0;
    yp3_watch_count = 0;
    watch_lo = lo;
    watch_hi = hi;
    if (!addr)
        return;
    watch_addr = (volatile uint32_t *)(addr & ~3u);
    __vectors_arm[12] = (uint32_t)(uintptr_t)debugmon_entry | 1u;
    __asm__ volatile("dsb\n\tisb" ::: "memory");
    REG(SCB_SHPR3) &= ~0xffu;       /* DebugMonitor at the highest priority */
    REG(SCB_DEMCR) |= DEMCR_TRCENA | DEMCR_MON_EN;
    REG(DWT_COMP0) = addr & ~3u;
    REG(DWT_MASK0) = 2;             /* the whole word */
    REG(DWT_FUNCTION0) = DWT_FN_WRITE;
}
#endif
