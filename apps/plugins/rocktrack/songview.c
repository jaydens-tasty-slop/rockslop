/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: the song screen. Song settings on top, then the order list
 * with a pitch sparkline of every pattern lane: the song's shape.
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

enum { S_TITLE = -5, S_TEMPO, S_SPEED, S_CHANNELS, S_LOOP };
#define NFIELDS 5

static int scroll;

static void field_text(int f, char *label, char *value, size_t len)
{
    switch (f) {
    case S_TITLE:
        rb->strcpy(label, "TITLE");
        rb->strlcpy(value, song.name[0] ? song.name : "(untitled)", len);
        break;
    case S_TEMPO:
        rb->strcpy(label, "TEMPO");
        rb->snprintf(value, len, "%d BPM", song.bpm);
        break;
    case S_SPEED:
        rb->strcpy(label, "SPEED");
        rb->snprintf(value, len, "%d TICKS/ROW", song.speed);
        break;
    case S_CHANNELS:
        rb->strcpy(label, "CHANNELS");
        rb->snprintf(value, len, "%d", song.nch);
        break;
    case S_LOOP:
        rb->strcpy(label, "LOOP TO");
        rb->snprintf(value, len, "POS %02X", song.restart);
        break;
    }
}

static void note_range(int *lo, int *hi)
{
    *lo = 96;
    *hi = 1;
    for (int p = 0; p < MAX_PAT; p++) {
        const struct pattern *pt = &song.pat[p];
        if (!pt->c)
            continue;
        for (int i = 0; i < pt->rows * song.nch; i++) {
            int n = pt->c[i].note;
            if (n >= 1 && n <= 96) {
                *lo = MIN(*lo, n);
                *hi = MAX(*hi, n);
            }
        }
    }
    if (*hi < *lo) {
        *lo = 37;
        *hi = 61;
    }
    if (*hi - *lo < 12)
        *hi = *lo + 12;
}

static void sparkline(int pat, int x, int y, int w, int h, int ch, int lo,
                      int hi)
{
    const struct pattern *pt = &song.pat[pat];
    fill(x, y, w, h, colour[C_PANEL]);
    if (!pt->c || w < 2)
        return;
    int rows = pt->rows;
    for (int r = 0; r < rows; r++) {
        const struct cell *c = &pt->c[r * song.nch + ch];
        if (c->note < 1 || c->note > 96)
            continue;
        int px = x + r * w / rows;
        int py = y + h - 1 - (c->note - lo) * (h - 1) / (hi - lo);
        int len = MAX(1, w / rows);
        fill(px, CLAMP(py, y, y + h - 1), len, 1,
             eng_mute[ch] ? colour[C_DIM] : ins_colour(c->ins));
    }
}

void songview_draw(void)
{
    int line = fnt.line, head_h = line + 1;
    int foot_y = LCD_HEIGHT - line - 1;
    int top = head_h + 1;
    int nlines = MAX(1, (foot_y - top) / line);
    int total = NFIELDS + song.nord;
    int cur = app.sline + NFIELDS;

    if (cur < scroll + 1)
        scroll = cur - 1;
    if (cur > scroll + nlines - 2)
        scroll = cur - nlines + 2;
    scroll = CLAMP(scroll, 0, MAX(0, total - nlines));

    fill(0, 0, LCD_WIDTH, LCD_HEIGHT, colour[C_BG]);
    header_draw(0, head_h);

    int play_ord = -1, play_row;
    if (eng_mode() != PLAY_STOP)
        eng_pos(&play_ord, &play_row);
    int lo, hi;
    note_range(&lo, &hi);
    int ty = (line - fnt.h) / 2;
    int lane_x = 4 * fnt.w + 6;
    int lanes_w = LCD_WIDTH - lane_x - 1;
    int lane_w = MAX(2, lanes_w / song.nch);

    for (int k = 0; k < nlines && scroll + k < total; k++) {
        int i = scroll + k - NFIELDS;
        int y = top + k * line;
        bool sel = i == app.sline;
        if (i < 0) {
            char label[12], value[32];
            field_text(i, label, value, sizeof(value));
            if (sel)
                fill(0, y, LCD_WIDTH, line, colour[C_CURROW]);
            text(2, y + ty, label, colour[C_GUTTER]);
            int vx = 2 + 9 * fnt.w;
            if (sel) {
                fill(vx - 1, y + ty - 1, text_width(value) + 1, fnt.h + 2,
                     colour[C_CURSOR]);
                text(vx, y + ty, value, colour[C_CURTEXT]);
            } else {
                text(vx, y + ty, value, colour[C_TEXT]);
            }
            if (i == S_LOOP)
                hline(0, LCD_WIDTH - 1, y + line, colour[C_LINE]);
            continue;
        }
        int pat = song.ord[i];
        bool play = i == play_ord;
        if (sel || play)
            fill(0, y, LCD_WIDTH, line, play ? colour[C_PLAYROW]
                                             : colour[C_CURROW]);
        if (play)
            fill(0, y, 1, line, colour[C_ACCENT]);
        char b[4];
        fmt_hex2(b, i);
        text(2, y + ty, b, i == song.restart ? colour[C_GUTTERBAR]
                                             : colour[C_GUTTER]);
        fmt_hex2(b, pat);
        int px = 2 + 2 * fnt.w + 2;
        if (sel) {
            fill(px - 1, y + ty - 1, 2 * fnt.w + 1, fnt.h + 2,
                 colour[C_CURSOR]);
            text(px, y + ty, b, colour[C_CURTEXT]);
        } else {
            text(px, y + ty, b, pattern_blank(pat) ? colour[C_DIM]
                                                   : colour[C_TEXT]);
        }
        for (int c = 0; c < song.nch && c * lane_w < lanes_w; c++)
            sparkline(pat, lane_x + c * lane_w, y + 1, lane_w - 1, line - 2,
                      c, lo, hi);
    }

    char hint[48];
    if (app.sline >= 0)
        rb->snprintf(hint, sizeof(hint), "Pattern %02X, %d rows \x0b open",
                     song.ord[app.sline], pattern_rows(app.sline));
    else if (app.sline == S_TITLE)
        rb->strlcpy(hint, "\x0b rename the song", sizeof(hint));
    else if (app.sline == S_LOOP)
        rb->strlcpy(hint, "Where the song loops to", sizeof(hint));
    else
        rb->strlcpy(hint, "Hold \x0b + arrows to change", sizeof(hint));
    status_draw(foot_y, hint);
    if (app.hint && input_select_held())
        card_draw(-1, top + (cur - scroll) * line,
                  app.sline >= 0 ? "+16" : "+10",
                  app.sline >= 0 ? "-16" : "-10", "-1", "+1");
}

