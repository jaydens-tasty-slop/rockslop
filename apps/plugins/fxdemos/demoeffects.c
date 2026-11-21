/* Old school demo effects (plasma, metaballs, rotozoomer, deformations) for
 * Rockbox. Ported from Mika Tuupola's esp_effects / sdl2_effects
 * (https://github.com/tuupola/esp_effects, MIT-0) to integer maths and
 * direct framebuffer drawing. One scratch buffer is shared by all effects. */
#include "plugin.h"

#include "fx_common.h"
#include "fx_head.h"

#define FX_NAME "Demo Effects"
#define FX_SETTINGS demo_settings
#define FX_HELP "Demo Effects:", "a sequence of", "classic demoscene", "effects.", "", \
  "LEFT/RIGHT: previous /", "next effect.", "Hold SELECT+UP: menu.", "", \
  "Settings: auto-advance", "to the next effect", "after a time."
#define FX_ABOUT "Demo Effects", "Effects by Mika", "Tuupola (esp_effects,", "sdl2_effects), MIT-0.", \
  "github.com/tuupola/", "esp_effects", "Head image: Second", "Reality, Future Crew."

#include "fx_main.h"

enum effect { E_PLASMA, E_RGBPLASMA, E_METABALLS, E_ROTOZOOM, E_DEFORM, E_SWIRL, E_COUNT };

static const char *const names[E_COUNT] = {
  "Plasma", "RGB plasma", "Metaballs", "Rotozoomer", "Deform", "Swirl"
};

/* One scratch area for every effect: 8-bit plasma/distance values, or the
 * deformation lookup table (two bytes per pixel). */
#define SCRATCH_SIZE (FX_W * FX_H * 2)
static uint8_t *scratch;

static int cur;                 /* running effect */
static int frame;               /* effect time in 1/30 s steps */
static int accum;               /* ms not yet turned into steps */
static int auto_secs;           /* 0 = off */
static int auto_ms;
static int label_ms;            /* time left showing the effect name */
static int fade;                /* transition: 0 = none, else 1..FADE_STEPS */
static int fade_dir;
static int pending;             /* effect to switch to at the dark point */
static fb_data pal[256];

#define STEP_MS 33
#define FADE_STEPS 8
#define LABEL_MS 1500

/* 128 + 128 * sin(turn / 256), clamped to a byte. */
static inline int sin8(int a)
{
  int v = 128 + (fx_sin(a << 2) >> 8);
  return v > 255 ? 255 : v;
}

/* ---- plasma: static pattern, animated by cycling the palette ---- */

static void plasma_init(void)
{
  uint8_t *p = scratch;

  for (int i = 0; i < 256; i++)
  {
    int r = 128 + (fx_sin(i * 4 + 163) >> 8);
    int g = 128 + (fx_sin(i * 8 + 163) >> 8);
    pal[i] = fx_rgb(r > 255 ? 255 : r, g > 255 ? 255 : g, 64);
  }
  for (int y = 0; y < FX_H; y++)
    for (int x = 0; x < FX_W; x++)
    {
      int v1 = 128 + (fx_sin(x * 1024 / 201) >> 8);
      int v2 = 128 + (fx_sin(y * 1024 * 7 / 1056) >> 8);
      int d = fx_isqrt(x * x + y * y);
      int v3 = 128 + (fx_sin(d * 1024 * 7 / 1056) >> 8);
      *p++ = (v1 + v2 + v3) / 3;
    }
}

static void plasma_draw(void)
{
  const uint8_t *p = scratch;
  int phase = frame * 4;

  for (int i = 0; i < FX_W * FX_H; i++)
    fx_fb[i] = pal[(p[i] + phase) & 255];
}

/* ---- RGB plasma: three sinusoids, the radial one from a distance table ---- */

static void rgbplasma_init(void)
{
  uint8_t *p = scratch;

  for (int i = 0; i < 256; i++)
    pal[i] = fx_rgb(i, 255 - i, 128);
  for (int y = 0; y < FX_H; y++)
    for (int x = 0; x < FX_W; x++)
      *p++ = fx_isqrt(x * x + y * y);
}

static void rgbplasma_draw(void)
{
  const uint8_t *p = scratch;
  int xs[FX_W];

  for (int x = 0; x < FX_W; x++)
    xs[x] = sin8((x + frame * 4) * 256 / 201);

  for (int y = 0; y < FX_H; y++)
  {
    int v2 = sin8((y + frame * 4) * 256 * 7 / 1056);
    for (int x = 0; x < FX_W; x++)
    {
      int v3 = sin8(*p++ * 256 * 14 / 1056);
      fx_fb[y * FX_W + x] = pal[(xs[x] + v2 + v3) / 3];
    }
  }
}

