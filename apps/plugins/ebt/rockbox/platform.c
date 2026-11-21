/* EBT Tracker player for Rockbox: song browser, audio, keys, menu.
 * Upstream (synth, player, song format): EBT by shiru8bit, WTFPL.
 *
 * Songs are the .ebt files of EBT in /.rockbox/rocks/apps/ebt/. The mixer
 * beep channel plays the synth.
 * Keys: LEFT/RIGHT choose a channel, SELECT toggles its mute, UP/DOWN
 * previous/next song (on release), hold SELECT+UP for 500 ms for the menu. */
#include "plugin.h"
#include "lib/helper.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"
#include "ebt_types.h"
#include "ebt_view.h"

#define EBT_DIR PLUGIN_APPS_DIR "/ebt"
#define MAX_SONGS 48
#define NAME_LEN 28
#define BLOCK_FRAMES 512

static char songs[MAX_SONGS][NAME_LEN];
static int song_count, song_cur = -1;
static int song_fd = -1;
static unsigned mute;
static int sel_ch;
static bool playing, paused;
static bool resume_music;

static int16_t pcm[BLOCK_FRAMES * 2];
static int pcm_play;

/* ------------------------------------------------------------ song files */

static char file_buf[256];
static int file_len, file_pos;

static void ebt_file_reset(void)
{
    file_len = file_pos = 0;
}

const char *ebt_file_get_line(void)
{
    static char line[96];
    int n = 0;
    if (song_fd < 0)
        return NULL;
    for (;;) {
        if (file_pos >= file_len) {
            file_len = rb->read(song_fd, file_buf, sizeof(file_buf));
            file_pos = 0;
            if (file_len <= 0) {
                file_len = 0;
                if (n == 0)
                    return NULL;
                break;
            }
        }
        char c = file_buf[file_pos++];
        if (c == '\n')
            break;
        if (c != '\r' && n < (int)sizeof(line) - 1)
            line[n++] = c;
    }
    line[n] = '\0';
    return line;
}

static void scan_songs(void)
{
    DIR *d = rb->opendir(EBT_DIR);
    struct dirent *e;
    song_count = 0;
    if (!d)
        return;
    while (song_count < MAX_SONGS && (e = rb->readdir(d))) {
        int len = rb->strlen(e->d_name);
        if (len > 4 && len < NAME_LEN &&
            !rb->strcasecmp(e->d_name + len - 4, ".ebt"))
            rb->strlcpy(songs[song_count++], e->d_name, NAME_LEN);
    }
    rb->closedir(d);
    /* directory order is arbitrary: sort by name */
    for (int i = 1; i < song_count; i++) {
        char t[NAME_LEN];
        int j = i;
        rb->strlcpy(t, songs[i], NAME_LEN);
        while (j > 0 && rb->strcasecmp(songs[j - 1], t) > 0) {
            rb->strlcpy(songs[j], songs[j - 1], NAME_LEN);
            j--;
        }
        rb->strlcpy(songs[j], t, NAME_LEN);
    }
}

/* ----------------------------------------------------------------- audio */

static void audio_more(const void **start, size_t *size)
{
    ebt_core_render(pcm, BLOCK_FRAMES);
    *start = pcm;
    *size = sizeof(pcm);
}

static void pcm_begin(void)
{
    static const struct mixer_play_cbs cbs = { .get_more = audio_more };
    if (pcm_play)
        return;
    rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &cbs, NULL, 0);
    pcm_play = 1;
}

static void pcm_halt(void)
{
    if (pcm_play)
        rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
    pcm_play = 0;
}

static bool load_song(int i)
{
    char path[MAX_PATH];
    bool ok;
    pcm_halt();
    rb->snprintf(path, sizeof(path), EBT_DIR "/%s", songs[i]);
    song_fd = rb->open(path, O_RDONLY);
    if (song_fd < 0)
        return false;
    ebt_file_reset();
    ok = ebt_core_load();
    rb->close(song_fd);
    song_fd = -1;
    if (!ok) {
        rb->splash(HZ * 2, "Not an EBT song");
        return false;
    }
    song_cur = i;
    mute = 0;
    ebt_core_start(rb->mixer_get_frequency(), mute);
    paused = false;
    playing = true;
    pcm_begin();
    return true;
}