static bool channels_used(int ch)
{
    for (int p = 0; p < MAX_PAT; p++)
        for (int r = 0; r < song.pat[p].rows; r++)
            if (!cell_empty(cell_at(p, r, ch)))
                return true;
    return false;
}

static void edit(int dir)
{
    static const int big[4] = { 1, -1, 0, 0 }, small[4] = { 0, 0, -1, 1 };
    int d = big[dir] * (app.sline >= 0 ? 16 : 10) + small[dir];
    switch (app.sline) {
    case S_TITLE:
        return;
    case S_TEMPO:
        song.bpm = CLAMP(song.bpm + d, 32, 255);
        break;
    case S_SPEED:
        song.speed = CLAMP(song.speed + CLAMP(d, -1, 1), 1, 31);
        break;
    case S_CHANNELS: {
        int n = CLAMP(song.nch + CLAMP(d, -1, 1), 1, MAX_CH);
        if (n < song.nch && channels_used(song.nch - 1)) {
            app_message("Channel %d has notes: clear it first", song.nch);
            return;
        }
        if (!song_set_channels(n))
            app_message("Out of memory");
        undo_clear();   /* cell undo entries refer to the old layout */
        break;
    }
    case S_LOOP:
        song.restart = CLAMP(song.restart + CLAMP(d, -1, 1), 0,
                             song.nord - 1);
        break;
    default: {
        int old = song.ord[app.sline];
        int v = CLAMP(old + d, 0, MAX_PAT - 1);
        undo_order(U_ORDER, app.sline, old, v);
        eng_lock();
        song.ord[app.sline] = v;
        eng_unlock();
        break;
    }
    }
    app_changed();
}

static void open_pattern(void)
{
    if (app.sline < 0)
        return;
    if (app.ord != app.sline) {
        app.ord = app.sline;
        app.row = 0;
    }
    app_set_view(VIEW_PATTERN);
}

void songview_op(int op)
{
    int pos = MAX(app.sline, 0);
    undo_begin();
    switch (op) {
    case 0:                                     /* insert: same pattern */
        if (order_insert(pos + 1, song.ord[pos])) {
            undo_order(U_ORDINS, pos + 1, 0, song.ord[pos + 1]);
            app.sline = pos + 1;
        }
        break;
    case 1:                                     /* delete */
        if (song.nord > 1) {
            undo_order(U_ORDDEL, pos, song.ord[pos], 0);
            order_delete(pos);
            app.sline = MIN(pos, song.nord - 1);
        }
        break;
    case 2:                                     /* clone */
    case 3: {                                   /* new empty */
        int p = pattern_unused();
        if (p < 0) {
            app_message("No free pattern");
            break;
        }
        int src = song.ord[pos];
        song.pat[p].rows = op == 2 ? song.pat[src].rows : cfg.newrows;
        if (op == 2 && song.pat[src].c) {
            if (!pattern_alloc(p)) {
                app_message("Out of memory");
                break;
            }
            rb->memcpy(song.pat[p].c, song.pat[src].c,
                       song.pat[src].rows * song.nch * sizeof(struct cell));
        }
        if (order_insert(pos + 1, p)) {
            undo_order(U_ORDINS, pos + 1, 0, p);
            app.sline = pos + 1;
            if (op == 2)
                app_message("Pattern %02X copied to %02X", src, p);
            else
                app_message("New pattern %02X", p);
        }
        break;
    }
    }
    undo_begin();
    app_changed();
}

void songview_event(const struct ev *ev)
{
    switch (ev->type) {
    case EV_MOVE:
        if (ev->dir == DIR_UP)
            app.sline--;
        else if (ev->dir == DIR_DOWN)
            app.sline++;
        else if (ev->dir == DIR_RIGHT)
            open_pattern();
        break;
    case EV_WHEEL:
        app.sline += ev->dir == DIR_UP ? -1 : 1;
        break;
    case EV_EDIT:
        edit(ev->dir);
        break;
    case EV_EDIT_END:
        undo_begin();
        break;
    case EV_TAP:
        if (app.sline == S_TITLE) {
            char buf[21];
            rb->strlcpy(buf, song.name, sizeof(buf));
            if (!rb->kbd_input(buf, sizeof(buf), NULL)) {
                rb->strlcpy(song.name, buf, sizeof(song.name));
                app_changed();
            }
            app.dirty = true;
        } else {
            open_pattern();
        }
        break;
    case EV_DOUBLE:
        if (app.sline >= 0) {
            app.ord = app.sline;
            app.row = 0;
        }
        app_play_toggle();
        break;
    case EV_BACK:
        app_hub();
        break;
    }
    app.sline = CLAMP(app.sline, -NFIELDS, song.nord - 1);
}
