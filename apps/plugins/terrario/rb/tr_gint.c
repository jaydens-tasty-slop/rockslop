/* Terrario platform layer: display, keys, heap. See tr_gint.h. */
#include "tr_gint.h"

#undef memset
#undef memcpy
#undef strlen
#undef strncpy
#undef strcmp

#define TR_Y ((LCD_HEIGHT - TR_H) / 2)

/* ------------------------------------------------------------- heap -- */

#define HEAP_SIZE 16384
#define HEAP_ALIGN 8

struct blk { uint32_t size; uint32_t used; }; /* size includes the header */

static uint8_t heap[HEAP_SIZE] __attribute__((aligned(HEAP_ALIGN)));
static bool heap_ready;

static void heap_init(void)
{
  struct blk *b = (struct blk *)heap;
  b->size = HEAP_SIZE;
  b->used = 0;
  heap_ready = true;
}

void *tr_malloc(size_t n)
{
  if (!heap_ready)
    heap_init();
  size_t need = (sizeof(struct blk) + n + HEAP_ALIGN - 1) & ~(size_t)(HEAP_ALIGN - 1);
  for (uint8_t *p = heap; p < heap + HEAP_SIZE;)
  {
    struct blk *b = (struct blk *)p;
    if (!b->used && b->size >= need)
    {
      if (b->size >= need + sizeof(struct blk) + HEAP_ALIGN)
      {
        struct blk *rest = (struct blk *)(p + need);
        rest->size = b->size - need;
        rest->used = 0;
        b->size = need;
      }
      b->used = 1;
      return p + sizeof(struct blk);
    }
    p += b->size;
  }
  return NULL;
}

void tr_free(void *ptr)
{
  if (!ptr)
    return;
  struct blk *b = (struct blk *)((uint8_t *)ptr - sizeof(struct blk));
  b->used = 0;
  /* merge with following free blocks */
  for (uint8_t *p = (uint8_t *)b + b->size; p < heap + HEAP_SIZE;)
  {
    struct blk *n = (struct blk *)p;
    if (n->used)
      break;
    b->size += n->size;
    p = (uint8_t *)b + b->size;
  }
}

void *tr_realloc(void *ptr, size_t n)
{
  if (!ptr)
    return tr_malloc(n);
  struct blk *b = (struct blk *)((uint8_t *)ptr - sizeof(struct blk));
  size_t have = b->size - sizeof(struct blk);
  if (n <= have)
    return ptr;
  void *q = tr_malloc(n);
  if (!q)
    return NULL;
  rb->memcpy(q, ptr, have);
  tr_free(ptr);
  return q;
}

int tr_heap_used(void)
{
  int used = 0;
  if (!heap_ready)
    return 0;
  for (uint8_t *p = heap; p < heap + HEAP_SIZE; p += ((struct blk *)p)->size)
    if (((struct blk *)p)->used)
      used += ((struct blk *)p)->size;
  return used;
}

static uint32_t rng = 12345;

void tr_srand(unsigned seed) { rng = seed ? seed : 1; }

int tr_rand(void)
{
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return (int)(rng & 0x7fffffff);
}

/* ---------------------------------------------------------- display -- */

static uint8_t vram[TR_H][TR_W];

static const struct { const char *name; uint16_t c[4]; } palettes[] = {
  { "Grey",     { 0xffff, 0xad55, 0x5aeb, 0x0000 } },
  { "Daylight", { 0xdf7f, 0x8d4c, 0x4a69, 0x0841 } },
  { "LCD",      { 0xcf2a, 0x9dc7, 0x5ba4, 0x1ae3 } },
};
static fb_data pal[4];

int tr_palette_count(void) { return ARRAYLEN(palettes); }
const char *tr_palette_name(int which) { return palettes[which].name; }

#if LCD_PIXELFORMAT == RGB565SWAPPED
#define PIX(c) ((fb_data)((((c) >> 8) | ((c) << 8)) & 0xffff))
#else
#define PIX(c) ((fb_data)(c))
#endif

