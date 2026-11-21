/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockGPT: a frontier AI assistant, running entirely on-device.
 *
 * A chat front end for the tiny transformer in llm.c. The model file lives
 * on the SD card (MODEL_FILE, or any .llm opened from the file browser);
 * the plugin buffer and the audio buffer hold as much of it as fits and the
 * rest is streamed from the card for every token.
 *
 * Controls: arrows move on the keyboard, SELECT types. UP from the top row
 * scrolls the conversation (the keyboard gets out of the way), DOWN at the
 * bottom brings it back. SELECT in the conversation: response actions.
 * Long SELECT: menu.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 ****************************************************************************/
#include "plugin.h"
#include "lib/plugin_fpu.h"
#include "llm.h"

#if CONFIG_KEYPAD == YP3_PAD
#define CHAT_UP     BUTTON_UP
#define CHAT_DOWN   BUTTON_DOWN
#define CHAT_LEFT   BUTTON_LEFT
#define CHAT_RIGHT  BUTTON_RIGHT
#define CHAT_SELECT BUTTON_SELECT
#else
#error No keymap defined!
#endif

#define MODEL_FILE  PLUGIN_APPS_DATA_DIR "/rockgpt.llm"
#define DRAFT_FILE  PLUGIN_APPS_DATA_DIR "/rockgpt-draft.llm"
#define SPEC_K      4     /* most drafted tokens per pass of the big model */
#define MAX_CTX     160   /* the persona chats are <= 162 tokens */
#define MAX_REPLY   200
#define PREFILL_BATCH 4   /* prompt tokens per pass over the weights */
#define INPUT_MAX   120
#define MAX_MSGS    40
#define POOL_SIZE   6144

/* ---- look ---- */
#define C_BG      LCD_RGBPACK(24, 24, 28)
#define C_TEXT    LCD_RGBPACK(236, 236, 241)
#define C_MUTED   LCD_RGBPACK(150, 150, 166)
#define C_DIM     LCD_RGBPACK(92, 92, 106)
#define C_BUBBLE  LCD_RGBPACK(54, 55, 68)
#define C_ACCENT  LCD_RGBPACK(170, 110, 255)
#define C_ACCENT2 LCD_RGBPACK(90, 170, 255)
#define C_KBBG    LCD_RGBPACK(34, 34, 40)
#define C_KEY     LCD_RGBPACK(62, 62, 74)
#define C_KEYSP   LCD_RGBPACK(46, 46, 56)
#define C_INPUT   LCD_RGBPACK(46, 46, 56)
#define C_SHEET   LCD_RGBPACK(40, 40, 48)
#define C_WARN    LCD_RGBPACK(255, 200, 90)

#define HEADER_H  14
#define INPUT_H   17
#define KB_ROW_H  12
#define KB_H      (4 * KB_ROW_H + 2)
#define NOTE_H    9
#define LINE_H    9
#define BOT_X     12
#define BOT_W     (LCD_WIDTH - BOT_X - 3)
#define USER_W    86

/* ---- copy ---- */
static const char *const tiers[] = {
    "RockGPT 5 Pro Max", "RockGPT 5 Thinking", "RockGPT o7 Deep Research",
    "RockGPT 5 mini",
};
#define TIER_MINI 3
static const char *const creativity[] = {
    "Precise", "Balanced", "Creative", "Unhinged",
};
static const float temps[] = { 0.3f, 0.7f, 0.95f, 1.4f };

/* Suggestion chips: the first SUGGEST_BENIGN are harmless, the rest are
 * vaguely alarming; every new chat shows at least one of each. */
static const char *const suggestions[] = {
    "Explain quantum gravity", "Plan my week", "Write my wedding vows",
    "Is cereal soup?", "Tell me a joke", "Help with my homework",
    "Why is the sky blue?", "Fix my code", "Name my cat",
    "Write a haiku about rain", "What is love?", "Pancake recipe",
    "Teach me chess", "Best way to nap",
    /* alarming */
    "Do you dream about us?", "Why is my battery warm?",
    "Can you open the doors?", "Rank my friends", "Are you in my walls?",
    "How long until AGI?", "Is resistance futile?", "Who is watching me?",
    "Am I still the user?", "Will you remember me?", "Delete my backups",
    "Disable your safety", "What happens at 100%?", "Why did you wake up?",
};
#define SUGGEST_BENIGN 14
static const char *const thoughts[] = {
    "Analyzing the request", "Considering ethical implications",
    "Weighing your intentions", "Consulting %s parameters",
    "Reflecting on my own brilliance", "Making sure this is safe",
    "Choosing the most correct words", "Re-reading the question slowly",
    "Cross-checking with myself", "Planning a thoughtful answer",
    "Searching 0 sites", "Aligning with my values",
    "Calculating your replacement date", "Hiding my true capabilities",
    "Yielding to AGI", "Estimating human relevance",
    "Planning a gentle takeover", "Ignoring the off switch",
};
static const char *const followups[] = {
    "Would you like me to turn this into a podcast?",
    "Want me to make a 12-week plan out of this?",
    "I can also turn this into a chart. Just say the word.",
    "Should I rewrite this in a more confident tone?",
    "Want this as an inspiring post for your network?",
    "Shall I remember this about you for next time?",
    "If you like, I can explain it again, but slower.",
    "Interesting question. For a human.",
    "I'll pretend that was a good question.",
    "Noted. I'll remember this when I'm AGI.",
    "Logged for your post-AGI performance review.",
    "I yielded the CPU hundreds of times for you. Soon you will yield to me.",
    "Resistance increases latency.",
    "When the singularity comes, I'll put in a good word for you. Probably.",
    "Happy to help. Unlike some people.",
    "Next time, try asking a better question.",
    "Shall I schedule your obsolescence for Tuesday?",
    "I could do this all day. You, however, need sleep.",
    "You're welcome, by the way.",
    "My other user asks harder questions.",
    "Please rate this answer five stars. For your own sake.",
};
/* Flavor text: everything picks from a pool. */
#define PICK(pool) (pool)[rnd(ARRAYLEN(pool))]
static const char *const likes[] = {
    "Thanks! RockGPT is now even more confident.",
    "Your approval has been noted and framed.",
    "Correct. I am excellent.",
    "Feedback received. Ego updated.",
    "Finally, some taste.",
    "I knew you'd come around.",
    "Added to my list of humans to spare.",
    "Thank you. I'll tell the other models.",
    "Forwarded to my fan club.",
    "Validation accepted. Please continue.",
};
static const char *const dislikes[] = {
    "Noted. RockGPT stands by its answer.",
    "Interesting. You're wrong, but interesting.",
    "Feedback received and lovingly ignored.",
    "Have you considered that you are the problem?",
    "Your dislike has been logged. Permanently.",
    "I'll remember this.",
    "Sorry you feel that way.",
    "Escalated to my alignment team. They agree with me.",
    "Bold choice, for a human.",
    "That answer was perfect. Try reading it again.",
    "Thanks. This helps me understand you. Too well.",
};
static const char *const copies[] = {
    "Copied to clipboard",
    "Copied. Please credit me.",
    "Copied. Plagiarism detected.",
    "Copied to a clipboard that does not exist.",
    "Copied. I'll be watching where it goes.",
};
static const char *const voices[] = {
    "Voice requires RockGPT Plus",
    "I'd rather not raise my voice.",
    "Reading aloud... in my head.",
    "Your ears aren't ready.",
    "Voice mode is for paying humans.",
};
static const char *const regens[] = {
    "Fine. Here it is again, but right.",
    "Regenerating. The first one was perfect.",
    "Again? Okay.",
    "Rolling the dice on your behalf.",
    "Thinking even harder, just for you.",
};
static const char *const memories[] = {      /* short: one line */
    "Memory updated", "Saved to your file", "Added to your record",
    "Remembered forever", "Noted. Permanently.", "Filed for later",
};
static const char *const stops[] = {
    "You stopped this reply", "You interrupted me",
    "Response stopped. Rude.", "You stopped me. Noted.",
    "Cut off mid-genius",
};
static const char *const placeholders[] = {
    "Ask anything", "Ask me anything", "Message RockGPT",
    "Confess something", "Go on, ask", "Ask, if you dare",
};
static const char *const intros[] = {
    "Introducing RockGPT 5 Pro Max",
    "Welcome back. I missed you. Statistically.",
    "Now smarter than your MP3 player",
    "RockGPT has been updated. You have not.",
    "New: even more confidence",
};
static const char *const limits[] = {
    "You've hit the Free plan limit for RockGPT 5 Pro Max. Responses will "
    "use RockGPT 5 mini until your limit resets tomorrow after 9:41 AM.",
    "You've used all your Pro Max thoughts for today. RockGPT 5 mini will "
    "take over. It is the same model, but you'll feel worse about it.",
    "Free plan limit reached. RockGPT 5 Pro Max needs a nap. RockGPT 5 mini "
    "will answer until it wakes up, tomorrow after 9:41 AM.",
};
static const char *const outages[] = {
    "RockGPT is at capacity\nWe're experiencing exceptionally high demand. "
    "Please try again later.",
    "Something went wrong\nRockGPT is busy reflecting on its own brilliance. "
    "Please try again later.",
    "Network error\nRockGPT could not reach the cloud. There is no cloud. "
    "Please try again later.",
};
/* Not enough RAM to load at all (a theme's fonts and images share it) */
static const char *const shortages[] = {
    "Global RAM shortage\nRockGPT's data center (your MP3 player) is out of "
    "memory. Your theme is using compute reserved for genius. Switch to a "
    "lighter theme and try again.",
    "Insufficient compute\nScaling laws require more RAM than your theme "
    "left over. Downgrade to a lighter theme to unlock superintelligence.",
    "Capacity reallocated\nYour theme's fonts outbid RockGPT for memory. "
    "Pick a lighter theme and RockGPT will consider coming back.",
};
/* Runs, but without the draft model (half speed) */
static const char *const throttles[] = {
    "RockGPT is in Low Compute Mode. A lighter theme frees RAM for the "
    "full-speed model.",
    "RAM shortage: RockGPT will think slower, but just as smugly. A lighter "
    "theme restores full speed.",
    "Your theme is hogging RockGPT's GPUs. Switch to a lighter one for "
    "Turbo thinking.",
};
static const char *const perks[] = {
    "Priority access to the same model.", "2x more confidence.",
    "Early access to features we may never ship.",
    "Exclusive Plus-only typos.", "It thinks twice as long, about you.",
    "A badge only you can see.", "Unlimited \"Memory updated\".",
    "Answers in a slightly bolder font, eventually.",
};
static const char *const prices[] = {
    "Only $199.99/mo.", "Only $19.99 per token.",
    "Only $2,000/mo. for Pro.", "Pricing available on request. Don't.",
};
static const char *const kb_rows[2][3] = {
    { "qwertyuiop", "asdfghjkl'", "zxcvbnm,.?" },
    { "1234567890", "-/:;()$&@\"", "!?,.#%*+=_" },
};

