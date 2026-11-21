/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: keymaps and the five-key gesture grammar (DESIGN.md "Input").
 *
 * Arrows move; Select held + arrows edits; a Select tap acts, a double tap
 * plays/stops, Select held alone opens the menu grid. Keypads with more
 * keys add Play/Back/Mark shortcuts and a scroll wheel.
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

#if CONFIG_KEYPAD == YP3_PAD
#define RT_UP       BUTTON_UP
#define RT_DOWN     BUTTON_DOWN
#define RT_LEFT     BUTTON_LEFT
#define RT_RIGHT    BUTTON_RIGHT
#define RT_SEL      BUTTON_SELECT
/* Up+Left together, the player's back chord (keymap-yp3.c): it leaves to
 * the start menu as QUIT does, so the YP3 has a way out without the grid */
#define RT_LEAVE    (BUTTON_UP | BUTTON_LEFT)

#elif CONFIG_KEYPAD == IPOD_4G_PAD || CONFIG_KEYPAD == IPOD_3G_PAD || \
      CONFIG_KEYPAD == IPOD_1G2G_PAD
/* the wheel moves rows (or edits while Select is held) */
#define RT_UP       BUTTON_MENU
#define RT_DOWN     BUTTON_PLAY
#define RT_LEFT     BUTTON_LEFT
#define RT_RIGHT    BUTTON_RIGHT
#define RT_SEL      BUTTON_SELECT
#define RT_WHEEL_UP BUTTON_SCROLL_BACK
#define RT_WHEEL_DN BUTTON_SCROLL_FWD

#elif defined(BUTTON_UP) && defined(BUTTON_DOWN) && defined(BUTTON_LEFT) && \
      defined(BUTTON_RIGHT) && defined(BUTTON_SELECT)
#define RT_UP       BUTTON_UP
#define RT_DOWN     BUTTON_DOWN
#define RT_LEFT     BUTTON_LEFT
#define RT_RIGHT    BUTTON_RIGHT
#define RT_SEL      BUTTON_SELECT
#if defined(BUTTON_PLAY)
#define RT_PLAY     BUTTON_PLAY
#endif
#if defined(BUTTON_BACK)
#define RT_BACK     BUTTON_BACK
#elif defined(BUTTON_HOME)
#define RT_BACK     BUTTON_HOME
#endif
#if defined(BUTTON_MENU)
#define RT_MENU     BUTTON_MENU
#endif
#if defined(BUTTON_REC)
#define RT_MARK     BUTTON_REC
#endif
#if defined(BUTTON_SCROLL_FWD) && defined(BUTTON_SCROLL_BACK)
#define RT_WHEEL_UP BUTTON_SCROLL_BACK
#define RT_WHEEL_DN BUTTON_SCROLL_FWD
#endif

#else
#error RockTrack: no keymap for this keypad
#endif

#ifndef RT_PLAY
#define RT_PLAY 0
#endif
#ifndef RT_BACK
#define RT_BACK 0
#endif
#ifndef RT_MENU
#define RT_MENU 0
#endif
#ifndef RT_MARK
#define RT_MARK 0
#endif
#ifndef RT_LEAVE
#define RT_LEAVE 0
#endif
#ifndef RT_WHEEL_UP
#define RT_WHEEL_UP 0
#define RT_WHEEL_DN 0
#endif

#define ARROWS  (RT_UP | RT_DOWN | RT_LEFT | RT_RIGHT)
#define KEYS    (ARROWS | RT_SEL | RT_PLAY | RT_BACK | RT_MENU | RT_MARK)
#ifdef HAVE_BLUETOOTH_HID
/* A game controller: the d-pad is the arrows, A Select (input_get), B Back,
 * X play/stop, Y mark, Options the menu grid, PS back to the start menu */
#define PAD_KEYS (BUTTON_PAD_B | BUTTON_PAD_X | BUTTON_PAD_Y | BUTTON_PAD_MENU | \
                  BUTTON_PAD_HOME)
#undef KEYS
#define KEYS    (ARROWS | RT_SEL | RT_PLAY | RT_BACK | RT_MENU | RT_MARK | PAD_KEYS)
#endif

/* timings in ticks */
#define TAP_MAX     (HZ * 3 / 10)
#define DOUBLE_GAP  (HZ * 3 / 10)
#define HINT_DELAY  (HZ * 3 / 10)
#define REP_DELAY   (HZ * 3 / 10)
#define REP_SLOW    (HZ / 10)
#define REP_FAST    (HZ / 25)

static int held;
static bool sel_down, sel_used, hint_sent;
static long sel_t0, last_tap_end;
static int rep_dir = -1;
static long rep_next;
static int rep_count;

#define QLEN 4
static struct ev queue[QLEN];
static int qhead, qlen;

static void push(int type, int dir, bool rep)
{
    if (qlen == QLEN)
        return;
    struct ev *e = &queue[(qhead + qlen++) % QLEN];
    e->type = type;
    e->dir = dir;
    e->rep = rep;
    e->sys = 0;
}

static int dir_key(int dir)
{
    static const int keys[4] = { RT_UP, RT_DOWN, RT_LEFT, RT_RIGHT };
    return keys[dir];
}

void input_reset(void)
{
    held = 0;
    sel_down = sel_used = hint_sent = false;
    rep_dir = -1;
    qlen = 0;
    last_tap_end = 0;
    rb->button_clear_queue();
}

bool input_select_held(void)
{
    return sel_down;
}

static void arrow_event(int dir, bool rep)
{
    if (sel_down) {
        sel_used = true;
        push(EV_EDIT, dir, rep);
    } else {
        push(EV_MOVE, dir, rep);
    }
}

