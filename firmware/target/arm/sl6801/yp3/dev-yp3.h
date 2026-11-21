/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef DEV_YP3_H
#define DEV_YP3_H

/* YP3 development builds (YP3_DEV; the shared part is devlog.h): the log
 * ring sits in watchdog-retained SRAM that the host reads from ROM USB after
 * the player returns there (yp3dev/yp3.py log), and the run config in flash.
 * Compiles to nothing in normal builds. */
#include "devlog.h"         /* dev_log() calls compile to nothing there */
#ifdef YP3_DEV
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Flash areas written by yp3dev (offsets; XIP at 0xC00000 + offset). */
#define YP3_DEV_BUNDLE_OFFSET 0x150000  /* sdpush file bundle, to CONFIG */
#define YP3_DEV_CONFIG_OFFSET 0x1F3000  /* "key=value\n" text, 4 KiB */
#define YP3_DEV_XIP(off) ((const void *)(uintptr_t)(0xC00000u + (off)))
/* Code for the stack scans: the first MiB of the XIP window (flash offset 0
 * maps to 0xc00000), which holds the Rockbox image (SL6801_FLASH_BASE up to
 * _flashend, which app.lds keeps below the XIP codecs at 0xce0000) and the
 * XIP codecs' first slots. */
#define YP3_DEV_CODE_BEGIN 0x00c00000u
#define YP3_DEV_CODE_END   0x00d00000u

void yp3_dev_init(void);
/* Tick hook: record the interrupted PC in the retained trace ring. */
void yp3_dev_trace_tick(uint32_t pc);
/* Retained area for the scheduler snapshot (kernel-sl6801.c; yp3.py
 * threads): the ROM reuses the scheduler's own SRAM after a watchdog
 * return. Its first two words (magic, size) belong to dev-yp3.c. */
#define YP3_DEV_SNAP_BYTES 640
void *yp3_dev_snap_area(void);
struct yp3_watch_hit { uint32_t pc, lr, value; unsigned thread; };
extern struct yp3_watch_hit yp3_watch_hits[];
extern volatile unsigned yp3_watch_count;
void yp3_dev_watch(uintptr_t addr);       /* 0 disarms */
void yp3_dev_watch_range(uintptr_t addr, uint32_t lo, uint32_t hi);
void yp3_dev_watch_thread_sp(unsigned int slot); /* saved sp leaves its
                                                    stack */
/* EP1 IN's transfer, CSR and DMA state as text (usb-drv-yp3.c).
 * TODO(cleanup): lost shell replies. */
void yp3_usb_dev_in_state(char *buf, size_t size);
/* USB driver counters (usb-drv-yp3.c). */
extern struct yp3_usb_stats {
    uint32_t irqs, resets, ep0, last, error, isr_us, isr_max_us;
    uint32_t src[4];        /* IRQs with EP1 RX, EP1 TX, EP0, none of these */
} yp3_usb_stats;
/* Seconds after boot to stop watchdog feeds (0 = never). */
extern unsigned yp3_dev_return_seconds;
/* Time spent in core_sleep WFI (system-arm-micro.h). */
extern unsigned long yp3_idle_us;
/* Power-off parks and returns to ROM USB instead (power-yp3.c) */
#else
static inline void yp3_dev_init(void) {}
#endif

#endif
