/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: a pocket tracker. Entry point, start menu, files, setup and
 * the editor loop. Design: DESIGN.md.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "rt.h"
#include "lib/configfile.h"
#include "lib/display_text.h"
#include "lib/helper.h"

struct app app;
struct settings cfg = {
    .textsize = 0, .follow = true, .editstep = 0, .smooth = true,
    .beat = 4, .bar = 16, .decimal = false, .newrows = 64,
};

static int cfg_smooth = 1, cfg_follow = 1, cfg_decimal;
static const char *const size_names[] = { "auto", "small", "large" };
static const char *const noyes[] = { "no", "yes" };
static struct configdata config[] = {
    { TYPE_ENUM, 0, 3, { .int_p = &cfg.textsize }, "text size",
      (char **)size_names },
    { TYPE_ENUM, 0, 2, { .int_p = &cfg_follow }, "follow", (char **)noyes },
    { TYPE_INT, 0, 16, { .int_p = &cfg.editstep }, "edit step", NULL },
    { TYPE_ENUM, 0, 2, { .int_p = &cfg_smooth }, "smooth", (char **)noyes },
    { TYPE_INT, 2, 16, { .int_p = &cfg.beat }, "beat", NULL },
    { TYPE_INT, 4, 64, { .int_p = &cfg.bar }, "bar", NULL },
    { TYPE_ENUM, 0, 2, { .int_p = &cfg_decimal }, "decimal", (char **)noyes },
    { TYPE_INT, 16, 256, { .int_p = &cfg.newrows }, "new rows", NULL },
};

static long preview_until;
static bool preview_on;
static bool in_editor;

/* ---- small helpers used by the views ---- */

void app_message(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rb->vsnprintf(app.msg, sizeof(app.msg), fmt, ap);
    va_end(ap);
    app.msg_until = *rb->current_tick + 2 * HZ;
    app.dirty = true;
}

void app_changed(void)
{
    app.modified = true;
    app.dirty = true;
}

void app_set_view(int view)
{
    app.view = view;
    app.hint = false;
    app.dirty = true;
}

void app_preview(int ins, int note)
{
    eng_note(0, ins, note);
    preview_on = true;
    preview_until = *rb->current_tick + HZ * 4 / 10;
    app.last_note = note;
}

void app_play(int mode)
{
    if (mode == PLAY_PATTERN)
        eng_play(PLAY_PATTERN, app.ord, 0);
    else
        eng_play(PLAY_SONG, app.ord, app.view == VIEW_PATTERN ? app.row : 0);
}

void app_play_toggle(void)
{
    if (eng_mode() != PLAY_STOP)
        eng_stop();
    else if (app.view == VIEW_SONG)
        app_play(PLAY_SONG);
    else
        app_play(PLAY_PATTERN);
    app.dirty = true;
}

/* After an undo/redo, show what changed. */
void app_after_undo(void)
{
    if (undo_where.kind == U_CELL) {
        for (int o = 0; o < song.nord; o++)
            if (song.ord[o] == undo_where.a) {
                if (song.ord[app.ord] != undo_where.a)
                    app.ord = o;
                app.row = undo_where.b;
                app.ch = undo_where.c;
                break;
            }
    } else if (undo_where.kind == U_IPARAM) {
        app.cur_ins = undo_where.a;
    }
    pattern_clamp();
    app_changed();
}

bool app_confirm(const char *line1, const char *line2)
{
    const char *lines[] = { line1, line2 };
    const struct text_message msg = { lines, line2 ? 2 : 1 };
    bool yes = rb->gui_syncyesno_run(&msg, NULL, NULL) == YESNO_YES;
    app.dirty = true;
    return yes;
}

static const char *pick_exts;

static bool show_item(char *name, int attr, struct tree_context *tc)
{
    (void)tc;
    if (attr & ATTR_DIRECTORY)
        return true;
    const char *dot = rb->strrchr(name, '.');
    if (!dot)
        return false;
    /* pick_exts: ".xm.mod.mid" */
    const char *p = pick_exts;
    int n = rb->strlen(dot);
    while ((p = rb->strcasestr(p, dot))) {
        if (p[n] == 0 || p[n] == '.')
            return true;
        p++;
    }
    return false;
}

