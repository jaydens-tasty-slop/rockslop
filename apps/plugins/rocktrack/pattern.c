/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: the pattern screen. The song is one continuous timeline;
 * the cursor's channel opens to all its fields ("fisheye"), the others
 * get whatever detail fits the screen.
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

enum { F_GUTTER = -1, F_NOTE, F_INS, F_VOL, F_FX, F_PARAM, F_COUNT };

static const char note_names[12][3] = {
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-",
};
static const char hexd[] = "0123456789ABCDEF";
static const char fx_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
/* effect types in edit order */
static const uint8_t fx_order[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 20, 21, 25, 27, 29, 33,
};
static const char vol_cmd[] = "-+DUSVPLRM";

/* ---- formatting ---- */

void fmt_note(char *b, int note)
{
    if (note == NOTE_OFF) {
        rb->strcpy(b, "OFF");
    } else if (note >= 1 && note <= 96) {
        b[0] = note_names[(note - 1) % 12][0];
        b[1] = note_names[(note - 1) % 12][1];
        b[2] = '0' + (note - 1) / 12;
        b[3] = 0;
    } else {
        rb->strcpy(b, "\x01\x01\x01");
    }
}

void fmt_hex2(char *b, int v)
{
    b[0] = hexd[(v >> 4) & 15];
    b[1] = hexd[v & 15];
    b[2] = 0;
}

void fmt_vol(char *b, int v)
{
    if (!v) {
        rb->strcpy(b, "\x01\x01");
    } else if (v >= 0x10 && v <= 0x50) {
        fmt_hex2(b, v - 0x10);
    } else if (v >= 0x60) {
        b[0] = vol_cmd[(v >> 4) - 6];
        b[1] = hexd[v & 15];
        b[2] = 0;
    } else {
        rb->strcpy(b, "??");
    }
}

void fmt_fx(char *b, int fx, int param)
{
    if (!fx && !param) {
        rb->strcpy(b, "\x01\x01\x01");
        return;
    }
    b[0] = fx_chars[fx % 36];
    fmt_hex2(b + 1, param);
}

void describe_vol(char *b, size_t len, int v)
{
    static const char *const names[10] = {
        "Volume slide down", "Volume slide up", "Fine volume down",
        "Fine volume up", "Vibrato speed", "Vibrato", "Pan",
        "Pan slide left", "Pan slide right", "Glide",
    };
    if (!v)
        rb->strlcpy(b, "No volume", len);
    else if (v >= 0x10 && v <= 0x50)
        rb->snprintf(b, len, "Volume %d of 64", v - 0x10);
    else if (v >= 0x60)
        rb->snprintf(b, len, "%s %d", names[(v >> 4) - 6], v & 15);
    else
        rb->strlcpy(b, "Unused value", len);
}