/* ---- metaballs ---- */

#define NUM_BALLS 3

static struct { int x, y, vx, vy, r2; } balls[NUM_BALLS];

static void metaballs_init(void)
{
  for (int i = 0; i < NUM_BALLS; i++)
  {
    int r = 20 + fx_rand() % 30;
    balls[i].r2 = r * r;
    balls[i].x = fx_rand() % FX_W;
    balls[i].y = fx_rand() % FX_H;
    balls[i].vx = 3 + fx_rand() % 5;
    balls[i].vy = 3 + fx_rand() % 5;
  }
}

static void metaballs_step(void)
{
  for (int i = 0; i < NUM_BALLS; i++)
  {
    balls[i].x += balls[i].vx;
    balls[i].y += balls[i].vy;
    if (balls[i].x < 0 || balls[i].x > FX_W)
      balls[i].vx = -balls[i].vx;
    if (balls[i].y < 0 || balls[i].y > FX_H)
      balls[i].vy = -balls[i].vy;
  }
}

/* http://www.geisswerks.com/ryan/BLOBS/blobs.html */
static void metaballs_draw(void)
{
  const fb_data black = fx_rgb(0, 0, 0), white = fx_rgb(255, 255, 255),
                green = fx_rgb(0, 255, 0);
  fb_data *o = fx_fb;

  for (int y = 0; y < FX_H; y++)
    for (int x = 0; x < FX_W; x++)
    {
      int sum = 0; /* sum of r^2 / d^2 in Q8 */
      for (int i = 0; i < NUM_BALLS; i++)
      {
        int dx = x - balls[i].x, dy = y - balls[i].y;
        int d2 = dx * dx + dy * dy;
        sum += (balls[i].r2 << 8) / (d2 ? d2 : 1);
      }
      *o++ = sum > 166 ? black : sum > 128 ? white : sum > 102 ? green : black;
    }
}

/* ---- rotozoomer ---- */

static void rotozoom_draw(void)
{
  int a = frame & 359;                      /* degrees, as the original */
  int s = fx_sin(a * 1024 / 360), c = fx_cos(a * 1024 / 360);
  int z = s * 6 / 5;                        /* zoom follows the sine, Q15 */
  int cz = (int)(((long long)c * z) >> 14); /* Q16 */
  int sz = (int)(((long long)s * z) >> 14);
  int x0 = -FX_W / 2, y0 = -FX_H / 2;
  fb_data *o = fx_fb;

  for (int y = 0; y < FX_H; y++)
  {
    int yy = y + y0;
    int u = x0 * cz - yy * sz;
    int v = x0 * sz + yy * cz;
    for (int x = 0; x < FX_W; x++)
    {
      int tu = (u / 65536) % HEAD_WIDTH;
      int tv = (v / 65536) % HEAD_HEIGHT;
      if (tu < 0)
        tu = -tu;
      if (tv < 0)
        tv += HEAD_HEIGHT;
      *o++ = FX_FROM565(fx_head[tv * HEAD_WIDTH + tu]);
      u += cz;
      v += sz;
    }
  }
}

/* ---- deform: precomputed texture coordinates scrolled over time ---- */

static void deform_build(bool swirl)
{
  int8_t *p = (int8_t *)scratch;

  for (int j = 0; j < FX_H; j++)
    for (int i = 0; i < FX_W; i++)
    {
      int X = 2 * i - FX_W, Y = 2 * j - FX_H;      /* x = X / W, y = Y / H */
      int xq = X * 65536 / FX_W, yq = Y * 65536 / FX_H; /* Q16 */
      int u, v;                                    /* Q8 */

      if (!swirl)
      {
        /* u = cos(a) / r, v = sin(a) / r: x / r^2, y / r^2 */
        int r2 = (int)(((long long)xq * xq + (long long)yq * yq) >> 16);
        if (r2 == 0)
          r2 = 1;
        u = (int)(((long long)xq * 256) / r2);
        v = (int)(((long long)yq * 256) / r2);
      }
      else
      {
        /* x, y rotated by an angle equal to the radius */
        int r2 = (int)(((long long)xq * xq + (long long)yq * yq) >> 16);
        int a = fx_isqrt(r2) * 163 / 256;           /* radians (Q8) -> 1024/turn */
        int cs = fx_cos(a), sn = fx_sin(a);
        u = (int)(((long long)xq * cs - (long long)yq * sn) >> 23);
        v = (int)(((long long)xq * sn + (long long)yq * cs) >> 23);
      }
      /* Same int8 wrap as the original's (int8_t)(HEAD_WIDTH * u). */
      *p++ = (int8_t)((HEAD_WIDTH * u) / 256) % HEAD_WIDTH;
      *p++ = (int8_t)((HEAD_HEIGHT * v) / 256) % HEAD_HEIGHT;
    }
}