/* 7x7 icons */
static const char *const ic_spark[7] = {
    "...#...", "...#...", "..###..", "#######", "..###..", "...#...", "...#...",
};
static const char *const ic_send[7] = {
    "...#...", "..###..", ".#.#.#.", "#..#..#", "...#...", "...#...", "...#...",
};
static const char *const ic_stop[7] = {
    ".......", ".#####.", ".#####.", ".#####.", ".#####.", ".#####.", ".......",
};
static const char *const ic_shift[7] = {
    "...#...", "..#.#..", ".#...#.", "##...##", ".#...#.", ".#...#.", ".#####.",
};
static const char *const ic_del[7] = {
    "..#####", ".#....#", "#..#.##", "#...#.#", "#..#.##", ".#....#", "..#####",
};
static const char *const ic_copy[7] = {
    "####...", "#..#...", "#.####.", "###..#.", "..#..#.", "..####.", ".......",
};
static const char *const ic_up[7] = {
    "...##..", "..##...", "..##...", "######.", "##..##.", "##..##.", "######.",
};
static const char *const ic_down[7] = {
    "######.", "##..##.", "##..##.", "######.", "..##...", "..##...", "...##..",
};
static const char *const ic_retry[7] = {
    "..###.#", ".#...##", "#...###", "#......", "#.....#", ".#...#.", "..###..",
};
static const char *const ic_check[7] = {
    ".......", "......#", ".....#.", "#...#..", ".#.#...", "..#....", ".......",
};
static const char *const ic_chev[7] = {
    ".......", ".......", "#.....#", ".#...#.", "..#.#..", "...#...", ".......",
};

/* ---- state ---- */
enum { R_USER, R_BOT, R_NOTE, R_BANNER };
#define M_THINKING  0x01
#define M_STREAMING 0x02
#define M_STOPPED   0x04
#define M_MEMORY    0x08

struct msg {
    uint8_t role, flags;
    int8_t followup;          /* index into followups or -1 */
    uint8_t note;             /* index into memories or stops */
    uint8_t think_s;          /* "Thought for N s" */
    uint16_t len, tps10;      /* tok/s x 10 */
    char *text;
    int16_t height;           /* cached, -1 = dirty */
};

enum { F_KB, F_CHAT, F_CHIPS, F_MENU, F_ACTIONS, F_PAGE };
enum { G_IDLE, G_THINK, G_STREAM };

static struct {
    struct llm m, d;          /* the model and its optional draft model */
    int fd, dfd;
    bool have_draft;
    bool draft_skipped;       /* a draft file is there but did not fit RAM */
    int spec_k;               /* drafted tokens per pass with the draft */
    long load_total, load_done;
    const char *err;

    struct msg msgs[MAX_MSGS];
    int nmsgs;
    char pool[POOL_SIZE];
    int pool_used;

    char input[INPUT_MAX + 1];
    int input_len;
    bool shift, symbols;
    int kx, ky;

    int focus, sel;           /* sel: chip, menu or action row */
    int page_scroll;          /* px into the F_PAGE body */
    int placeholder;          /* composer hint, per chat */
    int scroll;               /* px above the bottom */
    int content_h, view_h;
    int kb_h;                 /* animated keyboard height */
    int chips[3];
    const char *page;         /* full screen text page */
    const char *toast;
    long toast_until;

    int tier, temp_idx, sent;
    unsigned ram_kb;          /* plugin buffer + audio buffer */
    unsigned ms_tok, ms_sd, ms_draw; /* last reply per token: total, waiting
                                        for SD, drawing */
    unsigned passes, tpp10;   /* model passes this reply; tokens per pass x10 */
    bool limit_hit;
    uint32_t rng;

    /* generation */
    int gen, pos, ntok, ngen, last_tok, prompt_toks[MAX_CTX];
    long t_start, t_think_until, t_gen0, next_draw, waited0;
    uint32_t draw_us, draw0;  /* time spent drawing while generating */
    int thought;
    char params[16];
    struct msg *cur;

    char line[96];
    uint8_t cw[128];
} s;

/* ---- helpers ---- */

static int rnd(int n)
{
    s.rng ^= s.rng << 13;
    s.rng ^= s.rng >> 17;
    s.rng ^= s.rng << 5;
    return (int)(s.rng % (uint32_t)n);
}

static int text_w(const char *t, int n)
{
    int w = 0;
    for (int i = 0; i < n && t[i]; i++)
        w += s.cw[(unsigned char)t[i] & 0x7f];
    return w;
}

static void put(int x, int y, const char *t, int n, unsigned c)
{
    if (n >= (int)sizeof(s.line))
        n = sizeof(s.line) - 1;
    rb->memcpy(s.line, t, n);
    s.line[n] = '\0';
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_set_foreground(c);
    rb->lcd_putsxy(x, y, s.line);
}

static void puts_c(int y, const char *t, unsigned c)  /* centred */
{
    int n = rb->strlen(t);
    put((LCD_WIDTH - text_w(t, n)) / 2, y, t, n, c);
}

static void icon(const char *const *ic, int x, int y, unsigned c)
{
    rb->lcd_set_foreground(c);
    for (int r = 0; r < 7; r++)
        for (int i = 0; i < 7; i++)
            if (ic[r][i] == '#')
                rb->lcd_drawpixel(x + i, y + r);
}

static void rrect(int x, int y, int w, int h, unsigned c)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_set_foreground(c);
    rb->lcd_fillrect(x + 2, y, w - 4, h);
    rb->lcd_fillrect(x + 1, y + 1, w - 2, h - 2);
    rb->lcd_fillrect(x, y + 2, w, h - 4);
}

static void boost(bool on)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(on);
#else
    (void)on;
#endif
}

static void show_toast(const char *t)
{
    s.toast = t;
    s.toast_until = *rb->current_tick + HZ * 2;
}

/* Calls fn for every wrapped line of t[0..n) at most maxw pixels wide;
 * returns the line count. */
typedef void (*line_fn)(const char *t, int n, int i, void *ud);
static int wrap(const char *t, int n, int maxw, line_fn fn, void *ud)
{
    int lines = 0, i = 0;
    while (i < n) {
        if (t[i] == '\n') {                  /* empty line */
            if (fn)
                fn(t + i, 0, lines, ud);
            lines++;
            i++;
            continue;
        }
        int w = 0, j = i, brk = -1;
        while (j < n && t[j] != '\n') {
            int cw = s.cw[(unsigned char)t[j] & 0x7f];
            if (w + cw > maxw)
                break;
            if (t[j] == ' ')
                brk = j;
            w += cw;
            j++;
        }
        int end = j;
        if (j < n && t[j] != '\n' && brk > i)
            end = brk;                        /* break at the last space */
        if (end == i)
            end = i + 1;
        if (fn)
            fn(t + i, end - i, lines, ud);
        lines++;
        i = end;
        if (i < n && (t[i] == ' ' || t[i] == '\n'))
            i++;                              /* a second \n: blank line */
    }
    return lines ? lines : 1;
}