void tr_set_palette(int which)
{
  for (int i = 0; i < 4; i++)
    pal[i] = PIX(palettes[which].c[i]);
}

void dclear(int color)
{
  rb->memset(vram, color, sizeof vram);
}

void dupdate(void)
{
  struct frame_buffer_t *buf = (*rb->screens[SCREEN_MAIN]->current_viewport)->buffer;
  fb_data *fb = buf->fb_ptr + TR_Y * buf->stride;
  for (int y = 0; y < TR_H; y++, fb += buf->stride)
    for (int x = 0; x < TR_W; x++)
      fb[x] = pal[vram[y][x]];
  rb->lcd_update_rect(0, TR_Y, TR_W, TR_H);
}

void dpixel(int x, int y, int color)
{
  if ((unsigned)x < TR_W && (unsigned)y < TR_H && color >= 0)
    vram[y][x] = color;
}

void dline(int x1, int y1, int x2, int y2, int color)
{
  int dx = abs(x2 - x1), dy = -abs(y2 - y1), sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
  int err = dx + dy;
  while (true)
  {
    dpixel(x1, y1, color);
    if (x1 == x2 && y1 == y2)
      break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x1 += sx; }
    if (e2 <= dx) { err += dx; y1 += sy; }
  }
}

void drect(int x1, int y1, int x2, int y2, int color)
{
  if (x1 > x2) swap(x1, x2);
  if (y1 > y2) swap(y1, y2);
  x1 = max(x1, 0); y1 = max(y1, 0);
  x2 = min(x2, TR_W - 1); y2 = min(y2, TR_H - 1);
  for (int y = y1; y <= y2; y++)
    for (int x = x1; x <= x2; x++)
      vram[y][x] = color;
}

void drect_border(int x1, int y1, int x2, int y2, int fill, int bw, int bc)
{
  drect(x1, y1, x2, y2, bc);
  drect(x1 + bw, y1 + bw, x2 - bw, y2 - bw, fill);
}

/* font: fixed 5x6 glyphs, 6 px advance, 7 px line pitch */
extern const unsigned char tr_font_glyphs[96][6];
#define GLYPH_W 6
#define GLYPH_H 6

void dsize(const char *s, const font_t *f, int *w, int *h)
{
  (void)f;
  int n = 0, best = 0;
  for (; *s; s++)
  {
    if (*s == '\n')
      n = 0;
    else
      best = max(best, ++n);
  }
  if (w) *w = best * GLYPH_W;
  if (h) *h = GLYPH_H;
}

static void draw_text(int x, int y, int fg, int bg, const char *s)
{
  int w;
  dsize(s, NULL, &w, NULL);
  if (bg >= 0)
    drect(x, y, x + w - 1, y + GLYPH_H, bg);
  for (int cx = x; *s; s++)
  {
    unsigned c = (unsigned char)*s;
    if (c == '\n')
    {
      cx = x;
      y += GLYPH_H + 1;
      continue;
    }
    if (c >= 32 && c < 128)
      for (int gy = 0; gy < 6; gy++)
        for (int gx = 0; gx < 5; gx++)
          if (tr_font_glyphs[c - 32][gy] & (0x10 >> gx))
            dpixel(cx + gx, y + gy, fg);
    cx += GLYPH_W;
  }
}

void dtext_opt(int x, int y, int fg, int bg, int ha, int va, const char *s)
{
  int w, h;
  dsize(s, NULL, &w, &h);
  if (ha == DTEXT_CENTER) x -= w / 2;
  else if (ha == DTEXT_RIGHT) x -= w;
  if (va == DTEXT_MIDDLE) y -= h / 2;
  else if (va == DTEXT_BOTTOM) y -= h;
  draw_text(x, y, fg, bg, s);
}

void dtext(int x, int y, int fg, const char *s)
{
  draw_text(x, y, fg, C_NONE, s);
}

void dprint_opt(int x, int y, int fg, int bg, int ha, int va, const char *fmt, ...)
{
  char buf[64];
  va_list ap;
  va_start(ap, fmt);
  rb->vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  dtext_opt(x, y, fg, bg, ha, va, buf);
}

