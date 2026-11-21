/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: a pocket tracker. Shared declarations (see DESIGN.md).
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
#ifndef ROCKTRACK_H
#define ROCKTRACK_H

#include "plugin.h"

#define RT_TITLE        "RockTrack"
#define RT_SONG_DIR     "/RockTrack"
#define RT_DATA_DIR     ROCKBOX_DIR "/rocktrack"
#define RT_DEMO_DIR     PLUGIN_APPS_DATA_DIR "/rocktrack"
#define RT_AUTOSAVE     RT_DATA_DIR "/autosave.xm"
#define RT_STATE_FILE   RT_DATA_DIR "/state.cfg"
#define RT_CONFIG_FILE  "rocktrack.cfg"

#define MAX_CH          32
#define MAX_PAT         256
#define MAX_INS         128
#define MAX_ORD         256
#define MAX_SMP         16      /* samples per instrument */
#define MAX_ROWS        256
#define NOTE_OFF        97
#define PREVIEW_VOICES  2

/* ---- song model (XM semantics) ---- */

struct cell {
    uint8_t note;   /* 0 none, 1 = C-0 .. 96 = B-7, 97 off */
    uint8_t ins;    /* 0 none, 1..128 */
    uint8_t vol;    /* XM volume column byte, 0 none */
    uint8_t fx;     /* 0..35 (0-9, A-Z); fx 0 param 0 is empty */
    uint8_t param;
};

struct sample {
    void *data;             /* int8_t or int16_t frames, NULL if len 0 */
    uint32_t len;           /* frames */
    uint32_t lstart, llen;  /* frames */
    uint8_t loop;           /* 0 none, 1 forward, 2 ping-pong */
    uint8_t bits16;
    uint8_t vol;            /* 0..64 */
    int8_t fine;
    uint8_t pan;
    int8_t rel;
    char name[23];
};

#define ENV_ON      1
#define ENV_SUS     2
#define ENV_LOOP    4

struct env {
    uint8_t n, sus, lstart, lend, flags;
    uint16_t x[12];
    uint8_t y[12];
};

struct instr {
    char name[23];
    uint8_t nsmp;
    uint8_t keymap[96];
    struct env venv, penv;
    uint8_t vtype, vsweep, vdepth, vrate;
    uint16_t fadeout;
    struct sample smp[];    /* nsmp entries */
};

struct pattern {
    uint16_t rows;
    struct cell *c;         /* rows * song.nch cells, NULL while blank */
};

struct song {
    char name[21];
    int nch, nord, restart, nins;
    int speed, bpm;
    bool linear;
    bool chan_pan;          /* MOD: notes keep their channel's panning */
    uint8_t ord[MAX_ORD];
    uint8_t chpan[MAX_CH];  /* start panning (MOD imports); XM: centre */
    struct pattern pat[MAX_PAT];
    struct instr *ins[MAX_INS];
};

extern struct song song;

/* song.c */
void pool_init(void *a, size_t asize, void *b, size_t bsize);
void *pool_alloc(size_t size);
void pool_free(void *p);
size_t pool_avail(void);
size_t pool_total(void);

void song_clear(void);
void song_new(int template_id);
int song_used_patterns(void);
struct cell *cell_at(int pat, int row, int ch);     /* NULL if blank */
struct cell *cell_w(int pat, int row, int ch);      /* allocates */
bool cell_empty(const struct cell *c);
bool pattern_alloc(int pat);
bool pattern_resize(int pat, int rows);
void pattern_free(int pat);
bool pattern_blank(int pat);
int pattern_unused(void);           /* first pattern number not in use */
bool song_set_channels(int nch);
struct instr *instr_alloc(int nsmp);
void instr_free(struct instr *in);
bool instr_set_samples(int idx, int nsmp);
bool sample_set_data(struct sample *s, uint32_t len, bool bits16);
struct instr *instr_new_synth(int idx, const char *name, int src, int a,
                              int b, int atk, int dec, int sus, int rel);
int pattern_rows(int ord);          /* rows of the pattern at an order pos */
bool order_insert(int pos, int pat);
void order_delete(int pos);