void describe_fx(char *b, size_t len, int fx, int p)
{
    int x = p >> 4, y = p & 15;
    switch (fx) {
    case 0:
        if (!p)
            rb->strlcpy(b, "No effect", len);
        else
            rb->snprintf(b, len, "Arpeggio +%d +%d%s", x, y,
                         (x == 4 && y == 7) ? " major" :
                         (x == 3 && y == 7) ? " minor" : "");
        break;
    case 1: rb->snprintf(b, len, "Slide up %d", p); break;
    case 2: rb->snprintf(b, len, "Slide down %d", p); break;
    case 3: rb->snprintf(b, len, "Glide to note, speed %d", p); break;
    case 4: rb->snprintf(b, len, "Vibrato speed %d depth %d", x, y); break;
    case 5: rb->snprintf(b, len, "Glide + volume slide"); break;
    case 6: rb->snprintf(b, len, "Vibrato + volume slide"); break;
    case 7: rb->snprintf(b, len, "Tremolo speed %d depth %d", x, y); break;
    case 8:
        rb->snprintf(b, len, "Pan %s%d", p < 128 ? "L" : p > 128 ? "R" : "",
                     ABS(p - 128));
        break;
    case 9: rb->snprintf(b, len, "Start sample at %d", p * 256); break;
    case 10:
        if (x)
            rb->snprintf(b, len, "Volume slide up %d", x);
        else
            rb->snprintf(b, len, "Volume slide down %d", y);
        break;
    case 11: rb->snprintf(b, len, "Jump to position %d", p); break;
    case 12: rb->snprintf(b, len, "Set volume %d", MIN(p, 64)); break;
    case 13: rb->snprintf(b, len, "Next pattern at row %d", x * 10 + y); break;
    case 14: {
        static const char *const e[16] = {
            "Unused", "Fine slide up", "Fine slide down", "Glissando",
            "Vibrato wave", "Finetune", "Loop", "Tremolo wave", "Unused",
            "Retrigger every", "Fine volume up", "Fine volume down",
            "Cut note at tick", "Delay note by", "Repeat row",
            "Unused",
        };
        if (x == 6 && !y)
            rb->strlcpy(b, "Loop start", len);
        else
            rb->snprintf(b, len, "%s %d", e[x], y);
        break;
    }
    case 15:
        if (p >= 0x20)
            rb->snprintf(b, len, "Tempo %d BPM", p);
        else
            rb->snprintf(b, len, "Speed %d ticks per row", p);
        break;
    case 16: rb->snprintf(b, len, "Global volume %d", MIN(p, 64)); break;
    case 17: rb->snprintf(b, len, "Global volume slide"); break;
    case 20: rb->snprintf(b, len, "Key off at tick %d", p); break;
    case 21: rb->snprintf(b, len, "Envelope position %d", p); break;
    case 25: rb->snprintf(b, len, "Pan slide"); break;
    case 27: rb->snprintf(b, len, "Retrigger every %d, vol %d", y, x); break;
    case 29: rb->snprintf(b, len, "Tremor on %d off %d", x + 1, y + 1); break;
    case 33: rb->snprintf(b, len, "Extra fine slide %s %d",
                          x == 1 ? "up" : "down", y); break;
    default: rb->strlcpy(b, "Unknown effect", len); break;
    }
}

/* ---- song positions ---- */

/* Step (ord, row) by d rows across order positions; false off the song. */
static bool pos_step(int *ord, int *row, int d)
{
    int o = *ord, r = *row;
    while (d > 0) {
        int rows = pattern_rows(o);
        if (r + d < rows) {
            r += d;
            d = 0;
        } else {
            d -= rows - r;
            if (++o >= song.nord)
                return false;
            r = 0;
        }
    }
    while (d < 0) {
        if (r + d >= 0) {
            r += d;
            d = 0;
        } else {
            d += r + 1;
            if (--o < 0)
                return false;
            r = pattern_rows(o) - 1;
        }
    }
    *ord = o;
    *row = r;
    return true;
}

void pattern_clamp(void)
{
    app.ord = CLAMP(app.ord, 0, song.nord - 1);
    app.row = CLAMP(app.row, 0, pattern_rows(app.ord) - 1);
    app.ch = CLAMP(app.ch, 0, song.nch - 1);
    app.field = CLAMP(app.field, F_GUTTER, F_PARAM);
}

static int cur_pat(void)
{
    return song.ord[app.ord];
}

/* ---- layout ---- */

#define MAX_VIS MAX_CH

static struct {
    int gutter_w, gap, sep;
    int head_h, chan_y, chan_h, grid_y, nrows, center, foot_y;
    int first, nvis;
    int level[MAX_VIS];
    int sx[MAX_VIS], sw[MAX_VIS];   /* slot incl. separator and padding */
    int x[MAX_VIS], w[MAX_VIS];     /* content */
} L;

static int win_first;

static int level_w(int lv)
{
    static const int chars[4] = { 3, 5, 7, 10 };
    return chars[lv] * fnt.w + lv * L.gap;
}

static int field_x(int f)
{
    static const int chars[5] = { 0, 3, 5, 7, 8 };
    static const int gaps[5] = { 0, 1, 2, 3, 3 };
    return chars[f] * fnt.w + gaps[f] * L.gap;
}

static int fits(int first, int n, int cc, int lv_other)
{
    int avail = LCD_WIDTH - L.gutter_w, w = 0;
    for (int c = first; c < first + n; c++)
        w += L.sep + level_w(c == cc ? 3 : lv_other);
    return w <= avail;
}

