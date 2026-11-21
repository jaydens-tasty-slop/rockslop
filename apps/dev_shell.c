/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Development builds (HAVE_DEVLOG, devlog.h): boot tasks from the run config
 * the host writes, and the dev shell on USB serial.
 *
 * Run config ("key=value" lines; the target keeps them, dev_config()):
 *   ls=/,/.rockbox     log directory listings
 *   play=/a/b.mp3      start playback of one file
 *   keys=D D S 2000 R  a key script (devlog.h, dev_keys())
 *   volume=-30         set the volume (dB) before play
 *   bluetooth=off      Bluetooth off for this boot, whatever the setting says
 *   poweroff=real|dry  a clean Rockbox shutdown after 3 s; dry: power_off()
 *                      returns to the host instead (dev_poweroff_dry)
 *   monitor=N          log playback state every 2 s for N s
 *   crash=udf|panic    exercise the fault / panic paths after boot
 *   usb=1|storage      attach USB after these tasks: charge-only mode with
 *                      the serial dev shell (below), or mass storage
 *   unplug=N           with usb=: detach it again after N s, as if unplugged;
 *                      play= plays then, not at boot
 *   sdlog=1            append the log to the card every 15 s (`sdlog`)
 *   sdtest=KB [BUFKB [odd]]  the shell's sdtest once after boot
 *   sh=CMD;CMD...      shell commands run in order after boot (a run with
 *                      no one at the serial port); "wait S" sleeps S s
 * and the target's own (return=, which every target has, and others; its
 * dev-*.c and dev_target_boot()).
 *
 * The shell: one command a line (`help` lists them); each reply ends with a
 * line "ok" or "err COMMAND". */
#include "config.h"
#include <ctype.h>
#include "dev_shell.h"
#include "devlog.h"
#include "plugin.h"
#include "module_arena.h"
#include "misc.h"
#include "gui/wps.h"
#include "gui/list.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#include "tagtree.h"
#endif
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "button.h"
#include "backlight.h"
#include "adc.h"
#include "powermgmt.h"
#include "dir.h"
#include "fat.h"
#include "ata_idle_notify.h"
#include "file.h"
#include "playlist.h"
#include "pathfuncs.h"
#include "panic.h"
#include "audio.h"
#include "tree.h"
#include "root_menu.h"
#include "menu.h"
#include "lang.h"
#include "settings.h"
#include "sound.h"
#include "usb.h"
#include "core_alloc.h"
#include "buffering.h"
#include "lcd.h"
#include "storage.h"
#include "metadata.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_mixer.h"
#include "pcmbuf.h"
#ifdef HAVE_BLUETOOTH
#include "bluetooth.h"
#endif
#if CONFIG_TUNER
#include "tuner.h"
#include "radio.h"
#endif
#ifdef HAVE_RECORDING
#include "recording.h"
#include "enc_config.h"
#include "pcm_record.h"
#endif
#include "timefuncs.h"
#include "power.h"
#ifdef CPU_ARM_MICRO
#include "regs/cortex-m/cm_scb.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void list_dir(const char *path)
{
    DIR *d = opendir(path);
    if (!d) {
        dev_log("ls %s: error", path);
        return;
    }
    dev_log("ls %s:", path);
    struct dirent *e;
    while ((e = readdir(d))) {
        struct dirinfo info = dir_get_info(d, e);
        dev_log("  %s%s %ld", e->d_name,
                    (info.attribute & ATTR_DIRECTORY) ? "/" : "", (long)info.size);
    }
    closedir(d);
}

/* DB in the volume setting's units */
static int volume_from_db(const char *db)
{
    int v = atoi(db);
    for (int i = sound_numdecimals(SOUND_VOLUME); i > 0; i--)
        v *= 10;
    return v;
}

/* play PATH[;PATH...]: a new playlist of these files, from the first */
static void play(const char *path)
{
    char dir[MAX_PATH], files[MAX_PATH];
    strlcpy(dir, path, sizeof dir);
    dir[strcspn(dir, ";")] = '\0';
    char *slash = strrchr(dir, '/');
    if (slash && slash != dir)
        *slash = '\0';
    else
        strcpy(dir, "/");
    int rc = playlist_create(dir, NULL);
    strlcpy(files, path, sizeof files);
    char *save, *f = strtok_r(files, ";", &save);
    for (; rc >= 0 && f; f = strtok_r(NULL, ";", &save))
        rc = playlist_insert_track(NULL, f, PLAYLIST_INSERT_LAST, false, true);
    dev_log("play %s: create/insert rc=%d", path, rc);
    if (rc >= 0) {
        playlist_start(0, 0, 0);
        dev_log("play: playlist_start returned");
    }
}

/* playlist PATH [shuffle]: a playlist file (.m3u8) from its first track */
static void play_playlist(char *arg)
{
    char *shuffle = strstr(arg, " shuffle");
    if (shuffle)
        *shuffle = '\0';
    char dir[MAX_PATH];
    strlcpy(dir, arg, sizeof dir);
    char *slash = strrchr(dir, '/');
    if (!slash)
        return;
    const char *file = arg + (slash - dir) + 1;
    if (slash == dir)
        slash[1] = '\0';
    else
        *slash = '\0';
    int rc = playlist_create(dir, file);
    if (rc >= 0 && shuffle)
        playlist_shuffle(current_tick, -1);
    dev_log("playlist %s: rc=%d, %d tracks%s", arg, rc, playlist_amount(),
                shuffle ? ", shuffled" : "");
    if (rc >= 0 && playlist_amount() > 0)
        playlist_start(0, 0, 0);
}

/* `sdlog on` (or sdlog=1): the health thread appends the dev log to
 * SDLOG_PATH every SDLOG_PERIOD, for runs away from USB (a Bluetooth
 * walk) that outlast the ring */
#define SDLOG_PATH      DEV_CARD_DIR "/devlog.txt"
#define SDLOG_PERIOD    (15 * HZ)
static bool sdlog_on;
static uint32_t sdlog_pos;      /* 0: from the oldest byte the ring holds */
static long sdlog_next;

static void sdlog_flush(void)
{
    static char buf[512];
    int fd = open(SDLOG_PATH, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0)
        return;
    size_t n;
    while ((n = dev_log_read(&sdlog_pos, buf, sizeof buf)) > 0)
        write(fd, buf, n);
    close(fd);
}

static long monitor_stack[DEFAULT_STACK_SIZE / sizeof(long)];
static int monitor_seconds;

static void monitor_thread(void)
{
    for (int t = 0; t < monitor_seconds; t += 2) {
        struct mp3entry *id3 = audio_current_track();
        dev_log("tick=%ld batt=%dmV buttons=%x audio=%d elapsed=%lums",
                current_tick, battery_voltage(), button_status(), audio_status(),
                id3 ? (unsigned long)id3->elapsed : 0);
        sleep(2 * HZ);
    }
    thread_exit();
}

/* Plugin the shell asked the UI thread to run (`run PATH`). */
static char ui_run_path[MAX_PATH];

/* Hand a request (fmt with one %s) to the UI thread. One at a time: a second
 * request sent before the UI thread took the first used to overwrite it
 * (two `cfg` in a row loaded only the second). */
static void ui_run_post(const char *fmt, const char *arg)
{
    for (int i = 0; ui_run_path[0] && i < 50; i++)
        sleep(HZ / 10);
    if (ui_run_path[0])
        dev_log("ui run: %s replaced (UI busy)", ui_run_path);
    snprintf(ui_run_path, sizeof ui_run_path, fmt, arg);
    button_queue_post(SYS_DEV_RUN, 0);
}

/* Health counters (`health`): what a torture run checks between steps. */
static struct {
    unsigned starved;           /* pcmbuf found no next chunk */
    unsigned starved_seen;      /* health thread: already classified */
    unsigned starved_playing;   /* ... while playing and not near the end */
    unsigned end_starved;       /* ran dry on a track's last chunk */
    unsigned end_seen;          /* health thread: already classified */
    unsigned end_stops;         /* pcm_stops at the last of those */
    unsigned pcm_stops;         /* pcmbuf_play_stop() calls */
    long end_tick;
    unsigned stalls;            /* playing, elapsed stuck >= 3 s */
    unsigned ui_late;           /* UI did not answer a ping within 10 s */
    long last_starve_tick;
    long ui_pong_tick;
    unsigned int ui_thread;
    unsigned long max_stall_ms;
} health;

/* PCM DMA interrupt: count only (no mutexes here); the health thread
 * classifies and logs. */
#define STARVE_TRACE_PCS 30
static uint32_t starve_trace[STARVE_TRACE_PCS];
static unsigned starve_trace_count;
static bool starve_trace_new;

/* TRACK_END: it ran dry on a track's last chunk, the next track not ready:
 * the playlist's end, a next track at another sample rate (playback remakes
 * its buffer, "playback frequency: auto": a gap by design), or a late next
 * track (an underrun). The first two stop the PCM buffer right after; the
 * health thread counts those with no stop. */
void dev_pcm_starved(bool track_end)
{
    if (!starve_trace_new) {
        /* what ran while it drained */
        starve_trace_count = dev_trace_copy(starve_trace, STARVE_TRACE_PCS);
        starve_trace_new = true;
    }
    if (track_end) {
        health.end_stops = health.pcm_stops;
        health.end_tick = current_tick;
        health.end_starved++;
    } else {
        health.starved++;
    }
    health.last_starve_tick = current_tick;
}

void dev_pcm_stopped(void)
{
    health.pcm_stops++;
}

/* The last messages a user would have seen (splashes, prompts, plugin
 * errors) and the decode hashes, numbered, for `msgs`: torture reads them
 * after each step, and the 4 KB log ring loses lines between steps. */
