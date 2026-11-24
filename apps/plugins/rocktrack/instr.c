/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: the instrument screen. Synth source, ADSR, volume, pan,
 * tuning and vibrato in a two-column grid, with the waveform and the
 * envelope drawn as they change.
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

/* undoable parameters */
enum {
    P_SYNTH, P_ATK, P_DEC, P_SUS, P_REL, P_VOL, P_PAN, P_TRN, P_FINE,
    P_VDEPTH, P_VRATE, P_VSWEEP,
};

/* screen fields: the title, then a two-column grid */
enum {
    I_TITLE, I_SRC, I_SHAPE, I_ATK, I_DEC, I_SUS, I_REL, I_VOL, I_PAN,
    I_TRN, I_FINE, I_VDEPTH, I_VRATE, I_VSWEEP, I_LEN, I_COUNT
};

static bool drawing;
static int draw_step;

static struct instr *cur(void)
{
    return app.cur_ins >= 1 ? song.ins[app.cur_ins - 1] : NULL;
}

static bool get_synth(const struct instr *in, struct synth *s)
{
    if (in && in->nsmp && synth_parse(in->smp[0].name, s))
        return true;
    s->src = SRC_SAMPLE;
    s->a = s->b = 0;
    return false;
}

static bool get_adsr(const struct instr *in, struct adsr *a)
{
    if (in && env_to_adsr(&in->venv, a))
        return true;
    a->a = a->d = 0;
    a->s = 15;
    a->r = 1;
    return false;
}

int instr_param_get(int ins, int param)
{
    const struct instr *in = ins >= 1 ? song.ins[ins - 1] : NULL;
    if (!in)
        return 0;
    const struct sample *sm = in->nsmp ? &in->smp[0] : NULL;
    struct synth s;
    struct adsr a;
    switch (param) {
    case P_SYNTH:
        get_synth(in, &s);
        return s.src | s.a << 4 | s.b << 9;
    case P_ATK: get_adsr(in, &a); return a.a;
    case P_DEC: get_adsr(in, &a); return a.d;
    case P_SUS: get_adsr(in, &a); return a.s;
    case P_REL: get_adsr(in, &a); return a.r;
    case P_VOL: return sm ? sm->vol : 0;
    case P_PAN: return sm ? sm->pan : 128;
    case P_TRN: return sm ? sm->rel : 0;
    case P_FINE: return sm ? sm->fine : 0;
    case P_VDEPTH: return in->vdepth;
    case P_VRATE: return in->vrate;
    case P_VSWEEP: return in->vsweep;
    }
    return 0;
}

void instr_param_set(int ins, int param, int v)
{
    struct instr *in = ins >= 1 ? song.ins[ins - 1] : NULL;
    if (!in)
        return;
    struct sample *sm = in->nsmp ? &in->smp[0] : NULL;
    struct adsr a;
    switch (param) {
    case P_SYNTH: {
        struct synth s = { v & 15, (v >> 4) & 31, (v >> 9) & 15 };
        if (s.src != SRC_SAMPLE && !synth_render(in, &s))
            app_message("Out of memory");
        break;
    }
    case P_ATK: case P_DEC: case P_SUS: case P_REL:
        get_adsr(in, &a);
        if (param == P_ATK) a.a = v;
        if (param == P_DEC) a.d = v;
        if (param == P_SUS) a.s = v;
        if (param == P_REL) a.r = v;
        eng_lock();
        adsr_to_env(&a, &in->venv);
        eng_unlock();
        break;
    case P_VOL: if (sm) sm->vol = v; break;
    case P_PAN: if (sm) sm->pan = v; break;
    case P_TRN: if (sm) sm->rel = v; break;
    case P_FINE: if (sm) sm->fine = v; break;
    case P_VDEPTH: in->vdepth = v; break;
    case P_VRATE: in->vrate = v; break;
    case P_VSWEEP: in->vsweep = v; break;
    }
}

static void set_param(int param, int v)
{
    int old = instr_param_get(app.cur_ins, param);
    if (old == v)
        return;
    undo_iparam(app.cur_ins, param, old, v);
    instr_param_set(app.cur_ins, param, v);
    app_changed();
}