void dprint(int x, int y, int fg, const char *fmt, ...)
{
  char buf[64];
  va_list ap;
  va_start(ap, fmt);
  rb->vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  dtext(x, y, fg, buf);
}

/* ---------------------------------------------------------- sprites -- */

static int sprite_fd = -1;
static uint8_t *pool;
static size_t pool_size, pool_used;

bool tr_sprites_open(const char *path)
{
  size_t sz;
  sprite_fd = rb->open(path, O_RDONLY);
  if (sprite_fd < 0)
    return false;
  pool = rb->plugin_get_buffer(&sz);
  pool_size = sz;
  pool_used = 0;
  return pool != NULL;
}

void tr_sprites_close(void)
{
  if (sprite_fd >= 0)
    rb->close(sprite_fd);
  sprite_fd = -1;
}

/* Sheets are read from the card on first use; when the pool is full all are
 * dropped and re-read as needed. */
static const uint8_t *image_data(bopti_image_t *img)
{
  if (img->data)
    return img->data;
  if (pool_used + img->len > pool_size)
  {
    for (int i = 0; i < tr_num_images; i++)
      tr_all_images[i]->data = NULL;
    pool_used = 0;
  }
  uint8_t *p = pool + pool_used;
  rb->lseek(sprite_fd, img->off, SEEK_SET);
  if (rb->read(sprite_fd, p, img->len) != (ssize_t)img->len)
    rb->memset(p, 0, img->len);
  pool_used += (img->len + 3) & ~3u;
  img->data = p;
  return p;
}

void dimage(int x, int y, bopti_image_t *img)
{
  dsubimage(x, y, img, 0, 0, img->w, img->h, DIMAGE_NONE);
}

void dsubimage(int x, int y, bopti_image_t *img, int left, int top, int w, int h, int flags)
{
  (void)flags;
  const uint8_t *grey = image_data(img);
  const uint8_t *mask = grey + ((img->w * img->h + 3) >> 2);
  int sx0 = 0, sy0 = 0;

  if (x < 0) { sx0 = -x; }
  if (y < 0) { sy0 = -y; }
  int sx1 = min(w, TR_W - x), sy1 = min(h, TR_H - y);
  for (int sy = sy0; sy < sy1; sy++)
  {
    uint8_t *row = vram[y + sy];
    int i = (top + sy) * img->w + left;
    for (int sx = sx0; sx < sx1; sx++)
    {
      int p = i + sx;
      if (img->mask && (mask[p >> 3] & (1 << (p & 7))))
        continue;
      row[x + sx] = (grey[p >> 2] >> ((p & 3) * 2)) & 3;
    }
  }
}

/* ------------------------------------------------------------- keys -- */

#define DIRS (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)

bool tr_usb;

static struct
{
  unsigned held;      /* buttons held now */
#ifdef HAVE_BLUETOOTH_HID
  unsigned pad;       /* controller buttons held now */
#endif
  bool sel_used;      /* SELECT chorded with a direction since pressed */
  bool down_rep;      /* DOWN held past the first repeat */
  bool down_chord;    /* DOWN pressed while SELECT held */
  long pause_start;   /* tick SELECT+UP were both held since, 0 = not held */
  key_event_t q[8];
  int nq;
} in;

/* SELECT+UP also moves the cursor up, so the pause menu needs a long hold. */
#define PAUSE_TICKS HZ

static void post(int type, int key)
{
#ifdef HAVE_BLUETOOTH_HID
  if (key == KEY_8 && (in.pad & BUTTON_PAD_L1))
    return; /* L1 held: the d-pad aims the cursor, no jump */
#endif
  if (in.nq < 8)
  {
    in.q[in.nq].type = type;
    in.q[in.nq].key = key;
    in.nq++;
  }
}

void tr_input_reset(void)
{
  rb->button_clear_queue();
  in.held = 0;
#ifdef HAVE_BLUETOOTH_HID
  in.pad = 0;
#endif
  in.sel_used = in.down_rep = in.down_chord = false;
  in.pause_start = 0;
  in.nq = 0;
}

