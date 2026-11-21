/* SPDX-License-Identifier: GPL-2.0-or-later */
/* HID host (Bluetooth HID Profile 1.1): one game controller or keyboard
 * as buttons. Its control (PSM 0x11) and interrupt (PSM 0x13) channels
 * are opened by us after we paged it, or by the device when it connects
 * by itself.
 *
 * Keyboards are switched to the boot protocol, whose report layout every
 * keyboard has; one that refuses usually sends that layout anyway.
 * Controllers are read as a DualShock 4: its basic report 0x01, or the
 * extended 0x11 that it sends once told how often to report (it would
 * report about 1000 times a second) and which colour its light bar shows.
 *
 * The d-pad and the left stick hold the target's directions, the other
 * buttons BUTTON_PAD_* (button.h); the button driver adds them to its own
 * (bt_hid_buttons()). The keymaps (apps/keymaps/keymap-pad.c) and plugins
 * give the pad buttons their meaning. */
#include "bt.h"
#include <string.h>
#include "button.h"
#include "crc32.h"

/* HIDP transaction header: type << 4 | parameter */
#define HIDP_TYPE_MASK      0xf0
#define HIDP_PARAM_MASK     0x0f
#define HIDP_HANDSHAKE      0x00    /* parameter: result, 0 success */
#define HIDP_CONTROL        0x10
#define HIDP_SET_PROTOCOL   0x70
#define HIDP_DATA           0xa0
#define CONTROL_UNPLUG      0x05    /* virtual cable unplug */
#define PROTOCOL_BOOT       0x00
#define REPORT_INPUT        0x01    /* DATA parameter: report type */
#define REPORT_OUTPUT       0x02
#define INPUT_HDR           2       /* DATA header, report ID */

#define OPEN_WAIT   (10 * HZ)       /* for both channels after security */

/* Keys, bit numbers in hid.keys, and the buttons they hold */
enum {
    K_UP, K_DOWN, K_LEFT, K_RIGHT, K_A, K_B, K_X, K_Y,
    K_L1, K_R1, K_L2, K_R2, K_MENU, K_VIEW, K_HOME,
    K_COUNT
};
#define KEY(k)  (1u << (k))

static const int key_buttons[K_COUNT] = {
    BUTTON_PAD_UP, BUTTON_PAD_DOWN, BUTTON_PAD_LEFT, BUTTON_PAD_RIGHT,
    BUTTON_PAD_A, BUTTON_PAD_B, BUTTON_PAD_X, BUTTON_PAD_Y,
    BUTTON_PAD_L1, BUTTON_PAD_R1, BUTTON_PAD_L2, BUTTON_PAD_R2,
    BUTTON_PAD_MENU, BUTTON_PAD_VIEW, BUTTON_PAD_HOME,
};

/* Boot keyboard report 0x01: modifiers, reserved, six key usages
 * (USB HID Usage Tables, keyboard page) */
#define KBD_REPORT      0x01
#define KBD_LEN         8
#define KBD_KEYS        2
#define USAGE_ROLLOVER  0x01    /* too many keys down */
static const struct { uint8_t usage, key; } kbd_map[] = {
    { 0x52, K_UP }, { 0x51, K_DOWN }, { 0x50, K_LEFT }, { 0x4f, K_RIGHT },
    { 0x28, K_A },              /* Enter */
    { 0x58, K_A },              /* keypad Enter */
    { 0x2c, K_A },              /* space */
    { 0x29, K_B },              /* Escape */
    { 0x2a, K_B },              /* Backspace */
    { 0x4b, K_L1 },             /* Page Up */
    { 0x4e, K_R1 },             /* Page Down */
    { 0x65, K_MENU },           /* Application (menu key) */
    { 0x2b, K_VIEW },           /* Tab */
    { 0x4a, K_HOME },           /* Home */
};

/* DualShock 4 input reports 0x01 and 0x11 (two more bytes first): the
 * sticks, then three button bytes; the hat in B0's low nibble counts
 * clockwise from up (8: centred) */
