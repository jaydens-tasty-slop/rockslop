/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_NOTIFICATION_H
#define FLIPPER_NOTIFICATION_H
#include <furi.h>

/* Only notes, sound_off and delays do anything (flipper_notify.c plays them
 * as square-wave tones); the LED, vibration and backlight messages have no
 * hardware behind them here. */
typedef struct NotificationApp NotificationApp;
enum { NM_OTHER, NM_NOTE, NM_DELAY, NM_SOUND_OFF };
typedef struct NotificationMessage {
    unsigned short kind;
    unsigned short value;       /* NM_NOTE: Hz, NM_DELAY: ms */
} NotificationMessage;
typedef const NotificationMessage* NotificationSequence[];

#ifdef __cplusplus
extern "C" {
#endif
void notification_message(NotificationApp* app, const NotificationSequence* sequence);
void notification_message_block(NotificationApp* app, const NotificationSequence* sequence);
#ifdef __cplusplus
}
#endif
#endif