static void deform_draw(void)
{
  const int8_t *p = (const int8_t *)scratch;
  int sc = frame * 2;
  fb_data *o = fx_fb;

  for (int i = 0; i < FX_W * FX_H; i++)
  {
    int u = (*p++ + sc) % HEAD_WIDTH;
    int v = (*p++ + sc) % HEAD_HEIGHT;
    if (u < 0)
      u = -u;
    if (v < 0)
      v = -v;
    *o++ = FX_FROM565(fx_head[v * HEAD_WIDTH + u]);
  }
}

/* ---- sequencing ---- */

static void effect_init(int e)
{
  cur = e;
  frame = 0;
  accum = 0;
  switch (e)
  {
    case E_PLASMA: plasma_init(); break;
    case E_RGBPLASMA: rgbplasma_init(); break;
    case E_METABALLS: metaballs_init(); break;
    case E_DEFORM: deform_build(false); break;
    case E_SWIRL: deform_build(true); break;
    default: break;
  }
}

static void effect_draw(void)
{
  switch (cur)
  {
    case E_PLASMA: plasma_draw(); break;
    case E_RGBPLASMA: rgbplasma_draw(); break;
    case E_METABALLS: metaballs_draw(); break;
    case E_ROTOZOOM: rotozoom_draw(); break;
    case E_DEFORM:
    case E_SWIRL: deform_draw(); break;
  }
}

static void switch_to(int e)
{
  pending = (e + E_COUNT) % E_COUNT;
  fade = 1;
  fade_dir = 1;
  auto_ms = 0;
}

static void demo_settings(void)
{
  static const struct opt_items opts[] = {
    { "Off", -1 }, { "10 seconds", -1 }, { "20 seconds", -1 }, { "40 seconds", -1 }
  };
  static const int secs[] = { 0, 10, 20, 40 };
  int sel = 0;

  for (int i = 0; i < 4; i++)
    if (secs[i] == auto_secs)
      sel = i;
  rb->set_option("Auto-advance", &sel, RB_INT, opts, 4, NULL);
  auto_secs = secs[sel];
}

static bool demo_init(void)
{
  size_t size;

  scratch = rb->plugin_get_buffer(&size);
  if (!scratch || size < SCRATCH_SIZE)
  {
    rb->splash(HZ * 2, "Not enough memory");
    return false;
  }
  effect_init(E_PLASMA);
  auto_secs = 20;
  return true;
}

static void demo_start(void)
{
  label_ms = LABEL_MS;
  fade = 0;
}

static bool demo_frame(unsigned keys, unsigned edge, int dt_ms)
{
  (void)keys;

  if (!fade)
  {
    if (edge & BUTTON_RIGHT)
      switch_to(cur + 1);
    else if (edge & BUTTON_LEFT)
      switch_to(cur - 1);
    else if (auto_secs && (auto_ms += dt_ms) >= auto_secs * 1000)
      switch_to(cur + 1);
  }

  for (accum += dt_ms; accum >= STEP_MS; accum -= STEP_MS)
  {
    frame++;
    if (cur == E_METABALLS)
      metaballs_step();
  }

  effect_draw();

  if (fade)
  {
    int k = 256 - fade * 256 / FADE_STEPS;
    for (int i = 0; i < FX_W * FX_H; i++)
      fx_fb[i] = fx_scale(fx_fb[i], k);
    fade += fade_dir;
    if (fade > FADE_STEPS)
    {
      effect_init(pending);
      fade_dir = -1;
      fade = FADE_STEPS;
      label_ms = LABEL_MS;
    }
    else if (fade < 1)
      fade = 0;
  }
  if (label_ms > 0)
  {
    label_ms -= dt_ms;
    fx_text_shadow(2, 2, names[cur], fx_rgb(255, 255, 255));
  }
  return false;
}