/* --------------------------------------------------------------- drawing */

void view_fill(int x, int y, int w, int h, uint32_t rgb)
{
    rb->lcd_set_foreground(LCD_RGBPACK(rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff));
    rb->lcd_fillrect(x, y, w, h);
}

void view_text(int x, int y, uint32_t rgb, const char *s)
{
    rb->lcd_set_foreground(LCD_RGBPACK(rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff));
    rb->lcd_putsxy(x, y, s);
}

static void redraw(void)
{
    struct ebt_status st;
    ebt_core_status(&st);
    ebt_view_draw(&st, sel_ch, mute, paused);
    rb->lcd_update();
}

/* ----------------------------------------------------------------- menus */

static void show_text(const char *text)
{
    static char buf[480];
    static char *words[96];
    int n = 0;
    char *p;
    rb->strlcpy(buf, text, sizeof(buf));
    for (p = buf; *p && n < (int)ARRAYLEN(words) - 1;) {
        words[n++] = p;
        while (*p && *p != ' ' && *p != '\n')
            p++;
        if (*p == '\n') {
            *p++ = '\0';
            words[n++] = (char *)"";
        } else if (*p) {
            *p++ = '\0';
        }
    }
    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_UI);
    display_text(n, words, NULL, NULL, true);
    rb->lcd_setfont(FONT_SYSFIXED);
}

static const char *song_name(int item, void *data, char *buf, size_t len)
{
    (void)data; (void)buf; (void)len;
    return songs[item];
}

/* Returns the chosen song or -1. */
static int choose_song(void)
{
    struct simplelist_info info;
    if (!song_count) {
        show_text("No songs found. Copy .ebt files into\n"
                  "/.rockbox/rocks/apps/ebt/");
        return -1;
    }
    rb->simplelist_info_init(&info, "Songs", song_count, NULL);
    info.get_name = song_name;
    info.selection = song_cur < 0 ? 0 : song_cur;
    if (rb->simplelist_show_list(&info))
        return -2;
    return info.selection;
}

static void help(void)
{
    show_text("EBT plays four-channel chiptune songs (.ebt files from "
              "the EBT tracker). LEFT and RIGHT pick a channel, SELECT "
              "mutes it, UP and DOWN change song, hold SELECT and UP "
              "together for half a second for this menu. Music playback stops while "
              "the plugin runs. The tracker editor is not included.");
}

static void about(void)
{
    show_text("EBT Tracker player.\nEBT by shiru8bit (shiru@mail.ru), "
              "shiru.untergrund.net\ngithub.com/ESPboy-edu/ESPboy_EBT_Tracker\n"
              "License: WTFPL (full text: LICENSE.txt in rocks/apps/ebt/). "
              "Songs by their authors; two demo songs by others were removed (no licence).\nRockbox port for the YP3 player.");
}

enum { M_PLAY, M_SONGS, M_HELP, M_ABOUT, M_PLAYBACK, M_QUIT };

/* Returns true to quit the plugin; sets *usb when USB was connected. */
static bool menu(bool *usb)
{
    MENUITEM_STRINGLIST(m, "EBT Tracker", NULL, "Play / Resume", "Songs",
                        "Help", "About", "Playback Control", "Quit");
    int sel = 0;
    for (;;) {
        switch (rb->do_menu(&m, &sel, NULL, false)) {
        case M_PLAY:
            if (song_cur < 0) {
                int s = choose_song();
                if (s == -2) { *usb = true; return true; }
                if (s < 0 || !load_song(s))
                    break;
            }
            return false;
        case M_SONGS: {
            int s = choose_song();
            if (s == -2) { *usb = true; return true; }
            if (s >= 0 && load_song(s))
                return false;
            break;
        }
        case M_HELP: help(); break;
        case M_ABOUT: about(); break;
        case M_PLAYBACK:
            if (playback_control(NULL)) { *usb = true; return true; }
            break;
        case MENU_ATTACHED_USB:
            *usb = true;
            return true;
        case M_QUIT:
        case GO_TO_PREVIOUS:
            return true;
        }
    }
}

/* ------------------------------------------------------------------ main */