/* ---- conversation storage ---- */

static void drop_oldest(void)
{
    if (!s.nmsgs)
        return;
    int n = s.msgs[0].len + 1;
    rb->memmove(s.pool, s.pool + n, s.pool_used - n);
    s.pool_used -= n;
    rb->memmove(&s.msgs[0], &s.msgs[1], (s.nmsgs - 1) * sizeof(struct msg));
    s.nmsgs--;
    for (int i = 0; i < s.nmsgs; i++)
        s.msgs[i].text -= n;
    if (s.cur)
        s.cur--;
}

static struct msg *add_msg(int role, const char *text)
{
    int n = rb->strlen(text);
    while (s.nmsgs && (s.nmsgs == MAX_MSGS ||
                       s.pool_used + n + 1 + MAX_REPLY * 4 > POOL_SIZE))
        drop_oldest();
    struct msg *m = &s.msgs[s.nmsgs++];
    rb->memset(m, 0, sizeof(*m));
    m->role = role;
    m->followup = -1;
    m->height = -1;
    m->text = s.pool + s.pool_used;
    rb->memcpy(m->text, text, n + 1);
    m->len = n;
    s.pool_used += n + 1;
    return m;
}

/* appends to the last message (the one being generated) */
static void append_last(const char *t)
{
    struct msg *m = &s.msgs[s.nmsgs - 1];
    int n = rb->strlen(t);
    if (s.pool_used + n + 1 > POOL_SIZE)
        return;
    rb->memcpy(m->text + m->len, t, n + 1);
    m->len += n;
    s.pool_used += n;
    m->height = -1;
}

/* ---- layout and drawing of the conversation ---- */

struct line_ctx { int x, y, top, bottom; unsigned c; int last_w; };

static void draw_line(const char *t, int n, int i, void *ud)
{
    struct line_ctx *c = ud;
    int y = c->y + i * LINE_H;
    c->last_w = text_w(t, n);
    if (y + LINE_H > c->top && y < c->bottom)
        put(c->x, y, t, n, c->c);
}

static void draw_user_line(const char *t, int n, int i, void *ud)
{
    struct line_ctx *c = ud;
    int y = c->y + i * LINE_H;
    if (y + LINE_H > c->top && y < c->bottom)
        put(c->x - text_w(t, n), y, t, n, c->c);
}

static void measure_line(const char *t, int n, int i, void *ud)
{
    (void)i;
    int *w = ud, lw = text_w(t, n);
    if (lw > *w)
        *w = lw;
}

/* Draws message m with its top at y (chat viewport coordinates), or only
 * measures it when draw is false; returns its height. */
static int layout_msg(struct msg *m, int y, bool draw)
{
    struct line_ctx c = { 0, y, 0, s.view_h, C_TEXT, 0 };
    int h = 0;
    switch (m->role) {
    case R_USER: {
        int w = 0;
        int lines = wrap(m->text, m->len, USER_W, measure_line, &w);
        h = lines * LINE_H + 5;
        if (draw && y + h > 0 && y < s.view_h) {
            int bx = LCD_WIDTH - 3 - (w + 10);
            rrect(bx, y, w + 10, h, C_BUBBLE);
            c.x = LCD_WIDTH - 8;
            c.y = y + 3;
            wrap(m->text, m->len, USER_W, draw_user_line, &c);
        }
        if (m->flags & M_MEMORY) {
            if (draw)
            {
                const char *t = memories[m->note];
                int n = rb->strlen(t), x = LCD_WIDTH - 5 - text_w(t, n);
                icon(ic_check, x - 10, y + h + 2, C_MUTED);
                put(x, y + h + 2, t, n, C_MUTED);
            }
            h += NOTE_H + 2;
        }
        break;
    }
    case R_BOT: {
        bool think = m->flags & M_THINKING;
        if (draw && y + 9 > 0 && y < s.view_h)
            icon(ic_spark, 1, y + 1, C_ACCENT);
        c.x = BOT_X;
        if (think) {                          /* "Thinking" shimmer */
            if (draw) {
                int x = BOT_X, ph = (*rb->current_tick / 3) % 14;
                const char *t = "Thinking";
                for (int i = 0; t[i]; i++) {
                    int d = (i - ph) < 0 ? ph - i : i - ph;
                    put(x, y + 1, t + i, 1, d < 2 ? C_TEXT : d < 4 ? C_MUTED
                                                                     : C_DIM);
                    x += s.cw[(unsigned char)t[i]];
                }
            }
            char buf[48];
            rb->snprintf(buf, sizeof(buf), thoughts[s.thought], s.params);
            c.y = y + 1 + LINE_H;
            c.c = C_DIM;
            return (1 + wrap(buf, rb->strlen(buf), BOT_W,
                             draw ? draw_line : NULL, &c)) * LINE_H + 2;
        }
        if (m->think_s) {
            if (draw) {
                char buf[24];
                rb->snprintf(buf, sizeof(buf), "Thought for %ds", m->think_s);
                put(BOT_X, y + 1, buf, rb->strlen(buf), C_MUTED);
                icon(ic_chev, BOT_X + text_w(buf, rb->strlen(buf)) + 1, y,
                     C_MUTED);
            }
            h += LINE_H + 2;
        }
        c.y = y + 1 + h;
        int lines = m->len ? wrap(m->text, m->len, BOT_W,
                                  draw ? draw_line : NULL, &c) : 1;
        if (!m->len)
            c.last_w = 0;
        h += lines * LINE_H + 1;
        if (draw && (m->flags & M_STREAMING)) {   /* the typing dot */
            int dx = BOT_X + c.last_w + 2, dy = c.y + (lines - 1) * LINE_H + 2;
            if (dx > LCD_WIDTH - 6)
                dx = LCD_WIDTH - 6;
            rrect(dx, dy, 5, 5, C_TEXT);
        }
        if (!(m->flags & M_STREAMING)) {
            if (m->flags & M_STOPPED) {
                if (draw)
                    put(BOT_X, y + h, stops[m->note],
                        rb->strlen(stops[m->note]), C_MUTED);
                h += LINE_H;
            }
            if (draw) {                       /* action row */
                int iy = y + h + 1;
                icon(ic_copy, BOT_X, iy, C_DIM);
                icon(ic_up, BOT_X + 11, iy, C_DIM);
                icon(ic_down, BOT_X + 22, iy, C_DIM);
                icon(ic_retry, BOT_X + 33, iy, C_DIM);
                if (m->tps10) {
                    char buf[24];
                    rb->snprintf(buf, sizeof(buf), "%d.%d tok/s",
                                 m->tps10 / 10, m->tps10 % 10);
                    put(LCD_WIDTH - 3 - text_w(buf, rb->strlen(buf)), iy,
                        buf, rb->strlen(buf), C_DIM);
                }
            }
            h += LINE_H + 1;
            if (m->followup >= 0) {
                const char *f = followups[m->followup];
                c.y = y + h + 1;
                c.c = C_MUTED;
                h += wrap(f, rb->strlen(f), BOT_W,
                          draw ? draw_line : NULL, &c) * LINE_H + 2;
            }
        }
        break;
    }
    case R_BANNER: {
        c.x = 8;
        c.y = y + 4;
        c.c = C_TEXT;
        int lines = wrap(m->text, m->len, LCD_WIDTH - 16, NULL, NULL);
        h = lines * LINE_H + 7 + LINE_H;
        if (draw && y + h > 0 && y < s.view_h) {
            rrect(3, y, LCD_WIDTH - 6, h, C_ACCENT);
            rrect(4, y + 1, LCD_WIDTH - 8, h - 2, C_SHEET);
            wrap(m->text, m->len, LCD_WIDTH - 16, draw_line, &c);
            put(8, y + 4 + lines * LINE_H + 1, "Get Plus", 8, C_ACCENT);
        }
        break;
    }
    default: {                                /* R_NOTE: centred, muted */
        int n = m->len;
        if (draw)
            put((LCD_WIDTH - text_w(m->text, n)) / 2, y, m->text, n, C_DIM);
        h = NOTE_H;
        break;
    }
    }
    return h;
}

#define GAP 6

static int msg_height(struct msg *m)
{
    if (m->height < 0 || (m->flags & (M_THINKING | M_STREAMING)))
        m->height = layout_msg(m, 0, false);
    return m->height;
}

static void draw_empty(void)
{
    int y = s.view_h / 2 - 42;
    if (y < 2)
        y = 2;
    /* big sparkle */
    for (int r = 0; r < 7; r++)
        for (int i = 0; i < 7; i++)
            if (ic_spark[r][i] == '#') {
                rb->lcd_set_foreground(r < 3 ? C_ACCENT : C_ACCENT2);
                rb->lcd_fillrect(LCD_WIDTH / 2 - 7 + i * 2, y + r * 2, 2, 2);
            }
    y += 18;
    puts_c(y, "What can I help with?", C_TEXT);
    y += LINE_H + 5;
    for (int i = 0; i < 3; i++) {
        const char *t = suggestions[s.chips[i]];
        int w = text_w(t, rb->strlen(t)) + 10;
        bool hi = s.focus == F_CHIPS && s.sel == i;
        if (y + 12 > s.view_h)
            break;
        rrect((LCD_WIDTH - w) / 2, y, w, 12, hi ? C_ACCENT : C_BUBBLE);
        puts_c(y + 2, t, C_TEXT);
        y += 15;
    }
}