static void layout(void)
{
    bool small = fnt.w <= 4;
    L.gap = small ? 1 : 2;
    L.sep = small ? 2 : 3;
    L.gutter_w = 2 * fnt.w + (small ? 2 : 4);
    L.head_h = fnt.line + 1;
    L.chan_y = L.head_h;
    L.chan_h = fnt.h + 4;
    L.grid_y = L.chan_y + L.chan_h;
    L.foot_y = LCD_HEIGHT - fnt.line - 1;
    L.nrows = MAX(1, (L.foot_y - L.grid_y) / fnt.line);
    L.center = L.nrows / 2;

    int nch = song.nch, cc = app.ch;
    /* window of channels: keep the previous start while the cursor fits */
    if (win_first > cc)
        win_first = cc;
    while (win_first < cc && !fits(win_first, cc - win_first + 1, cc, 0))
        win_first++;
    int n = cc - win_first + 1;
    while (win_first + n < nch && fits(win_first, n + 1, cc, 0))
        n++;
    while (win_first > 0 && fits(win_first - 1, n + 1, cc, 0)) {
        win_first--;
        n++;
    }
    L.first = win_first;
    L.nvis = n;
    int lv = 0;
    while (lv < 3 && fits(L.first, n, cc, lv + 1))
        lv++;
    int used = 0;
    for (int i = 0; i < n; i++) {
        L.level[i] = L.first + i == cc ? 3 : lv;
        used += L.sep + level_w(L.level[i]);
    }
    /* spread what is left so the grid fills the width */
    int extra = LCD_WIDTH - L.gutter_w - used;
    int x = L.gutter_w;
    for (int i = 0; i < n; i++) {
        int e = extra / (n - i);
        extra -= e;
        L.sx[i] = x;
        L.sw[i] = L.sep + level_w(L.level[i]) + e;
        L.x[i] = x + L.sep + e / 2;
        L.w[i] = level_w(L.level[i]);
        x += L.sw[i];
    }
}

/* ---- drawing ---- */

static bool in_mark(int ord, int row, int ch)
{
    if (!app.marking || ord != app.mark_ord)
        return false;
    int r0 = MIN(app.mark_row0, app.row), r1 = MAX(app.mark_row0, app.row);
    int c0 = MIN(app.mark_ch0, app.ch), c1 = MAX(app.mark_ch0, app.ch);
    return row >= r0 && row <= r1 && ch >= c0 && ch <= c1;
}

static void draw_field(int x, int y, const char *s, int n, unsigned c,
                       bool cursor)
{
    if (cursor) {
        fill(x - 1, y - 1, n * fnt.w + 1, fnt.h + 2, colour[C_CURSOR]);
        c = colour[C_CURTEXT];
    }
    textn(x, y, s, n, c);
}

static void draw_cell(int i, int y, const struct cell *e, bool cur_line,
                      bool muted)
{
    char b[8];
    int x = L.x[i], lv = L.level[i];
    int ch = L.first + i;
    bool cur_ch = cur_line && ch == app.ch;
    unsigned dim = colour[C_DIM];
    static const struct cell blank;
    if (!e)
        e = &blank;

    fmt_note(b, e->note);
    unsigned nc = !e->note ? dim : e->note == NOTE_OFF ? colour[C_OFF]
                : ins_colour(e->ins);
    draw_field(x, y, b, 3, muted && e->note ? dim : nc,
               cur_ch && app.field == F_NOTE);
    if (lv >= 1) {
        if (e->ins)
            fmt_hex2(b, e->ins);
        else
            rb->strcpy(b, "\x01\x01");
        draw_field(x + field_x(F_INS), y, b, 2,
                   e->ins && !muted ? ins_colour(e->ins) : dim,
                   cur_ch && app.field == F_INS);
    }
    if (lv >= 2) {
        fmt_vol(b, e->vol);
        draw_field(x + field_x(F_VOL), y, b, 2,
                   e->vol && !muted ? colour[C_VOL] : dim,
                   cur_ch && app.field == F_VOL);
    }
    if (lv >= 3) {
        fmt_fx(b, e->fx, e->param);
        bool has = e->fx || e->param;
        draw_field(x + field_x(F_FX), y, b, 1,
                   has && !muted ? colour[C_FX] : dim,
                   cur_ch && app.field == F_FX);
        draw_field(x + field_x(F_PARAM), y, b + 1, 2,
                   has && !muted ? colour[C_PARAM] : dim,
                   cur_ch && app.field == F_PARAM);
    }
}