bool app_pick_file(char *path, size_t len, const char *title,
                   const char *exts)
{
    const char *root = path[0] ? path : RT_SONG_DIR;
    char start[MAX_PATH];
    /* a trailing slash opens the folder rather than its parent */
    rb->snprintf(start, sizeof(start), "%s/", rb->dir_exists(root) ? root
                                                                   : "");
    pick_exts = exts;
    struct browse_context browse = {
        .dirfilter = SHOW_ALL,
        .flags = BROWSE_SELECTONLY | BROWSE_NO_CONTEXT_MENU |
                 BROWSE_DIRFILTER,
        .title = (char *)title,
        .icon = Icon_NOICON,
        .root = start,
        .buf = path,
        .bufsize = len,
        .callback_show_item = show_item,
    };
    path[0] = 0;
    rb->rockbox_browse(&browse);
    app.dirty = true;
    rb->lcd_setfont(FONT_SYSFIXED);
    return (browse.flags & BROWSE_SELECTED) && path[0];
}

/* ---- files ---- */

static void reset_cursor(void)
{
    app.ord = app.row = app.ch = 0;
    app.field = 0;
    app.sline = 0;
    app.ifield = 0;
    app.cur_ins = 1;
    app.marking = false;
    app.pen_note = 49;
    app.pen_evvol = 0;
    app.pen_vol = 0x50;
    app.pen_fx = 0;
    app.pen_param = 0x47;
    for (int i = 0; i < MAX_CH; i++)
        eng_mute[i] = false;
    undo_clear();
}

static bool ext_is(const char *path, const char *ext)
{
    const char *dot = rb->strrchr(path, '.');
    return dot && !rb->strcasecmp(dot, ext);
}

/* The demo songs ship read-only: nothing is ever written in their folder. */
static bool in_demo_dir(const char *path)
{
    return !rb->strncasecmp(path, RT_DEMO_DIR "/", sizeof(RT_DEMO_DIR));
}

static bool load_any(const char *path)
{
    char err[48] = "";
    int r;
    eng_stop();
    rb->splash(0, "Loading...");
    if (ext_is(path, ".mod"))
        r = mod_load(path, err, sizeof(err));
    else if (ext_is(path, ".mid") || ext_is(path, ".midi"))
        r = midi_load(path, err, sizeof(err));
    else
        r = xm_load(path, err, sizeof(err));
    reset_cursor();
    app.dirty = true;
    if (r < 0) {
        rb->splashf(2 * HZ, "%s", err[0] ? err : "Cannot load");
        song_new(0);
        app.path[0] = 0;
        app.modified = false;
        return false;
    }
    if (song.nch < 1 || song.nord < 1)
        song_new(0);
    /* A demo or an import has no file to save over: the first save asks
     * for a name in the song folder. */
    bool own = ext_is(path, ".xm") && !in_demo_dir(path);
    rb->strlcpy(app.path, own ? path : "", sizeof(app.path));
    app.modified = !ext_is(path, ".xm");
    return true;
}

void app_open(void)
{
    char path[MAX_PATH] = "";
    if (app.modified && !app_confirm("Open another song?",
                                     "Unsaved changes are lost"))
        return;
    if (app_pick_file(path, sizeof(path), "Open song", ".xm.mod.mid.midi"))
        load_any(path);
}

static bool ask_name(char *path, size_t len, const char *ext)
{
    char name[64];
    const char *slash = rb->strrchr(path, '/');
    if (path[0] && slash) {
        rb->strlcpy(name, slash + 1, sizeof(name));
        char *dot = rb->strrchr(name, '.');
        if (dot)
            *dot = 0;
    } else {
        rb->strlcpy(name, song.name[0] ? song.name : "song", sizeof(name));
    }
    if (rb->kbd_input(name, sizeof(name), NULL) < 0 || !name[0]) {
        app.dirty = true;
        return false;
    }
    app.dirty = true;
    for (char *p = name; *p; p++)               /* a name, not a path */
        if (rb->strchr("\\/:*?\"<>|", *p))
            *p = '_';
    if (!rb->dir_exists(RT_SONG_DIR))
        rb->mkdir(RT_SONG_DIR);
    rb->snprintf(path, len, RT_SONG_DIR "/%s%s", name, ext);
    return !rb->file_exists(path) || !rb->strcasecmp(path, app.path) ||
           app_confirm("Replace the file?", rb->strrchr(path, '/') + 1);
}