static void press(int keys, long now)
{
    if (keys & RT_SEL) {
        sel_down = true;
        sel_used = hint_sent = false;
        sel_t0 = now;
    }
#ifdef HAVE_BLUETOOTH_HID
    if ((keys & BUTTON_PAD_HOME) && !sel_down) {
        while (rb->button_status() & BUTTON_PAD_HOME)   /* as RT_LEAVE */
            rb->sleep(HZ / 50);
        input_reset();
        push(EV_LEAVE, 0, false);
        return;
    }
    if (keys & BUTTON_PAD_B)
        push(EV_BACK, 0, false);
    if (keys & BUTTON_PAD_X)
        push(EV_PLAY, 0, false);
    if (keys & BUTTON_PAD_Y)
        push(EV_MARK, 0, false);
    if (keys & BUTTON_PAD_MENU)
        push(EV_MENU, 0, false);
#endif
    if (RT_LEAVE && (keys & RT_LEAVE) && (held & RT_LEAVE) == RT_LEAVE &&
        !sel_down) {
        /* Delivered once the chord is let go, its keys gone from the queue:
         * Left alone in the start menu would leave the plugin too. The
         * chord's first key has already moved the cursor. */
        while (rb->button_status() & RT_LEAVE)
            rb->sleep(HZ / 50);
        input_reset();
        push(EV_LEAVE, 0, false);
        return;
    }
    for (int d = 0; d < 4; d++) {
        if (!(keys & dir_key(d)))
            continue;
        arrow_event(d, false);
        rep_dir = d;
        rep_next = now + REP_DELAY;
        rep_count = 0;
    }
    if (RT_PLAY && (keys & RT_PLAY))
        push(EV_PLAY, 0, false);
    if (RT_BACK && (keys & RT_BACK))
        push(EV_BACK, 0, false);
    if (RT_MENU && (keys & RT_MENU))
        push(EV_MENU, 0, false);
    if (RT_MARK && (keys & RT_MARK))
        push(EV_MARK, 0, false);
}

static void release(int keys, long now)
{
    if ((keys & RT_SEL) && sel_down) {
        sel_down = false;
        if (sel_used) {
            push(EV_EDIT_END, 0, false);
        } else if (now - sel_t0 < TAP_MAX) {
            if (last_tap_end && sel_t0 - last_tap_end < DOUBLE_GAP) {
                push(EV_DOUBLE, 0, false);
                last_tap_end = 0;
            } else {
                push(EV_TAP, 0, false);
                last_tap_end = now;
            }
        } else {
            push(EV_MENU, 0, false);
        }
    }
    if (rep_dir >= 0 && (keys & dir_key(rep_dir)))
        rep_dir = -1;
}

static void timers(long now)
{
    if (rep_dir >= 0 && TIME_AFTER(now, rep_next - 1)) {
        if (!(held & dir_key(rep_dir))) {
            rep_dir = -1;
        } else {
            arrow_event(rep_dir, true);
            rep_count++;
            rep_next = now + (rep_count < 8 ? REP_SLOW : REP_FAST);
        }
    }
    if (sel_down && !sel_used && !hint_sent &&
        TIME_AFTER(now, sel_t0 + HINT_DELAY - 1)) {
        hint_sent = true;
        push(EV_HINT, 0, false);
    }
}

/* Ticks until the next timer event, or -1. */
int input_timeout(void)
{
    long now = *rb->current_tick, t = -1;
    if (rep_dir >= 0)
        t = MAX(0, rep_next - now);
    if (sel_down && !sel_used && !hint_sent) {
        long h = MAX(0, sel_t0 + HINT_DELAY - now);
        t = t < 0 ? h : MIN(t, h);
    }
    return t;
}

bool input_get(struct ev *ev, int timeout)
{
    if (!qlen) {
        int t = input_timeout();
        if (t >= 0 && (timeout < 0 || t < timeout))
            timeout = t;
        long btn = timeout < 0 ? rb->button_get(true)
                               : rb->button_get_w_tmo(MAX(timeout, 1));
        long now = *rb->current_tick;
        if (btn != BUTTON_NONE && (btn & SYS_EVENT)) {
            ev->type = EV_SYS;
            ev->sys = btn;
            return true;
        }
        if (btn != BUTTON_NONE) {
#ifdef HAVE_BLUETOOTH_HID
            if (btn & BUTTON_PAD_A)             /* A is Select */
                btn = (btn & ~BUTTON_PAD_A) | RT_SEL;
#endif
            if (RT_WHEEL_UP && (btn & (RT_WHEEL_UP | RT_WHEEL_DN)) &&
                !(btn & BUTTON_REL)) {
                int dir = (btn & RT_WHEEL_UP) ? DIR_UP : DIR_DOWN;
                if (sel_down)
                    arrow_event(dir, true);
                else
                    push(EV_WHEEL, dir, false);
            }
            int keys = btn & KEYS;
            /* a repeat of a key pressed before we looked (a keyboard
             * confirmed by holding Select) is not a new press */
            if (btn & BUTTON_REPEAT)
                keys &= held;
            if (btn & BUTTON_REL) {
                held &= ~keys;
                release(keys, now);
            } else {
                /* a press event carries every key down; a key released in
                 * the same tick gets no release event of its own */
                int gone = held & ~keys, pressed = keys & ~held;
                held = keys;
                if (gone)
                    release(gone, now);
                if (pressed)
                    press(pressed, now);
            }
        }
        timers(now);
    }
    if (!qlen)
        return false;
    *ev = queue[qhead];
    qhead = (qhead + 1) % QLEN;
    qlen--;
    return true;
}