#define DEV_MSGS 8
static struct { unsigned seq; char text[96]; } dev_msgs[DEV_MSGS];
static unsigned dev_msg_seq;

static void msg_put(const char *kind, const char *text)
{
    unsigned seq = ++dev_msg_seq;
    snprintf(dev_msgs[seq % DEV_MSGS].text, sizeof dev_msgs[0].text, "%s %s", kind, text);
    dev_msgs[seq % DEV_MSGS].seq = seq;
    dev_log("%s: %s", kind, text);
}

void dev_msg(const char *kind, const char *fmt, ...)
{
    char text[96];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    msg_put(kind, text);
}

/* a and b equal apart from their digits */
static bool same_but_digits(const char *a, const char *b)
{
    for (;;) {
        while (isdigit(*a))
            a++;
        while (isdigit(*b))
            b++;
        if (*a != *b)
            return false;
        if (!*a)
            return true;
        a++, b++;
    }
}

void dev_splash(const char *fmt, va_list ap)
{
    char text[96], *last = dev_msgs[dev_msg_seq % DEV_MSGS].text;
    vsnprintf(text, sizeof text, fmt, ap);
    /* A progress splash ("Building database... 612 found") updates the
     * newest message in place: the list would hold nothing else */
    if (dev_msg_seq && !strncmp(last, "splash ", 7) &&
        same_but_digits(last + 7, text)) {
        strlcpy(last + 7, text, sizeof dev_msgs[0].text - 7);
        return;
    }
    msg_put("splash", text);
}

void dev_yesno(const char **lines, int count)
{
    char text[96];
    size_t n = 0;
    text[0] = '\0';
    for (int i = 0; i < count && n < sizeof text; i++)
        n += snprintf(text + n, sizeof text - n, "%s%s", i ? " / " : "",
                      P2STR((const unsigned char *)lines[i]));
    msg_put("yesno", text);
}

/* The list drawn last (dev_list_drawn), for `sel` */
static struct {
    unsigned draws;
    int selected, count;
    char title[32], item[48];
} drawn_list;

void dev_list_drawn(struct gui_synclist *list)
{
    char buf[48];
    const char *item = NULL;
    if (list->callback_get_item_name && list->selected_item >= 0 &&
        list->selected_item < list->nb_items)
        item = list->callback_get_item_name(list->selected_item, list->data,
                                            buf, sizeof buf);
    strlcpy(drawn_list.item, item ? P2STR((const unsigned char *)item) : "",
            sizeof drawn_list.item);
    strlcpy(drawn_list.title,
            list->title ? P2STR((const unsigned char *)list->title) : "",
            sizeof drawn_list.title);
    drawn_list.selected = list->selected_item;
    drawn_list.count = list->nb_items;
    drawn_list.draws++;
}

static bool sh_sel(char *arg)
{
    /* sel: the list drawn last: its selected item (from 0) of how many, how
     * many lists were drawn so far, its title and the selected item's text */
    (void)arg;
    dev_log("sel %d/%d draw %u | %s | %s", drawn_list.selected, drawn_list.count,
            drawn_list.draws, drawn_list.title, drawn_list.item);
    return true;
}

static bool sh_msgs(char *arg)
{
    /* msgs [AFTER]: those numbered after AFTER, then the next number */
    unsigned after = strtoul(arg, NULL, 10);
    if (dev_msg_seq > after + DEV_MSGS)
        dev_log("msgs lost %u", dev_msg_seq - after - DEV_MSGS);
    for (unsigned seq = after + 1; seq <= dev_msg_seq; seq++)
        if (dev_msgs[seq % DEV_MSGS].seq == seq)
            dev_log("msg %u %s", seq, dev_msgs[seq % DEV_MSGS].text);
    dev_log("msgs next %u", dev_msg_seq + 1);
    return true;
}

/* entries in DIR whose name starts with PREFIX */
static int count_prefix(const char *dir, const char *prefix)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    int n = 0;
    while (d && (e = readdir(d)))
        n += !strncmp(e->d_name, prefix, strlen(prefix));
    if (d)
        closedir(d);
    return n;
}

static char running_plugin[32];     /* started by `run`, for `health` */
char *plugin_get_current_filename(void);    /* plugin.c */

/* `health`'s plugin: one `run` started, or one opened from the menus */
bool plugin_is_running(void);     /* plugin.c (HAVE_DEVLOG) */

static const char *health_plugin(void)
{
    if (running_plugin[0])
        return running_plugin;
    if (!plugin_is_running())
        return "-";
    const char *path = plugin_get_current_filename(), *name = strrchr(path, '/');
    return name ? name + 1 : path;
}

static bool dev_db_busy(void)
{
#ifdef TAGCACHE_LOWMEM
    return tagcache_build_now_busy();
#else
    return false;
#endif
}

static int dev_usb_mode = -1;       /* usb= config: kept across a `cfg` */

static void ui_run_request(char *path)
{
    if (dev_db_busy()) {    /* in the build's splash: the commit holds the heap */
        dev_log("%s: busy (database update)", path);
        return;
    }
#ifdef HAVE_TAGCACHE
    if (!strcmp(path, "@dbrebuild") || !strcmp(path, "@dbupdate")) {
        bool rebuild = !strcmp(path, "@dbrebuild");
        /* as the Database menu does */
#ifdef TAGCACHE_LOWMEM
        tagcache_build_now(rebuild);
#else
        if (rebuild)
            tagcache_rebuild();     /* on the tagcache thread, in the background */
        else
            tagcache_update();
#endif
        return;
    }
#endif
    if (!strncmp(path, "@cfg:", 5)) {
        /* as loading a theme or .cfg from the file browser */
        bool ok = settings_load_config(path + 5, true);
        if (dev_usb_mode >= 0)  /* settings_apply() set the user's USB mode */
            usb_set_mode(dev_usb_mode);
        dev_log("cfg %s: %s", path + 5, ok ? "loaded" : "failed");
        return;
    }
    if (!strncmp(path, "@play:", 6)) {
        play(path + 6);
        return;
    }
    if (!strncmp(path, "@playlist:", 10)) {
        play_playlist(path + 10);
        return;
    }
    if (!strncmp(path, "@browse:", 8)) {
        /* the file browser opens on this file next time (tree.c) */
        set_current_file(path + 8);
        return;
    }
    if (!strcmp(path, "@wps")) {
        /* as root_menu's wpsscrn, from wherever the UI is. The request also
         * arrives inside Now Playing: not again there, nor under a menu
         * over one opened here (each nested WPS kept ~400 bytes of the 6 KB
         * main stack; torture's play + wps steps overflowed it) */
        static bool dev_wps;
        if (audio_status() && !dev_wps &&
            get_current_activity() != ACTIVITY_WPS) {
            dev_wps = true;
            push_current_activity(ACTIVITY_WPS);
            gui_wps_show();
            pop_current_activity();
            dev_wps = false;
        }
        return;
    }
#if CONFIG_TUNER
    if (!strcmp(path, "@radio")) {
        /* the FM screen, as the main menu's FM Radio entry */
        push_current_activity(ACTIVITY_FM);
        radio_screen();
        pop_current_activity();
        return;
    }
#endif
#ifdef HAVE_RECORDING
    if (!strcmp(path, "@rec")) {
        /* the recording screen, as the main menu's Recording entry */
        push_current_activity(ACTIVITY_RECORDING);
        recording_screen(false);
        pop_current_activity();
        return;
    }
#endif
    /* PLUGIN [PARAMETER]: a viewer is given its file */
    char *param = strchr(path, ' ');
    if (param)
        *param++ = '\0';
    dev_log("run %s%s%s", path, param ? " " : "", param ? param : "");
    strlcpy(running_plugin, strrchr(path, '/') ? strrchr(path, '/') + 1 : path,
            sizeof running_plugin);
    int rc = plugin_load(path, param);
    running_plugin[0] = '\0';
    dev_log("run %s: rc %d", path, rc);
}

bool dev_ui_run(void)
{
    char path[MAX_PATH];
    health.ui_pong_tick = current_tick;     /* doubles as the UI ping reply */
    health.ui_thread = thread_self();
    strlcpy(path, ui_run_path, sizeof path);
    ui_run_path[0] = '\0';
    if (!path[0])
        return false;
    ui_run_request(path);
    return true;
}

static char keys_script[256];
static long keys_stack[DEFAULT_STACK_SIZE / sizeof(long)];

static void keys_thread(void)
{
    sleep(HZ);
    dev_log("keys: %s", keys_script);
    dev_keys(keys_script);
    dev_log("keys: done");
    thread_exit();
}

#ifdef USB_ENABLE_SERIAL
/* The shell: the USB thread collects a line; the shell thread runs it and
 * answers through dev_log (ring + serial), ending each reply with "ok" or
 * "err" (serial only). */
static char sh_rx[128], sh_line[128];
/* 4 KiB scratch for the SD commands, taken from the heap only while one
 * runs (dev RAM comes out of the heap the release build has). */
#define DEV_BUF_BYTES 4096
static int dev_buf_handle;
static uint32_t *dev_buf_get(void)
{
    if (dev_buf_handle <= 0) {
        dev_buf_handle = core_alloc(DEV_BUF_BYTES);
        if (dev_buf_handle <= 0) {
            dev_log("no %d bytes of heap free", DEV_BUF_BYTES);
            return NULL;
        }
        core_pin(dev_buf_handle);       /* commands yield while using it */
    }
    return core_get_data(dev_buf_handle);
}

static void dev_buf_free(void)
{
    if (dev_buf_handle > 0)
        dev_buf_handle = core_free(dev_buf_handle);
}
static size_t sh_rx_len;
static struct semaphore sh_sem;
static bool sh_busy;
static long sh_stack[4 * DEFAULT_STACK_SIZE / sizeof(long)]; /* `meta` runs the metadata
                                                               parsers (~3 KiB) */

