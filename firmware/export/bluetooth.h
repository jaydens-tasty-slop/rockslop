/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BLUETOOTH_H
#define BLUETOOTH_H
/* Bluetooth host stack (firmware/bluetooth/), only with HAVE_BLUETOOTH:
 * A2DP source plus AVRCP buttons and volume, one audio device at a time;
 * a hands-free link only for the headset's battery level. Or, instead of
 * the audio device, a game controller or keyboard (HID) as buttons. The stack
 * runs in its own thread, which exists only while Bluetooth is powered;
 * most calls below just start an action and return. Call them from
 * threads, not interrupts (bt_status() excepted). */
#include <stdbool.h>
#include <stdint.h>
#include "gcc_extensions.h"

#define BT_NAME_MAX 32          /* device name buffer incl. NUL (UTF-8) */
#define BT_ADDR_STR 18          /* bt_addr_format() buffer incl. NUL */

/* Device address, least significant byte first as in HCI. */
typedef struct { uint8_t b[6]; } bt_addr_t;

struct bt_device {
    bt_addr_t addr;
    uint32_t cod;               /* class of device, 0 if unknown */
    char name[BT_NAME_MAX];     /* "" until known */
};

/* Class of device: major class peripheral (keyboards, game controllers),
 * and of those a keyboard (or a combined keyboard and mouse) */
#define BT_COD_INPUT(cod)       ((((cod) >> 8) & 0x1f) == 0x05)
#define BT_COD_KEYBOARD(cod)    (BT_COD_INPUT(cod) && ((cod) & 0x40))

/* Coarse state for the status bar and skins (%Bt). Cheap, lock-free,
 * callable from any thread. */
enum bt_status {
    BT_STATUS_OFF,          /* powered down */
    BT_STATUS_ON,           /* powered, no device connected */
    BT_STATUS_CONNECTED,    /* an audio or input device is connected */
    BT_STATUS_STREAMING,    /* ...and the A2DP stream is started */
};
enum bt_status bt_status(void);

/* bt_power(true) allocates the stack (about 10 KB of buflib), starts its
 * thread and waits until the controller is up; false if memory, the
 * thread or the controller is not available. bt_power(false) disconnects,
 * powers the controller down and frees everything (blocks up to ~2 s). */
bool bt_power(bool on);

/* Inquiry for about 10 s; the names of the devices found are fetched
 * afterwards. bt_scanning() stays true until the names are in. */
bool bt_scan(bool start);
bool bt_scanning(void);
/* Copies up to max devices found by the last scan; returns the count. */
int bt_scan_results(struct bt_device *devs, int max);

/* Connect to an audio sink or input device: page, pair if needed (Secure
 * Simple Pairing "Just Works", or PIN 0000; a keyboard with a code typed
 * on it), then open A2DP and AVRCP, or HID. Replaces any current
 * connection. Headsets and input devices may also connect by themselves. */
bool bt_connect(const bt_addr_t *addr);
void bt_disconnect(void);
/* The connected (or connecting) device; false if none. */
bool bt_connected_device(struct bt_device *dev);

/* While a keyboard pairs: the code to type on it, then Enter (buf:
 * BT_CODE_MAX bytes); false at other times. */
#define BT_CODE_MAX 8
bool bt_pairing_code(char *buf);

#ifdef HAVE_BLUETOOTH_HID
/* The buttons that the connected game controller or keyboard holds
 * (BUTTON_PAD_*, firmware/bluetooth/hid.c); the button driver adds them to
 * the target's own. Any context. */
int bt_hid_buttons(void);
#endif

/* The stack posts SYS_BT_AUDIO_CONNECTED when a headset's stream is ready
 * and SYS_BT_AUDIO_DISCONNECTED when it is gone. A screen that drops system
 * events would lose one, so it comes again every 2 s until the apps,
 * having routed the audio for it, call bt_audio_acknowledge(event). */
void bt_audio_acknowledge(long event);

/* Paired devices, most recently used first; their link keys are kept in
 * ROCKBOX_DIR "/bluetooth.dat". */
int bt_paired_devices(struct bt_device *devs, int max);
bool bt_forget(const bt_addr_t *addr);

/* AVRCP absolute volume: a headset that has it plays at the Rockbox volume
 * itself. firmware/sound.c hands every volume change to bt_volume() (cb in
 * centibels within [min_cb, max_cb]), which returns true while the headset
 * is the output and takes the volume: the PCM then goes at full scale. The
 * headset's own changes come back as SYS_BT_VOLUME (data: centibels), as
 * does the volume whenever this true/false answer changes. Any thread. */
bool bt_volume(int cb, int min_cb, int max_cb);

/* The connected headset's battery level in percent, -1 until it reports
 * one (over hands-free; headsets without it never do). Any thread. */
int bt_battery(void);

/* As "12:34:56:78:9A:BC" (buf: BT_ADDR_STR bytes); parse returns false
 * on junk. */
void bt_addr_format(const bt_addr_t *addr, char *buf);
bool bt_addr_parse(const char *s, bt_addr_t *addr);

/* The debug screen: what the stack knows of the controller, the link, the
 * stream and the profiles, one printf-style call of line() per line
 * (apps/debug_menu.c passes simplelist_addline). Any thread. */
typedef void (*bt_dbg_line_fn)(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
void bt_dbg_info(bt_dbg_line_fn line);

#endif /* BLUETOOTH_H */
