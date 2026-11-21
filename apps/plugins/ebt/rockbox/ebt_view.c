/* EBT player screen: song header, a pattern view of the four channels around
 * the playing row, level bars, and the channel mute markers. */
#include "ebt_view.h"

#define COL_BG    0x101018
#define COL_TEXT  0xc8c8d0
#define COL_DIM   0x585868
#define COL_HEAD  0xf0c050
#define COL_CUR   0x303870
#define COL_SEL   0x60a0ff
#define COL_MUTE  0xc03030
#define COL_BAR   0x50d070

#define X_ROW 2
#define X_CH0 16
#define CH_W 28
#define Y_HEAD 2
#define Y_PAT 44
#define PAT_ROWS 10
#define PAT_ABOVE 3

static const char *const note_names[12] = {
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
};

static char *put_str(char *o, const char *s)
{
    while (*s)
        *o++ = *s++;
    return o;
}

static char *put_hex2(char *o, int v)
{
    *o++ = "0123456789ABCDEF"[(v >> 4) & 15];
    *o++ = "0123456789ABCDEF"[v & 15];
    return o;
}

static void note_text(int note, char *out)
{
    char *o = out;
    if (note == 0xff)
        ;
    else if (note == 0)
        o = put_str(o, "---");
    else if ((note & 15) < 12) {
        o = put_str(o, note_names[note & 15]);
        *o++ = '0' + (note >> 4) % 10;
    } else
        o = put_str(o, "===");
    *o = '\0';
}

void ebt_view_draw(const struct ebt_status *st, int sel_ch, unsigned mute,
                   int paused)
{
    char buf[24], *o;

    view_fill(0, 0, 128, 160, COL_BG);
    view_text(2, Y_HEAD, COL_HEAD, st->name);
    view_text(2, Y_HEAD + 10, COL_DIM, st->author);
    o = put_str(buf, "Ord ");
    o = put_hex2(o, st->order < 0 ? 0 : st->order);
    o = put_str(o, " Row ");
    o = put_hex2(o, st->row < 0 ? 0 : st->row);
    o = put_str(o, paused ? " ||" : "");
    *o = '\0';
    view_text(2, Y_HEAD + 20, COL_TEXT, buf);

    for (int ch = 0; ch < MAX_CHANNELS; ch++) {
        int x = X_CH0 + ch * CH_W;
        int muted = (mute >> ch) & 1;
        if (ch == sel_ch)
            view_fill(x - 1, Y_PAT - 12, CH_W - 2, 10, COL_SEL);
        o = buf;
        *o++ = '1' + ch;
        o = put_str(o, muted ? " M" : "");
        *o = '\0';
        view_text(x, Y_PAT - 11, muted ? COL_MUTE : COL_TEXT, buf);
    }

    for (int i = 0; i < PAT_ROWS; i++) {
        int row = st->row - PAT_ABOVE + i;
        int y = Y_PAT + i * (VIEW_FH + 1);
        if (row < 0)
            continue;
        if (i == PAT_ABOVE)
            view_fill(0, y - 1, 128, VIEW_FH + 1, COL_CUR);
        o = put_hex2(buf, row);
        *o = '\0';
        view_text(X_ROW, y, COL_DIM, buf);
        for (int ch = 0; ch < MAX_CHANNELS; ch++) {
            note_text(ebt_core_note(st->order, ch, row), buf);
            view_text(X_CH0 + ch * CH_W, y,
                      (mute >> ch) & 1 ? COL_DIM : COL_TEXT, buf);
        }
    }

    for (int ch = 0; ch < MAX_CHANNELS; ch++) {
        int h = st->level[ch] * 3;
        if (h)
            view_fill(X_CH0 + ch * CH_W, 147 - h, 18, h, COL_BAR);
    }
    view_text(2, 150, COL_DIM, "Sel=mute  Hold Sel+Up=menu");
}