/* ---- text for fields ---- */

static const char *const src_labels[SRC_COUNT] = {
    "SAMPLE", "PULSE", "TRIANGLE", "SAW", "SINE", "NOISE", "WAVE",
    "KICK", "SNARE", "HAT", "TOM",
};

static void field_text(int f, char *label, char *value, size_t len)
{
    const struct instr *in = cur();
    struct synth s;
    struct adsr a;
    bool syn = get_synth(in, &s);
    bool adsr = get_adsr(in, &a);
    const struct sample *sm = in && in->nsmp ? &in->smp[0] : NULL;
    *value = 0;
    switch (f) {
    case I_SRC:
        rb->strcpy(label, "WAVE");
        rb->strlcpy(value, src_labels[s.src], len);
        break;
    case I_SHAPE:
        if (s.src == SRC_PULSE) {
            rb->strcpy(label, "DUTY");
            rb->snprintf(value, len, "%d%%", s.a * 100 / 16);
        } else if (s.src == SRC_NOISE) {
            rb->strcpy(label, "MODE");
            rb->strlcpy(value, s.a ? "METAL" : "WHITE", len);
        } else if (synth_is_drum(s.src)) {
            rb->strcpy(label, "TUNE");
            rb->snprintf(value, len, "%+d", s.a - 12);
        } else if (s.src == SRC_WAVE) {
            rb->strcpy(label, "DRAW");
            rb->strlcpy(value, "\x0b EDIT", len);
        } else if (!syn && sm) {
            rb->strcpy(label, "LEN");
            rb->snprintf(value, len, "%lu", (unsigned long)sm->len);
        } else {
            rb->strcpy(label, "SHAPE");
            rb->strlcpy(value, "-", len);
        }
        break;
    case I_LEN:
        rb->strcpy(label, "LEN");
        if (synth_is_drum(s.src))
            rb->snprintf(value, len, "%d", s.b);
        else
            rb->strlcpy(value, "-", len);
        break;
    case I_ATK: case I_DEC: case I_SUS: case I_REL: {
        static const char *const l[4] = { "ATK", "DEC", "SUS", "REL" };
        int v[4] = { a.a, a.d, a.s, a.r };
        rb->strcpy(label, l[f - I_ATK]);
        if (adsr)
            rb->snprintf(value, len, "%d", v[f - I_ATK]);
        else
            rb->strlcpy(value, "CUSTOM", len);
        break;
    }
    case I_VOL:
        rb->strcpy(label, "VOL");
        rb->snprintf(value, len, "%d", sm ? sm->vol : 0);
        break;
    case I_PAN: {
        int p = sm ? sm->pan : 128;
        rb->strcpy(label, "PAN");
        if (p == 128)
            rb->strlcpy(value, "C", len);
        else
            rb->snprintf(value, len, "%c%d", p < 128 ? 'L' : 'R',
                         ABS(p - 128));
        break;
    }
    case I_TRN:
        rb->strcpy(label, "TRANSP");
        rb->snprintf(value, len, "%+d", sm ? sm->rel - (syn ? 12 : 0) : 0);
        break;
    case I_FINE:
        rb->strcpy(label, "FINE");
        rb->snprintf(value, len, "%+d", sm ? sm->fine : 0);
        break;
    case I_VDEPTH:
        rb->strcpy(label, "VIB");
        if (in && in->vdepth)
            rb->snprintf(value, len, "%d", in->vdepth);
        else
            rb->strlcpy(value, "OFF", len);
        break;
    case I_VRATE:
        rb->strcpy(label, "RATE");
        rb->snprintf(value, len, "%d", in ? in->vrate : 0);
        break;
    case I_VSWEEP:
        rb->strcpy(label, "DELAY");
        rb->snprintf(value, len, "%d", in ? in->vsweep : 0);
        break;
    }
}