void dev_serial_rx(const unsigned char *data, int length)
{
    dev_serial_heard();
    for (int i = 0; i < length; i++) {
        char c = data[i];
        if (c == '\r' || c == '\n') {
            if (!sh_rx_len)
                continue;
            sh_rx[sh_rx_len] = '\0';
            sh_rx_len = 0;
            if (!strcmp(sh_rx, "!rom")) {
                /* Out of band, also while a command blocks the shell: back
                 * to the loader, as `rom` does. An unattended run with a
                 * wedged shell had no other way back. */
                char state[120];    /* TODO(cleanup): lost shell replies */
                dev_serial_state(state, sizeof state);
                dev_log("!rom: shell %s; %s", sh_busy ? "busy" : "idle", state);
                dev_return();
                dev_serial_end("ok\n", 3);
                continue;
            }
            if (sh_busy) {
                dev_serial_end("err busy\n", 9);
                continue;
            }
            strcpy(sh_line, sh_rx);
            sh_busy = true;
            semaphore_release(&sh_sem);
        } else if (sh_rx_len < sizeof sh_rx - 1) {
            sh_rx[sh_rx_len++] = c;
        }
    }
}

/* menutree: every visible main menu node as "mt PATH TYPE NAME [CFG]".
 * PATH is the dotted visible index at each level, i.e. the Down presses
 * after Home (main menu at its first item) and between Selects. TYPE: M
 * submenu, S setting, F function, L list (MT_RETURN_ID), V value. Mirrors
 * init_menu_lists() (ACTION_REQUEST_MENUITEM decides visibility) and
 * get_menu_item_name(). Functions are not entered: what they show is
 * code, not menu data. */
static struct gui_synclist menutree_list;   /* callbacks may look at it */
const struct menu_item_ex *root_menu_item_menu(int go_to);  /* root_menu.c */
bool settings_dev_save_all(const char *path);                /* settings.c */

static menu_callback_type menutree_callback(const struct menu_item_ex *m)
{
    menu_callback_type cb = (m->flags & (MENU_HAS_DESC | MENU_DYNAMIC_DESC)) ?
                            m->callback_and_desc->menu_callback : m->menu_callback;
    return cb;
}

static void menutree_walk(const struct menu_item_ex *menu, char *path,
                          size_t len, int depth)
{
    static const char types[] = "MSSFFLV";
    static char name[64];
    int count = MENU_GET_COUNT(menu->flags);
    int type = menu->flags & MENU_TYPE_MASK;
    menu_callback_type list_cb = menutree_callback(menu);
    int shown = 0;

    if (type != MT_MENU && type != MT_RETURN_ID)
        return;
    for (int i = 0; i < count; i++) {
        const struct menu_item_ex *m = type == MT_MENU ? menu->submenus[i] : NULL;
        menu_callback_type cb = m ? menutree_callback(m) : list_cb;
        if (cb && cb(ACTION_REQUEST_MENUITEM,
                     m ? m : (void *)(intptr_t)i, &menutree_list) == ACTION_EXIT_MENUITEM)
            continue;
        const char *n, *cfg = "";
        int t = m ? (m->flags & MENU_TYPE_MASK) : MT_RETURN_VALUE;
        if (!m)
            n = (menu->flags & MENU_DYNAMIC_DESC) ?
                menu->menu_get_name_and_icon->list_get_name(i,
                    menu->menu_get_name_and_icon->list_get_name_data, name, sizeof name) :
                P2STR((unsigned char *)menu->strings[i]);
        else if ((m->flags & MENU_DYNAMIC_DESC) && t != MT_SETTING_W_TEXT)
            n = m->menu_get_name_and_icon->list_get_name(i,
                    m->menu_get_name_and_icon->list_get_name_data, name, sizeof name);
        else if (t == MT_SETTING || t == MT_SETTING_W_TEXT) {
            const struct settings_list *v = find_setting(m->variable);
            n = v ? (const char *)str(v->lang_id) : "?";
            cfg = v && v->cfg_name ? v->cfg_name : "";
        } else
            n = P2STR(m->callback_and_desc->desc);
        size_t l = len + snprintf(path + len, 48 - len, "%s%d", len ? "." : "", shown);
        dev_log("mt %s %c %s%s%s", path, t < (int)sizeof types - 1 ? types[t] : '?',
                    n ? n : "", *cfg ? " " : "", cfg);
        const struct menu_item_ex *sub = !m ? NULL : t == MT_MENU ? m :
            (depth == 0 && t == MT_RETURN_VALUE) ? root_menu_item_menu(m->value) : NULL;
        if (sub && depth < 8 && l < 40)
            menutree_walk(sub, path, l, depth + 1);
        path[len] = '\0';
        shown++;
        yield();
    }
}

static bool sh_help(char *arg);

static bool sh_log(char *arg)
{
    /* Up to the head at the start: lines logged meanwhile wait. */
    (void)arg;
    uint32_t pos = 0, end = dev_log_head();
    char chunk[64];
    size_t n;
    while (pos < end &&
           (n = dev_log_read(&pos, chunk, MIN(sizeof chunk, end - pos))))
        dev_serial_write(chunk, n), sleep(1);
    return true;
}

static bool sh_keys(char *arg)
{
    dev_keys(arg);
    return true;
}

static bool sh_r32(char *arg)
{
    char *save, *a = strtok_r(arg, " ", &save), *c = strtok_r(NULL, " ", &save);
    if (!a)
        return false;
    uint32_t addr = strtoul(a, NULL, 0) & ~3u;
    int n = c ? atoi(c) : 1;
    for (int i = 0; i < n; i++, addr += 4)
        dev_log("%08lx: %08lx", (unsigned long)addr,
                    (unsigned long)*(volatile uint32_t *)addr);
    return true;
}

static bool sh_w32(char *arg)
{
    char *save, *a = strtok_r(arg, " ", &save), *v = strtok_r(NULL, " ", &save);
    if (!a || !v)
        return false;
    *(volatile uint32_t *)(strtoul(a, NULL, 0) & ~3u) = strtoul(v, NULL, 0);
    return true;
}

static bool sh_bat(char *arg)
{
    (void)arg;
    dev_log("battery %d mV %d%% charging=%d power=%d", battery_voltage(),
                battery_level(), charging_state(), power_input_present());
    return true;
}

static bool sh_ls(char *arg)
{
    list_dir(*arg ? arg : "/");
    return true;
}

/* The playback, PCM buffer and buffering state (playback.c, pcmbuf.c,
 * buffering.c) */
extern void audio_dev_state(void);
extern void pcmbuf_dev_state(void);
extern unsigned long pcmbuf_dev_chunks_played(void);
extern void buffering_dev_state(void);

static bool sh_status(char *arg)
{
    struct mp3entry *id3 = audio_current_track();
    dev_log("audio %d elapsed %lu/%lu ms %lu Hz buffered %lu/%lu pcm %d file %s",
            audio_status(),
            id3 ? (unsigned long)id3->elapsed : 0, id3 ? (unsigned long)id3->length : 0,
            id3 ? (unsigned long)id3->frequency : 0,
            (unsigned long)buf_used(), (unsigned long)buf_length(),
            (int)pcm_is_playing(), id3 && id3->path[0] ? id3->path : "-");
    if (!strcmp(arg, "full")) {
        audio_dev_state();
        pcmbuf_dev_state();
        buffering_dev_state();
        dev_files_open(true);
    }
    return true;
}

/* play and playlist: not while a plugin holds the audio buffer */
static bool audio_buffer_lent(const char *cmd)
{
    if (!plugin_holds_audio_buffer())
        return false;
    dev_log("%s: busy (%s holds the audio buffer)", cmd, plugin_get_current_filename());
    return true;
}

static bool sh_playlist(char *arg)
{
    /* playlist PATH [shuffle]: play a playlist file (.m3u8) from its
     * first track, shuffled first if asked (on the UI thread) */
    if (audio_buffer_lent("playlist"))
        return false;
    ui_run_post("@playlist:%s", arg);
    return true;
}

static bool sh_repeat(char *arg)
{
    /* as the setting's callback does: playback drops what it buffered for
     * the old mode (else a one-track playlist's repeat, already buffered,
     * is decoded after playback has ended at the track's end) */
    if (*arg && atoi(arg) != global_settings.repeat_mode) {
        global_settings.repeat_mode = atoi(arg);
        if ((audio_status() & AUDIO_STATUS_PLAY) == AUDIO_STATUS_PLAY)
            audio_flush_and_reload_tracks();
    }
    dev_log("repeat_mode %d", global_settings.repeat_mode);
    return true;
}

static bool sh_rm(char *arg)
{
    dev_log("rm %s: %d", arg, remove(arg));
    return true;
}

static bool sh_hang(char *arg)
{
    /* hang [irq]: spin forever (IRQs off with "irq") to exercise the
     * recovery watchdog and the retained tick trace */
    dev_log("hang%s: spinning", arg[0] ? " (IRQs off)" : "");
    if (!strcmp(arg, "irq"))
        disable_irq();
    for (volatile unsigned spin = 0;; spin++)
        ;
    return true;
}

static bool sh_sdlog(char *arg)
{
    /* sdlog on|off: append the dev log to SDLOG_PATH every 15 s */
    sdlog_on = !strcmp(arg, "on");
    dev_log("sdlog %s", sdlog_on ? "on" : "off");
    return true;
}

