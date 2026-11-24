/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: screen chrome (header, status line, chord card) and the menu
 * grid that holding Select opens.
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

void header_draw(int y, int h)
{
    char b[48];
    int ty = y + (h - 1 - fnt.h) / 2;
    fill(0, y, LCD_WIDTH, h, colour[C_HEAD]);
    bool playing = eng_mode() != PLAY_STOP;
    int x = 2;
    x += text(x, ty, playing ? "\x02" : "\x03",
              playing ? colour[C_ACCENT] : colour[C_DIM]) + fnt.w / 2;
    rb->snprintf(b, sizeof(b), "P%02X", song.ord[app.ord]);
    x += text(x, ty, b, colour[C_TEXT]) + fnt.w / 2;
    if (app.view == VIEW_PATTERN) {
        fmt_hex2(b, app.row);
        x += text(x, ty, b, colour[C_GUTTER]) + fnt.w / 2;
    }
    rb->snprintf(b, sizeof(b), "%d", song.bpm);
    x += text(x, ty, b, colour[C_GUTTER]) + fnt.w;
    if (app.modified)
        pixel(x - fnt.w / 2 - 1, ty, colour[C_ACCENT]);

    /* the current instrument: colour chip, number, name */
    const struct instr *in = app.cur_ins ? song.ins[app.cur_ins - 1] : NULL;
    fill(x, ty, fnt.w - 1, fnt.h, ins_colour(app.cur_ins));
    x += fnt.w + 1;
    rb->snprintf(b, sizeof(b), "%02X %s", app.cur_ins, in ? in->name : "");
    int room = (LCD_WIDTH - 1 - x) / fnt.w;
    if (room > 0)
        textn(x, ty, b, room, ins_colour(app.cur_ins));
}

int status_draw(int y, const char *hint)
{
    int h = LCD_HEIGHT - y;
    int ty = y + 1 + (h - 1 - fnt.h) / 2;
    int room = (LCD_WIDTH - 4) / fnt.w;
    fill(0, y, LCD_WIDTH, h, colour[C_HEAD]);
    if (app.msg[0] && TIME_BEFORE(*rb->current_tick, app.msg_until))
        textn(2, ty, app.msg, room, colour[C_ACCENT]);
    else
        textn(2, ty, hint, room, colour[C_GUTTER]);
    return h;
}

/* What each arrow does while Select is held; centred on x when x < 0,
 * kept clear of the row at y. */
void card_draw(int x, int y, const char *up, const char *down,
               const char *left, const char *right)
{
    int lw = rb->strlen(left), rw = rb->strlen(right);
    int cols = MAX(MAX(lw + rw + 5, (int)rb->strlen(up) + 2),
                   MAX((int)rb->strlen(down) + 2, 13));
    int w = cols * fnt.w + 6, h = 4 * fnt.line + 4;
    if (x < 0)
        x = (LCD_WIDTH - w) / 2;
    int cy = y > LCD_HEIGHT / 2 ? y - h - 2 : y + fnt.line + 2;
    cy = CLAMP(cy, 0, LCD_HEIGHT - h);
    fill(x, cy, w, h, colour[C_PANEL]);
    frame(x, cy, w, h, colour[C_ACCENT]);
    int ty = cy + 2 + (fnt.line - fnt.h) / 2;
    char b[32];
    int mid = x + w / 2;
    if (*up) {
        rb->snprintf(b, sizeof(b), "\x04%s", up);
        text(mid - text_width(b) / 2, ty, b, colour[C_TEXT]);
    }
    if (*left) {
        rb->snprintf(b, sizeof(b), "\x06%s", left);
        text(x + 3, ty + fnt.line, b, colour[C_TEXT]);
    }
    if (*right) {
        rb->snprintf(b, sizeof(b), "%s\x07", right);
        text(x + w - 3 - text_width(b), ty + fnt.line, b, colour[C_TEXT]);
    }
    if (*down) {
        rb->snprintf(b, sizeof(b), "\x05%s", down);
        text(mid - text_width(b) / 2, ty + 2 * fnt.line, b, colour[C_TEXT]);
    }
    const char *rel = "RELEASE: MENU";
    text(mid - text_width(rel) / 2, ty + 3 * fnt.line, rel, colour[C_DIM]);
}

/* ---- menu grid ---- */