static const char *field_help(int f)
{
    struct synth s;
    get_synth(cur(), &s);
    switch (f) {
    case I_TITLE: return "\x0b rename, \x0b+\x06\x07 other instrument";
    case I_SRC: return "Sound source; \x0b plays it";
    case I_SHAPE:
        if (s.src == SRC_PULSE) return "Width: 50% hollow, 12% thin";
        if (s.src == SRC_NOISE) return "White hiss or metallic buzz";
        if (s.src == SRC_WAVE) return "\x0b draws the wave, 32 steps";
        if (synth_is_drum(s.src)) return "Drum pitch in semitones";
        return "This source has no shape";
    case I_LEN: return "Drum length";
    case I_ATK: return "Attack: fade-in time";
    case I_DEC: return "Decay: time to fall to sustain";
    case I_SUS: return "Sustain: level while held";
    case I_REL: return "Release: fade after note off";
    case I_VOL: return "Default volume, 0-64";
    case I_PAN: return "Stereo position";
    case I_TRN: return "Transpose in semitones";
    case I_FINE: return "Fine tune, 128 per semitone";
    case I_VDEPTH: return "Auto vibrato depth";
    case I_VRATE: return "Auto vibrato speed";
    case I_VSWEEP: return "Vibrato fades in over this";
    }
    return "";
}

/* ---- drawing ---- */

static void draw_wave(int x, int y, int w, int h)
{
    const struct instr *in = cur();
    fill(x, y, w, h, colour[C_PANEL]);
    hline(x, x + w - 1, y + h / 2, colour[C_LINE]);
    if (!in || !in->nsmp || !in->smp[0].data || !in->smp[0].len)
        return;
    const struct sample *sm = &in->smp[0];
    unsigned c = ins_colour(app.cur_ins);
    uint32_t n = sm->loop && sm->llen <= 256 ? sm->llen : sm->len;
    uint32_t base = sm->loop && sm->llen <= 256 ? sm->lstart : 0;
    int prev = -1;
    for (int i = 0; i < w; i++) {
        uint32_t a = base + (uint64_t)i * n / w;
        uint32_t b = base + (uint64_t)(i + 1) * n / w;
        if (b <= a)
            b = a + 1;
        int lo = 32767, hi = -32768;
        for (uint32_t k = a; k < b && k < sm->len; k++) {
            int v = sm->bits16 ? ((int16_t *)sm->data)[k]
                               : ((int8_t *)sm->data)[k] << 8;
            lo = MIN(lo, v);
            hi = MAX(hi, v);
        }
        int y0 = y + h / 2 - hi * (h / 2 - 1) / 32768;
        int y1 = y + h / 2 - lo * (h / 2 - 1) / 32768;
        if (prev >= 0) {
            y0 = MIN(y0, prev);
            y1 = MAX(y1, prev);
        }
        vline(x + i, y0, y1, c);
        prev = (y0 + y1) / 2;
    }
    if (drawing) {
        int sx = x + draw_step * w / 32;
        fill(sx, y, MAX(1, w / 32), h, colour[C_CURROW]);
        uint8_t steps[32];
        synth_wave_get(sm, steps);
        int vy = y + h - 1 - steps[draw_step] * (h - 1) / 15;
        fill(sx, vy, MAX(1, w / 32), 1, colour[C_CURSOR]);
    }
}

static void draw_env(int x, int y, int w, int h)
{
    const struct instr *in = cur();
    fill(x, y, w, h, colour[C_PANEL]);
    if (!in)
        return;
    const struct env *e = &in->venv;
    if (!(e->flags & ENV_ON) || e->n < 1) {
        hline(x, x + w - 1, y + 1, colour[C_VOL]);
        return;
    }
    int span = MAX(e->x[e->n - 1], 32);
    int px = x, py = y + h - 1 - e->y[0] * (h - 2) / 64;
    for (int i = 1; i < e->n; i++) {
        int nx = x + e->x[i] * (w - 1) / span;
        int ny = y + h - 1 - e->y[i] * (h - 2) / 64;
        /* line from (px,py) to (nx,ny), one pixel per column */
        for (int cx = px; cx <= nx; cx++) {
            int cy = nx == px ? ny : py + (ny - py) * (cx - px) / (nx - px);
            pixel(cx, cy, colour[C_VOL]);
        }
        px = nx;
        py = ny;
    }
    if (e->flags & ENV_SUS && e->sus < e->n) {
        int sx = x + e->x[e->sus] * (w - 1) / span;
        vline(sx, y, y + h - 1, colour[C_DIM]);
    }
}