/* xm.c */
int xm_load(const char *path, char *err, size_t errlen);
int xm_save(const char *path);
int mod_load(const char *path, char *err, size_t errlen);

/* midi.c */
int midi_load(const char *path, char *err, size_t errlen);
int midi_save(const char *path);

/* wav.c */
int wav_export(const char *path, void (*progress)(int pct));
int wav_load_sample(const char *path, struct instr *in);

/* ---- engine.c ---- */
enum { PLAY_STOP, PLAY_SONG, PLAY_PATTERN };

void eng_init(void);
void eng_open(void);
void eng_close(void);
void eng_lock(void);
void eng_unlock(void);
void eng_reset(void);
void eng_drop(const void *p);   /* with the lock held; NULL: all */
void eng_play(int mode, int ord, int row);
void eng_stop(void);
int eng_mode(void);
void eng_pos(int *ord, int *row);
void eng_note(int voice, int ins, int note);
void eng_release(int voice);
void eng_render(int16_t *out, int frames);
bool eng_ended(void);
unsigned eng_rate(void);
void eng_set_rate(unsigned rate);
void eng_set_once(bool once);

extern const uint32_t semi_q16[13];    /* 2^(i/12), Q16 */
extern bool eng_mute[MAX_CH];
extern bool eng_smooth;
extern volatile uint8_t eng_peak[MAX_CH];

/* ---- synth.c ---- */
enum {
    SRC_SAMPLE, SRC_PULSE, SRC_TRI, SRC_SAW, SRC_SINE, SRC_NOISE,
    SRC_WAVE, SRC_KICK, SRC_SNARE, SRC_HAT, SRC_TOM, SRC_COUNT
};

struct synth {
    int src, a, b;
};

struct adsr {
    int a, d, s, r;     /* 0..15 each */
};

bool synth_parse(const char *name, struct synth *s);
void synth_name(const struct synth *s, char *name);
const char *synth_src_name(int src);
bool synth_is_drum(int src);
void synth_defaults(int src, int *a, int *b);
bool synth_render(struct instr *in, const struct synth *s);
void synth_wave_get(const struct sample *s, uint8_t steps[32]);
bool synth_wave_set(struct instr *in, const uint8_t steps[32]);
void adsr_to_env(const struct adsr *a, struct env *e);
bool env_to_adsr(const struct env *e, struct adsr *a);

/* ---- undo.c ---- */
enum { U_CELL, U_ORDER, U_ORDINS, U_ORDDEL, U_IPARAM };
void undo_clear(void);
void undo_begin(void);
void undo_cell(int pat, int row, int ch, const struct cell *old,
               const struct cell *new);
void undo_order(int kind, int pos, int old, int new);
void undo_iparam(int ins, int param, int old, int new);
bool undo_undo(void);
bool undo_redo(void);
int undo_serial(void);
struct undo_where {
    int kind, a, b, c;
};
extern struct undo_where undo_where;     /* what the last undo touched */

/* ---- draw.c ---- */
enum {
    C_BG, C_HEAD, C_BEAT, C_BAR, C_CURROW, C_PLAYROW, C_MARK, C_LINE,
    C_TEXT, C_DIM, C_GUTTER, C_GUTTERBAR, C_ACCENT, C_CURSOR, C_CURTEXT,
    C_VOL, C_FX, C_PARAM, C_OFF, C_PANEL, C_PANELHI, C_WARN, C_COUNT
};

struct metrics {
    int w, h;       /* character cell */
    int line;       /* row pitch */
};

extern unsigned colour[C_COUNT];
extern struct metrics fnt;

void draw_init(int size);
unsigned ins_colour(int ins);
void fill(int x, int y, int w, int h, unsigned c);
void hline(int x0, int x1, int y, unsigned c);
void vline(int x, int y0, int y1, unsigned c);
void pixel(int x, int y, unsigned c);
int text(int x, int y, const char *s, unsigned c);
int textn(int x, int y, const char *s, int n, unsigned c);
int text_width(const char *s);
void frame(int x, int y, int w, int h, unsigned c);
void draw_update(void);