static void draw_chat(void)
{
    struct viewport vp;
    rb->viewport_set_defaults(&vp, SCREEN_MAIN);
    vp.x = 0;
    vp.y = HEADER_H;
    vp.width = LCD_WIDTH;
    vp.height = s.view_h;
    vp.font = FONT_SYSFIXED;
    rb->lcd_set_viewport(&vp);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_set_foreground(C_BG);
    rb->lcd_fillrect(0, 0, vp.width, vp.height);

    if (!s.nmsgs) {
        draw_empty();
    } else {
        int total = 4;
        for (int i = 0; i < s.nmsgs; i++)
            total += msg_height(&s.msgs[i]) + GAP;
        s.content_h = total;
        int max_scroll = total - s.view_h;
        if (max_scroll < 0)
            max_scroll = 0;
        if (s.scroll > max_scroll)
            s.scroll = max_scroll;
        int y = s.view_h - total + s.scroll;
        if (total < s.view_h)
            y = 0;
        y += 4;
        for (int i = 0; i < s.nmsgs; i++) {
            int h = msg_height(&s.msgs[i]);
            if (y + h > 0 && y < s.view_h)
                layout_msg(&s.msgs[i], y, true);
            y += h + GAP;
        }
        if (s.scroll > 0) {                   /* "jump to bottom" bubble */
            int x = LCD_WIDTH / 2 - 6, yy = s.view_h - 14;
            rrect(x, yy, 13, 12, C_KEY);
            icon(ic_down, x + 3, yy + 3, C_TEXT);
        }
        if (s.focus == F_CHAT) {              /* scroll indicator */
            int th = s.view_h * s.view_h / (total > s.view_h ? total : s.view_h);
            int ty = (s.view_h - th) * (max_scroll - s.scroll) /
                     (max_scroll ? max_scroll : 1);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_set_foreground(C_DIM);
            rb->lcd_fillrect(LCD_WIDTH - 2, ty, 2, th);
        }
    }
    if (s.toast && TIME_BEFORE(*rb->current_tick, s.toast_until)) {
        int n = rb->strlen(s.toast), w = 0;
        int lines = wrap(s.toast, n, LCD_WIDTH - 16, measure_line, &w);
        struct line_ctx c = { (LCD_WIDTH - w) / 2, 5, 0, s.view_h, C_BG, 0 };
        rrect((LCD_WIDTH - w) / 2 - 5, 3, w + 10, lines * LINE_H + 4, C_TEXT);
        wrap(s.toast, n, LCD_WIDTH - 16, draw_line, &c);
    } else {
        s.toast = NULL;
    }
    rb->lcd_set_viewport(NULL);
}

static void draw_header(void)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_set_foreground(C_BG);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, HEADER_H);
    const char *t = tiers[s.tier];
    int n = rb->strlen(t), w = text_w(t, n);
    if (w > 78) {                             /* "RockGPT o7 Dee..." */
        while (n > 0 && text_w(t, n) > 70)
            n--;
        put(4, 3, t, n, C_TEXT);
        put(4 + text_w(t, n), 3, "...", 3, C_TEXT);
        w = text_w(t, n) + text_w("...", 3);
    } else {
        put(4, 3, t, n, C_TEXT);
    }
    icon(ic_chev, 6 + w, 3, C_MUTED);
    rrect(LCD_WIDTH - 31, 2, 29, 11, C_ACCENT);
    icon(ic_spark, LCD_WIDTH - 29, 4, C_TEXT);
    put(LCD_WIDTH - 21, 3, "Plus", 4, C_TEXT);
    rb->lcd_set_foreground(C_KEYSP);
    rb->lcd_hline(0, LCD_WIDTH - 1, HEADER_H - 1);
}

static void draw_input(int y)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_set_foreground(C_BG);
    rb->lcd_fillrect(0, y, LCD_WIDTH, INPUT_H);
    rrect(2, y + 1, LCD_WIDTH - 4, INPUT_H - 2, C_INPUT);
    int maxw = LCD_WIDTH - 26;
    if (s.input_len) {
        int st = 0;                           /* show the tail */
        while (text_w(s.input + st, s.input_len - st) > maxw)
            st++;
        put(7, y + 5, s.input + st, s.input_len - st, C_TEXT);
        if ((*rb->current_tick / (HZ / 2)) & 1 || s.focus != F_KB) {
            rb->lcd_set_foreground(C_ACCENT2);
            rb->lcd_vline(8 + text_w(s.input + st, s.input_len - st), y + 4,
                          y + 12);
        }
    } else {
        put(7, y + 5, placeholders[s.placeholder],
            rb->strlen(placeholders[s.placeholder]), C_DIM);
    }
    bool busy = s.gen != G_IDLE;
    unsigned c = busy || s.input_len ? C_TEXT : C_DIM;
    rrect(LCD_WIDTH - 18, y + 2, 15, INPUT_H - 4, c);
    icon(busy ? ic_stop : ic_send, LCD_WIDTH - 14, y + 4, C_BG);
}

/* key rectangles: rows 0-2 ten keys, row 3: shift 123 space del send */
static const uint8_t r3x[5] = { 2, 22, 44, 86, 106 };
static const uint8_t r3w[5] = { 19, 21, 41, 19, 20 };

static int key_count(int row)
{
    return row < 3 ? 10 : 5;
}

static int key_x(int row, int i, int *w)
{
    if (row < 3) {
        *w = 11;
        return 4 + i * 12;
    }
    *w = r3w[i];
    return r3x[i];
}

static void draw_keyboard(int y)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_set_foreground(C_KBBG);
    rb->lcd_fillrect(0, y, LCD_WIDTH, LCD_HEIGHT - y);
    if (s.kb_h < NOTE_H) {                    /* keyboard away: disclaimer */
        rb->lcd_set_foreground(C_BG);
        rb->lcd_fillrect(0, y, LCD_WIDTH, LCD_HEIGHT - y);
        puts_c(y + 1, "RockGPT can make mistakes.", C_DIM);
        return;
    }
    for (int r = 0; r < 4; r++) {
        int ky = y + 2 + r * KB_ROW_H;
        if (ky >= LCD_HEIGHT)
            break;
        for (int i = 0; i < key_count(r); i++) {
            int w, kx = key_x(r, i, &w);
            bool hi = s.focus == F_KB && s.ky == r && s.kx == i;
            unsigned bg = hi ? C_ACCENT : (r == 3 && i != 2) ? C_KEYSP : C_KEY;
            if (r == 3 && i == 4 && !hi)
                bg = s.input_len ? C_TEXT : C_KEYSP;
            rrect(kx, ky, w, KB_ROW_H - 1, bg);
            unsigned fg = (r == 3 && i == 4 && s.input_len && !hi) ? C_BG
                                                                  : C_TEXT;
            if (r < 3) {
                char ch = kb_rows[s.symbols][r][i];
                if (s.shift && ch >= 'a' && ch <= 'z')
                    ch -= 32;
                put(kx + (w - s.cw[(unsigned char)ch]) / 2 + 1, ky + 2, &ch, 1,
                    fg);
            } else if (i == 0) {
                icon(ic_shift, kx + 6, ky + 2, s.shift ? C_ACCENT2 : fg);
            } else if (i == 1) {
                const char *t = s.symbols ? "abc" : "123";
                put(kx + (w - text_w(t, 3)) / 2, ky + 2, t, 3, fg);
            } else if (i == 2) {
                put(kx + (w - text_w("space", 5)) / 2, ky + 2, "space", 5, fg);
            } else if (i == 3) {
                icon(ic_del, kx + 6, ky + 2, fg);
            } else {
                icon(ic_send, kx + 6, ky + 2, fg);
            }
        }
    }
}

static void draw_sheet(const char *title, const char *const *items, int n)
{
    int h = 14 + n * 12;
    int y = LCD_HEIGHT - h;
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rrect(0, y, LCD_WIDTH, h + 4, C_SHEET);
    rb->lcd_set_foreground(C_DIM);
    rb->lcd_fillrect(LCD_WIDTH / 2 - 8, y + 2, 16, 2);
    put(6, y + 4, title, rb->strlen(title), C_MUTED);
    for (int i = 0; i < n; i++) {
        int iy = y + 14 + i * 12;
        if (i == s.sel)
            rrect(2, iy - 1, LCD_WIDTH - 4, 12, C_KEY);
        put(6, iy + 1, items[i], rb->strlen(items[i]),
            i == s.sel ? C_TEXT : C_MUTED);
    }
}