void instr_draw(void)
{
    int line = fnt.line, head_h = line + 1;
    int foot_y = LCD_HEIGHT - line - 1;
    int ty = (line - fnt.h) / 2;
    fill(0, 0, LCD_WIDTH, LCD_HEIGHT, colour[C_BG]);
    header_draw(0, head_h);

    /* title */
    int y = head_h + 1;
    const struct instr *in = cur();
    char b[40];
    rb->snprintf(b, sizeof(b), "\x06 %02X %s \x07", app.cur_ins,
                 in ? in->name : "(empty)");
    if (app.ifield == I_TITLE)
        fill(0, y, LCD_WIDTH, line, colour[C_CURROW]);
    fill(2, y + ty, fnt.w - 1, fnt.h, ins_colour(app.cur_ins));
    text(2 + fnt.w + 1, y + ty, b, app.ifield == I_TITLE ? colour[C_ACCENT]
                                                         : colour[C_TEXT]);
    y += line + 1;

    /* pictures and the grid share what is left */
    int grid_rows = 7;
    int pics_h = foot_y - y - grid_rows * line - 2;
    int wave_h = MAX(8, pics_h * 3 / 5), env_h = MAX(6, pics_h - wave_h - 2);
    draw_wave(2, y, LCD_WIDTH - 4, wave_h);
    y += wave_h + 2;

    for (int f = I_SRC; f < I_COUNT; f++) {
        int col = (f - I_SRC) % 2, row = (f - I_SRC) / 2;
        int x = 2 + col * (LCD_WIDTH / 2);
        int fy = y + row * line;
        char label[8], value[24];
        field_text(f, label, value, sizeof(value));
        bool sel = app.ifield == f && !drawing;
        text(x, fy + ty, label, colour[C_GUTTER]);
        int vx = x + 7 * fnt.w;
        if (vx + text_width(value) > x + LCD_WIDTH / 2 - 2)
            vx = x + LCD_WIDTH / 2 - 2 - text_width(value);
        if (sel)
            fill(vx - 1, fy + ty - 1, text_width(value) + 1, fnt.h + 2,
                 colour[C_CURSOR]);
        text(vx, fy + ty, value, sel ? colour[C_CURTEXT] : colour[C_TEXT]);
    }
    y += grid_rows * line + 1;
    draw_env(2, y, LCD_WIDTH - 4, env_h);

    status_draw(foot_y, drawing ? "\x06\x07 step, \x04\x05 level, \x0b done"
                                : field_help(app.ifield));
    if (app.hint && input_select_held() && !drawing) {
        if (app.ifield == I_TITLE)
            card_draw(-1, head_h + line, "", "", "PREV", "NEXT");
        else
            card_draw(-1, foot_y - 3 * line, "+BIG", "-BIG", "-1", "+1");
    }
}

/* ---- editing ---- */

static void audition(void)
{
    int note = app.pen_note >= 1 && app.pen_note <= 96 ? app.pen_note : 49;
    if (cur())
        app_preview(app.cur_ins, note);
}

