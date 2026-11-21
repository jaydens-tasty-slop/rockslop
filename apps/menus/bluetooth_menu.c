/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Settings > Bluetooth: power, status, the connected device and its
 * battery, scan and connect, paired devices, disconnect, auto-connect. */
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "action.h"
#include "bluetooth.h"
#include "bluetooth_policy.h"
#include "exported_menus.h"
#include "lang.h"
#include "list.h"
#include "menu.h"
#include "misc.h"
#include "settings.h"
#include "splash.h"
#include "string-extra.h"

#define MAX_DEVICES 8            /* entries in the scan and paired lists */

/* connect_to(): give up after CONNECT_TIMEOUT (CODE_TIMEOUT once a keyboard
 * shows a code to type), or once the stack has dropped the attempt (no
 * connected device) after CONNECT_GRACE; poll each CONNECT_POLL */
#define CONNECT_TIMEOUT (15 * HZ)
#define CODE_TIMEOUT    (30 * HZ)   /* the stack's own limit for pairing */
#define CONNECT_GRACE   HZ
#define CONNECT_POLL    (HZ / 5)
#define SCAN_REFRESH    (HZ / 2)    /* scan list redraw while results arrive */
#define DISCONNECT_WAIT (2 * HZ)
#define STACK_POLL      (HZ / 10)   /* time for the stack to act on a call */

struct device_list {
    struct bt_device dev[MAX_DEVICES];
    int count;
    bool scanning;
};

/* List entry: the device's name, else its address */
static const char *device_name(int i, void *data, char *buf, size_t len)
{
    struct device_list *l = data;
    if (i >= l->count)
        return "";
    if (l->dev[i].name[0])
        return l->dev[i].name;
    bt_addr_format(&l->dev[i].addr, buf);   /* len >= BT_ADDR_STR in lists */
    (void)len;
    return buf;
}

/* Wait until the headset's audio stream or the input device is up (paging
 * and pairing take a few seconds) or the attempt failed, showing the code
 * that a keyboard wants typed; a key stops waiting, not the attempt */
static void connect_to(const bt_addr_t *addr)
{
    struct bt_device dev;
    char code[BT_CODE_MAX], shown[BT_CODE_MAX] = "";
    int timeout = CONNECT_TIMEOUT;
    if (!bt_connect(addr))
        return;
    splash(0, ID2P(LANG_BLUETOOTH_CONNECTING));
    for (int t = 0; t < timeout; t += CONNECT_POLL) {
        int action = get_action(CONTEXT_STD, CONNECT_POLL);
        /* SYS_BT_AUDIO_CONNECTED among them: routed now, not at the
         * stack's next repeat after this screen */
        default_event_handler(action);
        if (action == ACTION_STD_CANCEL || bt_status() >= BT_STATUS_CONNECTED)
            return;
        if (t >= CONNECT_GRACE && !bt_connected_device(&dev))
            break;
        if (bt_pairing_code(code) && strcmp(code, shown)) {
            strcpy(shown, code);
            splashf(0, str(LANG_BLUETOOTH_TYPE_CODE), code);
            timeout = t + CODE_TIMEOUT;
        }
    }
    splash(HZ * 2, ID2P(LANG_BLUETOOTH_CONNECT_FAILED));
}

/* Menu item callbacks: hide what needs power or a connection */
static int needs_power(int action, const struct menu_item_ex *item,
                       struct gui_synclist *list)
{
    (void)item;
    (void)list;
    if (action == ACTION_REQUEST_MENUITEM && bt_status() == BT_STATUS_OFF)
        return ACTION_EXIT_MENUITEM;
    return action;
}

static int needs_device(int action, const struct menu_item_ex *item,
                        struct gui_synclist *list)
{
    struct bt_device dev;
    (void)item;
    (void)list;
    if (action == ACTION_REQUEST_MENUITEM && !bt_connected_device(&dev))
        return ACTION_EXIT_MENUITEM;
    return action;
}

/* Power */
static int power_toggle(void)
{
    bool on = bt_status() == BT_STATUS_OFF;
    if (on)
        splash(0, ID2P(LANG_BLUETOOTH_TURNING_ON));
    if (!bluetooth_set_power(on))
        splash(HZ * 2, ID2P(LANG_BLUETOOTH_UNAVAILABLE));
    settings_save();
    return 0;
}

static char *power_text(int sel, void *data, char *buf, size_t len)
{
    (void)sel;
    (void)data;
    snprintf(buf, len, "%s: %s", str(LANG_BLUETOOTH),
             str(bt_status() == BT_STATUS_OFF ? LANG_OFF : LANG_ON));
    return buf;
}

/* Status line, then the device on a line of its own (a name rarely fits
 * beside the status) */
static int status_nop(void)
{
    return 0;
}

static char *status_text(int sel, void *data, char *buf, size_t len)
{
    struct bt_device dev;
    (void)sel;
    (void)data;
    (void)buf;
    (void)len;
    if (!bt_connected_device(&dev))
        return (char *)str(LANG_BLUETOOTH_NOT_CONNECTED);
    return (char *)str(bt_status() == BT_STATUS_STREAMING ?
                       LANG_BLUETOOTH_STREAMING :
                       bt_status() == BT_STATUS_CONNECTED ?
                       LANG_BLUETOOTH_CONNECTED : LANG_BLUETOOTH_CONNECTING);
}