static void draw_channel_strip(void)
{
    int y = L.chan_y;
    fill(0, y, LCD_WIDTH, L.chan_h, colour[C_HEAD]);
    for (int i = 0; i < L.nvis; i++) {
        int ch = L.first + i;
        char b[4];
        rb->snprintf(b, sizeof(b), "%d", ch + 1);
        unsigned c = ch == app.ch ? colour[C_ACCENT] : colour[C_GUTTER];
        int x = L.x[i];
        int tw = text(x, y + 1, b, eng_mute[ch] ? colour[C_DIM] : c);
        if (eng_mute[ch])
            text(x + tw + 1, y + 1, "\x11", colour[C_WARN]);
        /* level meter under the number */
        int mw = L.w[i];
        int lvl = eng_peak[ch] * mw / 255;
        int my = y + fnt.h + 2;
        fill(x, my, mw, 1, colour[C_PANEL]);
        fill(x, my, lvl, 1, lvl > mw * 4 / 5 ? colour[C_WARN]
                            : colour[C_VOL]);
    }
    if (L.first > 0)
        text(0, y + 1, "\x06", colour[C_ACCENT]);
    if (L.first + L.nvis < song.nch)
        text(LCD_WIDTH - fnt.w, y + 1, "\x07", colour[C_ACCENT]);
}

void pattern_draw(void)
{
    layout();
    fill(0, 0, LCD_WIDTH, LCD_HEIGHT, colour[C_BG]);
    header_draw(0, L.head_h);
    draw_channel_strip();

    int play_ord = -1, play_row = -1;
    if (eng_mode() != PLAY_STOP)
        eng_pos(&play_ord, &play_row);

    /* the top line's song position; lines before the song stay empty */
    int o = app.ord, r = app.row, back = 0;
    while (back < L.center && pos_step(&o, &r, -1))
        back++;
    int k0 = L.center - back;
    int ty = (fnt.line - fnt.h) / 2;
    for (int k = k0; k < L.nrows; k++) {
        if (k > k0 && !pos_step(&o, &r, 1))
            break;
        int d = k - L.center;
        int y = L.grid_y + k * fnt.line;
        bool cur_line = d == 0;
        bool play = o == play_ord && r == play_row;
        unsigned bg = r % cfg.bar == 0 ? colour[C_BAR]
                    : r % cfg.beat == 0 ? colour[C_BEAT] : colour[C_BG];
        if (play)
            bg = colour[C_PLAYROW];
        if (cur_line)
            bg = play ? colour[C_PLAYROW] : colour[C_CURROW];
        if (bg != colour[C_BG])
            fill(0, y, LCD_WIDTH, fnt.line, bg);
        if (r == 0 && o != 0)
            hline(0, LCD_WIDTH - 1, y, colour[C_LINE]);
        if (play)
            fill(0, y, 1, fnt.line, colour[C_ACCENT]);

        char b[8];
        if (cfg.decimal)
            rb->snprintf(b, sizeof(b), "%02d", r % 100);
        else
            fmt_hex2(b, r);
        unsigned gc = r % cfg.bar == 0 ? colour[C_GUTTERBAR]
                                       : colour[C_GUTTER];
        if (o != app.ord)
            gc = colour[C_DIM];
        draw_field(1 + (fnt.w <= 4 ? 0 : 1), y + ty, b, 2, gc,
                   cur_line && app.field == F_GUTTER);

        int pat = song.ord[o];
        for (int i = 0; i < L.nvis; i++) {
            int ch = L.first + i;
            if (in_mark(o, r, ch))
                fill(L.sx[i] + 1, y, L.sw[i] - 1, fnt.line, colour[C_MARK]);
            vline(L.sx[i], y, y + fnt.line - 1, colour[C_HEAD]);
            draw_cell(i, y + ty, cell_at(pat, r, ch), cur_line,
                      eng_mute[ch]);
        }
    }
    char hint[64];
    pattern_hint(hint, sizeof(hint));
    status_draw(L.foot_y, hint);
    if (app.hint && input_select_held()) {
        switch (app.field) {
        case F_GUTTER:
            card_draw(-1, L.grid_y + L.center * fnt.line, "PREV POS",
                      "NEXT POS", "PAT-1", "PAT+1");
            break;
        case F_NOTE:
            card_draw(-1, L.grid_y + L.center * fnt.line, "+OCTAVE",
                      "-OCTAVE", "-1", "+1");
            break;
        case F_FX:
            card_draw(-1, L.grid_y + L.center * fnt.line, "+4 TYPES",
                      "-4 TYPES", "PREV", "NEXT");
            break;
        default:
            card_draw(-1, L.grid_y + L.center * fnt.line, "+16", "-16",
                      "-1", "+1");
            break;
        }
    }
}