#define DS4_BASIC       0x01
#define DS4_EXTENDED    0x11
#define DS4_EXT_SKIP    2
#define DS4_LX          0
#define DS4_LY          1       /* 0 is up */
#define DS4_B0          4
#define DS4_B1          5
#define DS4_B2          6
#define DS4_LEN         7
#define DS4_HAT_MASK    0x0f
#define STICK_CENTRE    128
#define STICK_ON        64      /* this far out holds the direction */
#define STICK_OFF       40      /* back within this lets go */
static const struct { uint8_t byte, mask, key; } pad_map[] = {
    { DS4_B0, 0x20, K_A },      /* cross */
    { DS4_B0, 0x40, K_B },      /* circle */
    { DS4_B0, 0x10, K_X },      /* square */
    { DS4_B0, 0x80, K_Y },      /* triangle */
    { DS4_B1, 0x01, K_L1 },
    { DS4_B1, 0x02, K_R1 },
    { DS4_B1, 0x04, K_L2 },
    { DS4_B1, 0x08, K_R2 },
    { DS4_B1, 0x20, K_MENU },   /* options */
    { DS4_B1, 0x10, K_VIEW },   /* share */
    { DS4_B2, 0x01, K_HOME },   /* PS */
};
/* DualShock 4 output report 0x11, with a CRC-32 over the DATA header and
 * the report before it */
#define DS4_OUTPUT      0x11
#define DS4_OUT_LEN     78
#define DS4_OUT_CONTROL 1
#define DS4_OUT_HID_CRC 0xc0    /* control: HID, CRC, report interval (ms) */
#define DS4_OUT_FLAGS   3
#define DS4_OUT_LIGHT   0x02    /* flags: light bar colour valid */
#define DS4_OUT_BLUE    10      /* red, green, blue */
#define DS4_OUT_CRC     74
#define DS4_INTERVAL    10      /* ms: a button tick */
#define DS4_BLUE        64      /* dim: the controller's battery */
static const uint8_t hat_keys[8] = {
    KEY(K_UP), KEY(K_UP) | KEY(K_RIGHT), KEY(K_RIGHT),
    KEY(K_DOWN) | KEY(K_RIGHT), KEY(K_DOWN), KEY(K_DOWN) | KEY(K_LEFT),
    KEY(K_LEFT), KEY(K_UP) | KEY(K_LEFT),
};

/* The buttons held; the button driver reads it on its tick */
static volatile int held;

int bt_hid_buttons(void)
{
    return held;
}

bool hid_device(void)
{
    return BT_COD_INPUT(bt->link.cod);
}

/* The device holds these keys now: hold their buttons */
static void keys_set(uint16_t keys)
{
    struct hid *h = &bt->hid;
    int b = 0;

    if (keys == h->keys)
        return;
    for (int k = 0; k < K_COUNT; k++)
        if (keys & KEY(k))
            b |= key_buttons[k];
    held = b;
    logf("bt: hid keys %04x", keys);
    h->keys = keys;
}

static void kbd_input(const uint8_t *r)
{
    uint16_t keys = 0;
    for (int i = KBD_KEYS; i < KBD_LEN; i++) {
        if (r[i] == USAGE_ROLLOVER)
            return;             /* says nothing: keep what is held */
        for (size_t j = 0; j < ARRAYLEN(kbd_map); j++)
            if (r[i] == kbd_map[j].usage)
                keys |= KEY(kbd_map[j].key);
    }
    keys_set(keys);
}

/* A stick axis: the key of the end it is pushed to, with hysteresis */
static uint16_t axis(int v, int lo, int hi)
{
    uint16_t was = bt->hid.stick;
    int d = v - STICK_CENTRE;
    if (d < -((was & KEY(lo)) ? STICK_OFF : STICK_ON))
        return KEY(lo);
    if (d > ((was & KEY(hi)) ? STICK_OFF : STICK_ON))
        return KEY(hi);
    return 0;
}

static void pad_input(const uint8_t *r)
{
    struct hid *h = &bt->hid;
    unsigned hat = r[DS4_B0] & DS4_HAT_MASK;
    h->stick = axis(r[DS4_LX], K_LEFT, K_RIGHT) | axis(r[DS4_LY], K_UP, K_DOWN);
    uint16_t keys = h->stick;
    if (hat < ARRAYLEN(hat_keys))
        keys |= hat_keys[hat];
    for (size_t i = 0; i < ARRAYLEN(pad_map); i++)
        if (r[pad_map[i].byte] & pad_map[i].mask)
            keys |= KEY(pad_map[i].key);
    keys_set(keys);
}

/* Interrupt channel: input reports [DATA|input][report ID][report] */
static void input(const uint8_t *p, size_t len)
{
    if (len < INPUT_HDR || p[0] != (HIDP_DATA | REPORT_INPUT))
        return;
    if (BT_COD_KEYBOARD(bt->link.cod)) {
        if (p[1] == KBD_REPORT && len >= INPUT_HDR + KBD_LEN)
            kbd_input(p + INPUT_HDR);
    } else if (p[1] == DS4_BASIC && len >= INPUT_HDR + DS4_LEN) {
        pad_input(p + INPUT_HDR);
    } else if (p[1] == DS4_EXTENDED &&
               len >= INPUT_HDR + DS4_EXT_SKIP + DS4_LEN) {
        pad_input(p + INPUT_HDR + DS4_EXT_SKIP);
    }
}