static bool sh_health(char *arg)
{
    /* one line for torture runs; `health reset` zeroes the counters */
    if (!strcmp(arg, "reset")) {
        health.starved_seen = health.starved;
        health.end_seen = health.end_starved;
        health.starved_playing = health.stalls = 0;
        health.ui_late = 0;
        health.max_stall_ms = 0;
    }
    struct mp3entry *id3 = audio_current_track();
    /* abuf: the playback buffer, while playback holds it; it takes nearly
     * all free memory, and on targets that keep it after a stop, free alone
     * says whether it is held, not whether memory leaked */
    dev_log("health up %lu s audio %d elapsed %lu/%lu underruns %u/%u stalls %u "
                "maxstall %lu ui_late %u ui_age %ld free %lu cpu %ld act %d plugin %s db %d "
                "abuf %lu arena %x",
                (unsigned long)(current_tick / HZ), audio_status(),
                id3 ? (unsigned long)id3->elapsed : 0, id3 ? (unsigned long)id3->length : 0,
                health.starved_playing, health.starved - health.starved_seen,
                health.stalls,
                health.max_stall_ms, health.ui_late,
                (current_tick - health.ui_pong_tick) / HZ,
                (unsigned long)core_available(), (long)(cpu_frequency / 1000000),
                (int)get_current_activity(),   /* enum current_activity, misc.h */
                health_plugin(), (int)dev_db_busy(),
                (unsigned long)audio_buffer_size(), module_arena_state());
    return true;
}

static bool sh_stop(char *arg)
{
    (void)arg;
    audio_stop();
    return true;
}

static bool sh_play(char *arg)
{
    if (dev_db_busy()) {
        dev_log("play: busy (database update)");
        return false;
    }
    if (audio_buffer_lent("play"))
        return false;
    /* on the UI thread, as the file browser does: playlist code from
     * this thread raced Now Playing's render (suspect for a WPS hang) */
    ui_run_post("@play:%s", arg);
    return true;
}

static bool sh_vol(char *arg)
{
    global_status.volume = volume_from_db(arg);
    sound_set_volume(global_status.volume);
    return true;
}

#if CONFIG_RTC
static bool sh_date(char *arg)
{
    struct tm tm;
    if (*arg) {
        memset(&tm, 0, sizeof tm);
        if (sscanf(arg, "%d-%d-%d %d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                   &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 6)
            return false;
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        set_day_of_week(&tm);
        set_day_of_year(&tm);
        if (!valid_time(&tm))
            return false;
        set_time(&tm);
    }
    tm = *get_time();
    dev_log("%04d-%02d-%02d %02d:%02d:%02d wday %d", tm.tm_year + 1900,
                tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday);
    return true;
}
#endif

#if CONFIG_TUNER
static bool sh_radio(char *arg)
{
    /* radio [KHZ]: open the FM screen (UI thread), tuned to KHZ */
    if (*arg) {
        const struct fm_region_data *fmr = &fm_region_data[global_settings.fm_region];
        global_status.last_frequency = (atoi(arg) * 1000 - fmr->freq_min) / fmr->freq_step;
    }
    ui_run_post("%s", "@radio");
    return true;
}

#ifdef HAVE_RDS_EXTENDED
static bool sh_rds(char *arg)
{
    /* rds: what the RDS decoder holds now */
    (void)arg;
    char text[200], name[48];
    uint16_t pi = 0;
    time_t ct = 0;
    tuner_get_rds_info(RADIO_RDS_PROGRAM_INFO, &pi, sizeof pi);
    tuner_get_rds_info(RADIO_RDS_CURRENT_TIME, &ct, sizeof ct);
    tuner_get_rds_info(RADIO_RDS_CALL_LETTERS, name, sizeof name);
    tuner_get_rds_info(RADIO_RDS_PTY_NAME, text, sizeof text);
    struct tm *tm = gmtime(&ct);
    dev_log("rds pi=%04x call='%s' pty='%s' ct=%s%04d-%02d-%02d %02d:%02d",
                pi, name, text, ct ? "" : "none ", tm->tm_year + 1900,
                tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);
    tuner_get_rds_info(RADIO_RDS_NAME, name, sizeof name);
    tuner_get_rds_info(RADIO_RDS_TEXT, text, sizeof text);
    dev_log("rds ps='%s' rt='%s'", name, text);
    tuner_get_rds_info(RADIO_RDS_ARTIST, text, sizeof text);
    dev_log("rds artist='%s'", text);
    tuner_get_rds_info(RADIO_RDS_TITLE, text, sizeof text);
    dev_log("rds title='%s'", text);
    return true;
}
#endif

#ifdef HAVE_RECORDING
static bool sh_rec(char *arg)
{
    /* rec SECONDS [NAME [mic|fm [mono|stereo [wav|aiff|wv|mp3 [PRE]]]]]:
     * record the FM input (or the built-in mic) to DEV_CARD_DIR/NAME.EXT;
     * PRE seconds of pre-recording, waited for before recording starts */
    static const char * const exts[REC_NUM_FORMATS] = {
        [REC_FORMAT_PCM_WAV] = "wav", [REC_FORMAT_AIFF] = "aiff",
        [REC_FORMAT_WAVPACK] = "wv", [REC_FORMAT_MPA_L3] = "mp3",
    };
    char *save, *secs = strtok_r(arg, " ", &save), *name = strtok_r(NULL, " ", &save);
    char *src = strtok_r(NULL, " ", &save), *chans = strtok_r(NULL, " ", &save);
    char *fmt = strtok_r(NULL, " ", &save), *pre = strtok_r(NULL, " ", &save);
    int format = REC_FORMAT_PCM_WAV;
    for (int i = 0; fmt && i < REC_NUM_FORMATS; i++)
        if (!strcmp(fmt, exts[i]))
            format = i;
    char path[64];
    snprintf(path, sizeof path, DEV_CARD_DIR "/%s.%s", name ? name : "rec",
             exts[format]);
    /* the UI's own recording or a database commit owns the buffer */
    if (dev_db_busy() || in_recording_screen()) {
        dev_log("rec: busy (%s)", in_recording_screen() ? "recording screen"
                                                            : "database update");
        return false;
    }
    struct audio_recording_options o;
    rec_init_recording_options(&o);
    o.rec_source = src && !strcmp(src, "mic") ? AUDIO_SRC_MIC : AUDIO_SRC_FMRADIO;
    o.rec_channels = chans && !strcmp(chans, "mono") ? 1 : 0;
    o.enc_config.rec_format = format;
    global_to_encoder_config(&o.enc_config);
    o.rec_prerecord_time = pre ? atoi(pre) : 0;
    audio_init_recording();
#ifdef PCMREC_LOWMEM /* audio.h: an engine that may not fit */
    if (!audio_recording_ready()) {
        dev_log("rec %s: not enough memory", path);
        return false;
    }
#endif
    rec_set_recording_options(&o);
    if (o.rec_prerecord_time)
        sleep((o.rec_prerecord_time + 1) * HZ);
    audio_record(path);
    sleep(HZ / 2);
    dev_log("rec %s: status %x", path, audio_status());
    unsigned long warned = 0;
    for (int t = atoi(secs ? secs : "5"); t > 0; t--) {
        sleep(HZ);
        unsigned long w = pcm_rec_get_warnings();
        if (w != warned)
            dev_log("rec: warnings %lx at %lu ms", w, audio_recorded_time() * 1000 / HZ);
        warned = w;
    }
    dev_log("rec: status %x before stop", audio_status());
    dev_log("rec: %lu ms recorded", audio_recorded_time() * 1000 / HZ);
    audio_stop_recording();
    sleep(HZ);
    audio_close_recording();
    return true;
}
#endif

static bool sh_fmmono(char *arg)
{
    tuner_set(RADIO_FORCE_MONO, atoi(arg));
    return true;
}

static bool sh_fmmute(char *arg)
{
    tuner_set(RADIO_MUTE, atoi(arg));
    return true;
}
#endif /* CONFIG_TUNER */

static bool sh_cpu(char *arg)
{
    /* cpu max: hold a boost; cpu normal: drop to the normal clock until the
     * next boost from idle; cpu auto: the clock the boost count asks for
     * (after "cpu normal" a codec's boost stayed counted at the normal
     * clock: underruns) */
    static bool pinned;
    if (pinned)
        cpu_boost(false), pinned = false;
    if (!strcmp(arg, "max"))
        cpu_boost(true), pinned = true;
    if (!strcmp(arg, "normal"))
        set_cpu_frequency(CPUFREQ_NORMAL);
    else
        set_cpu_frequency(get_cpu_boost_counter() ? CPUFREQ_MAX :
                                                    CPUFREQ_NORMAL);
    dev_log("cpu %ld MHz", cpu_frequency / 1000000);
    return true;
}

static bool sh_sdtest(char *arg)
{
    /* sdtest KB [BUFKB [odd]]: write a pseudo-random file in BUFKB pieces
     * (default 4), fsync, read back, compare. odd: a 2-byte prefix puts
     * every whole sector on a buffer address = 2 mod 4 (as AIFF does) */
    static const char path[] = DEV_CARD_DIR "/sdtest.bin";
    char *save, *kbs = strtok_r(arg, " ", &save), *bufs = strtok_r(NULL, " ", &save);
    int kb = kbs ? atoi(kbs) : 0, bad = 0;
    int bufkb = bufs ? atoi(bufs) : 4;
    char *odd = strtok_r(NULL, " ", &save);
    uint16_t prefix = 0;
    int big = 0;
    uint32_t *buf;
    if (bufkb < 4 || bufkb > 64 || kb % bufkb)
        return false;
    if (bufkb > 4) {
        big = core_alloc(bufkb * 1024);
        if (big <= 0) {
            dev_log("sdtest: no %d KiB buffer", bufkb);
            return false;
        }
        core_pin(big);
        buf = core_get_data(big);
    } else {
        buf = dev_buf_get();
        if (!buf)
            return false;
    }
    size_t bytes = bufkb * 1024, words = bytes / 4;
    uint32_t seed = current_tick, x;
    long t0 = current_tick;
    struct dev_sd_stats st0 = dev_sd_stats;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        if (big > 0)
            core_free(big);
        return false;
    }
    if (odd)
        write(fd, &prefix, sizeof prefix);
    x = seed;
    for (int k = 0; k < kb; k += bufkb) {
        for (size_t i = 0; i < words; i++)
            buf[i] = x = x * 1664525u + 1013904223u;
        if (write(fd, buf, bytes) != (ssize_t)bytes)
            bad = -1;
        dev_feed();
    }
    fsync(fd);
    close(fd);
    long t1 = current_tick;
    fd = open(path, O_RDONLY);
    if (odd)
        read(fd, &prefix, sizeof prefix);
    x = seed;
    unsigned long in_read = 0;
    for (int k = 0; fd >= 0 && k < kb; k += bufkb) {
        uint32_t r0 = dev_usec();
        if (read(fd, buf, bytes) != (ssize_t)bytes)
            bad = -2;
        in_read += dev_usec() - r0;
        for (size_t i = 0; i < words; i++)
            if (buf[i] != (x = x * 1664525u + 1013904223u))
                bad++;
        dev_feed();
    }
    close(fd);
    if (big > 0)
        core_free(big);
    long t2 = current_tick;
    dev_log("sdtest %d KiB (%d KiB pieces%s) at %ld MHz: write %ld ms, read %ld ms, errors %d",
                kb, bufkb, odd ? ", odd" : "", cpu_frequency / 1000000, (t1 - t0) * 1000 / HZ,
                (t2 - t1) * 1000 / HZ, bad);
    dev_log("  read() total %lu ms", in_read / 1000);
    dev_log("  sd reads: %lu sectors in %lu calls, %lu ms",
                (unsigned long)(dev_sd_stats.read_sectors - st0.read_sectors),
                (unsigned long)(dev_sd_stats.read_calls - st0.read_calls),
                (unsigned long)(dev_sd_stats.read_us - st0.read_us) / 1000);
    dev_log("  sd writes: %lu sectors in %lu ms, %lu busy polls",
                (unsigned long)(dev_sd_stats.write_sectors - st0.write_sectors),
                (unsigned long)(dev_sd_stats.write_us - st0.write_us) / 1000,
                (unsigned long)(dev_sd_stats.write_busy_polls - st0.write_busy_polls));
    return bad == 0;
}

static bool sh_cfg(char *arg)
{
    /* cfg PATH: load a settings/theme .cfg (UI thread) */
    ui_run_post("@cfg:%s", arg);
    return true;
}

static bool sh_browse(char *arg)
{
    /* browse PATH: select PATH in the file browser (UI thread) */
    ui_run_post("@browse:%s", arg);
    return true;
}

#ifdef HAVE_RECORDING
static bool sh_recscreen(char *arg)
{
    /* open the recording screen (UI thread) */
    (void)arg;
    ui_run_post("%s", "@rec");
    return true;
}
#endif

static bool sh_wps(char *arg)
{
    /* show Now Playing (UI thread), without menu navigation */
    (void)arg;
    ui_run_post("%s", "@wps");
    return true;
}

static bool sh_run_plugin(char *arg)
{
    /* run PATH: the UI thread loads the plugin (default event handler) */
    ui_run_post("%s", arg);
    return true;
}

#ifdef HAVE_TAGCACHE
static bool sh_db(char *arg)
{
    /* db [rebuild|update|menus]: database scan, then its status */
#ifdef TAGCACHE_LOWMEM
    if (!strcmp(arg, "menus")) {
        /* heap taken by the parsed tagnavi.config */
        size_t before = core_available();
        bool ok = tagtree_ready();
        dev_log("db menus %d: %lu bytes", ok,
                    (unsigned long)(before - core_available()));
        tagtree_release(NULL);
    }
#endif
    if (!strcmp(arg, "rebuild") || !strcmp(arg, "update")) {
        /* as the Database menu does: blocking screen on the UI thread */
        ui_run_post("%s", !strcmp(arg, "rebuild") ? "@dbrebuild" : "@dbupdate");
    }
    struct tagcache_stat *st = tagcache_get_stat();
    dev_log("db init %d ready %d delayed %d commit_step %d progress %d "
                "processed %d total %d queue %d cur %s",
                st->initialized, st->ready, st->commit_delayed, st->commit_step,
                st->progress, st->processed_entries, st->total_entries,
                st->queue_length, st->curentry ? st->curentry : "-");
    return true;
}
#endif

static bool sh_delfiles(char *arg)
{
    /* delfiles DIR FIRST LAST: remove the mkfiles test files FIRST..LAST */
    char *save, *dir = strtok_r(arg, " ", &save), *a = strtok_r(NULL, " ", &save),
         *b = strtok_r(NULL, " ", &save);
    if (!dir || !a || !b)
        return false;
    for (int i = atoi(a); i <= atoi(b); i++) {
        char path[MAX_PATH];
        snprintf(path, sizeof path, "%s/fat_test_long_name_%02d.bin", dir, i);
        remove(path);
    }
    dev_log("delfiles: %s %s..%s", dir, a, b);
    return true;
}

static bool sh_count(char *arg)
{
    /* count DIR: number of entries readdir returns */
    DIR *d = opendir(arg);
    int n = 0;
    if (!d)
        return false;
    while (readdir(d))
        n++;
    closedir(d);
    dev_log("count %s: %d", arg, n);
    return true;
}

static bool sh_set(char *arg)
{
    /* set NAME: VALUE: one config.cfg line, saved; set NAME: show it */
    char *colon = strchr(arg, ':');
    if (!colon) {
        const struct settings_list *setting = find_setting_by_cfgname(arg);
        char value[64];
        if (!setting)
            return false;
        cfg_to_string(setting, value, sizeof value);
        dev_log("%s: %s", arg, value);
        return true;
    }
    *colon = '\0';
    char *value = colon + 1;
    while (*value == ' ')
        value++;
    bool theme_changed = false;
    bool ok = string_to_cfg(arg, value, &theme_changed);
    if (ok) {
        settings_save();
        call_storage_idle_notifys(true);    /* write now, as a shutdown does */
    }
    dev_log("set %s: %s -> %d", arg, value, ok);
    return true;
}

static bool sh_mkcheck(char *arg)
{
    /* mkcheck DIR N [nowrite]: mkfiles, counting DIR's fat_test files after
     * each open and each close; logs the first step where one is missing */
    char *save, *dir = strtok_r(arg, " ", &save), *nstr = strtok_r(NULL, " ", &save);
    int n = nstr ? atoi(nstr) : 10;
    bool nowrite = strtok_r(NULL, " ", &save) != NULL;
    const void *data = dev_buf_get();   /* 1 KB of whatever is there */
    if (!dir || !data)
        return false;
    mkdir(dir);
    dev_log("mkcheck: cluster %u bytes", fat_get_cluster_size(IF_MV(0)));
    int expect = count_prefix(dir, "fat_test_long_name_");
    for (int i = 0; i < n; i++) {
        char path[MAX_PATH];
        snprintf(path, sizeof path, "%s/fat_test_long_name_%02d.bin", dir, i);
        expect += !file_exists(path);   /* names may remain from earlier runs */
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            dev_log("mkcheck: open %s failed %d", path, fd);
            return false;
        }
        int found_open = count_prefix(dir, "fat_test_long_name_");
        if (!nowrite)
            write(fd, data, 1024);
        close(fd);
        int found = count_prefix(dir, "fat_test_long_name_");
        if (found_open != expect || found != expect) {
            dev_log("mkcheck: file %d: %d present after open, %d after close, "
                        "expected %d", i, found_open, found, expect);
            return true;
        }
        yield();
    }
    dev_log("mkcheck: %d files ok", n);
    return true;
}