void pattern_hint(char *b, size_t len)
{
    if (app.marking) {
        rb->strlcpy(b, "Mark: arrows extend, \x0b actions", len);
        return;
    }
    if (app.field == F_GUTTER) {
        rb->snprintf(b, len, "Pos %d pat %d \x0b play here", app.ord,
                     cur_pat());
        return;
    }
    const struct cell *e = cell_at(cur_pat(), app.row, app.ch);
    static const struct cell blank;
    if (!e)
        e = &blank;
    char nb[4];
    switch (app.field) {
    case F_NOTE:
        if (!e->note) {
            fmt_note(nb, app.pen_note);
            rb->snprintf(b, len, "\x0b puts %s %02X %s", nb, app.cur_ins,
                         song.ins[app.cur_ins - 1] ?
                         song.ins[app.cur_ins - 1]->name : "");
        } else if (e->note == NOTE_OFF) {
            rb->strlcpy(b, "Note off: ends the note", len);
        } else {
            fmt_note(nb, e->note);
            const struct instr *in = e->ins ? song.ins[e->ins - 1] : NULL;
            rb->snprintf(b, len, "%s %s", nb, in ? in->name :
                         e->ins ? "(no instrument)" : "");
        }
        break;
    case F_INS:
        if (e->ins) {
            const struct instr *in = song.ins[e->ins - 1];
            rb->snprintf(b, len, "Instrument %02X %s", e->ins,
                         in ? in->name : "(empty)");
        } else {
            rb->snprintf(b, len, "\x0b puts instrument %02X", app.cur_ins);
        }
        break;
    case F_VOL:
        describe_vol(b, len, e->vol);
        break;
    default:
        if (!e->fx && !e->param) {
            char d[40];
            describe_fx(d, sizeof(d), app.pen_fx, app.pen_param);
            rb->snprintf(b, len, "\x0b puts %s", d);
        } else {
            describe_fx(b, len, e->fx, e->param);
        }
        break;
    }
}

/* ---- editing ---- */

static void set_cell(int pat, int row, int ch, const struct cell *v)
{
    struct cell *c = cell_w(pat, row, ch);
    if (!c) {
        app_message("Out of memory");
        return;
    }
    undo_cell(pat, row, ch, c, v);
    *c = *v;
    app_changed();
}

static struct cell get_cell(int pat, int row, int ch)
{
    const struct cell *c = cell_at(pat, row, ch);
    struct cell v = { 0 };
    return c ? *c : v;
}

static void move_rows(int d)
{
    int o = app.ord, r = app.row;
    if (app.marking) {
        app.row = CLAMP(app.row + d, 0, pattern_rows(app.ord) - 1);
        return;
    }
    if (!pos_step(&o, &r, d)) {
        if (d > 0) {
            o = song.nord - 1;
            r = pattern_rows(o) - 1;
        } else {
            o = r = 0;
        }
    }
    app.ord = o;
    app.row = r;
}

static void move_field(int dir, bool rep)
{
    int f = app.field, c = app.ch;
    if (dir > 0) {
        if (rep && f != F_GUTTER) {
            if (c < song.nch - 1) {
                c++;
                f = F_NOTE;
            }
        } else if (f == F_GUTTER) {
            f = F_NOTE;
        } else if (f < F_PARAM) {
            f++;
        } else if (c < song.nch - 1) {
            c++;
            f = F_NOTE;
        }
    } else {
        if (rep && f != F_GUTTER) {
            if (f != F_NOTE)
                f = F_NOTE;
            else if (c > 0)
                c--;
            else
                f = F_GUTTER;
        } else if (f == F_GUTTER) {
            ;
        } else if (f > F_NOTE) {
            f--;
        } else if (c > 0) {
            c--;
            f = F_PARAM;
        } else {
            f = F_GUTTER;
        }
    }
    if (app.marking && f == F_GUTTER)
        f = F_NOTE;
    app.field = f;
    app.ch = c;
}

static int vol_index(int v)
{
    if (v >= 0x10 && v <= 0x50)
        return v - 0x10;
    if (v >= 0x60)
        return 65 + ((v >> 4) - 6) * 16 + (v & 15);
    return 64;
}

