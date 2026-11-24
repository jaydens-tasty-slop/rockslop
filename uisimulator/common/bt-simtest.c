/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Simulator-only Bluetooth test driver for yp3dev/bt_peer.py runs, set by
 * $ROCKBOX_BT_TEST (space separated words; unset: nothing happens):
 *   play=FILE      start playing FILE (simdisk path) 2 s after boot
 *   on=N           power Bluetooth on N s after boot (default 3)
 *   scan           then run an inquiry and list what it found
 *   connect=ADDR   then connect to ADDR (else wait for the sink to connect)
 *   off=N          power off N s after power-on
 *   repeat=N       power cycles (with off=), default 1
 *   squeeze=N      hold N bytes of buflib from boot (to mimic the target's
 *                  tighter heap, e.g. a YP3_DEV build)
 *   vol=S:V        set the Rockbox volume to V (setting units) S s after
 *                  power-on (up to VOL_MAX of them, first round only)
 *   shots          a screendump (simdisk/dump*.bmp) each time the buttons
 *                  held go back to none: the screen after each press
 * Power and audio routing go through apps/bluetooth_policy.c. Progress lines,
 * with free buflib (available / largest block), start with "bt-test:"; they
 * go to stderr, unbuffered and in order with the stack's own lines. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "audio.h"
#include "bluetooth.h"
#include "bluetooth_policy.h"
#include "button.h"
#include "core_alloc.h"
#include "kernel.h"
#include "misc.h"
#include "pcm.h"
#include "pcm_sink.h"
#include "playlist.h"
#include "settings.h"
#include "sim_tasks.h"

#define POLL        (HZ / 10)
#define SCAN_POLLS  150             /* wait for a scan: 15 s of POLL */
#define SCAN_MAX    8               /* scan results listed */
#define PAIRED_MAX  4               /* paired devices listed */
#define PLAY_DELAY  (2 * HZ)        /* play= starts this long after boot */
#define OFF_PAUSE   (2 * HZ)        /* between power off and the next round */
/* watch() logs audio status and sink as one value: sink << SINK_SHIFT */
#define SINK_SHIFT  4
#define AUDIO_MASK  ((1 << SINK_SHIFT) - 1)
#define VOL_MAX     4               /* vol= words */

static char play_path[MAX_PATH];
static bt_addr_t connect_addr;
static bool do_connect, do_scan, shots;
static int on_secs = 3, off_secs, repeat = 1, squeeze;
static struct { int secs, volume; } vols[VOL_MAX];
static int nvols;
static long test_stack[DEFAULT_STACK_SIZE / sizeof(long)];

/* Progress line with the free buflib space */
static void say(const char *what)
{
    fprintf(stderr, "bt-test: %s; buflib free %lu, largest %lu\n", what,
            (unsigned long)core_available(), (unsigned long)core_allocatable());
}

/* Play path as a one-track playlist in its directory */
static void play(const char *path)
{
    char dir[MAX_PATH];
    const char *slash = strrchr(path, '/');
    size_t n = slash && slash != path ? (size_t)(slash - path) : 1;
    memcpy(dir, path, n);
    dir[n] = '\0';
    if (playlist_create(dir, NULL) >= 0 &&
        playlist_insert_track(NULL, path, PLAYLIST_INSERT_LAST, false, true) >= 0)
        playlist_start(0, 0, 0);
    else
        fprintf(stderr, "bt-test: cannot play %s\n", path);
}

/* Inquiry, then list what it found */
static void scan(void)
{
    struct bt_device found[SCAN_MAX];
    if (!bt_scan(true))
        return;
    for (int i = 0; i < SCAN_POLLS && bt_scanning(); i++)
        sleep(POLL);
    int n = bt_scan_results(found, SCAN_MAX);
    fprintf(stderr, "bt-test: scan done, %d found\n", n);
    for (int i = 0; i < n; i++) {
        char addr[BT_ADDR_STR];
        bt_addr_format(&found[i].addr, addr);
        fprintf(stderr, "bt-test: found %s \"%s\"\n", addr, found[i].name);
    }
}

/* Log each change of Bluetooth status, audio status, sink, volume,
 * headset battery, buttons held or pairing code until the tick until (0:
 * forever); apply the vol= words due by then (counted from the tick on) */