static bool sh_mkfiles(char *arg)
{
    /* mkfiles DIR N: create DIR/fat_test_NN.bin (long names), 1 KB each */
    char *save, *dir = strtok_r(arg, " ", &save), *nstr = strtok_r(NULL, " ", &save);
    int n = nstr ? atoi(nstr) : 10;
    const void *data = dev_buf_get();
    if (!dir || !data)
        return false;
    mkdir(dir);
    for (int i = 0; i < n; i++) {
        char path[MAX_PATH];
        snprintf(path, sizeof path, "%s/fat_test_long_name_%02d.bin", dir, i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            dev_log("mkfiles: open %s failed %d", path, fd);
            return false;
        }
        write(fd, data, 1024);
        close(fd);
        yield();    /* between files, not inside the FS: heartbeat */
    }
    dev_log("mkfiles: %d files in %s", n, dir);
    return true;
}

static bool sh_usbcycle(char *arg)
{
    /* Simulated unplug: usb_detect() reports extracted for 3 s. */
    (void)arg;
    dev_log("usbcycle: detaching");
    sleep(HZ / 2);
    dev_usb_enabled = false;
    sleep(3 * HZ);
    dev_usb_enabled = true;
    return true;
}

static bool sh_keylog(char *arg)
{
    /* keylog SECONDS: log button state changes (arb TARGET checks keys) */
    long end = current_tick + atoi(arg) * HZ;
    int last = button_status();
    dev_log("keylog: buttons now %x; press keys now", last);
    while (TIME_BEFORE(current_tick, end)) {
        int b = button_status();
        if (b != last) {
#ifdef ADC_BUTTONS
            dev_log("buttons %x (adc %d)", b, adc_read(ADC_BUTTONS));
#else
            dev_log("buttons %x", b);
#endif
            last = b;
        }
        sleep(HZ / 50);
    }
    return true;
}