bool app_save(bool ask)
{
    char path[MAX_PATH];
    rb->strlcpy(path, app.path, sizeof(path));
    if ((ask || !path[0] || in_demo_dir(path)) &&
        !ask_name(path, sizeof(path), ".xm"))
        return false;
    rb->splash(0, "Saving...");
    if (!rb->dir_exists(RT_SONG_DIR))
        rb->mkdir(RT_SONG_DIR);
    if (xm_save(path) < 0) {
        rb->splash(2 * HZ, "Save failed");
        app.dirty = true;
        return false;
    }
    rb->strlcpy(app.path, path, sizeof(app.path));
    app.modified = false;
    app_message("Saved %s", rb->strrchr(path, '/') + 1);
    return true;
}

static void export_progress(int pct)
{
    rb->splash_progress(pct, 100, "Exporting... (any key stops)");
}

static void export_wav(void)
{
    char path[MAX_PATH];
    rb->strlcpy(path, app.path, sizeof(path));
    if (!ask_name(path, sizeof(path), ".wav"))
        return;
    eng_stop();
    int r = wav_export(path, export_progress);
    rb->splash(2 * HZ, r == 0 ? "WAV saved: it plays in the music player"
                       : r > 0 ? "Export stopped" : "Export failed");
    app.dirty = true;
}

static void export_midi(void)
{
    char path[MAX_PATH];
    rb->strlcpy(path, app.path, sizeof(path));
    if (!ask_name(path, sizeof(path), ".mid"))
        return;
    rb->splash(HZ, midi_save(path) < 0 ? "Export failed" : "MIDI saved");
    app.dirty = true;
}

/* ---- autosave and resume ---- */