static void player_loop(bool *usb, bool *quit)
{
    bool sel_repeated = false;   /* SELECT was held long or used in a chord */
    bool up_pending = false;     /* UP pressed alone: previous song on release */
    long hold_start = 0;
    long last_draw = 0;

    rb->lcd_set_viewport(NULL);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    redraw();
    for (;;) {
        long ev = rb->button_get_w_tmo(HZ / 12);
        bool open_menu = false;
        int st = rb->button_status();

        switch (ev) {
        case SYS_POWEROFF:
            break; /* SELECT is a game key here: do not power off */
        case BUTTON_LEFT: case BUTTON_LEFT | BUTTON_REPEAT:
            sel_ch = (sel_ch + MAX_CHANNELS - 1) % MAX_CHANNELS;
            break;
        case BUTTON_RIGHT: case BUTTON_RIGHT | BUTTON_REPEAT:
            sel_ch = (sel_ch + 1) % MAX_CHANNELS;
            break;
        case BUTTON_UP:
            if (st & BUTTON_SELECT)
                sel_repeated = true;  /* part of the menu chord */
            else
                up_pending = true;
            break;
        case BUTTON_UP | BUTTON_REL:
            if (up_pending && song_count &&
                !load_song((song_cur + song_count - 1) % song_count))
                playing = false;
            up_pending = false;
            break;
        case BUTTON_DOWN:
            if (song_count && !load_song((song_cur + 1) % song_count))
                playing = false;
            break;
        case BUTTON_SELECT:
            sel_repeated = up_pending;
            up_pending = false;
            break;
        case BUTTON_SELECT | BUTTON_REPEAT:
            sel_repeated = true;
            break;
        case BUTTON_SELECT | BUTTON_REL:
            if (!sel_repeated) {
                mute ^= 1u << sel_ch;
                ebt_core_set_mute(mute);
            }
            sel_repeated = false;
            break;
#ifdef HAVE_BLUETOOTH_HID
        case BUTTON_PAD_A:
            mute ^= 1u << sel_ch;
            ebt_core_set_mute(mute);
            break;
        case BUTTON_PAD_L1:
            if (song_count &&
                !load_song((song_cur + song_count - 1) % song_count))
                playing = false;
            break;
        case BUTTON_PAD_R1:
            if (song_count && !load_song((song_cur + 1) % song_count))
                playing = false;
            break;
        case BUTTON_PAD_MENU:
            open_menu = true;
            break;
        case BUTTON_PAD_HOME:
            *quit = true;
            goto out;
#endif
        default:
            if (rb->default_event_handler(ev) == SYS_USB_CONNECTED) {
                *usb = true;
                *quit = true;
                goto out;
            }
            break;
        }
        if ((st & (BUTTON_SELECT | BUTTON_UP)) == (BUTTON_SELECT | BUTTON_UP)) {
            if (!hold_start)
                hold_start = *rb->current_tick;
            else if (TIME_AFTER(*rb->current_tick, hold_start + HZ / 2)) {
                hold_start = 0;
                open_menu = true;
            }
        } else {
            hold_start = 0;
        }
        if (open_menu) {
            pcm_halt();
            if (menu(usb)) {
                *quit = true;
                goto out;
            }
            rb->button_clear_queue();
            sel_repeated = true;  /* no mute toggle on the release that ends the hold */
            up_pending = false;
            if (playing)
                pcm_begin();
            rb->lcd_set_viewport(NULL);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            redraw();
        }
        if (*rb->current_tick - last_draw >= HZ / 12) {
            last_draw = *rb->current_tick;
            redraw();
        }
    }
out:
    pcm_halt();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

enum plugin_status plugin_start(const void *parameter)
{
    bool usb = false, quit = false;
    (void)parameter;

    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_setfont(FONT_SYSFIXED);
    backlight_ignore_timeout();
    scan_songs();

    /* the synth uses the mixer beep channel; keep music from fighting it */
    resume_music = (rb->audio_status() & AUDIO_STATUS_PLAY) &&
                   !(rb->audio_status() & AUDIO_STATUS_PAUSE);
    if (resume_music)
        rb->audio_pause();

    if (!menu(&usb))
        player_loop(&usb, &quit);
    pcm_halt();
    if (resume_music && !usb)
        rb->audio_resume();
    backlight_use_settings();
    return usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