static bool sh_hog(char *arg)
{
    /* hog KB: hold KB of buflib (low-memory repros); hog 0 frees it */
    static int handle;
    if (handle > 0)
        handle = core_free(handle);
    size_t kb = atoi(arg);
    if (kb)
        handle = core_alloc(kb * 1024);
    dev_log("hog: %s %u KB, available %lu", handle > 0 ? "holding" : "none",
                handle > 0 ? (unsigned)kb : 0u, (unsigned long)core_available());
    return true;
}

void dev_log_heap(bool blocks)
{
    dev_log("core_alloc: available %lu, largest allocatable %lu",
                (unsigned long)core_available(), (unsigned long)core_allocatable());
    if (blocks) {
        char line[64];
        for (int i = 0; i < core_get_num_blocks(); i++)
            if (core_print_block_at(i, line, sizeof line))
                dev_log("  %s", line);
    }
}

static bool sh_mem(char *arg)
{
    /* buflib (core_alloc) space and each thread's stack high-water mark;
     * mem blocks: every buflib allocation too (address, size in words) */
    for (unsigned i = 0; i < MAXTHREADS; i++) {
        struct thread_debug_info info;
        if (thread_get_debug_info(i, &info) > 0)
            dev_log("  thread %u %-14s prio %d/%d stack peak %u%%", i, info.name,
                        info.base_priority, info.current_priority, info.stack_usage);
    }
    dev_log_heap(!strcmp(arg, "blocks"));
    return true;
}

static bool sh_fill(char *arg)
{
    /* fill R G B: whole screen one colour */
    char *save, *r = strtok_r(arg, " ", &save), *g = strtok_r(NULL, " ", &save),
         *b = strtok_r(NULL, " ", &save);
    if (!r || !g || !b)
        return false;
    lcd_set_foreground(LCD_RGBPACK(atoi(r), atoi(g), atoi(b)));
    lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
    lcd_update();
    backlight_on();
    return true;
}

#if LCD_PIXELFORMAT == RGB565SWAPPED
#define FB_RGB565(p) swap16(p)
#else
#define FB_RGB565(p) (p)
#endif

static bool sh_shot(char *arg)
{
    /* Framebuffer as hex RGB565 rows "fb Y HEX" (`arb TARGET shot`); paced
     * like `log` so the 512 B serial send buffer never overflows. "shot Y0
     * Y1": rows Y0..Y1 only (yp3.py fetches rows a slow host lost again). */
    static const char hex[] = "0123456789abcdef";
    char line[16 + LCD_WIDTH * 4 + 1];
    char *end, *end2;
    int first = strtol(arg, &end, 10), last = LCD_HEIGHT - 1;
    if (end != arg) {
        last = strtol(end, &end2, 10);
        if (end2 == end)
            last = first;
    }
    if (first < 0 || last >= LCD_HEIGHT || first > last)
        return false;
    for (int y = first; y <= last; y++) {
        const fb_data *row = FBADDR(0, y);
        int n = snprintf(line, sizeof line, "fb %d ", y);
        for (int x = 0; x < LCD_WIDTH; x++)
            for (int k = 12; k >= 0; k -= 4)
                line[n++] = hex[(FB_RGB565(row[x]) >> k) & 15];
        line[n++] = '\n';
        for (int i = 0; i < n; i += 128)
            dev_serial_write(line + i, MIN(128, n - i)), sleep(1);
    }
    return true;
}

static bool sh_cfgsave(char *arg)
{
    /* cfgsave PATH: every setting, to restore later with `cfg PATH` */
    dev_log("cfgsave %s: %s", arg, settings_dev_save_all(arg) ? "ok" : "failed");
    return true;
}

static bool sh_menutree(char *arg)
{
    (void)arg;
    char path[48] = "";
    menutree_walk(&root_menu_, path, 0, 0);
    dev_log("menutree done");
    return true;
}

#ifdef CPU_ARM_MICRO
static bool sh_tickprio(char *arg)
{
    /* tickprio N: SysTick's NVIC priority byte (boot: 0xff, below every
     * IRQ). 0 lets the tick preempt a spinning ISR, so the heartbeat
     * logs a HANG with the ISR in the tick trace instead of a silent
     * watchdog return. */
    if (*arg)
        reg_writef(CM_SCB_SHPR3, PRI_SYSTICK(strtoul(arg, NULL, 0)));
    dev_log("tickprio %u", (unsigned)reg_readf(CM_SCB_SHPR3, PRI_SYSTICK));
    return true;
}
#endif

static bool sh_threads(char *arg)
{
    /* where every thread waits */
    (void)arg;
    dev_thread_trace_all();
    return true;
}

/* dryoff [0|now]: a power-off from the UI returns to the host instead of
 * cutting power (unattended runs); 0: real again; now: a real power_off()
 * at once (on USB the player then stays dark until a reset) */
static bool sh_dryoff(char *arg)
{
    if (!strcmp(arg, "now")) {
        dev_poweroff_dry = false;
        power_off();
    }
    dev_poweroff_dry = strcmp(arg, "0") != 0;
    dev_log("dryoff %d", dev_poweroff_dry);
    return true;
}

static bool sh_uptime(char *arg)
{
    (void)arg;
    dev_log("tick %ld cpu %ld MHz", current_tick, cpu_frequency / 1000000);
    return true;
}

static bool sh_meta(char *arg)
{
    /* meta PATH: time get_metadata (as a database scan reads a file) */
    static struct mp3entry id3;
    int fd = open(arg, O_RDONLY);
    if (fd < 0)
        return false;
    struct dev_sd_stats st0 = dev_sd_stats;
    uint32_t t0 = dev_usec();
    bool ok = get_metadata(&id3, fd, arg);
    unsigned long us = dev_usec() - t0;
    unsigned char head[16] = {0};
    lseek(fd, 0, SEEK_SET);
    read(fd, head, sizeof head);
    close(fd);
    dev_log("meta: id3v2 %lu, id3v1 %lu, head %02x%02x%02x%02x %02x%02x%02x%02x "
                "%02x%02x%02x%02x %02x%02x%02x%02x", (unsigned long)id3.id3v2len,
                (unsigned long)id3.id3v1len, head[0], head[1], head[2], head[3],
                head[4], head[5], head[6], head[7], head[8], head[9], head[10],
                head[11], head[12], head[13], head[14], head[15]);
    dev_log("meta: %d in %lu ms: length %lu ms, first frame %lu, bitrate %u%s, "
                "sd reads %lu sectors in %lu calls", ok, us / 1000,
                (unsigned long)id3.length, (unsigned long)id3.first_frame_offset,
                id3.bitrate, id3.vbr ? " vbr" : "",
                (unsigned long)(dev_sd_stats.read_sectors - st0.read_sectors),
                (unsigned long)(dev_sd_stats.read_calls - st0.read_calls));
    return true;
}

#ifdef HAVE_BLUETOOTH
#define BT_LIST_MAX 8               /* devices listed per "bt stack list" */
#define STACK_FILL  0xdeadbeefu     /* crt0 fills the stacks with it */

/* Bytes of the interrupt stack (MSP) ever used, from the target's link
 * script and crt0's fill */
extern uint32_t irqstackbegin[], irqstackend[];
static unsigned irq_stack_used(void)
{
    const uint32_t *p = irqstackbegin;
    while (p < irqstackend && *p == STACK_FILL)
        p++;
    return (irqstackend - p) * 4u;
}

/* One log line per device: "bt WHAT I: ADDR cod COD "NAME"" */
static void bt_log_devices(const char *what, const struct bt_device *devs, int n)
{
    char addr[BT_ADDR_STR];
    for (int i = 0; i < n; i++) {
        bt_addr_format(&devs[i].addr, addr);
        dev_log("bt %s %d: %s cod %06lx \"%s\"", what, i, addr,
                (unsigned long)devs[i].cod, devs[i].name);
    }
}

/* bt stack on|off|scan|list|connect ADDR|disconnect|forget ADDR|status|
 *          sink bt|builtin: the host stack through its public API */
static bool bt_stack(char *arg)
{
    static struct bt_device devs[BT_LIST_MAX];
    char *val = strchr(arg, ' ');
    bt_addr_t addr;
    if (val)
        *val++ = '\0';
    if (!strcmp(arg, "on")) {
        dev_log("bt stack on: %d", bt_power(true));
    } else if (!strcmp(arg, "off")) {
        bt_power(false);
        dev_log("bt stack off");
    } else if (!strcmp(arg, "scan")) {
        dev_log("bt stack scan: %d", bt_scan(true));
    } else if (!strcmp(arg, "list")) {
        dev_log("bt stack: scanning %d", bt_scanning());
        bt_log_devices("found", devs, bt_scan_results(devs, BT_LIST_MAX));
        bt_log_devices("paired", devs, bt_paired_devices(devs, BT_LIST_MAX));
    } else if (!strcmp(arg, "connect") && val && bt_addr_parse(val, &addr)) {
        dev_log("bt stack connect: %d", bt_connect(&addr));
    } else if (!strcmp(arg, "disconnect")) {
        bt_disconnect();
    } else if (!strcmp(arg, "forget") && val && bt_addr_parse(val, &addr)) {
        dev_log("bt stack forget: %d", bt_forget(&addr));
    } else if (!strcmp(arg, "status")) {
        char code[BT_CODE_MAX];
        dev_log("bt stack: status %d sink %d, irq stack used %u of %u",
                bt_status(), pcm_current_sink(), irq_stack_used(),
                (unsigned)(irqstackend - irqstackbegin) * 4u);
        if (bt_connected_device(&devs[0]))
            bt_log_devices("connected", devs, 1);
        if (bt_pairing_code(code))
            dev_log("bt stack: pairing code %s", code);
    } else if (!strcmp(arg, "sink") && val) {
        enum pcm_sink_ids id = !strcmp(val, "bt") ? PCM_SINK_BT : PCM_SINK_BUILTIN;
        dev_log("bt stack sink %s: %d", val, mixer_switch_sink(id));
    } else {
        return false;
    }
    return true;
}