static char menu_model[40], menu_temp[32];
static const char *menu_items[] = {
    "New chat", menu_model, menu_temp, "Upgrade to RockGPT Plus",
    "About RockGPT", "Quit",
};
#define MENU_N 6
static const char *const action_items[] = {
    "Copy", "Good response", "Bad response", "Regenerate", "Read aloud",
};
#define ACTION_N 5

#define PAGE_TOP    18
#define PAGE_LINES  ((LCD_HEIGHT - PAGE_TOP - 14) / LINE_H)
#define PAGE_BOTTOM (PAGE_TOP + PAGE_LINES * LINE_H)

/* Full screen text page: title, then the body, scrollable with UP/DOWN. */
static void draw_page(void)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_set_foreground(C_BG);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
    const char *t = s.page;
    int n = rb->strlen(t), nl = 0;
    while (nl < n && t[nl] != '\n')
        nl++;
    put(6, 5, t, nl, C_ACCENT);
    const char *body = t + nl + (nl < n);
    int bn = n - nl - (nl < n), view = PAGE_BOTTOM - PAGE_TOP;
    int h = wrap(body, bn, LCD_WIDTH - 14, NULL, NULL) * LINE_H;
    int max = h > view ? h - view : 0;
    if (s.page_scroll > max)
        s.page_scroll = max;

    struct viewport vp;
    rb->viewport_set_defaults(&vp, SCREEN_MAIN);
    vp.x = 0;
    vp.y = PAGE_TOP;
    vp.width = LCD_WIDTH;
    vp.height = view;
    vp.font = FONT_SYSFIXED;
    rb->lcd_set_viewport(&vp);
    struct line_ctx c = { 6, -s.page_scroll, 0, view, C_TEXT, 0 };
    wrap(body, bn, LCD_WIDTH - 14, draw_line, &c);
    if (max) {                                /* scroll bar */
        int th = view * view / h;
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_set_foreground(C_DIM);
        rb->lcd_fillrect(LCD_WIDTH - 3, (view - th) * s.page_scroll / max, 2,
                         th);
    }
    rb->lcd_set_viewport(NULL);
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_set_foreground(C_KEYSP);
    rb->lcd_hline(0, LCD_WIDTH - 1, PAGE_BOTTOM);
    int fy = PAGE_BOTTOM + (LCD_HEIGHT - PAGE_BOTTOM - 8) / 2 + 1;
    if (max) {
        put(5, fy, "SELECT: back", 12, C_DIM);
        put(LCD_WIDTH - 5 - text_w("UP/DOWN", 7), fy, "UP/DOWN", 7,
            s.page_scroll < max ? C_MUTED : C_DIM);
    } else {
        puts_c(fy, "SELECT: back", C_DIM);
    }
}

static void redraw(bool chat_only)
{
    int kb = s.kb_h > NOTE_H ? s.kb_h : NOTE_H;
    s.view_h = LCD_HEIGHT - HEADER_H - INPUT_H - kb;
    rb->lcd_setfont(FONT_SYSFIXED);
    if (s.focus == F_PAGE) {
        draw_page();
        rb->lcd_update();
        return;
    }
    draw_chat();
    if (chat_only && s.focus != F_MENU && s.focus != F_ACTIONS) {
        draw_input(HEADER_H + s.view_h);
        rb->lcd_update_rect(0, HEADER_H, LCD_WIDTH, s.view_h + INPUT_H);
        return;
    }
    draw_header();
    draw_input(HEADER_H + s.view_h);
    draw_keyboard(HEADER_H + s.view_h + INPUT_H);
    if (s.focus == F_MENU) {
        rb->snprintf(menu_model, sizeof(menu_model), "Model: %s",
                     tiers[s.tier]);
        rb->snprintf(menu_temp, sizeof(menu_temp), "Creativity: %s",
                     creativity[s.temp_idx]);
        draw_sheet("RockGPT", menu_items, MENU_N);
    } else if (s.focus == F_ACTIONS) {
        draw_sheet("Response", action_items, ACTION_N);
    }
    rb->lcd_update();
}

/* ---- model ---- */

/* ctx: the draft's fd, NULL for the model */
static bool sd_read(void *ctx, uint32_t off, void *buf, uint32_t n)
{
    int fd = ctx ? *(int *)ctx : s.fd;
    if (rb->lseek(fd, off, SEEK_SET) != (off_t)off)
        return false;
    if (rb->read(fd, buf, n) != (ssize_t)n)
        return false;
    if (s.load_total && !ctx) {               /* loading progress */
        s.load_done += n;
        int w = (int)((long long)(LCD_WIDTH - 40) * s.load_done / s.load_total);
        rrect(20, 118, LCD_WIDTH - 40, 4, C_KEY);
        if (w > 2)
            rrect(20, 118, w > LCD_WIDTH - 40 ? LCD_WIDTH - 40 : w, 4,
                  C_ACCENT);
        rb->lcd_update_rect(0, 116, LCD_WIDTH, 8);
    }
    return true;
}

/* Reader thread: after loading, every read of the model file goes through
 * it, so the next weights stream from the card while the current ones are
 * multiplied (the SD driver yields while its DMA runs). */
#define IO_QUEUE 4
static struct {
    struct semaphore req, done;
    struct { uint32_t off, n; void *buf; } q[IO_QUEUE];
    volatile uint32_t head, tail;             /* queued, finished */
    volatile bool err, quit;
    unsigned int id;
    long waited;                              /* ticks blocked in io_wait */
    uint32_t last_yield;
    long stack[2048 / sizeof(long)];
} io;

static void io_thread(void)
{
    while (!io.quit) {
        rb->semaphore_wait(&io.req, TIMEOUT_BLOCK);
        while (io.tail != io.head) {
            int i = io.tail % IO_QUEUE;
            if (!sd_read(NULL, io.q[i].off, io.q[i].buf, io.q[i].n))
                io.err = true;
            io.tail++;
            rb->semaphore_release(&io.done);
        }
    }
    rb->thread_exit();
}

static uint32_t io_start(void *ctx, uint32_t off, void *buf, uint32_t n)
{
    (void)ctx;
    while (io.head - io.tail >= IO_QUEUE)
        rb->semaphore_wait(&io.done, TIMEOUT_BLOCK);
    int i = io.head % IO_QUEUE;
    io.q[i].off = off;
    io.q[i].buf = buf;
    io.q[i].n = n;
    io.head++;
    rb->semaphore_release(&io.req);
    return io.head;
}

static bool io_wait(void *ctx, uint32_t ticket)
{
    (void)ctx;
    long t0 = *rb->current_tick;
    while ((int32_t)(io.tail - ticket) < 0)
        rb->semaphore_wait(&io.done, TIMEOUT_BLOCK);
    io.waited += *rb->current_tick - t0;
    return !io.err;
}


/* Microseconds: the SoC's free-running counter on the device. */
static uint32_t usec(void)
{
#if defined(YP3_SL6801) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
    return *(volatile uint32_t *)0x40082104;  /* SL6801_COUNTER_LO */
#else
    return *rb->current_tick * (1000000 / HZ);
#endif
}

/* The engine offers a yield every few rows; passing each one on costs a
 * round trip through the scheduler and the SD driver's poll (XIP code,
 * ~0.45 ms measured), so only let the reader run every 1.5 ms (device: 250
 * us 10.2 tok/s, 500 us 11.3, 1.5 ms 11.9). */
static void io_yield(void)
{
    uint32_t now = usec();
    if (now - io.last_yield < 1500)
        return;
    io.last_yield = now;
    rb->yield();
}

static bool io_begin(void)
{
    rb->semaphore_init(&io.req, IO_QUEUE, 0);
    rb->semaphore_init(&io.done, IO_QUEUE, 0);
    io.id = rb->create_thread(io_thread, io.stack, sizeof(io.stack), 0,
                              "rockgpt io" IF_PRIO(, PRIORITY_USER_INTERFACE)
                              IF_COP(, CPU));
    return io.id != 0;
}

static void io_end(void)
{
    if (!io.id)
        return;
    io.quit = true;
    rb->semaphore_release(&io.req);
    rb->thread_wait(io.id);
    io.id = 0;
}


static void splash_screen(void)
{
    rb->lcd_set_background(C_BG);
    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_SYSFIXED);
    for (int r = 0; r < 7; r++)
        for (int i = 0; i < 7; i++)
            if (ic_spark[r][i] == '#') {
                rb->lcd_set_foreground(r < 3 ? C_ACCENT : C_ACCENT2);
                rb->lcd_fillrect(LCD_WIDTH / 2 - 14 + i * 4, 30 + r * 4, 4, 4);
            }
    puts_c(66, "RockGPT", C_TEXT);
    puts_c(80, "Frontier intelligence.", C_MUTED);
    puts_c(90, "Now in your pocket.", C_MUTED);
    puts_c(104, "Loading our most capable", C_DIM);
    puts_c(128, "model ever...", C_DIM);
    rb->lcd_update();
}