static void state_save(void)
{
    if (!rb->dir_exists(RT_DATA_DIR))
        rb->mkdir(RT_DATA_DIR);
    if (xm_save(RT_AUTOSAVE) < 0)
        return;
    int fd = rb->open(RT_STATE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return;
    rb->fdprintf(fd, "%d %d %d %d %d %d %d %d\n%s\n", app.modified,
                 app.view, app.ord, app.row, app.ch, app.field, app.cur_ins,
                 app.sline, app.path);
    rb->close(fd);
}

static bool state_load(void)
{
    if (!load_any(RT_AUTOSAVE))
        return false;
    int fd = rb->open(RT_STATE_FILE, O_RDONLY);
    app.path[0] = 0;
    if (fd >= 0) {
        char line[MAX_PATH + 8];
        int v[8] = { 0 };
        if (rb->read_line(fd, line, sizeof(line)) > 0) {
            const char *p = line;
            for (int i = 0; i < 8 && *p; i++) {
                v[i] = rb->atoi(p);
                while (*p && *p != ' ')
                    p++;
                while (*p == ' ')
                    p++;
            }
        }
        if (rb->read_line(fd, line, sizeof(line)) > 0)
            rb->strlcpy(app.path, line, sizeof(app.path));
        rb->close(fd);
        app.modified = v[0];
        app.view = CLAMP(v[1], 0, VIEW_COUNT - 1);
        app.ord = v[2];
        app.row = v[3];
        app.ch = v[4];
        app.field = v[5];
        app.cur_ins = MAX(1, v[6]);
        app.sline = v[7];
        pattern_clamp();
    }
    return true;
}

/* ---- setup, help, about ---- */

static void apply_cfg(void)
{
    cfg.follow = cfg_follow;
    cfg.decimal = cfg_decimal;
    cfg.smooth = cfg_smooth;
    eng_smooth = cfg.smooth;
    if (cfg.bar % cfg.beat)
        cfg.bar = cfg.beat * 4;
    draw_init(cfg.textsize);
}

void app_setup_menu(void)
{
    static const struct opt_items sizes[] = {
        { "Auto", -1 }, { "Small", -1 }, { "Large", -1 } };
    static const struct opt_items onoff[] = { { "Off", -1 }, { "On", -1 } };
    static const struct opt_items sound[] = {
        { "Crisp", -1 }, { "Smooth", -1 } };
    static const struct opt_items rownums[] = {
        { "Hex", -1 }, { "Decimal", -1 } };
    MENUITEM_STRINGLIST(menu, "Setup", NULL, "Text size", "Follow playback",
                        "Edit step", "Sound", "Beat every", "Bar every",
                        "Row numbers", "New pattern rows");
    int sel = 0;
    bool done = false;
    while (!done) {
        switch (rb->do_menu(&menu, &sel, NULL, false)) {
        case 0:
            rb->set_option("Text size", &cfg.textsize, RB_INT, sizes, 3,
                           NULL);
            break;
        case 1:
            rb->set_option("Follow playback", &cfg_follow, RB_INT, onoff, 2,
                           NULL);
            break;
        case 2:
            rb->set_int("Edit step", "rows", UNIT_INT, &cfg.editstep, NULL,
                        1, 0, 16, NULL);
            break;
        case 3:
            rb->set_option("Sound", &cfg_smooth, RB_INT, sound, 2, NULL);
            break;
        case 4:
            rb->set_int("Beat every", "rows", UNIT_INT, &cfg.beat, NULL, 1,
                        2, 16, NULL);
            break;
        case 5:
            rb->set_int("Bar every", "rows", UNIT_INT, &cfg.bar, NULL,
                        cfg.beat, cfg.beat, 64, NULL);
            break;
        case 6:
            rb->set_option("Row numbers", &cfg_decimal, RB_INT, rownums, 2,
                           NULL);
            break;
        case 7:
            rb->set_int("New pattern rows", "rows", UNIT_INT, &cfg.newrows,
                        NULL, 16, 16, 256, NULL);
            break;
        default:
            done = true;
            break;
        }
    }
    apply_cfg();
    configfile_save(RT_CONFIG_FILE, config, ARRAYLEN(config), 1);
    app.dirty = true;
}

static void show_text(const char *src)
{
    /* display_text() wants a word list: split a copy from the pool */
    size_t len = rb->strlen(src) + 1;
    int max = 1;
    for (const char *q = src; *q; q++)
        max += *q == ' ' || *q == '\n';
    char **words = pool_alloc(max * sizeof(char *) + len);
    if (!words) {
        app_message("Out of memory");
        return;
    }
    char *p = (char *)(words + max);
    rb->memcpy(p, src, len);
    int n = 0;
    while (*p && n < max) {
        if (*p == '\n') {
            *p++ = 0;
            words[n++] = "";
            continue;
        }
        words[n++] = p;
        while (*p && *p != ' ' && *p != '\n')
            p++;
        if (*p == ' ')
            *p++ = 0;
    }
    rb->lcd_setfont(FONT_UI);
    display_text(n, words, NULL, NULL, true);
    rb->lcd_setfont(FONT_SYSFIXED);
    pool_free(words);
    app.dirty = true;
}

static const char help_text[] =
    "RockTrack: three rules\n\n"
    "1. Arrows move.\n"
    "2. Hold Select and press arrows to change what is under the cursor: "
    "Up/Down big steps (octaves), Left/Right small steps. You hear notes "
    "as you change them.\n"
    "3. Tap Select for the obvious thing. Double-tap Select to play or "
    "stop. Hold Select alone, then let go, for the menu grid.\n\n"
    "Pattern: tap Select on an empty field to put the last value used "
    "there (the pen); tap a filled field to lift it into the pen. A "
    "stray tap is undone by tapping again. Below C-0 is OFF (note off). "
    "The column under the cursor opens to show instrument, volume and "
    "effect; the line at the bottom says what the value does. Left of "
    "the first channel are the row numbers: there Select+Up/Down moves "
    "between song positions, Select+Left/Right picks the pattern, a tap "
    "plays from that row. Hold Left or Right to jump channels.\n\n"
    "Song: one line per position with a sketch of its notes. Right or "
    "a tap opens the pattern; Select+arrows choose which pattern plays "
    "there. The menu grid inserts, removes and clones positions.\n\n"
    "Instrument: each sound is a synth (pulse, triangle, saw, sine, "
    "noise, a wave you draw, or drums) or a WAV sample, with attack, "
    "decay, sustain and release. Tap Select to hear it. Double-tap to "
    "hear it in the song.\n\n"
    "Songs are FastTracker 2 XM files in /RockTrack: they open in "
    "MilkyTracker, OpenMPT and other trackers. RockTrack also opens MOD "
    "and MIDI files and exports WAV (it plays in the music player) and "
    "MIDI. The demo songs never change: saving one asks for a new name. "
    "Your work is kept when you quit: Resume picks it up again."
#if CONFIG_KEYPAD == YP3_PAD
    " Up+Left together goes back to the start menu."
#endif
#ifdef HAVE_BLUETOOTH_HID
    "\n\nWith a game controller: A is Select, B Back, X play/stop, Y "
    "marks a block, Options opens the menu grid, PS goes back to the "
    "start menu."
#endif
    ;

static const char about_text[] =
    "RockTrack 1.0\n"
    "A pocket tracker for Rockbox.\n\n"
    "Code: GPL v2 or later, RockTrack contributors.\n"
    "Pixel font and chip synth: original, same licence.\n"
    "Demo songs: compositions in the public domain; arrangements and "
    "original tracks by RockTrack contributors, CC0 1.0. Credits: "
    "/.rockbox/rocks/apps/rocktrack/README.txt\n"
    "Chiptunes by Drozerix (public domain), ant512, Djego Flochs and "
    "M4kachu (CC BY 4.0), from The Mod Archive: see THIRD_PARTY.txt "
    "in the same folder.\n\n"
    "Song format: FastTracker 2 Extended Module (XM) by Fredrik Huss "
    "(Mr.H), Triton.";

static void mute_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Channels", NULL, "Mute/unmute this channel",
                        "Solo this channel", "Unmute all");
    int sel = 0;
    switch (rb->do_menu(&menu, &sel, NULL, false)) {
    case 0:
        eng_mute[app.ch] = !eng_mute[app.ch];
        break;
    case 1:
        for (int i = 0; i < MAX_CH; i++)
            eng_mute[i] = i != app.ch;
        break;
    case 2:
        for (int i = 0; i < MAX_CH; i++)
            eng_mute[i] = false;
        break;
    }
    app.dirty = true;
}