/* "bt stack ..." here; any other "bt PART ..." is the target's own "bt"
 * command (its controller and radio) */
static bool sh_bt(char *arg)
{
    if (!strncmp(arg, "stack ", 6))
        return bt_stack(arg + 6);
    const struct dev_cmd *c;
    for (size_t i = 0; (c = dev_target_cmd(i)); i++)
        if (!strcmp(c->name, "bt"))
            return c->fn(arg);
    return false;
}
#endif /* HAVE_BLUETOOTH */

static bool sh_reboot(char *arg)
{
    (void)arg;
    system_reboot();
    return true;
}

static bool sh_rom(char *arg)
{
    /* back to the host's loader, the log ring kept */
    (void)arg;
    dev_return();
    return true;
}

static const struct dev_cmd sh_cmds[] = {
    { "help",      sh_help,       NULL },
    { "log",       sh_log,        "log" },
    { "msgs",      sh_msgs,       "msgs [AFTER]" },
    { "keys",      sh_keys,       "keys SCRIPT" },
    { "r32",       sh_r32,        "r32 ADDR [N]" },
    { "w32",       sh_w32,        "w32 ADDR VAL" },
    { "bat",       sh_bat,        "bat" },
    { "ls",        sh_ls,         "ls DIR" },
    { "status",    sh_status,     "status [full]" },
    { "playlist",  sh_playlist,   "playlist M3U [shuffle]" },
    { "repeat",    sh_repeat,     "repeat [N]" },
    { "rm",        sh_rm,         "rm FILE" },
    { "hang",      sh_hang,       "hang [irq]" },
    { "health",    sh_health,     "health [reset]" },
    { "sdlog",     sh_sdlog,      "sdlog on|off" },
    { "stop",      sh_stop,       "stop" },
    { "play",      sh_play,       "play FILE" },
    { "vol",       sh_vol,        "vol DB" },
#if CONFIG_RTC
    { "date",      sh_date,       "date [Y-M-D H:M:S]" },
#endif
#if CONFIG_TUNER
    { "radio",     sh_radio,      "radio [KHZ] (FM screen)" },
#ifdef HAVE_RDS_EXTENDED
    { "rds",       sh_rds,        "rds" },
#endif
#ifdef HAVE_RECORDING
    { "rec",       sh_rec,        "rec SECONDS [NAME [mic|fm [mono|stereo [wav|aiff|wv|mp3 [PRE]]]]]" },
#endif
    { "fmmono",    sh_fmmono,     "fmmono 0|1" },
    { "fmmute",    sh_fmmute,     "fmmute 0|1" },
#endif
    { "cpu",       sh_cpu,        "cpu max|normal|auto" },
    { "sdtest",    sh_sdtest,     "sdtest KB [BUFKB [odd]]" },
    { "cfg",       sh_cfg,        "cfg PATH" },
    { "browse",    sh_browse,     "browse PATH" },
#ifdef HAVE_RECORDING
    { "recscreen", sh_recscreen,  "recscreen" },
#endif
    { "wps",       sh_wps,        "wps" },
    { "run",       sh_run_plugin, "run PLUGIN [PARAMETER]" },
#ifdef HAVE_TAGCACHE
    { "db",        sh_db,         "db [rebuild|update|menus]" },
#endif
    { "delfiles",  sh_delfiles,   NULL },
    { "count",     sh_count,      NULL },
    { "set",       sh_set,        NULL },
    { "mkcheck",   sh_mkcheck,    NULL },
    { "mkfiles",   sh_mkfiles,    NULL },
    { "usbcycle",  sh_usbcycle,   "usbcycle" },
    { "keylog",    sh_keylog,     "keylog S" },
    { "hog",       sh_hog,        "hog KB" },
    { "mem",       sh_mem,        "mem [blocks]" },
    { "fill",      sh_fill,       "fill R G B" },
    { "shot",      sh_shot,       "shot" },
    { "cfgsave",   sh_cfgsave,    "cfgsave PATH" },
    { "menutree",  sh_menutree,   "menutree" },
    { "sel",       sh_sel,        "sel" },
#ifdef CPU_ARM_MICRO
    { "tickprio",  sh_tickprio,   "tickprio N" },
#endif
    { "dryoff",    sh_dryoff,     "dryoff [0|now]" },
    { "uptime",    sh_uptime,     "uptime" },
    { "threads",   sh_threads,    "threads" },
    { "meta",      sh_meta,       "meta PATH" },
#ifdef HAVE_BLUETOOTH
    { "bt",        sh_bt,         "bt stack on|off|scan|list|connect ADDR|disconnect|"
                                  "forget ADDR|status|sink bt|builtin" },
#endif
    { "reboot",    sh_reboot,     "reboot" },
    { "rom",       sh_rom,        "rom" },
};

/* The shared commands, then the target's */
static const struct dev_cmd *sh_cmd_at(size_t i)
{
    if (i < ARRAYLEN(sh_cmds))
        return &sh_cmds[i];
    return dev_target_cmd(i - ARRAYLEN(sh_cmds));
}

static bool sh_help(char *arg)
{
    /* The usage column, " | " between entries, in lines that the
     * serial mirror sends whole (it cuts a line at 198 characters). */
    char line[192];
    size_t n = 0;
    const struct dev_cmd *c;
    (void)arg;
    for (size_t i = 0; (c = sh_cmd_at(i)); i++) {
        if (!c->usage)
            continue;
        if (n && n + 3 + strlen(c->usage) >= sizeof line) {
            dev_log("%s", line);
            n = 0;
        }
        n += snprintf(line + n, sizeof line - n, "%s%s", n ? " | " : "", c->usage);
    }
    if (n)
        dev_log("%s", line);
    return true;
}

static bool sh_run(char *cmd, char *arg)
{
    /* a shell command is someone at the player: no idle power-off (torture
       sat stopped between steps for minutes and powered off) */
    reset_poweroff_timer();
    const struct dev_cmd *c;
    for (size_t i = 0; (c = sh_cmd_at(i)); i++)
        if (!strcmp(cmd, c->name))
            return c->fn(arg);
    return false;
}

static void sh_thread(void)
{
    for (;;) {
        semaphore_wait(&sh_sem, TIMEOUT_BLOCK);
        char *arg = strchr(sh_line, ' ');
        if (arg)
            *arg++ = '\0';
        else
            arg = sh_line + strlen(sh_line);
        bool ok = sh_run(sh_line, arg);
        dev_buf_free();
        /* Serial only: an "ok" in the ring would end a later `log` dump. */
        char end[48];
        int n = snprintf(end, sizeof end, ok ? "ok\n" : "err %s\n", sh_line);
        dev_serial_end(end, MIN(n, (int)sizeof end - 1));
        sh_busy = false;
    }
}
#endif

/* A stall: the PCM ring fill, whether pcmbuf may start, the playback and
 * buffering state, and where the playback threads wait */
extern bool audio_pcmbuf_may_play(void);
static void dev_stall_state(void)
{
    dev_log("  stall: may_play %d chan %d pcm %d ring %lu/%lu file %lu/%lu",
                (int)audio_pcmbuf_may_play(),
                (int)mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK),
                (int)pcm_is_playing(),
                (unsigned long)(pcmbuf_get_bufsize() - pcmbuf_free()),
                (unsigned long)pcmbuf_get_bufsize(),
                (unsigned long)buf_used(), (unsigned long)buf_length());
    audio_dev_state();
    pcmbuf_dev_state();
    buffering_dev_state();
    dev_files_open(true);
    dev_thread_trace_name("audio");
    dev_thread_trace_name("codec");
    dev_thread_trace_name("buffering");
}

/* The output holds playback back while it gets ready: the Bluetooth sink
 * opening its stream (a headset connecting: page, pairing, signalling).
 * Not a stall unless it takes OUTPUT_WAIT_MS. */
#define OUTPUT_WAIT_MS 15000
static bool output_waits(void)
{
#ifdef HAVE_BLUETOOTH
    return pcm_current_sink() == PCM_SINK_BT &&
           bt_status() != BT_STATUS_STREAMING;
#else
    return false;
#endif
}

/* Attach USB as the dev config asks, after the boot tasks used the disk. */
static long health_stack[2 * DEFAULT_STACK_SIZE / sizeof(long)];

/* Once a second: playback stuck while playing, UI not answering pings.
 * Hard hangs (IRQs off, UI stuck) end in the recovery watchdog instead; the
 * retained tick trace (`yp3.py log`) shows where. */