/* Gameplay events. Called once per frame: LEFT/RIGHT walk, UP jumps, a DOWN
 * tap opens the game menu (inventory, crafting) and a held DOWN drops through platforms; SELECT
 * uses the held item (and interacts on press), SELECT+direction moves the
 * cursor. Holding SELECT+UP for 1000 ms opens the pause menu. */
void tr_input_frame(void)
{
  int ev;
  while ((ev = rb->button_get(false)) != BUTTON_NONE)
  {
    unsigned b = ev & ~(BUTTON_REPEAT | BUTTON_REL);
    bool rel = ev & BUTTON_REL, rep = ev & BUTTON_REPEAT;

    if (ev == SYS_POWEROFF)
      continue; /* SELECT is the power key; held SELECT is gameplay */
#ifdef HAVE_BLUETOOTH_HID
    if (!rel && !rep)
    {
      /* controller: A uses the item, B jumps, Y game menu, Options/PS pause menu */
      int pk = b == BUTTON_PAD_A ? KEY_9 : b == BUTTON_PAD_B ? KEY_8
               : b == BUTTON_PAD_Y ? KEY_OPTN
               : (b == BUTTON_PAD_MENU || b == BUTTON_PAD_HOME) ? KEY_MENU
               : KEY_NONE;
      if (pk != KEY_NONE)
      {
        post(KEYEV_DOWN, pk);
        continue;
      }
    }
#endif
    if (ev & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_SELECT))
    {
      if (b == BUTTON_DOWN)
      {
        if (rel)
        {
          if (!in.down_rep && !in.down_chord && !(in.held & BUTTON_SELECT))
            post(KEYEV_DOWN, KEY_OPTN);
          in.down_rep = in.down_chord = false;
        }
        else if (rep)
          in.down_rep = true;
        else if (in.held & BUTTON_SELECT)
          in.down_chord = true;
      }
      if (!rel && !rep)
      {
        if (b == BUTTON_UP && !(in.held & BUTTON_SELECT))
          post(KEYEV_DOWN, KEY_8);
        if (b == BUTTON_SELECT)
          post(KEYEV_DOWN, KEY_9);
        in.held |= b;
      }
      else if (rel)
        in.held &= ~b;
      continue;
    }
    if (rb->default_event_handler(ev) == SYS_USB_CONNECTED)
      tr_usb = true;
  }
  in.held = rb->button_status() & (DIRS | BUTTON_SELECT);
#ifdef HAVE_BLUETOOTH_HID
  in.pad = rb->button_status() & BUTTON_PAD_ALL;
#endif
  if ((in.held & (BUTTON_SELECT | BUTTON_UP)) == (BUTTON_SELECT | BUTTON_UP))
  {
    if (!in.pause_start)
      in.pause_start = *rb->current_tick;
    else if (TIME_AFTER(*rb->current_tick, in.pause_start + PAUSE_TICKS))
    {
      in.pause_start = 0;
      post(KEYEV_DOWN, KEY_MENU);
    }
  }
  else
    in.pause_start = 0;
}

key_event_t pollevent(void)
{
  key_event_t none = { KEYEV_NONE, KEY_NONE };
  if (!in.nq)
    return none;
  key_event_t k = in.q[0];
  for (int i = 1; i < in.nq; i++)
    in.q[i - 1] = in.q[i];
  in.nq--;
  return k;
}

bool keydown(int key)
{
  bool sel = in.held & BUTTON_SELECT;
#ifdef HAVE_BLUETOOTH_HID
  /* L1 is SELECT's chord: the d-pad aims the cursor; A holds the item, B jumps */
  if (in.pad & BUTTON_PAD_L1)
    sel = true;
  if ((key == KEY_7 && (in.pad & BUTTON_PAD_A)) || (key == KEY_8 && (in.pad & BUTTON_PAD_B)))
    return true;
#endif
  switch (key)
  {
    case KEY_4: return (in.held & BUTTON_LEFT) && !sel;
    case KEY_6: return (in.held & BUTTON_RIGHT) && !sel;
    case KEY_8: return (in.held & BUTTON_UP) && !sel;
    case KEY_2: return (in.held & BUTTON_DOWN) && in.down_rep && !sel;
    case KEY_7: return sel;
    case KEY_LEFT: return sel && (in.held & BUTTON_LEFT);
    case KEY_RIGHT: return sel && (in.held & BUTTON_RIGHT);
    case KEY_UP: return sel && (in.held & BUTTON_UP);
    case KEY_DOWN: return sel && (in.held & BUTTON_DOWN);
    default: return false;
  }
}