static void pattern_length(void)
{
    int pat = song.ord[app.ord];
    int rows = song.pat[pat].rows;
    rb->set_int("Pattern rows", "rows", UNIT_INT, &rows, NULL, 1, 1,
                MAX_ROWS, NULL);
    if (rows != song.pat[pat].rows) {
        if (!pattern_resize(pat, rows))
            app_message("Out of memory");
        undo_clear();
        pattern_clamp();
        app_changed();
    }
}

void app_more_menu(void)
{
    MENUITEM_STRINGLIST(menu, "More", NULL, "Save as...", "Export WAV",
                        "Export MIDI", "Pattern length", "Channels",
                        "New song", "Demo songs", "Setup", "Help",
                        "About");
    int sel = 0;
    switch (rb->do_menu(&menu, &sel, NULL, false)) {
    case 0: app_save(true); break;
    case 1: export_wav(); break;
    case 2: export_midi(); break;
    case 3: pattern_length(); break;
    case 4: mute_menu(); break;
    case 5: app_new_song(); break;
    case 6: app_demos(); break;
    case 7: app_setup_menu(); break;
    case 8: show_text(help_text); break;
    case 9: show_text(about_text); break;
    }
    app.dirty = true;
}

void app_hub(void)
{
    hub_run();
}

bool app_new_song(void)
{
    MENUITEM_STRINGLIST(menu, "New song", NULL, "Chip, 4 channels",
                        "Chip, 8 channels", "Empty");
    int sel = 0;
    int r = rb->do_menu(&menu, &sel, NULL, false);
    app.dirty = true;
    if (r < 0 || r > 2)
        return false;
    if (app.modified && in_editor &&
        !app_confirm("Start a new song?", "Unsaved changes are lost"))
        return false;
    eng_stop();
    song_new(r);
    reset_cursor();
    app.path[0] = 0;
    app.modified = false;
    app.view = VIEW_PATTERN;
    return true;
}

bool app_demos(void)
{
    char path[MAX_PATH];
    rb->strlcpy(path, RT_DEMO_DIR, sizeof(path));
    if (!rb->dir_exists(path)) {
        rb->splash(2 * HZ, "No demo songs installed");
        return false;
    }
    if (app.modified && in_editor &&
        !app_confirm("Open a demo song?", "Unsaved changes are lost"))
        return false;
    if (!app_pick_file(path, sizeof(path), "Demo songs", ".xm.mod.mid"))
        return false;
    return load_any(path);
}

/* ---- the editor loop ---- */

static void draw(void)
{
    switch (app.view) {
    case VIEW_SONG: songview_draw(); break;
    case VIEW_INSTR: instr_draw(); break;
    default: pattern_draw(); break;
    }
    draw_update();
    app.dirty = false;
}

static void handle(const struct ev *ev)
{
    switch (ev->type) {
    case EV_SYS:
        if (ev->sys == SYS_POWEROFF || ev->sys == SYS_REBOOT)
            state_save();
        if (rb->default_event_handler(ev->sys) == SYS_USB_CONNECTED) {
            app.usb = true;
            app.quit = true;
        }
        app.dirty = true;
        return;
    case EV_HINT:
        app.hint = true;
        break;
    case EV_MENU:
        app.hint = false;
        app_hub();
        break;
    case EV_LEAVE:
        app.quit = true;
        return;
    case EV_PLAY:
        app_play_toggle();
        break;
    default:
        if (ev->type == EV_MOVE && ev->dir == DIR_LEFT &&
            app.view == VIEW_PATTERN && app.field < 0) {
            app.sline = app.ord;            /* Left from the row numbers */
            app_set_view(VIEW_SONG);
            break;
        }
        app.hint = false;
        switch (app.view) {
        case VIEW_SONG: songview_event(ev); break;
        case VIEW_INSTR: instr_event(ev); break;
        default: pattern_event(ev); break;
        }
        break;
    }
    app.last_key_tick = *rb->current_tick;
    app.dirty = true;
}