static void health_thread(void)
{
    unsigned long last_elapsed = 0, last_chunks = 0;
    long stuck_since = 0, ping_tick = 0, post_tick = 0;
    bool stall_logged = false, stall_again = false, ui_logged = false;
    bool waited = false;
    struct dev_sd_stats last_sd = dev_sd_stats;
    health.ui_pong_tick = current_tick;
    for (;;) {
        sleep(HZ);
        if (sdlog_on && TIME_AFTER(current_tick, sdlog_next)) {
            sdlog_flush();
            sdlog_next = current_tick + SDLOG_PERIOD;
        }
        /* the usb= config wins over the "USB Mode" setting, which monkey
         * and crawl runs change through the menus: a USB cycle then came
         * back as mass storage, out of reach of the shell */
        if (dev_usb_mode >= 0 && global_settings.usb_mode != dev_usb_mode) {
            global_settings.usb_mode = dev_usb_mode;
            usb_set_mode(dev_usb_mode);
        }
        struct mp3entry *id3 = audio_current_track();
        bool playing = audio_status() == AUDIO_STATUS_PLAY && id3;
        /* underruns since last time: count those mid-track while playing */
        unsigned starved = health.starved, seen_starved = health.starved_seen;
        /* at a track's end: no PCM stop within 0.5 s (the playlist's end,
         * a rate change's remake) means a late next track */
        unsigned ended = health.end_starved, late = 0;
        if (ended != health.end_seen && current_tick - health.end_tick >= HZ / 2) {
            if (health.pcm_stops == health.end_stops)
                late = ended - health.end_seen;
            health.end_seen = ended;
        }
        struct dev_sd_stats sd = dev_sd_stats;
        if (starved != seen_starved || late) {
            if (playing && id3->elapsed + 1500 < id3->length) {
                health.starved_playing += starved - seen_starved + late;
                dev_log("UNDERRUN x%u%s at %lu/%lu ms, file buf %lu, cpu %ld, "
                            "last 1 s sd read %lu write %lu ms",
                            starved - seen_starved + late,
                            late ? " (the next track late)" : "",
                            (unsigned long)id3->elapsed,
                            (unsigned long)id3->length, (unsigned long)buf_used(),
                            (long)(cpu_frequency / 1000000),
                            (unsigned long)(sd.read_us - last_sd.read_us) / 1000,
                            (unsigned long)(sd.write_us - last_sd.write_us) / 1000);
                if (starve_trace_new && starve_trace_count) {
                    char line[STARVE_TRACE_PCS * 9 + 16];
                    size_t n = snprintf(line, sizeof line, "  ticks:");
                    for (unsigned i = 0; i < starve_trace_count; i++)
                        n += snprintf(line + n, sizeof line - n, " %lx",
                                      (unsigned long)starve_trace[i]);
                    dev_log("%s", line);
                }
            }
            health.starved_seen = starved;
            starve_trace_new = false;
        }
        bool waits = output_waits();
        /* elapsed that stands while the mixer takes chunks is a codec's
         * coarse position, not a stall: mod counts patterns, sid, gbs and
         * the like a subsong (TS17 torture 30 step 203: a mod 5 s at 0 ms
         * in its first pattern). A length of 0 counts anyway: a track that
         * never loaded (TS17 torture 8 step 75: playing at 0/0 after AVRCP
         * Stop, Previous, Play) */
        unsigned long chunks = pcmbuf_dev_chunks_played();
        /* the last second (or past the length) counts too, but later: a
         * track change takes a moment (YP3 torture gate2 shuffle 344: stuck
         * at 20481/19991 ms, no dump) */
        bool near_end = id3 && id3->length &&
                        id3->elapsed + 1000 >= id3->length;
        if (playing && id3->elapsed == last_elapsed &&
            (!id3->length || chunks == last_chunks)) {
            if (!stuck_since || (waited && !waits))
                stuck_since = current_tick;     /* from when it may play */
            unsigned long ms = (current_tick - stuck_since) * 1000 / HZ;
            if (ms > health.max_stall_ms && !waits)
                health.max_stall_ms = ms;
            if (ms >= (waits ? OUTPUT_WAIT_MS : near_end ? 6000 : 3000) &&
                !stall_logged) {
                health.stalls++;
                stall_logged = true;
                dev_log("STALL playback at %lu/%lu ms for %lu ms, file buf %lu",
                            (unsigned long)id3->elapsed, (unsigned long)id3->length,
                            ms, (unsigned long)buf_used());
                dev_stall_state();
            } else if (stall_logged && !stall_again && ms >= 10000) {
                /* again: did the mixer take chunks, the keys move? */
                stall_again = true;
                dev_log("STALL still at %lu ms:", ms);
                audio_dev_state();
                pcmbuf_dev_state();
            }
        } else {
            if (stall_logged)
                dev_log("STALL ended after %lu ms, audio %d elapsed %lu",
                            (current_tick - stuck_since) * 1000 / HZ,
                            audio_status(),
                            id3 ? (unsigned long)id3->elapsed : 0ul);
            stuck_since = 0;
            stall_logged = stall_again = false;
        }
        last_elapsed = playing ? id3->elapsed : 0;
        last_chunks = chunks;
        waited = waits;
        last_sd = sd;

        /* UI ping: an empty SYS_DEV_RUN; the default event handler
         * answers in the UI thread (dev_ui_run). */
        /* every 10 s: a ping is a button-queue event, which GUI-boosts
         * the CPU for a second */
        if ((health.ui_pong_tick - ping_tick >= 0 &&
             current_tick - ping_tick >= 10 * HZ) || !ping_tick) {
            ping_tick = post_tick = current_tick;
            ui_logged = false;
            if (!ui_run_path[0])
                button_queue_post(SYS_DEV_RUN, 0);
        } else if (health.ui_pong_tick - ping_tick < 0 &&
                   current_tick - post_tick >= 10 * HZ) {
            /* a screen that ignores system events drops the ping: resend */
            post_tick = current_tick;
            if (!ui_run_path[0])
                button_queue_post(SYS_DEV_RUN, 0);
        }
        if (health.ui_pong_tick - ping_tick < 0 &&
            current_tick - ping_tick > 20 * HZ && !ui_logged) {
            health.ui_late++;
            ui_logged = true;
            dev_log("UI not answering for %ld s", (current_tick - ping_tick) / HZ);
            if (health.ui_thread)
                dev_thread_trace(health.ui_thread);
        }
    }
}

/* Run config unplug=N: the cable comes out N s after USB attached (usb_detect()
 * sees none, the USB screen leaves as for a real unplug); play= plays after
 * that instead of at boot */
static long unplug_stack[DEFAULT_STACK_SIZE / sizeof(long)];
static int unplug_seconds;

static void unplug_thread(void)
{
    char v[MAX_PATH];
    sleep(unplug_seconds * HZ);
    dev_usb_enabled = false;
    dev_log("usb: unplugged");
    do
        sleep(HZ / 10);
    while (get_current_activity() == ACTIVITY_USBSCREEN);
    if (dev_config("play", v, sizeof v))
        ui_run_post("@play:%s", v);
    thread_exit();
}

static void dev_usb(void)
{
    char v[16];
    if (!dev_config("usb", v, sizeof v) || !strcmp(v, "0"))
        return;
    bool storage = !strcmp(v, "storage");
    dev_usb_mode = storage ? USB_MODE_MASS_STORAGE : USB_MODE_CHARGE;
    usb_set_mode(dev_usb_mode);
    sdlog_on = dev_config("sdlog", v, sizeof v) && strcmp(v, "0");
#ifdef USB_ENABLE_SERIAL
    if (!storage) {
        semaphore_init(&sh_sem, 1, 0);
        create_thread(health_thread, health_stack, sizeof health_stack, 0,
                      "dev health" IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));
        dev_serial_owner(
            create_thread(sh_thread, sh_stack, sizeof sh_stack, 0,
                          "dev sh" IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU)));
        usb_set_serial(true);
    }
#endif
    dev_usb_enabled = true;
    dev_log("usb: %s", storage ? "mass storage" : "charge only + serial shell");
    if (dev_config("unplug", v, sizeof v)) {
        unplug_seconds = atoi(v);
        create_thread(unplug_thread, unplug_stack, sizeof unplug_stack, 0,
                      "dev unplug" IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
    }
}

void dev_settings_loaded(void)
{
#ifdef HAVE_BLUETOOTH
    char v[8];
    /* wired tests: a player that pairs at boot sends its audio elsewhere */
    if (dev_config("bluetooth", v, sizeof v) && !strcmp(v, "off")) {
        global_settings.bt_enabled = false;
        dev_log("bluetooth off for this run");
    }
#endif
}

void dev_boot(void)
{
    char v[256], *save;
    dev_target_boot();
    if (dev_config("ls", v, sizeof v))
        for (char *d = strtok_r(v, ",", &save); d; d = strtok_r(NULL, ",", &save))
            list_dir(d);
    if (dev_config("sdtest", v, sizeof v))
        sh_sdtest(v);
    if (dev_config("sh", v, sizeof v))
        for (char *c = strtok_r(v, ";", &save); c; c = strtok_r(NULL, ";", &save)) {
            while (*c == ' ')
                c++;
            char *arg = strchr(c, ' ');
            if (arg)
                *arg++ = '\0';
            else
                arg = c + strlen(c);
            dev_log("sh: %s %s", c, arg);
            if (!strcmp(c, "wait"))
                sleep(atoi(arg) * HZ);
            else if (!sh_run(c, arg))
                dev_log("sh: err %s", c);
        }
    if (dev_config("poweroff", v, sizeof v)) {
        dev_poweroff_dry = strcmp(v, "real") != 0;
        sleep(3 * HZ);
        dev_log("poweroff test: posting SYS_POWEROFF");
        button_queue_post(SYS_POWEROFF, 0);
    }
    if (dev_config("crash", v, sizeof v)) {
        dev_log("crash test: %s", v);
        if (!strcmp(v, "panic"))
            panicf("dev crash test %d", 42);
        __asm__ volatile("udf #0");
    }
    if (dev_config("volume", v, sizeof v)) {
        global_status.volume = volume_from_db(v);
        sound_set_volume(global_status.volume);
        dev_log("volume %s dB", v);
    }
    char unplug[8];
    if (dev_config("play", v, sizeof v) && !dev_config("unplug", unplug, sizeof unplug))
        play(v);
    if (dev_config("monitor", v, sizeof v)) {
        monitor_seconds = atoi(v);
        create_thread(monitor_thread, monitor_stack, sizeof monitor_stack, 0,
                      "dev mon" IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
    }
    if (dev_config("keys", keys_script, sizeof keys_script))
        create_thread(keys_thread, keys_stack, sizeof keys_stack, 0,
                      "dev keys" IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
    dev_usb();
}