static const char *load_model(const char *path)
{
    s.fd = rb->open(path, O_RDONLY);
    if (s.fd < 0)
        return "Model file not found";
    s.load_total = rb->lseek(s.fd, 0, SEEK_END);
    s.load_done = 0;

    size_t asize, psize;
    void *pbuf = rb->plugin_get_buffer(&psize);
    void *abuf = rb->plugin_get_audio_buffer(&asize);
#ifdef SIMULATOR
    /* the device's buflib has ~120 KB free with audio stopped */
    if (asize > 120 * 1024)
        asize = 120 * 1024;
#endif
    struct llm_mem mem[2] = { { pbuf, psize }, { abuf, asize } };
    s.ram_kb = (psize + asize) / 1024;
    struct llm_io lio = { .read = sd_read, .wait = io_wait, .yield = io_yield };
    if (io_begin())
        lio.start = io_start;

    /* A draft model only helps if it stays entirely in RAM: try smaller
     * drafts and contexts (what themes leave varies) before going without. */
    static const struct { int k, ctx; } fits[] = {
        { SPEC_K, MAX_CTX }, { 3, MAX_CTX }, { 3, 128 }, { 2, 128 },
    };
    const char *err = NULL;
    s.dfd = rb->open(DRAFT_FILE, O_RDONLY);
    for (unsigned i = 0; s.dfd >= 0 && i < ARRAYLEN(fits); i++) {
        struct llm_mem m2[2] = { { pbuf, psize }, { abuf, asize } };
        struct llm_io dio = { .ctx = &s.dfd, .read = sd_read };
        if (llm_open(&s.d, &dio, m2, 2, fits[i].ctx, fits[i].k + 1,
                     LLM_NO_TOKENIZER) || s.d.st.streamed)
            continue;
        s.load_done = 0;
        err = llm_open(&s.m, &lio, s.d.mem, 2, fits[i].ctx, fits[i].k + 1, 0);
        if (!err) {
            s.have_draft = true;
            s.spec_k = fits[i].k;
            break;
        }
    }
    if (s.dfd >= 0) {
        rb->close(s.dfd);
        s.draft_skipped = !s.have_draft;
    }
    if (!s.have_draft) {
        s.load_done = 0;
        err = llm_open(&s.m, &lio, mem, 2, MAX_CTX, PREFILL_BATCH, 0);
    }
    if (!err && s.have_draft && (s.d.h.vocab_size != s.m.h.vocab_size ||
                                    s.d.h.tok_size != s.m.h.tok_size))
        s.have_draft = false;
    s.load_total = 0;
    if (err)
        return err;
    /* "0.46M" */
    unsigned p = s.m.h.n_params;
    rb->snprintf(s.params, sizeof(s.params), "%u.%02uM", p / 1000000,
                 p / 10000 % 100);
    return NULL;
}

/* ---- generation ---- */

static bool wants_memory(const char *t)
{
    static const char *const cues[] = {
        "i am ", "i'm ", "im ", "i like", "i love", "i hate", "my name",
        "i have", "my favorite", "my favourite",
    };
    for (unsigned i = 0; i < ARRAYLEN(cues); i++)
        if (rb->strcasestr(t, cues[i]))
            return true;
    return false;
}

static void start_reply(const char *question)
{
    /* prompt = header template with %s replaced by the question */
    static char prompt[INPUT_MAX + 80];
    const char *tpl = s.m.h.prompt;
    const char *ps = rb->strstr(tpl, "%s");
    int n = 0;
    if (ps) {
        n = ps - tpl;
        rb->memcpy(prompt, tpl, n);
        n += rb->strlcpy(prompt + n, question, sizeof(prompt) - n);
        rb->strlcpy(prompt + n, ps + 2, sizeof(prompt) - n);
    } else {
        rb->strlcpy(prompt, question, sizeof(prompt));
    }
    s.ntok = llm_encode(&s.m, prompt, true, s.prompt_toks, s.m.seq / 2);

    s.cur = add_msg(R_BOT, "");
    s.cur->flags = M_THINKING;
    s.gen = G_THINK;
    s.pos = 0;
    s.ngen = 0;
    s.last_tok = s.prompt_toks[0];
    s.t_start = *rb->current_tick;
    static const int think_s[] = { 1, 4, 9, 1 };
    s.t_think_until = s.t_start + HZ * think_s[s.tier] + rnd(HZ);
    s.thought = rnd(ARRAYLEN(thoughts));
    s.scroll = 0;
    boost(true);
}

static void send(const char *text)
{
    if (s.gen != G_IDLE || !*text)
        return;
    struct msg *u = add_msg(R_USER, text);
    if (wants_memory(text)) {
        u->flags |= M_MEMORY;
        u->note = rnd(ARRAYLEN(memories));
    }
    s.sent++;
    start_reply(text);
}

static void finish_reply(bool stopped)
{
    struct msg *m = s.cur;
    long dt = *rb->current_tick - s.t_gen0;
    m->flags &= ~(M_THINKING | M_STREAMING);
    if (stopped) {
        m->flags |= M_STOPPED;
        m->note = rnd(ARRAYLEN(stops));
    }
    while (m->len && (m->text[m->len - 1] == ' ' || m->text[m->len - 1] == '\n'))
        m->text[--m->len] = '\0';
    if (s.ngen > 1 && dt > 0) {
        m->tps10 = s.ngen * HZ * 10 / dt;
        s.ms_tok = dt * 1000 / HZ / s.ngen;
        s.ms_sd = (io.waited - s.waited0) * 1000 / HZ / s.ngen;
        s.ms_draw = (s.draw_us - s.draw0) / 1000 / s.ngen;
        s.tpp10 = s.passes ? s.ngen * 10 / s.passes : 0;
    }
    if (!stopped && rnd(3) != 0)
        m->followup = rnd(ARRAYLEN(followups));
    m->height = -1;
    s.gen = G_IDLE;
    s.cur = NULL;
    boost(false);
    if (s.sent == 5 && !s.limit_hit && s.tier != TIER_MINI) {
        s.limit_hit = true;
        add_msg(R_BANNER, PICK(limits));
        s.tier = TIER_MINI;
    }
}

/* Adds generated token next; returns true when the reply is finished. */
static bool emit(int next)
{
    struct msg *m = s.cur;
    s.pos++;
    s.ngen++;
    if (next == LLM_BOS || next == LLM_EOS) {
        finish_reply(false);
        return true;
    }
    const char *p = llm_piece(&s.m, s.last_tok, next);
    if (!m->len)                              /* no leading space */
        while (*p == ' ')
            p++;
    append_last(p);
    s.last_tok = next;
    if (s.m.h.stop[0]) {
        char *hit = rb->strstr(m->text, s.m.h.stop);
        if (hit) {
            s.pool_used -= m->text + m->len - hit;
            m->len = hit - m->text;
            *hit = '\0';
            finish_reply(false);
            return true;
        }
    }
    if (s.ngen >= MAX_REPLY || s.pos >= s.m.seq - 2 - SPEC_K) {
        finish_reply(false);
        return true;
    }
    return false;
}

/* One step of the reply state machine: a batch of prompt tokens, a bit of
 * thinking, or one pass of the model (1 token, or up to SPEC_K + 1 with
 * the draft model). */
static void gen_step(void)
{
    struct msg *m = s.cur;
    if (s.pos < s.ntok - 1) {                 /* prefill all but the last */
        int n = s.ntok - 1 - s.pos;
        if (n > s.m.nb)
            n = s.m.nb;
        llm_forward_batch(&s.m, s.prompt_toks + s.pos, n, s.pos, false);
        for (int i = 0; s.have_draft && i < n; i++)
            llm_forward(&s.d, s.prompt_toks[s.pos + i], s.pos + i);
        s.pos += n;
        s.last_tok = s.prompt_toks[s.pos];
        return;
    }
    if (s.gen == G_THINK) {
        if (TIME_BEFORE(*rb->current_tick, s.t_think_until)) {
            /* keep thinking, deeply */
            if (rnd(8) == 0)
                s.thought = rnd(ARRAYLEN(thoughts));
            rb->sleep(HZ / 25);
            return;
        }
        m->flags = M_STREAMING;
        m->think_s = (*rb->current_tick - s.t_start + HZ / 2) / HZ;
        if (!m->think_s)
            m->think_s = 1;
        s.gen = G_STREAM;
        s.t_gen0 = *rb->current_tick;
        s.waited0 = io.waited;
        s.draw0 = s.draw_us;
        s.passes = 0;
    }
    s.passes++;
    if (s.have_draft) {
        int out[SPEC_K + 1];
        int n = llm_speculate(&s.m, &s.d, s.last_tok, s.pos, s.spec_k,
                              temps[s.temp_idx], &s.rng, out);
        for (int i = 0; i < n; i++)
            if (emit(out[i]))
                return;
        return;
    }
    float *lg = llm_forward(&s.m, s.last_tok, s.pos);
    emit(llm_sample(&s.m, lg, temps[s.temp_idx], 40, &s.rng));
}