/* Control channel: answers to our requests, and the device unpairing */
static void control(const uint8_t *p, size_t len)
{
    if (!len)
        return;
    switch (p[0] & HIDP_TYPE_MASK) {
    case HIDP_HANDSHAKE:
        if (p[0] & HIDP_PARAM_MASK)
            logf("bt: hid request refused: %u", p[0] & HIDP_PARAM_MASK);
        break;
    case HIDP_CONTROL:
        if ((p[0] & HIDP_PARAM_MASK) == CONTROL_UNPLUG) {
            logf("bt: hid device unpaired itself");
            bt_key_forget(&bt->link.addr);
            hci_disconnect();
        }
        break;
    }
}

/* A controller: report each tick, and a blue light bar */
static void ds4_setup(void)
{
    uint8_t *p = bt->tx + BT_HDR, *r = p + 1;
    memset(p, 0, 1 + DS4_OUT_LEN);
    p[0] = HIDP_DATA | REPORT_OUTPUT;
    r[0] = DS4_OUTPUT;
    r[DS4_OUT_CONTROL] = DS4_OUT_HID_CRC | DS4_INTERVAL;
    r[DS4_OUT_FLAGS] = DS4_OUT_LIGHT;
    r[DS4_OUT_BLUE] = DS4_BLUE;
    put_le32(r + DS4_OUT_CRC, ~crc_32r(p, 1 + DS4_OUT_CRC, 0xffffffff));
    l2cap_send(bt->hid.intr, 1 + DS4_OUT_LEN);
}

/* Both channels are open: a keyboard goes to the boot protocol */
static void hid_up(void)
{
    struct hid *h = &bt->hid;
    h->up = true;
    bt_timer(T_HID, 0);
    logf("bt: hid %s connected",
         BT_COD_KEYBOARD(bt->link.cod) ? "keyboard" : "controller");
    if (BT_COD_KEYBOARD(bt->link.cod)) {
        bt->tx[BT_HDR] = HIDP_SET_PROTOCOL | PROTOCOL_BOOT;
        l2cap_send(h->ctrl, 1);
    } else {
        ds4_setup();
    }
}

/* Input only from a device we paged, or one paired before this link and
 * not paired afresh on it: pairing takes anyone with Just Works, and keys
 * can delete files */
static bool trusted(void)
{
    return bt->link.flags & (LF_OUTGOING | LF_BONDED);
}

bool hid_accept(uint16_t psm)
{
    return trusted() && (psm == PSM_HID_CTRL ? !bt->hid.ctrl : !bt->hid.intr);
}

void hid_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len)
{
    struct hid *h = &bt->hid;
    switch (ev) {
    case CHAN_OPEN:
        if (!trusted()) {           /* paired afresh while it waited */
            l2cap_close(c);
            break;
        }
        if (c->psm == PSM_HID_CTRL) {
            bool ours = h->ctrl == c;
            h->ctrl = c;
            bt_timer(T_AVDTP, 0);   /* not an audio device after all */
            if (ours && !h->intr)
                h->intr = l2cap_open(PSM_HID_INTR);
        } else {
            h->intr = c;
        }
        if (h->ctrl && h->intr && h->ctrl->state == CH_OPEN &&
            h->intr->state == CH_OPEN)
            hid_up();
        break;
    case CHAN_DATA:
        if (c == h->intr)
            input(p, len);
        else if (c == h->ctrl)
            control(p, len);
        break;
    case CHAN_CLOSED:
        if (c == h->ctrl)
            h->ctrl = NULL;
        else if (c == h->intr)
            h->intr = NULL;
        else
            break;
        h->up = false;
        h->stick = 0;
        keys_set(0);
        break;
    }
}

/* The link to an input device is secure: open its channels if we paged
 * it, else it opens them */
void hid_link_secured(void)
{
    bt_timer(T_HID, OPEN_WAIT);
    if ((bt->link.flags & LF_OUTGOING) && !bt->hid.ctrl)
        bt->hid.ctrl = l2cap_open(PSM_HID_CTRL);
}

/* T_HID: the channels did not come up */
void hid_timer(void)
{
    if (bt->link.state == LINK_UP && !bt->hid.up) {
        logf("bt: hid channels not up: disconnecting");
        hci_disconnect();
    }
}