enum {     /* the first row follows the VIEW_ numbers */
    A_PATTERN, A_SONG, A_INSTR, A_SETUP,
    A_PLAY, A_LOOP, A_FROM, A_STOP,
    A_UNDO, A_REDO, A_COPY, A_PASTE,
    A_MARK, A_NOTEOFF, A_INSROW, A_DELROW,
    A_POS_INS, A_POS_DEL, A_POS_CLONE, A_POS_NEW,
    A_INS_NEW, A_INS_COPY, A_INS_WAV, A_INS_DEL,
    A_SAVE, A_OPEN, A_MORE, A_QUIT,
};

struct tile {
    const char *label, *help;
};

static const struct tile tiles[] = {
    [A_SONG] = { "SONG", "Song: arrangement and settings" },
    [A_PATTERN] = { "PATTN", "Pattern: write notes" },
    [A_INSTR] = { "INSTR", "Instrument: shape the sound" },
    [A_SETUP] = { "SETUP", "Setup: text size, follow..." },
    [A_PLAY] = { "PLAY", "Play the song from the start" },
    [A_LOOP] = { "LOOP", "Loop this pattern (or \x0b\x0b)" },
    [A_FROM] = { "FROM", "Play the song from the cursor" },
    [A_STOP] = { "STOP", "Stop playing" },
    [A_UNDO] = { "UNDO", "Undo the last change" },
    [A_REDO] = { "REDO", "Redo what was undone" },
    [A_COPY] = { "COPY", "Copy the pattern (or mark)" },
    [A_PASTE] = { "PASTE", "Paste at the cursor" },
    [A_MARK] = { "MARK", "Mark a block: arrows extend" },
    [A_NOTEOFF] = { "OFF", "Note off here" },
    [A_INSROW] = { "+ROW", "Insert a row in this channel" },
    [A_DELROW] = { "-ROW", "Delete a row in this channel" },
    [A_POS_INS] = { "+POS", "Repeat this pattern after" },
    [A_POS_DEL] = { "-POS", "Remove this position" },
    [A_POS_CLONE] = { "CLONE", "Copy into a new pattern" },
    [A_POS_NEW] = { "NEW", "New empty pattern after" },
    [A_INS_NEW] = { "NEW", "New instrument" },
    [A_INS_COPY] = { "COPY", "Copy this instrument" },
    [A_INS_WAV] = { "WAV", "Load a WAV file as the sound" },
    [A_INS_DEL] = { "DEL", "Delete this instrument" },
    [A_SAVE] = { "SAVE", "Save the song (.xm)" },
    [A_OPEN] = { "OPEN", "Open a song: XM, MOD, MIDI" },
    [A_MORE] = { "MORE", "Export, channels, help..." },
    [A_QUIT] = { "QUIT", "Back to the start menu" },
};

#define GRID_ROWS 5

static int grid_action(int row, int col)
{
    if (row == 3) {
        int base = app.view == VIEW_SONG ? A_POS_INS
                 : app.view == VIEW_INSTR ? A_INS_NEW : A_MARK;
        return base + col;
    }
    static const int rows[GRID_ROWS] = { A_PATTERN, A_PLAY, A_UNDO, 0,
                                          A_SAVE };
    return rows[row] + col;
}

static void grid_draw(int sel_row, int sel_col)
{
    int cols = 4;
    int tw = (LCD_WIDTH - 4) / cols, th = fnt.line + 4;
    int help = 2 * fnt.line + 6;
    if (GRID_ROWS * th + help > LCD_HEIGHT)     /* short screens: tighter */
        th = fnt.line + 1;
    if (GRID_ROWS * th + help > LCD_HEIGHT)
        help = 0;
    int w = tw * cols, h = GRID_ROWS * th + help;
    int x0 = (LCD_WIDTH - w) / 2, y0 = LCD_HEIGHT - h;
    fill(x0 - 2, y0 - 2, w + 4, h + 2, colour[C_HEAD]);
    hline(x0 - 2, x0 + w + 1, y0 - 2, colour[C_ACCENT]);
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < cols; c++) {
            int a = grid_action(r, c);
            int x = x0 + c * tw, y = y0 + r * th;
            bool sel = r == sel_row && c == sel_col;
            bool here = r == 0 && c == app.view;
            fill(x + 1, y + 1, tw - 2, th - 2,
                 sel ? colour[C_CURSOR] : here ? colour[C_PANELHI]
                                               : colour[C_PANEL]);
            if (here && !sel)       /* the screen you are on */
                frame(x + 1, y + 1, tw - 2, th - 2, colour[C_ACCENT]);
            const char *l = tiles[a].label;
            text(x + (tw - text_width(l)) / 2, y + (th - fnt.h) / 2, l,
                 sel ? colour[C_CURTEXT] : r == 1 ? colour[C_ACCENT]
                                                  : colour[C_TEXT]);
        }
    if (!help)
        return;
    int hy = y0 + GRID_ROWS * th + 2;
    textn(x0, hy + (fnt.line - fnt.h) / 2,
          tiles[grid_action(sel_row, sel_col)].help,
          (LCD_WIDTH - x0) / fnt.w, colour[C_TEXT]);
    textn(x0, hy + fnt.line + (fnt.line - fnt.h) / 2,
          "\x0b do it   hold \x0b close", (LCD_WIDTH - x0) / fnt.w,
          colour[C_DIM]);
}