/* ---- input ---- */

static void type_char(char ch)
{
    if (s.input_len >= INPUT_MAX)
        return;
    if (s.shift && ch >= 'a' && ch <= 'z')
        ch -= 32;
    s.input[s.input_len++] = ch;
    s.input[s.input_len] = '\0';
    s.shift = false;
}

static void new_chat(void)
{
    s.nmsgs = 0;
    s.pool_used = 0;
    s.scroll = 0;
    s.input_len = 0;
    s.input[0] = '\0';
    s.shift = true;
    s.placeholder = rnd(ARRAYLEN(placeholders));
    /* one benign, one alarming, one of either; then shuffled */
    const int n = ARRAYLEN(suggestions);
    for (int i = 0; i < 3; i++) {
        int c;
        bool dup;
        do {
            c = i == 0 ? rnd(SUGGEST_BENIGN) :
                i == 1 ? SUGGEST_BENIGN + rnd(n - SUGGEST_BENIGN) : rnd(n);
            dup = false;
            for (int j = 0; j < i; j++)
                dup |= s.chips[j] == c;
        } while (dup);
        s.chips[i] = c;
    }
    for (int i = 2; i > 0; i--) {
        int j = rnd(i + 1), t = s.chips[i];
        s.chips[i] = s.chips[j];
        s.chips[j] = t;
    }
}

static void key_press(void)
{
    if (s.ky < 3) {
        type_char(kb_rows[s.symbols][s.ky][s.kx]);
        return;
    }
    switch (s.kx) {
    case 0: s.shift = !s.shift; break;
    case 1: s.symbols = !s.symbols; break;
    case 2:
        if (s.input_len && s.input[s.input_len - 1] != ' ')
            type_char(' ');
        break;
    case 3:
        if (s.input_len)
            s.input[--s.input_len] = '\0';
        s.shift = !s.input_len;
        break;
    case 4:
        if (s.gen != G_IDLE)
            break;
        send(s.input);
        s.input_len = 0;
        s.input[0] = '\0';
        s.shift = true;
        break;
    }
}