bool keydown_any(int key, ...)
{
  va_list ap;
  va_start(ap, key);
  bool r = false;
  for (; key && !r; key = va_arg(ap, int))
    r = keydown(key);
  va_end(ap);
  return r;
}

bool keydown_all(int key, ...)
{
  va_list ap;
  va_start(ap, key);
  bool r = true;
  for (; key && r; key = va_arg(ap, int))
    r = keydown(key);
  va_end(ap);
  return r;
}

/* Blocking UI input: arrows move, SELECT tap = EXE, SELECT+UP = F2,
 * SELECT+LEFT/RIGHT = F5/F6 (previous/next tab), SELECT+DOWN = EXIT. */
key_event_t getkey_opt(int opts, volatile int *timeout)
{
  static bool sel;
  static bool used;
  key_event_t k = { KEYEV_DOWN, KEY_NONE };
  (void)opts;
  (void)timeout;

  while (true)
  {
    int ev = rb->button_get_w_tmo(HZ / 10);
    unsigned b = ev & ~(BUTTON_REPEAT | BUTTON_REL);
    bool rel = ev & BUTTON_REL, rep = ev & BUTTON_REPEAT;

    if (ev == BUTTON_NONE)
      continue;
#ifdef HAVE_BLUETOOTH_HID
    if (!rel && !rep)
    {
      /* controller: A = EXE, B/Options/PS = EXIT, Y = F2, L1/R1 = F5/F6 */
      int pk = b == BUTTON_PAD_A ? KEY_EXE : b == BUTTON_PAD_Y ? KEY_F2
               : b == BUTTON_PAD_L1 ? KEY_F5 : b == BUTTON_PAD_R1 ? KEY_F6
               : (b == BUTTON_PAD_B || b == BUTTON_PAD_MENU || b == BUTTON_PAD_HOME)
                 ? KEY_EXIT : KEY_NONE;
      if (pk != KEY_NONE)
      {
        k.key = pk;
        return k;
      }
    }
#endif
    /* A press reports every button held (a direction with SELECT down comes
     * as SELECT|direction), a release the buttons let go */
    unsigned dir = b & DIRS;
    bool one_dir = dir == BUTTON_UP || dir == BUTTON_DOWN
                   || dir == BUTTON_LEFT || dir == BUTTON_RIGHT;
    if ((b & BUTTON_SELECT) && !(b & ~(BUTTON_SELECT | DIRS)))
    {
      if (rel)
      {
        bool tap = sel && !used && !dir;
        sel = used = false;
        if (tap)
        {
          k.key = KEY_EXE;
          return k;
        }
      }
      else if (!dir)
      {
        if (!rep)
          sel = true, used = false;
      }
      else if (one_dir && !rep)
      {
        sel = used = true;
        k.key = dir == BUTTON_UP ? KEY_F2 : dir == BUTTON_DOWN ? KEY_EXIT
                : dir == BUTTON_LEFT ? KEY_F5 : KEY_F6;
        return k;
      }
      continue;
    }
    if (b == dir && one_dir && !rel)
    {
      k.key = dir == BUTTON_UP ? KEY_UP : dir == BUTTON_DOWN ? KEY_DOWN
              : dir == BUTTON_LEFT ? KEY_LEFT : KEY_RIGHT;
      return k;
    }
    if (rel)
      continue;
    if (rb->default_event_handler(ev) == SYS_USB_CONNECTED)
    {
      tr_usb = true;
      k.key = KEY_EXIT;
      return k;
    }
  }
}