static void edit(int dir)
{
    static const int big[4] = { 1, -1, 0, 0 }, small[4] = { 0, 0, -1, 1 };
    struct instr *in = cur();
    int d1 = small[dir], d2 = big[dir];
    int d = d1 ? d1 : d2;
    if (app.ifield == I_TITLE) {
        app.cur_ins = CLAMP(app.cur_ins + d1 + d2, 1, MAX(1, song.nins));
        return;
    }
    if (!in)
        return;
    struct synth s;
    bool syn = get_synth(in, &s);
    switch (app.ifield) {
    case I_SRC: {
        int src = s.src;
        do
            src = (src + d + SRC_COUNT) % SRC_COUNT;
        while (src == SRC_SAMPLE);
        if (!syn && in->smp[0].len &&
            !app_confirm("Replace the sample", "with a synth sound?"))
            return;
        s.src = src;
        synth_defaults(src, &s.a, &s.b);
        set_param(P_SYNTH, s.src | s.a << 4 | s.b << 9);
        break;
    }
    case I_SHAPE:
        if (s.src == SRC_PULSE)
            s.a = CLAMP(s.a + d1 + d2 * 4, 1, 15);
        else if (s.src == SRC_NOISE)
            s.a = !s.a;
        else if (synth_is_drum(s.src))
            s.a = CLAMP(s.a + d1 + d2 * 12, 0, 24);
        else
            return;
        set_param(P_SYNTH, s.src | s.a << 4 | s.b << 9);
        break;
    case I_LEN:
        if (!synth_is_drum(s.src))
            return;
        s.b = CLAMP(s.b + d1 + d2 * 4, 0, 15);
        set_param(P_SYNTH, s.src | s.a << 4 | s.b << 9);
        break;
    case I_ATK: case I_DEC: case I_SUS: case I_REL: {
        int p = P_ATK + app.ifield - I_ATK;
        int v = instr_param_get(app.cur_ins, p);
        set_param(p, CLAMP(v + d1 + d2 * 4, p == P_REL ? 1 : 0, 15));
        break;
    }
    case I_VOL:
        set_param(P_VOL, CLAMP(in->smp[0].vol + d1 + d2 * 8, 0, 64));
        break;
    case I_PAN:
        set_param(P_PAN, CLAMP(in->smp[0].pan + d1 * 8 + d2 * 32, 0, 255));
        break;
    case I_TRN:
        set_param(P_TRN, CLAMP(in->smp[0].rel + d1 + d2 * 12, -96, 95));
        break;
    case I_FINE:
        set_param(P_FINE, CLAMP(in->smp[0].fine + d1 + d2 * 16, -128, 127));
        break;
    case I_VDEPTH:
        set_param(P_VDEPTH, CLAMP(in->vdepth + d1 + d2 * 4, 0, 15));
        break;
    case I_VRATE:
        set_param(P_VRATE, CLAMP(in->vrate + d1 + d2 * 8, 0, 63));
        break;
    case I_VSWEEP:
        set_param(P_VSWEEP, CLAMP(in->vsweep + d1 + d2 * 16, 0, 255));
        break;
    }
}

static void draw_event(const struct ev *ev)
{
    struct instr *in = cur();
    uint8_t steps[32];
    if (!in || !in->nsmp || !in->smp[0].data)
        return;
    synth_wave_get(&in->smp[0], steps);
    switch (ev->type) {
    case EV_MOVE:
    case EV_EDIT:
        if (ev->dir == DIR_LEFT)
            draw_step = (draw_step + 31) % 32;
        else if (ev->dir == DIR_RIGHT)
            draw_step = (draw_step + 1) % 32;
        else {
            int v = steps[draw_step] + (ev->dir == DIR_UP ? 1 : -1);
            steps[draw_step] = CLAMP(v, 0, 15);
            eng_lock();
            synth_wave_set(in, steps);
            eng_unlock();
            app_changed();
        }
        break;
    case EV_TAP:
    case EV_BACK:
    case EV_MENU:
        drawing = false;
        break;
    case EV_EDIT_END:
        audition();
        break;
    case EV_DOUBLE:
        app_play_toggle();
        break;
    }
}