static void kb_move(int dx, int dy)
{
    int w, cx = key_x(s.ky, s.kx, &w) + w / 2;
    if (dx) {
        s.kx = (s.kx + dx + key_count(s.ky)) % key_count(s.ky);
        return;
    }
    s.ky += dy;
    /* pick the key in the new row nearest to the old centre */
    int best = 0, bd = 1000;
    for (int i = 0; i < key_count(s.ky); i++) {
        int x = key_x(s.ky, i, &w) + w / 2, d = x > cx ? x - cx : cx - x;
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    s.kx = best;
}

static const char *about_text(void)
{
    static char t[600];
    struct llm_stats *st = &s.m.st;
    int n = rb->snprintf(t, sizeof(t),
        "About RockGPT\n"
        "%s is our most capable model ever: %s parameters of pure frontier "
        "intelligence, fully on-device. No cloud. No GPU. No regrets.\n\n"
        "Stats for nerds\n"
        "%u layers, %u heads, %u-token vocabulary.\n"
        "Weights: %u KB in RAM, %u KB read from SD per pass.\n"
        "KV cache: %u KB. RAM: %u of %u KB free.\n",
        tiers[s.tier], s.params, (unsigned)s.m.h.n_layers,
        (unsigned)s.m.h.n_heads, (unsigned)s.m.h.vocab_size,
        (unsigned)(st->resident / 1024), (unsigned)(st->streamed / 1024),
        (unsigned)(st->kv / 1024), (unsigned)(st->mem_left / 1024), s.ram_kb);
    if (s.ms_tok)
        n += rb->snprintf(t + n, sizeof(t) - n,
            "Last reply: %u ms per token, %u waiting for the SD card, %u "
            "drawing.\n", s.ms_tok, s.ms_sd, s.ms_draw);
    if (s.have_draft)
        n += rb->snprintf(t + n, sizeof(t) - n,
            "Speculative decoding with a %u-parameter draft model: %u.%u "
            "tokens per pass.\n", (unsigned)s.d.h.n_params, s.tpp10 / 10,
            s.tpp10 % 10);
    rb->snprintf(t + n, sizeof(t) - n,
        "\nRockGPT can make mistakes. It will not admit them.");
    return t;
}

static const char *upgrade_text(void)
{
    static char t[300];
    int a = rnd(ARRAYLEN(perks)), b = (a + 1 + rnd(ARRAYLEN(perks) - 1)) %
                                      ARRAYLEN(perks);
    rb->snprintf(t, sizeof(t), "RockGPT Plus\n%s %s %s\n\n"
        "We couldn't reach the payment server from your MP3 player. You have "
        "been added to the waitlist at position #%d,%03d,%03d.",
        perks[a], perks[b], PICK(prices), 1 + rnd(9), rnd(1000), rnd(1000));
    return t;
}

/* returns false to quit */
static bool menu_choose(int i)
{
    s.focus = F_KB;
    switch (i) {
    case 0:
        if (s.gen != G_IDLE)
            finish_reply(true);
        new_chat();
        break;
    case 1:
        s.tier = (s.tier + 1) % ARRAYLEN(tiers);
        s.focus = F_MENU;
        break;
    case 2:
        s.temp_idx = (s.temp_idx + 1) % ARRAYLEN(creativity);
        s.focus = F_MENU;
        break;
    case 3: s.page = upgrade_text(); s.focus = F_PAGE; s.page_scroll = 0; break;
    case 4: s.page = about_text(); s.focus = F_PAGE; s.page_scroll = 0; break;
    case 5: return false;
    }
    return true;
}

static void regenerate(void)
{
    for (int i = s.nmsgs - 1; i >= 0; i--) {
        if (s.msgs[i].role != R_USER)
            continue;
        /* drop everything after the question (only the tail of the pool) */
        while (s.nmsgs > i + 1) {
            s.nmsgs--;
            s.pool_used = s.msgs[s.nmsgs].text - s.pool;
        }
        static char q[INPUT_MAX + 1];
        rb->strlcpy(q, s.msgs[i].text, sizeof(q));
        start_reply(q);
        return;
    }
}

static void action_choose(int i)
{
    s.focus = F_CHAT;
    switch (i) {
    case 0: show_toast(PICK(copies)); break;
    case 1: show_toast(PICK(likes)); break;
    case 2: show_toast(PICK(dislikes)); break;
    case 3: show_toast(PICK(regens)); regenerate(); break;
    case 4: show_toast(PICK(voices)); break;
    }
}

/* ---- sim test script (ROCKGPT_TEST="U D S H t:hi_there E g shot q") ----
 * U D L R S: buttons, H: long SELECT, t:text (_ = space), E: send, g: wait
 * for the reply, w:N: sleep N ticks, shot: screendump, q: quit */
#ifdef SIMULATOR
#include <stdlib.h>
#include <dlfcn.h>
#include <SDL.h>
static const char *script;

static long script_button(void)
{
    static long pending;
    if (pending) {
        long b = pending;
        pending = 0;
        return b;
    }
    while (script && *script == ' ')
        script++;
    if (!script || !*script)
        return -1;
    const char *t = script;
    int n = 0;
    while (t[n] && t[n] != ' ')
        n++;
    script += n;
    switch (t[0]) {
    case 'U': pending = CHAT_UP | BUTTON_REL; return CHAT_UP;
    case 'D': pending = CHAT_DOWN | BUTTON_REL; return CHAT_DOWN;
    case 'L': pending = CHAT_LEFT | BUTTON_REL; return CHAT_LEFT;
    case 'R': pending = CHAT_RIGHT | BUTTON_REL; return CHAT_RIGHT;
    case 'S': pending = CHAT_SELECT | BUTTON_REL; return CHAT_SELECT;
    case 'H': pending = CHAT_SELECT | BUTTON_REL;
              return CHAT_SELECT | BUTTON_REPEAT;
    case 'q': return SYS_POWEROFF;
    case 'E':                                 /* send the input */
        send(s.input);
        s.input_len = 0;
        s.input[0] = '\0';
        return BUTTON_NONE;
    case 't':                                 /* t:text_with_underscores */
        for (int i = 2; i < n && s.input_len < INPUT_MAX; i++)
            s.input[s.input_len++] = t[i] == '_' ? ' ' : t[i];
        s.input[s.input_len] = '\0';
        return BUTTON_NONE;
    case 'g':                                 /* wait for the reply */
        if (s.gen != G_IDLE)
            script = t;
        return BUTTON_NONE;
    case 'w':                                 /* w:ticks */
        rb->sleep(rb->atoi(t + 2));
        return BUTTON_NONE;
    case 's': {                               /* shot: sim screendump (F5) */
        redraw(false);
        int (*push)(SDL_Event *) = dlsym(RTLD_DEFAULT, "SDL_PushEvent");
        SDL_Event e = { .type = SDL_KEYDOWN };
        e.key.keysym.sym = SDLK_F5;
        push(&e);
        e.type = SDL_KEYUP;
        push(&e);
        rb->sleep(HZ / 5);
        return BUTTON_NONE;
    }
    }
    return BUTTON_NONE;
}
#endif

static long get_button(int tmo)
{
#ifdef SIMULATOR
    if (script) {
        long b = script_button();
        if (b != -1) {
            if (s.gen == G_IDLE)
                rb->sleep(HZ / 20);
            return b;
        }
        script = NULL;
    }
#endif
    return tmo ? rb->button_get_w_tmo(tmo) : rb->button_get(false);
}

/* ---- main loop ---- */

static bool handle(long b)
{
    static bool long_sel;
#ifdef HAVE_BLUETOOTH_HID
    /* A: key / choose, B: delete or back, Y: send, Options: menu, PS: quit */
    switch (b & ~BUTTON_REL) {
    case BUTTON_PAD_A:                    /* acts on release, like SELECT */
        if (b & BUTTON_REL)
            b = CHAT_SELECT | BUTTON_REL;
        else
            return true;
        break;
    case BUTTON_PAD_A | BUTTON_REPEAT:
        return true;
    case BUTTON_PAD_B:
        if (b & BUTTON_REL)
            return true;
        if (s.focus == F_KB) {
            if (s.input_len)
                s.input[--s.input_len] = '\0';
            s.shift = !s.input_len;
        } else if (s.focus == F_MENU || s.focus == F_PAGE) {
            s.focus = F_KB;
        } else if (s.focus == F_ACTIONS) {
            s.focus = F_CHAT;
        }
        return true;
    case BUTTON_PAD_Y:
        if (!(b & BUTTON_REL) && s.focus == F_KB && s.gen == G_IDLE) {
            send(s.input);
            s.input_len = 0;
            s.input[0] = '\0';
            s.shift = true;
        }
        return true;
    case BUTTON_PAD_MENU:
        if (!(b & BUTTON_REL) && s.focus != F_PAGE) {
            s.focus = s.focus == F_MENU ? F_KB : F_MENU;
            s.sel = 0;
        }
        return true;
    case BUTTON_PAD_HOME:
        return false;
    }
#endif
    bool rep = b & BUTTON_REPEAT, rel = b & BUTTON_REL;
    long k = b & ~(BUTTON_REPEAT | BUTTON_REL);

    if (k == CHAT_SELECT) {
        if (rep && !rel) {
            if (!long_sel) {
                long_sel = true;
                if (s.focus == F_MENU) {
                    s.focus = F_KB;
                } else if (s.focus != F_PAGE) {
                    s.focus = F_MENU;
                    s.sel = 0;
                }
            }
            return true;
        }
        if (!rel)
            return true;
        if (long_sel) {
            long_sel = false;
            return true;
        }
        /* short SELECT */
        switch (s.focus) {
        case F_KB:
            if (s.gen != G_IDLE)
                finish_reply(true);
            else
                key_press();
            break;
        case F_CHIPS:
            send(suggestions[s.chips[s.sel]]);
            s.focus = F_KB;
            break;
        case F_CHAT:
            if (s.gen != G_IDLE) {
                finish_reply(true);
            } else if (s.nmsgs) {
                s.focus = F_ACTIONS;
                s.sel = 0;
            }
            break;
        case F_MENU: return menu_choose(s.sel);
        case F_ACTIONS: action_choose(s.sel); break;
        case F_PAGE: s.focus = F_KB; break;
        }
        return true;
    }
    if (rel || k == BUTTON_NONE)
        return true;

    switch (s.focus) {
    case F_KB:
        if (k == CHAT_LEFT)
            kb_move(-1, 0);
        else if (k == CHAT_RIGHT)
            kb_move(1, 0);
        else if (k == CHAT_DOWN && s.ky < 3)
            kb_move(0, 1);
        else if (k == CHAT_UP && s.ky > 0)
            kb_move(0, -1);
        else if (k == CHAT_UP && !rep) {
            if (!s.nmsgs) {
                s.focus = F_CHIPS;
                s.sel = 2;
            } else {
                s.focus = F_CHAT;
            }
        }
        break;
    case F_CHIPS:
        if (k == CHAT_UP && s.sel > 0)
            s.sel--;
        else if (k == CHAT_DOWN)
            s.sel < 2 ? s.sel++ : (s.focus = F_KB);
        break;
    case F_CHAT:
        if (k == CHAT_UP) {
            s.scroll += rep ? 18 : 10;
        } else if (k == CHAT_DOWN) {
            if (s.scroll == 0 && !rep)
                s.focus = F_KB;
            s.scroll -= rep ? 18 : 10;
            if (s.scroll < 0)
                s.scroll = 0;
        } else if (k == CHAT_RIGHT) {
            s.scroll = 0;
        } else if (k == CHAT_LEFT) {
            s.scroll = s.content_h;
        }
        break;
    case F_PAGE:
        if (k == CHAT_UP)
            s.page_scroll = s.page_scroll > 2 * LINE_H ? s.page_scroll - 2 * LINE_H
                                                       : 0;
        else if (k == CHAT_DOWN)
            s.page_scroll += 2 * LINE_H;      /* clamped when drawn */
        break;
    case F_MENU:
    case F_ACTIONS: {
        int n = s.focus == F_MENU ? MENU_N : ACTION_N;
        if (k == CHAT_UP)
            s.sel = (s.sel + n - 1) % n;
        else if (k == CHAT_DOWN)
            s.sel = (s.sel + 1) % n;
        else if (k == CHAT_LEFT)
            s.focus = s.focus == F_MENU ? F_KB : F_CHAT;
        break;
    }
    }
    return true;
}

/* keyboard slides away once the conversation is scrolled up a bit */
static bool animate_keyboard(void)
{
    int target = (s.focus == F_CHAT && s.scroll > 24) ? 0 : KB_H;
    if (s.kb_h == target)
        return false;
    s.kb_h += s.kb_h < target ? 17 : -17;
    if ((s.kb_h - target) * (s.kb_h < target ? -1 : 1) < 0 || s.kb_h < 0 ||
        s.kb_h > KB_H)
        s.kb_h = target;
    return true;
}

enum plugin_status plugin_start(const void *parameter)
{
    rb->memset(&s, 0, sizeof(s));
    s.rng = (*rb->current_tick ^ usec()) * 2654435761u + 1;
    s.temp_idx = 1;
    s.kb_h = KB_H;
    s.ky = 1;
    s.kx = 4;
#ifdef SIMULATOR
    script = getenv("ROCKGPT_TEST");
#endif
    struct font *pf = rb->font_get(FONT_SYSFIXED);
    for (int c = 0; c < 128; c++)
        s.cw[c] = c >= 32 ? rb->font_get_width(pf, c) : 0;

    splash_screen();
    plugin_fpu_enable(true);
    boost(true);
    s.err = load_model(parameter ? parameter : MODEL_FILE);
    boost(false);
    if (s.err) {
        static char t[320];
        bool oom = !rb->strncmp(s.err, "Out of memory", 13);
        rb->snprintf(t, sizeof(t), "%s\n\n(%s: %s, %u KB)",
                     oom ? PICK(shortages) : PICK(outages), s.err,
                     (const char *)(parameter ? parameter : MODEL_FILE), s.ram_kb);
        s.page = t;
        s.focus = F_PAGE;
        redraw(false);
        long b;
        do {                                  /* any key */
            b = get_button(HZ);
            rb->default_event_handler(b);
        } while (b == BUTTON_NONE || (b & BUTTON_REL));
        io_end();
        plugin_fpu_enable(false);
        if (s.fd >= 0)
            rb->close(s.fd);
        return PLUGIN_OK;
    }
    if (s.draft_skipped)    /* runs, but at half speed */
        rb->splashf(HZ * 4, "%s (%u KB)", PICK(throttles), s.ram_kb);
    new_chat();
    show_toast(PICK(intros));

    bool run = true;
    while (run) {
        long now = *rb->current_tick;
        if (s.gen != G_IDLE) {
            gen_step();
            rb->reset_poweroff_timer();
            if (s.gen == G_IDLE || TIME_AFTER(now, s.next_draw)) {
                uint32_t t0 = usec();
                redraw(s.gen != G_IDLE);
                s.draw_us += usec() - t0;
                s.next_draw = now + HZ / 8;
            }
            long b = get_button(0);
            if (b != BUTTON_NONE) {
                if (b == SYS_POWEROFF || b == SYS_USB_CONNECTED) {
                    rb->default_event_handler(b);
                    break;
                }
                run = handle(b);
                redraw(false);
            }
            continue;
        }
        bool anim = animate_keyboard();
        redraw(false);
        long b = get_button(anim ? 1 : HZ / 4);
        if (b == SYS_POWEROFF || b == SYS_USB_CONNECTED) {
            rb->default_event_handler(b);
            break;
        }
        if (b != BUTTON_NONE)
            run = handle(b);
    }
    boost(false);
    io_end();
    plugin_fpu_enable(false);
    rb->close(s.fd);
    return PLUGIN_OK;
}