static void editor(void)
{
    int last_ord = -1, last_row = -1;
    in_editor = true;
    app.quit = false;
    app.dirty = true;
    input_reset();
    while (!app.quit) {
        bool playing = eng_mode() != PLAY_STOP;
        long now = *rb->current_tick;
        if (playing) {
            int o, r;
            eng_pos(&o, &r);
            if (o != last_ord || r != last_row) {
                last_ord = o;
                last_row = r;
                if (cfg.follow && TIME_AFTER(now, app.last_key_tick + 2 * HZ)
                    && !input_select_held()) {
                    if (app.view == VIEW_PATTERN && !app.marking)
                        pattern_follow();
                    else if (app.view == VIEW_SONG)
                        app.sline = o;
                }
                app.dirty = true;
            }
            rb->reset_poweroff_timer();
        }
        /* meters fall back */
        bool meters = false;
        for (int i = 0; i < song.nch; i++)
            if (eng_peak[i]) {
                eng_peak[i] = eng_peak[i] * 3 / 4;
                meters = true;
            }
        if (meters && app.view == VIEW_PATTERN)
            app.dirty = true;
        if (preview_on && TIME_AFTER(now, preview_until) &&
            !input_select_held()) {
            eng_release(0);
            preview_on = false;
        }
        if (app.msg[0] && TIME_AFTER(now, app.msg_until)) {
            app.msg[0] = 0;
            app.dirty = true;
        }
        if (app.dirty)
            draw();
        int timeout = playing || meters || preview_on || app.msg[0]
                      ? HZ / 12 : -1;
        struct ev ev;
        while (input_get(&ev, timeout)) {
            handle(&ev);
            if (app.quit)
                break;
            timeout = 0;    /* drain what is queued, then redraw */
        }
    }
    in_editor = false;
}

/* ---- start menu ---- */

static int start_menu(void)
{
    bool resume = rb->file_exists(RT_AUTOSAVE);
    MENUITEM_STRINGLIST(menu, "RockTrack", NULL, "Resume", "New song",
                        "Open...", "Demo songs", "Setup", "Help", "About",
                        "Quit");
    int sel = resume ? 0 : 1;
    while (true) {
        int r = rb->do_menu(&menu, &sel, NULL, false);
        switch (r) {
        case 0:
            if (!resume) {
                rb->splash(HZ, "Nothing to resume yet");
                break;
            }
            if (state_load())
                return 1;
            break;
        case 1:
            if (app_new_song())
                return 1;
            break;
        case 2: {
            char path[MAX_PATH] = "";
            if (app_pick_file(path, sizeof(path), "Open song",
                              ".xm.mod.mid.midi") && load_any(path))
                return 1;
            break;
        }
        case 3:
            if (app_demos())
                return 1;
            break;
        case 4:
            app_setup_menu();
            break;
        case 5:
            show_text(help_text);
            break;
        case 6:
            show_text(about_text);
            break;
        case MENU_ATTACHED_USB:
            app.usb = true;
            return 0;
        default:
            return 0;
        }
    }
}

enum plugin_status plugin_start(const void *parameter)
{
    size_t asize = 0, bsize = 0;
    void *a = rb->plugin_get_buffer(&asize);
    rb->audio_stop();
    void *b = rb->plugin_get_audio_buffer(&bsize);
    pool_init(a, asize, b, bsize);

    configfile_load(RT_CONFIG_FILE, config, ARRAYLEN(config), 1);
    apply_cfg();
    eng_init();
    song_new(0);
    reset_cursor();
    eng_open();
    backlight_ignore_timeout();

    bool direct = parameter && load_any(parameter);
    while (direct || start_menu()) {
        direct = false;
        editor();
        eng_stop();
        state_save();
        if (app.usb)
            break;
    }
    eng_close();
    backlight_use_settings();
    configfile_save(RT_CONFIG_FILE, config, ARRAYLEN(config), 1);
    return app.usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