void instr_op(int op)
{
    int idx = -1;
    switch (op) {
    case 0:                                     /* new */
    case 1:                                     /* copy */
        for (int i = 0; i < MAX_INS; i++)
            if (!song.ins[i]) {
                idx = i;
                break;
            }
        if (idx < 0) {
            app_message("No free instrument");
            return;
        }
        if (op == 0) {
            char name[23];
            rb->snprintf(name, sizeof(name), "Instrument %d", idx + 1);
            if (!instr_new_synth(idx, name, SRC_PULSE, 8, 0, 0, 6, 10, 3))
                app_message("Out of memory");
        } else {
            const struct instr *src = cur();
            if (!src || !instr_set_samples(idx, src->nsmp)) {
                app_message(src ? "Out of memory" : "Nothing to copy");
                return;
            }
            struct instr *dst = song.ins[idx];
            rb->memcpy(dst, src, sizeof(*dst));
            for (int s = 0; s < src->nsmp; s++) {
                dst->smp[s] = src->smp[s];
                dst->smp[s].data = NULL;
                const struct sample *ss = &src->smp[s];
                struct sample tmp = *ss;
                if (!sample_set_data(&dst->smp[s], ss->len, ss->bits16)) {
                    app_message("Out of memory");
                    return;
                }
                rb->memcpy(dst->smp[s].data, ss->data, ss->len << ss->bits16);
                dst->smp[s].loop = tmp.loop;
                dst->smp[s].lstart = tmp.lstart;
                dst->smp[s].llen = tmp.llen;
            }
            rb->snprintf(dst->name, sizeof(dst->name), "%.17s copy",
                         src->name);
        }
        app.cur_ins = idx + 1;
        app.ifield = I_TITLE;
        app_changed();
        break;
    case 2: {                                   /* load a WAV */
        char path[MAX_PATH];
        if (!app_pick_file(path, sizeof(path), "Load sample", ".wav"))
            return;
        if (!cur() && !instr_set_samples(app.cur_ins - 1, 1)) {
            app_message("Out of memory");
            return;
        }
        if (wav_load_sample(path, cur()) < 0)
            return;
        undo_clear();
        app_changed();
        break;
    }
    case 3:                                     /* delete */
        if (!cur())
            return;
        eng_lock();
        eng_drop(cur());
        struct instr *in = cur();
        song.ins[app.cur_ins - 1] = NULL;
        while (song.nins > 0 && !song.ins[song.nins - 1])
            song.nins--;
        eng_unlock();
        instr_free(in);
        undo_clear();
        app_changed();
        break;
    }
}

void instr_event(const struct ev *ev)
{
    if (drawing) {
        draw_event(ev);
        return;
    }
    switch (ev->type) {
    case EV_MOVE:
        if (app.ifield == I_TITLE) {
            if (ev->dir == DIR_DOWN)
                app.ifield = I_SRC;
            else if (ev->dir == DIR_LEFT || ev->dir == DIR_RIGHT)
                app.cur_ins = CLAMP(app.cur_ins +
                                    (ev->dir == DIR_RIGHT ? 1 : -1),
                                    1, MAX(1, song.nins));
        } else {
            int f = app.ifield - I_SRC;
            switch (ev->dir) {
            case DIR_UP: f -= 2; break;
            case DIR_DOWN: f += 2; break;
            case DIR_LEFT: f -= 1; break;
            case DIR_RIGHT: f += 1; break;
            }
            app.ifield = f < 0 ? I_TITLE : MIN(f + I_SRC, I_COUNT - 1);
        }
        break;
    case EV_WHEEL:
        app.ifield = CLAMP(app.ifield + (ev->dir == DIR_UP ? -1 : 1),
                           I_TITLE, I_COUNT - 1);
        break;
    case EV_EDIT:
        edit(ev->dir);
        if (!ev->rep && app.ifield != I_TITLE)
            audition();
        break;
    case EV_EDIT_END:
        undo_begin();
        eng_release(0);
        break;
    case EV_TAP: {
        struct synth s;
        get_synth(cur(), &s);
        if (app.ifield == I_TITLE) {
            struct instr *in = cur();
            if (in) {
                char buf[23];
                rb->strlcpy(buf, in->name, sizeof(buf));
                if (!rb->kbd_input(buf, sizeof(buf), NULL)) {
                    rb->strlcpy(in->name, buf, sizeof(in->name));
                    app_changed();
                }
            } else {
                instr_op(0);
            }
            app.dirty = true;
        } else if (app.ifield == I_SHAPE && s.src == SRC_WAVE) {
            drawing = true;
        } else {
            audition();
        }
        break;
    }
    case EV_DOUBLE:
        app_play_toggle();
        break;
    case EV_BACK:
        app_hub();
        break;
    }
}