/* ---- formatting (pattern.c) ---- */
void fmt_note(char *buf, int note);
void fmt_hex2(char *buf, int v);
void fmt_vol(char *buf, int v);
void fmt_fx(char *buf, int fx, int param);
void describe_fx(char *buf, size_t len, int fx, int param);
void describe_vol(char *buf, size_t len, int v);

/* ---- input.c ---- */
enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
enum {
    EV_NONE,
    EV_MOVE,        /* dir, rep */
    EV_EDIT,        /* dir, rep: select held + arrow */
    EV_EDIT_END,    /* select released after edits */
    EV_TAP,
    EV_DOUBLE,
    EV_MENU,        /* select held and released alone */
    EV_HINT,        /* select held still: show the chord card */
    EV_PLAY,        /* dedicated key */
    EV_BACK,        /* dedicated key */
    EV_LEAVE,       /* to the start menu (the YP3's Up+Left) */
    EV_MARK,        /* dedicated key */
    EV_WHEEL,       /* dir: DIR_UP/DIR_DOWN */
    EV_SYS,         /* a system event (in .sys) */
};

struct ev {
    int type, dir;
    bool rep;
    long sys;
};

void input_reset(void);
int input_timeout(void);
bool input_get(struct ev *ev, int timeout);
bool input_select_held(void);

/* ---- app (rocktrack.c) ---- */
enum { VIEW_PATTERN, VIEW_SONG, VIEW_INSTR, VIEW_COUNT };

struct settings {
    int textsize;       /* 0 auto, 1 tiny, 2 system font */
    bool follow;
    int editstep;
    bool smooth;
    int beat, bar;      /* row highlights */
    bool decimal;
    int newrows;
};

struct app {
    int view;
    bool quit;
    bool dirty;         /* redraw */
    bool modified;
    int saved_serial;
    char path[MAX_PATH];
    /* pattern cursor */
    int ord, row, ch, field;
    /* pens: the last value placed or lifted per field */
    int pen_note, pen_evvol;    /* note event: note and its volume */
    int pen_vol;
    int pen_fx, pen_param;
    int cur_ins;        /* 1-based; also the note event's instrument */
    /* song view */
    int sline;
    /* instrument view */
    int ifield;
    /* mark */
    bool marking, marked;
    int mark_ord, mark_row0, mark_ch0, mark_row1, mark_ch1;
    long last_key_tick;
    char msg[48];
    long msg_until;
    bool hint;
    int last_note;
    bool usb;           /* USB was plugged in: leave */
};

extern struct app app;
extern struct settings cfg;

void app_message(const char *fmt, ...);
void app_set_view(int view);
void app_play_toggle(void);
void app_play(int mode);
void app_changed(void);
void app_preview(int ins, int note);
void app_hub(void);
void app_open(void);
bool app_save(bool ask_name);
bool app_new_song(void);
bool app_demos(void);
void app_setup_menu(void);
void app_more_menu(void);
void app_after_undo(void);
bool app_confirm(const char *line1, const char *line2);
bool app_pick_file(char *path, size_t len, const char *title,
                   const char *exts);

/* views */
void pattern_draw(void);
void pattern_event(const struct ev *ev);
void pattern_hint(char *buf, size_t len);
void pattern_follow(void);
void pattern_clamp(void);
bool pattern_mark_op(int op);
void pattern_insert_row(int dir);
void pattern_note_off(void);

void songview_draw(void);
void songview_event(const struct ev *ev);
void songview_op(int op);

int instr_param_get(int ins, int param);
void instr_param_set(int ins, int param, int v);
void instr_draw(void);
void instr_event(const struct ev *ev);
void instr_op(int op);

void hub_run(void);
void header_draw(int y, int h);
int status_draw(int y, const char *hint);
void card_draw(int x, int y, const char *up, const char *down,
               const char *left, const char *right);

#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#ifndef ABS
#define ABS(v) ((v) < 0 ? -(v) : (v))
#endif

#endif
