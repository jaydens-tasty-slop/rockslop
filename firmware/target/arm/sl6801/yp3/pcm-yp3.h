/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PCM_YP3_H
#define PCM_YP3_H

#include <stdbool.h>
#include <stdint.h>

/* Periods the DMA engine has completed since boot (diagnostics). */
uint32_t yp3_pcm_completed_periods(void);
/* Stop the playback DMA channel immediately and discard pending state. */
void yp3_pcm_halt(void);
/* 0 when the codec is open; otherwise (stage << 16) | result, or
 * 0xffff0000 when never opened or closed (audiohw-yp3.c). */
unsigned yp3_audiohw_status(void);
/* The DMA started or stopped: the speaker amplifier follows
 * (audiohw-yp3.c). */
void yp3_audiohw_pcm_active(bool active);
/* The codec's audio PLL runs while any user needs it (audiohw-yp3.c);
 * thread context. The output is a user from boot. */
enum { YP3_CODEC_OUTPUT = 1, YP3_CODEC_CAPTURE = 2 };
void yp3_audiohw_use(unsigned user, bool use);

#ifdef YP3_DEV
/* Late period starts ([0] from the IRQ, [1] after a stall). */
struct yp3_pcm_late {
    unsigned long max_us[2], over100[2], over300[2];
    uintptr_t lock_caller;      /* lock holder at the latest [1] maximum */
    unsigned long lock_us;      /* how long it held the lock */
};
extern struct yp3_pcm_late yp3_pcm_late;
long yp3_pcm_late_now(void);
#endif

#endif