static int vol_value(int i)
{
    if (i <= 64)
        return 0x10 + i;
    i -= 65;
    return ((i / 16) + 6) << 4 | (i % 16);
}

static int fx_pos(int fx)
{
    for (unsigned i = 0; i < sizeof(fx_order); i++)
        if (fx_order[i] == fx)
            return i;
    return 0;
}

static void edit(int dir)
{
    static const int big[4] = { 1, -1, 0, 0 }, small[4] = { 0, 0, -1, 1 };
    int pat = cur_pat();
    struct cell e = get_cell(pat, app.row, app.ch);
    int coarse = big[dir], fine = small[dir];

    switch (app.field) {
    case F_GUTTER:
        if (coarse) {
            int o = CLAMP(app.ord - coarse, 0, song.nord - 1);
            app.ord = o;
            app.row = MIN(app.row, pattern_rows(o) - 1);
        } else {
            int old = song.ord[app.ord];
            int v = CLAMP(old + fine, 0, MAX_PAT - 1);
            undo_order(U_ORDER, app.ord, old, v);
            song.ord[app.ord] = v;
            app.row = MIN(app.row, pattern_rows(app.ord) - 1);
            app_changed();
        }
        return;
    case F_NOTE: {
        int v = e.note;
        if (!v) {
            v = app.pen_note;
            if (!e.ins)
                e.ins = app.cur_ins;
            if (!e.vol)
                e.vol = app.pen_evvol;
        }
        int idx = v == NOTE_OFF ? 0 : v;
        if (idx == 0 && (coarse > 0 || fine > 0))
            idx = app.pen_note != NOTE_OFF ? app.pen_note : 49;
        else
            idx = CLAMP(idx + coarse * 12 + fine, 0, 96);
        e.note = idx ? idx : NOTE_OFF;
        if (e.note != NOTE_OFF) {
            if (!e.ins)
                e.ins = app.cur_ins;
            app.pen_note = e.note;
            app_preview(e.ins, e.note);
        } else {
            e.ins = 0;
        }
        break;
    }
    case F_INS: {
        int v = e.ins ? e.ins : app.cur_ins;
        v = CLAMP(v + coarse * 16 + fine, 1, MAX(1, song.nins));
        e.ins = v;
        app.cur_ins = v;
        if (e.note && e.note != NOTE_OFF)
            app_preview(e.ins, e.note);
        break;
    }
    case F_VOL: {
        int i = e.vol ? vol_index(e.vol) : vol_index(app.pen_vol);
        i = CLAMP(i + coarse * 16 + fine, 0, 65 + 10 * 16 - 1);
        e.vol = vol_value(i);
        app.pen_vol = e.vol;
        break;
    }
    case F_FX: {
        if (!e.fx && !e.param) {
            e.fx = app.pen_fx;
            e.param = app.pen_param;
        }
        int n = sizeof(fx_order);
        int i = (fx_pos(e.fx) + coarse * 4 + fine + n) % n;
        e.fx = fx_order[i];
        app.pen_fx = e.fx;
        app.pen_param = e.param;
        break;
    }
    case F_PARAM: {
        if (!e.fx && !e.param) {
            e.fx = app.pen_fx;
            e.param = app.pen_param;
        }
        e.param = CLAMP(e.param + coarse * 16 + fine, 0, 255);
        app.pen_fx = e.fx;
        app.pen_param = e.param;
        break;
    }
    }
    set_cell(pat, app.row, app.ch, &e);
}

static int tap_serial = -1;
static int tap_ord, tap_row;
static bool tap_modified;