static void watch(long until, long on)
{
    int last = -1, last_audio = -1, last_vol = INT_MIN, next_vol = 0;
    int last_battery = -1, last_buttons = 0;
    char code[BT_CODE_MAX], last_code[BT_CODE_MAX] = "";
    while (!until || TIME_BEFORE(current_tick, until)) {
        sleep(POLL);
        if (button_status() != last_buttons) {
            last_buttons = button_status();
            fprintf(stderr, "bt-test: buttons %04x\n", last_buttons);
            if (shots && !last_buttons)
                sim_trigger_screendump();
        }
        if (bt_pairing_code(code) && strcmp(code, last_code)) {
            strcpy(last_code, code);
            fprintf(stderr, "bt-test: pairing code %s\n", code);
        }
        if (on && next_vol < nvols &&
            !TIME_BEFORE(current_tick, on + vols[next_vol].secs * HZ)) {
            fprintf(stderr, "bt-test: volume %d\n", vols[next_vol].volume);
            global_status.volume = vols[next_vol++].volume;
            setvol();
        }
        int st = bt_status();
        int as = audio_status() | pcm_current_sink() << SINK_SHIFT;
        if (global_status.volume != last_vol && last_vol != INT_MIN)
            fprintf(stderr, "bt-test: volume now %d\n", global_status.volume);
        last_vol = global_status.volume;
        if (bt_battery() != last_battery) {
            last_battery = bt_battery();
            fprintf(stderr, "bt-test: headset battery %d\n", last_battery);
        }
        if (st != last || as != last_audio) {
            struct bt_device dev;
            char line[128], addr[BT_ADDR_STR] = "-";
            bool conn = bt_connected_device(&dev);
            if (conn)
                bt_addr_format(&dev.addr, addr);
            snprintf(line, sizeof line, "status %d, audio %d, sink %s, device %s \"%s\"",
                     st, as & AUDIO_MASK, as >> SINK_SHIFT ? "bt" : "builtin",
                     addr, conn ? dev.name : "");
            say(line);
            last = st;
            last_audio = as;
        }
    }
}

/* The $ROCKBOX_BT_TEST sequence: squeeze, play, then repeat power on,
 * scan/connect, watch, list paired devices, power off */
static void test_thread(void)
{
    long boot = current_tick;
    if (squeeze > 0) {
        int h = core_alloc(squeeze);
        if (h > 0)
            core_pin(h);
        say(h > 0 ? "squeezed" : "squeeze failed");
    }
    if (play_path[0]) {
        sleep(PLAY_DELAY);
        play(play_path);
        sleep(HZ);
        say("playing");
    }
    for (int round = 0; round < repeat; round++) {
        long at = boot + on_secs * HZ;
        if (TIME_BEFORE(current_tick, at))
            sleep(at - current_tick);
        say("powering on");
        if (!bluetooth_set_power(true)) {
            say("power on FAILED");
            return;
        }
        say("power on ok");
        if (do_scan)
            scan();
        if (do_connect)
            bt_connect(&connect_addr);
        long on = current_tick;
        watch(off_secs ? on + off_secs * HZ : 0, round ? 0 : on);

        struct bt_device paired[PAIRED_MAX];
        int n = bt_paired_devices(paired, PAIRED_MAX);
        for (int i = 0; i < n; i++) {
            char addr[BT_ADDR_STR];
            bt_addr_format(&paired[i].addr, addr);
            fprintf(stderr, "bt-test: paired %s \"%s\"\n", addr, paired[i].name);
        }
        bluetooth_set_power(false);
        say("power off");
        sleep(OFF_PAUSE);
        boot = current_tick - on_secs * HZ;
    }
}

/* The value of word w if it is "name=value" (name includes the '='),
 * else NULL */
static const char *opt(const char *w, const char *name)
{
    size_t n = strlen(name);
    return strncmp(w, name, n) ? NULL : w + n;
}

/* Parse $ROCKBOX_BT_TEST and start the test thread (nothing if unset) */
void bt_simtest_init(void)
{
    static char cfg[256];
    const char *env = getenv("ROCKBOX_BT_TEST");
    const char *v;
    if (!env)
        return;
    strncpy(cfg, env, sizeof cfg - 1);
    for (char *w = strtok(cfg, " "); w; w = strtok(NULL, " ")) {
        if ((v = opt(w, "connect=")))
            do_connect = bt_addr_parse(v, &connect_addr);
        else if ((v = opt(w, "play=")))
            strncpy(play_path, v, sizeof play_path - 1);
        else if ((v = opt(w, "on=")))
            on_secs = atoi(v);
        else if ((v = opt(w, "off=")))
            off_secs = atoi(v);
        else if ((v = opt(w, "repeat=")))
            repeat = atoi(v);
        else if ((v = opt(w, "squeeze=")))
            squeeze = atoi(v);
        else if ((v = opt(w, "vol=")) && nvols < VOL_MAX &&
                 sscanf(v, "%d:%d", &vols[nvols].secs, &vols[nvols].volume) == 2)
            nvols++;
        else if (!strcmp(w, "scan"))
            do_scan = true;
        else if (!strcmp(w, "shots"))
            shots = true;
    }
    create_thread(test_thread, test_stack, sizeof test_stack, 0, "bt test"
                  IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));
}
