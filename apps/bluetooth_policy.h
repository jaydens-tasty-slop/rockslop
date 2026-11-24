/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BLUETOOTH_POLICY_H
#define BLUETOOTH_POLICY_H
/* Bluetooth in the apps: power from the setting, auto-connect, and audio
 * routing between the built-in output and PCM_SINK_BT. The stack itself is
 * firmware/bluetooth/ (firmware/export/bluetooth.h). */
#include <stdbool.h>

/* At boot: power on if the setting says so */
void bluetooth_init(void);

/* settings_apply() (a .cfg loaded, settings reset): power on or off to
 * match global_settings.bt_enabled; nothing before bluetooth_init(). UI
 * thread. */
void bluetooth_apply_setting(void);

/* Power on or off (with the playback buffer resized to make room) and
 * store the result in global_settings.bt_enabled; false if power-on
 * failed. Call from the UI thread. */
bool bluetooth_set_power(bool on);

/* SYS_BT_AUDIO_CONNECTED / _DISCONNECTED (default_event_handler): route,
 * then acknowledge the event to the stack */
void bluetooth_audio_event(long event);

/* SYS_BT_VOLUME: the headset's volume in centibels (absolute volume)
 * becomes the Bluetooth volume */
void bluetooth_volume_event(int cb);

/* SYS_BT_PLAY: the headset's Play (play) or Pause (!play) */
void bluetooth_play_event(bool play);

#endif /* BLUETOOTH_POLICY_H */