static void tap(void)
{
    int pat = cur_pat();
    struct cell e = get_cell(pat, app.row, app.ch);
    bool placed = false;
    tap_ord = app.ord;
    tap_row = app.row;
    tap_modified = app.modified;
    undo_begin();
    switch (app.field) {
    case F_GUTTER:
        app_play(PLAY_SONG);
        return;
    case F_NOTE:
        if (!e.note) {
            e.note = app.pen_note;
            e.ins = e.note != NOTE_OFF ? app.cur_ins : 0;
            e.vol = app.pen_evvol;
            placed = true;
        } else {
            if (e.note != NOTE_OFF) {
                app.pen_note = e.note;
                if (e.ins)
                    app.cur_ins = e.ins;
            }
            app.pen_evvol = e.vol;
            e.note = e.ins = e.vol = 0;
        }
        break;
    case F_INS:
        if (!e.ins) {
            e.ins = app.cur_ins;
            placed = true;
        } else {
            app.cur_ins = e.ins;
            e.ins = 0;
        }
        break;
    case F_VOL:
        if (!e.vol) {
            e.vol = app.pen_vol;
            placed = true;
        } else {
            app.pen_vol = e.vol;
            e.vol = 0;
        }
        break;
    default:
        if (!e.fx && !e.param) {
            e.fx = app.pen_fx;
            e.param = app.pen_param;
            placed = true;
        } else {
            app.pen_fx = e.fx;
            app.pen_param = e.param;
            e.fx = e.param = 0;
        }
        break;
    }
    set_cell(pat, app.row, app.ch, &e);
    tap_serial = undo_serial();
    if (placed && e.note && e.note != NOTE_OFF)
        app_preview(e.ins, e.note);
    if (placed && cfg.editstep)
        move_rows(cfg.editstep);
    undo_begin();
}

/* A double tap: undo what its first tap did, so it never edits. */
static void untap(void)
{
    if (tap_serial >= 0 && tap_serial == undo_serial()) {
        undo_undo();
        app.ord = tap_ord;
        app.row = tap_row;
        app.modified = tap_modified;
    }
    tap_serial = -1;
}

void pattern_follow(void)
{
    int o, r;
    eng_pos(&o, &r);
    if (o < song.nord && r < pattern_rows(o)) {
        app.ord = o;
        app.row = r;
    }
}

void pattern_note_off(void)
{
    struct cell e = get_cell(cur_pat(), app.row, app.ch);
    e.note = NOTE_OFF;
    e.ins = 0;
    undo_begin();
    set_cell(cur_pat(), app.row, app.ch, &e);
    undo_begin();
}

/* Insert (dir 1) or delete (dir -1) a row in the cursor's channel. */
void pattern_insert_row(int dir)
{
    int pat = cur_pat(), rows = song.pat[pat].rows;
    undo_begin();
    if (dir > 0) {
        for (int r = rows - 1; r > app.row; r--) {
            struct cell v = get_cell(pat, r - 1, app.ch);
            set_cell(pat, r, app.ch, &v);
        }
        struct cell z = { 0 };
        set_cell(pat, app.row, app.ch, &z);
    } else {
        for (int r = app.row; r < rows - 1; r++) {
            struct cell v = get_cell(pat, r + 1, app.ch);
            set_cell(pat, r, app.ch, &v);
        }
        struct cell z = { 0 };
        set_cell(pat, rows - 1, app.ch, &z);
    }
    undo_begin();
}

/* ---- marks and the clipboard ---- */

static struct {
    int w, h;
    struct cell *c;
} clip;

static void mark_rect(int *r0, int *r1, int *c0, int *c1)
{
    if (app.marking) {
        *r0 = MIN(app.mark_row0, app.row);
        *r1 = MAX(app.mark_row0, app.row);
        *c0 = MIN(app.mark_ch0, app.ch);
        *c1 = MAX(app.mark_ch0, app.ch);
    } else {                    /* no mark: the whole pattern */
        *r0 = 0;
        *r1 = pattern_rows(app.ord) - 1;
        *c0 = 0;
        *c1 = song.nch - 1;
    }
}

enum { MK_COPY, MK_CUT, MK_CLEAR, MK_PASTE, MK_UP1, MK_DOWN1, MK_UP12,
       MK_DOWN12, MK_INTERP, MK_SETINS };