static void run_action(int a)
{
    struct ev mark = { .type = EV_MARK };
    switch (a) {
    case A_SONG:
        app.sline = app.ord;
        app_set_view(VIEW_SONG);
        break;
    case A_PATTERN:
        if (app.view == VIEW_SONG && app.sline >= 0)
            app.ord = app.sline;
        app_set_view(VIEW_PATTERN);
        break;
    case A_INSTR:
        app_set_view(VIEW_INSTR);
        break;
    case A_SETUP:
        app_setup_menu();
        break;
    case A_PLAY:
        eng_play(PLAY_SONG, 0, 0);
        break;
    case A_LOOP:
        app_play(PLAY_PATTERN);
        break;
    case A_FROM:
        app_play(PLAY_SONG);
        break;
    case A_STOP:
        eng_stop();
        break;
    case A_UNDO:
        if (!undo_undo())
            app_message("Nothing to undo");
        else
            app_after_undo();
        break;
    case A_REDO:
        if (!undo_redo())
            app_message("Nothing to redo");
        else
            app_after_undo();
        break;
    case A_COPY:
    case A_PASTE:
        if (app.view == VIEW_SONG && app.sline >= 0)
            app.ord = app.sline;
        pattern_mark_op(a == A_COPY ? 0 : 3);
        break;
    case A_MARK:
        pattern_event(&mark);
        break;
    case A_NOTEOFF:
        pattern_note_off();
        break;
    case A_INSROW:
    case A_DELROW:
        pattern_insert_row(a == A_INSROW ? 1 : -1);
        break;
    case A_POS_INS: case A_POS_DEL: case A_POS_CLONE: case A_POS_NEW:
        songview_op(a - A_POS_INS);
        break;
    case A_INS_NEW: case A_INS_COPY: case A_INS_WAV: case A_INS_DEL:
        instr_op(a - A_INS_NEW);
        break;
    case A_SAVE:
        app_save(false);
        break;
    case A_OPEN:
        app_open();
        break;
    case A_MORE:
        app_more_menu();
        break;
    case A_QUIT:
        app.quit = true;
        break;
    }
}

void hub_run(void)
{
    static int last_row = 1, last_col;
    int row = last_row, col = last_col;
    if (row == 0)
        col = app.view;
    app.hint = false;
    while (true) {
        switch (app.view) {
        case VIEW_SONG: songview_draw(); break;
        case VIEW_INSTR: instr_draw(); break;
        default: pattern_draw(); break;
        }
        grid_draw(row, col);
        draw_update();
        struct ev ev;       /* redraw while playing, else wait */
        if (!input_get(&ev, eng_mode() != PLAY_STOP ? HZ / 10 : -1))
            continue;
        switch (ev.type) {
        case EV_MOVE:
        case EV_EDIT:
            if (ev.dir == DIR_UP)
                row = (row + GRID_ROWS - 1) % GRID_ROWS;
            else if (ev.dir == DIR_DOWN)
                row = (row + 1) % GRID_ROWS;
            else if (ev.dir == DIR_LEFT)
                col = (col + 3) % 4;
            else
                col = (col + 1) % 4;
            break;
        case EV_TAP: {
            int a = grid_action(row, col);
            last_row = row;
            last_col = col;
            app.dirty = true;
            if (!(row == 0 && col == app.view))
                run_action(a);
            return;
        }
        case EV_SYS:
            if (rb->default_event_handler(ev.sys) == SYS_USB_CONNECTED) {
                app.usb = true;
                app.quit = true;
                return;
            }
            break;
        case EV_MENU:
        case EV_BACK:
            app.dirty = true;
            return;
        case EV_LEAVE:
            app.quit = true;
            app.dirty = true;
            return;
        case EV_PLAY:
            app_play_toggle();
            break;
        }
    }
}