static char *device_text(int sel, void *data, char *buf, size_t len)
{
    struct bt_device dev;
    (void)sel;
    (void)data;
    if (!bt_connected_device(&dev))
        return "";
    if (!dev.name[0])
        bt_addr_format(&dev.addr, dev.name);
    strlcpy(buf, dev.name, len);
    return buf;
}

/* The headset's battery, when it reports one */
static int needs_battery(int action, const struct menu_item_ex *item,
                         struct gui_synclist *list)
{
    (void)item;
    (void)list;
    if (action == ACTION_REQUEST_MENUITEM && bt_battery() < 0)
        return ACTION_EXIT_MENUITEM;
    return action;
}

static char *battery_text(int sel, void *data, char *buf, size_t len)
{
    (void)sel;
    (void)data;
    int percent = bt_battery();
    snprintf(buf, len, "%s: %d%%", str(LANG_BLUETOOTH_BATTERY),
             percent < 0 ? 0 : percent);
    return buf;
}

/* Scan and connect: results appear as they come in; the title says
 * whether the scan is still running. OK on an entry not (yet) listed does
 * nothing. */
static int scan_action(int action, struct gui_synclist *list)
{
    struct device_list *l = list->data;
    if (action == ACTION_NONE || action == ACTION_REDRAW) {
        l->scanning = bt_scanning();
        l->count = bt_scan_results(l->dev, MAX_DEVICES);
        gui_synclist_set_nb_items(list, l->count);
        gui_synclist_set_title(list, str(l->scanning ? LANG_BLUETOOTH_SCANNING :
                               l->count ? LANG_BLUETOOTH_SCAN :
                               LANG_BLUETOOTH_NO_DEVICES), Icon_NOICON);
        return ACTION_REDRAW;
    }
    if (action == ACTION_STD_OK && list->selected_item >= l->count)
        return ACTION_NONE;
    return action;
}

static int scan_and_connect(void)
{
    struct device_list l = { .count = 0 };
    struct simplelist_info info;

    if (!bt_scan(true))
        return 0;
    simplelist_info_init(&info, str(LANG_BLUETOOTH_SCANNING), 0, &l);
    info.get_name = device_name;
    info.action_callback = scan_action;
    info.timeout = SCAN_REFRESH;
    simplelist_show_list(&info);
    if (bt_scanning())
        bt_scan(false);
    if (info.selection >= 0 && info.selection < l.count)
        connect_to(&l.dev[info.selection].addr);
    return 0;
}

/* Paired devices: connect or forget */
enum { PAIRED_CONNECT, PAIRED_FORGET };     /* paired_actions entries */
MENUITEM_STRINGLIST(paired_actions, ID2P(LANG_BLUETOOTH_PAIRED), NULL,
                    ID2P(LANG_BLUETOOTH_CONNECT), ID2P(LANG_BLUETOOTH_FORGET));

/* Pick a paired device, then connect to it or forget it (and pick again) */
static int paired_devices(void)
{
    struct device_list l;
    struct simplelist_info info;

    for (;;) {
        l.count = bt_paired_devices(l.dev, MAX_DEVICES);
        if (!l.count) {
            splash(HZ, ID2P(LANG_BLUETOOTH_NO_DEVICES));
            return 0;
        }
        simplelist_info_init(&info, str(LANG_BLUETOOTH_PAIRED), l.count, &l);
        info.get_name = device_name;
        simplelist_show_list(&info);
        if (info.selection < 0)
            return 0;
        switch (do_menu(&paired_actions, NULL, NULL, false)) {
        case PAIRED_CONNECT:
            connect_to(&l.dev[info.selection].addr);
            return 0;
        case PAIRED_FORGET:
            bt_forget(&l.dev[info.selection].addr);
            sleep(STACK_POLL);  /* the stack updates its list */
            break;
        default:
            return 0;
        }
    }
}

/* Disconnect, then wait up to DISCONNECT_WAIT for the device to go (the
 * menu hides the entries that need one) */
static int disconnect(void)
{
    struct bt_device dev;
    bt_disconnect();
    for (int t = 0; t < DISCONNECT_WAIT && bt_connected_device(&dev);
         t += STACK_POLL)
        sleep(STACK_POLL);
    return 0;
}

MENUITEM_FUNCTION_DYNTEXT(bt_power_item, 0, power_toggle, power_text,
                          NULL, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(bt_status_item, 0, status_nop, status_text,
                          NULL, NULL, needs_power, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(bt_device_item, 0, status_nop, device_text,
                          NULL, NULL, needs_device, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(bt_battery_item, 0, status_nop, battery_text,
                          NULL, NULL, needs_battery, Icon_NOICON);
MENUITEM_FUNCTION(bt_scan_item, 0, ID2P(LANG_BLUETOOTH_SCAN),
                  scan_and_connect, needs_power, Icon_NOICON);
MENUITEM_FUNCTION(bt_paired_item, 0, ID2P(LANG_BLUETOOTH_PAIRED),
                  paired_devices, needs_power, Icon_NOICON);
MENUITEM_FUNCTION(bt_disconnect_item, 0, ID2P(LANG_BLUETOOTH_DISCONNECT),
                  disconnect, needs_device, Icon_NOICON);
MENUITEM_SETTING(bt_autoconnect_item, &global_settings.bt_autoconnect, NULL);

MAKE_MENU(bluetooth_menu, ID2P(LANG_BLUETOOTH), NULL, Icon_Menu_setting,
          &bt_power_item, &bt_status_item, &bt_device_item, &bt_battery_item,
          &bt_scan_item,
          &bt_paired_item, &bt_disconnect_item, &bt_autoconnect_item);