bool pattern_mark_op(int op)
{
    int pat = cur_pat(), r0, r1, c0, c1;
    mark_rect(&r0, &r1, &c0, &c1);
    if (op == MK_PASTE) {
        if (!clip.c) {
            app_message("Clipboard is empty");
            return false;
        }
        undo_begin();
        if (app.marking) {
            app.row = r0;
            app.ch = c0;
        }
        for (int r = 0; r < clip.h; r++)
            for (int c = 0; c < clip.w; c++) {
                int rr = app.row + r, cc = app.ch + c;
                if (rr < song.pat[pat].rows && cc < song.nch)
                    set_cell(pat, rr, cc, &clip.c[r * clip.w + c]);
            }
        undo_begin();
        app.marking = false;
        return true;
    }
    if (op == MK_COPY || op == MK_CUT) {
        int w = c1 - c0 + 1, h = r1 - r0 + 1;
        pool_free(clip.c);
        clip.c = pool_alloc(w * h * sizeof(struct cell));
        if (!clip.c) {
            app_message("Out of memory");
            return false;
        }
        clip.w = w;
        clip.h = h;
        for (int r = 0; r < h; r++)
            for (int c = 0; c < w; c++)
                clip.c[r * w + c] = get_cell(pat, r0 + r, c0 + c);
        app_message("Copied %d x %d", w, h);
        if (op == MK_COPY) {
            app.marking = false;
            return true;
        }
    }
    undo_begin();
    for (int c = c0; c <= c1; c++) {
        struct cell first = get_cell(pat, r0, c), last = get_cell(pat, r1, c);
        for (int r = r0; r <= r1; r++) {
            struct cell v = get_cell(pat, r, c);
            switch (op) {
            case MK_CUT:
            case MK_CLEAR:
                rb->memset(&v, 0, sizeof(v));
                break;
            case MK_UP1: case MK_DOWN1: case MK_UP12: case MK_DOWN12: {
                static const int8_t d[4] = { 1, -1, 12, -12 };
                if (v.note >= 1 && v.note <= 96)
                    v.note = CLAMP(v.note + d[op - MK_UP1], 1, 96);
                break;
            }
            case MK_INTERP:
                if (r1 > r0) {
                    int t = r - r0, n = r1 - r0;
                    if (first.vol >= 0x10 && first.vol <= 0x50 &&
                        last.vol >= 0x10 && last.vol <= 0x50)
                        v.vol = first.vol + (last.vol - first.vol) * t / n;
                    if (first.fx == last.fx && (first.fx || first.param))
                    {
                        v.fx = first.fx;
                        v.param = first.param +
                                  (last.param - first.param) * t / n;
                    }
                }
                break;
            case MK_SETINS:
                if (v.note && v.note != NOTE_OFF)
                    v.ins = app.cur_ins;
                break;
            }
            if (cell_at(pat, r, c) || !cell_empty(&v))
                set_cell(pat, r, c, &v);
        }
    }
    undo_begin();
    app.marking = false;
    return true;
}

static void mark_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Marked block", NULL, "Copy", "Cut", "Clear",
                        "Paste", "Transpose +1", "Transpose -1",
                        "Transpose +12", "Transpose -12", "Interpolate",
                        "Set instrument", "Cancel");
    int sel = 0;
    int r = rb->do_menu(&menu, &sel, NULL, false);
    if (r >= 0 && r <= MK_SETINS)
        pattern_mark_op(r);
    else
        app.marking = false;
    app.dirty = true;
}

void pattern_event(const struct ev *ev)
{
    switch (ev->type) {
    case EV_MOVE:
        if (ev->dir == DIR_UP || ev->dir == DIR_DOWN)
            move_rows(ev->dir == DIR_UP ? -1 : 1);
        else
            move_field(ev->dir == DIR_RIGHT ? 1 : -1, ev->rep);
        break;
    case EV_WHEEL:
        move_rows(ev->dir == DIR_UP ? -1 : 1);
        break;
    case EV_EDIT:
        if (app.marking) {
            if (ev->dir == DIR_UP || ev->dir == DIR_DOWN)
                move_rows(ev->dir == DIR_UP ? -1 : 1);
            else
                move_field(ev->dir == DIR_RIGHT ? 1 : -1, true);
            break;
        }
        edit(ev->dir);
        break;
    case EV_EDIT_END:
        undo_begin();
        eng_release(0);
        break;
    case EV_TAP:
        if (app.marking)
            mark_menu();
        else
            tap();
        break;
    case EV_DOUBLE:
        untap();
        app_play_toggle();
        break;
    case EV_MARK:
        if (app.marking) {
            mark_menu();
        } else {
            app.marking = true;
            app.mark_ord = app.ord;
            app.mark_row0 = app.row;
            app.mark_ch0 = app.ch;
            if (app.field == F_GUTTER)
                app.field = F_NOTE;
        }
        break;
    case EV_BACK:
        if (app.marking)
            app.marking = false;
        else
            app_hub();
        break;
    }
    pattern_clamp();
}
