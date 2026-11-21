/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_FURI_HAL_H
#define FLIPPER_FURI_HAL_H
#include <furi.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Speaker: a single square-ish tone through the Rockbox beep service. */
bool furi_hal_speaker_acquire(uint32_t timeout);
bool furi_hal_speaker_is_mine(void);
void furi_hal_speaker_start(float frequency, float volume);
void furi_hal_speaker_set_volume(float volume);
void furi_hal_speaker_stop(void);
void furi_hal_speaker_release(void);

typedef enum { FuriHalRtcFlagStealthMode = 1 } FuriHalRtcFlag;
bool furi_hal_rtc_is_flag_set(FuriHalRtcFlag flag);

#ifdef __cplusplus
}
#endif
#include <furi_hal_light.h>
#include <furi_hal_vibro.h>
#endif
