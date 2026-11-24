/* "A tour through time": a twelve scene demoscene production for Rockbox.
 * Ported from the ESP32-C6 Geek Amiga Demo by mcarey42
 * (https://github.com/mcarey42/esp32-c6-geek-amiga-demo, MIT). The scenes are
 * the original's, rewritten in fixed point for the 128x160 portrait LCD and
 * drawing straight into the LCD framebuffer. All scenes share one small
 * arena; the silhouette scene streams its frames from the SD card. */
#include "plugin.h"

#include "fx_common.h"

#define FX_NAME "Amiga Demo"
#define FX_MIN_TICKS 3
#define FX_HELP "Amiga Demo:", "twelve scenes play in", "a loop, from 1990", "to now.", "", \
  "LEFT/RIGHT: previous /", "next scene.", "UP: pause.", "DOWN: scene names.", \
  "Hold SELECT+UP: menu.", "", "The silhouette scene", "needs silhouette.bin", "in rocks/demos/amiga/."
#define FX_ABOUT "Amiga Demo", "after the ESP32-C6", "Geek Amiga Demo by", "mcarey42 and Claude,", \
  "MIT licence.", "github.com/mcarey42/", "esp32-c6-geek-amiga-", "demo", "5x7 font: Adafruit", "GFX, BSD-2."

#define FX_STOP tune_stop

#ifndef FX_HOST
#include "amiga_tune.h"
#else
static bool tune_hold;
static void tune_start(void) {}
static void tune_stop(void) {}
#endif
#include "fx_main.h"

#define STEP_MS 33
#define SIL_FILE ROCKBOX_DIR "/rocks/demos/amiga/silhouette.bin"

/* ---- small helpers ---- */

#define ANG(radians_x1000) ((radians_x1000) * 163 / 1000) /* 1024 per turn */

static inline int iclamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

/* Hue 0..1535 at brightness v (0..255), full saturation. */
static fb_data hue_rgb(int h, int v)
{
  int f = h & 255, q = v * (255 - f) / 255, r = v * f / 255;

  switch ((h >> 8) % 6)
  {
    case 0: return fx_rgb(v, r, 0);
    case 1: return fx_rgb(q, v, 0);
    case 2: return fx_rgb(0, v, r);
    case 3: return fx_rgb(0, q, v);
    case 4: return fx_rgb(r, 0, v);
    default: return fx_rgb(v, 0, q);
  }
}

/* Copper bands: hue steps down the screen and drifts with time. */
static void copper_bands(int band_h, int period_ms, int v, int t)
{
  int bands = (FX_H + band_h - 1) / band_h;

  for (int b = 0; b < bands; b++)
  {
    int h = (b * 1536 / FX_H + t * 1536 / period_ms) % 1536;
    fx_fill_rect(0, b * band_h, FX_W, band_h, hue_rgb(h, v));
  }
}

static void fade_to_white(int k) /* k = 0..256 */
{
  const fb_data white = fx_rgb(255, 255, 255);

  if (k > 0)
    for (int i = 0; i < FX_W * FX_H; i++)
      fx_fb[i] = fx_blend(fx_fb[i], white, k);
}

static void fade_to_black(int k)
{
  if (k > 0)
    for (int i = 0; i < FX_W * FX_H; i++)
      fx_fb[i] = fx_scale(fx_fb[i], 256 - k);
}

/* Centred text with its x position nudged by dx (for shadows). */
static void ctext(int dx, int y, const char *s, fb_data c)
{
  fx_text((FX_W - fx_text_w(s)) / 2 + dx, y, s, c);
}

static void label(const char *s, int y, fb_data c)
{
  ctext(1, y + 1, s, fx_rgb(0, 0, 0));
  ctext(0, y, s, c);
}

/* Time-stepped scenes advance in 1/30 s steps. */
static int steps_since(int t, int *last)
{
  int n = t / STEP_MS - *last;

  *last = t / STEP_MS;
  return n < 0 ? 0 : n > 4 ? 4 : n;
}

/* ---- 3D: wireframes with 12.12 vertices and 2.14 rotation matrices ---- */

#define Q(f) ((int16_t)((f) * 4096))
#define PROJ_K 139 /* 60 degree fov: (1 / tan 30) / 2 * FX_H */

typedef struct { int r[9]; int t[3]; } Xf; /* r Q14, t Q12 */

typedef struct
{
  const int16_t *v;   /* x, y, z triples */
  int nv;
  const uint8_t *e;   /* index pairs */
  int ne;
} Model;

static void xf_set(Xf *m, int tx, int ty, int tz)
{
  memset(m, 0, sizeof *m);
  m->r[0] = m->r[4] = m->r[8] = 1 << 14;
  m->t[0] = tx;
  m->t[1] = ty;
  m->t[2] = tz;
}

/* m = m * rotation about axis (0 x, 1 y, 2 z) by a (1024 per turn). */
static void xf_rot(Xf *m, int axis, int a)
{
  int c = fx_cos(a) >> 1, s = fx_sin(a) >> 1;
  int R[9] = { 1 << 14, 0, 0, 0, 1 << 14, 0, 0, 0, 1 << 14 };
  int n[9];

  if (axis == 0)
  {
    R[4] = c; R[5] = -s; R[7] = s; R[8] = c;
  }
  else if (axis == 1)
  {
    R[0] = c; R[2] = s; R[6] = -s; R[8] = c;
  }
  else
  {
    R[0] = c; R[1] = -s; R[3] = s; R[4] = c;
  }
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      n[i * 3 + j] = (int)(((int64_t)m->r[i * 3] * R[j] + (int64_t)m->r[i * 3 + 1] * R[3 + j]
                            + (int64_t)m->r[i * 3 + 2] * R[6 + j]) >> 14);
  memcpy(m->r, n, sizeof n);
}

static void xf_apply(const Xf *m, const int16_t *v, int out[3])
{
  for (int i = 0; i < 3; i++)
    out[i] = (int)(((int64_t)m->r[i * 3] * v[0] + (int64_t)m->r[i * 3 + 1] * v[1]
                    + (int64_t)m->r[i * 3 + 2] * v[2]) >> 14) + m->t[i];
}

/* Camera space (looking down -z) to screen; false when behind the near plane. */
static bool project(const int p[3], int *sx, int *sy)
{
  if (p[2] >= -205) /* -0.05 */
    return false;
  *sx = FX_W / 2 + p[0] * PROJ_K / -p[2];
  *sy = FX_H / 2 - p[1] * PROJ_K / -p[2];
  return true;
}

#define MAX_VERTS 64

static void draw_model(const Model *m, const Xf *x, fb_data c)
{
  int sx[MAX_VERTS], sy[MAX_VERTS];
  bool ok[MAX_VERTS];

  for (int i = 0; i < m->nv; i++)
  {
    int p[3];
    xf_apply(x, m->v + i * 3, p);
    ok[i] = project(p, &sx[i], &sy[i]);
  }
  for (int i = 0; i < m->ne; i++)
  {
    int a = m->e[i * 2], b = m->e[i * 2 + 1];
    if (ok[a] && ok[b])
      fx_line(sx[a], sy[a], sx[b], sy[b], c);
  }
}

/* ---- the scene arena ---- */

#define PL_W (FX_W / 2)
#define PL_H (FX_H / 2)
#define TEX 64
#define MAP_SZ 128
#define N_STARS 256

struct star { int x, y, z; };

static union
{
  struct { struct star s[N_STARS]; int last; } stars;
  struct { uint8_t radial[PL_H][PL_W]; } plasma;
  struct { fb_data tex[TEX * TEX]; int which; } roto;
  struct { uint8_t h[MAP_SZ][MAP_SZ]; int cam_h; } voxel;  /* cam_h in 1/16 */
  struct { uint8_t u[PL_H][PL_W], v[PL_H][PL_W]; fb_data tex[TEX * TEX]; } tunnel;
  struct
  {
    int16_t cyl[24 * 3]; uint8_t cyl_e[36 * 2];
    int16_t tie[18 * 3]; uint8_t tie_e[(12 + 14) * 2];
    int16_t ds[52 * 3]; uint8_t ds_e[(5 * 10 + 4 * 10 + 20) * 2];
    int16_t xw[19 * 3]; uint8_t xw_e[28 * 2];
  } parade;
} A;

/* ---- scene 1: cold boot and title card ---- */

static void sc1_render(int t)
{
  static const char *const post[] = {
    "CORTEX-M4 @ 192 MHZ", "MEMORY ... 512K OK", "LCD 128X160 ... OK", "LOADING DEMO ..."
  };

  if (t < 2000)
  {
    int n = iclamp(t / 400 + 1, 1, 4);

    fx_clear(fx_rgb(0, 0, 0));
    fx_text(4, 4, "YP3-DEMOSYSTEM V1.0", fx_rgb(80, 200, 80));
    for (int i = 0; i < n; i++)
      fx_text(4, 24 + i * 12, post[i], fx_rgb(200, 200, 200));
    if ((t / 250) & 1)
      fx_text(4, 24 + n * 12, "_", fx_rgb(200, 200, 200));
    return;
  }
  if (t < 3000)
  {
    int v = ((t - 2000) / 80) & 1 ? 255 : 0;
    fx_clear(fx_rgb(v, v, v));
    return;
  }
  copper_bands(6, 8000, 200, t);
  int y = FX_H / 2 - 14;
  ctext(2, y + 2, "YP3 DEMO", fx_rgb(0, 0, 0));
  ctext(1, y + 1, "YP3 DEMO", fx_rgb(255, 0, 200));
  ctext(0, y, "YP3 DEMO", fx_rgb(255, 255, 255));
  label("PRESENTS", y + 16, fx_rgb(255, 255, 255));
  label("A TOUR THROUGH TIME", y + 28, fx_rgb(255, 255, 255));
}

/* ---- scene 2: copper bars and a sine scroller ---- */

static void sc2_render(int t)
{
  static const char text[] = ">>> GREETZ TO SPACEBALLS - SCOOPEX - KEFRENS - SANITY - ANARCHY - FUTURE CREW - TBL <<< ";
  const int n = sizeof text - 1, total_w = n * 6, scroller_y = FX_H - 16;
  int shift = (t / 30) % total_w;

  copper_bands(4, 4000, 255, t);
  fx_fill_rect(0, scroller_y - 2, FX_W, 11, fx_rgb(0, 0, 0));
  for (int i = 0; i < n; i++)
  {
    int cx = ((i * 6 - shift) % total_w + total_w) % total_w - 32;
    if (cx > FX_W || cx < -6)
      continue;
    int dy = 6 * fx_sin((cx * 8150 + t * 407) / 1000) / 32768;
    char one[2] = { text[i], 0 };
    fx_text(cx, scroller_y + dy, one, fx_rgb(255, 255, 255));
  }
}

/* ---- scene 3: starfield into a hyperspace jump ---- */

#define HYPERSPACE_MS 12000
#define FLASH_MS 17500

static unsigned unit_rand(int range) /* 0..range */
{
  return (unsigned)fx_rand() * range / 32767;
}

static void star_recycle(struct star *s)
{
  s->x = unit_rand(10240) - 5120;     /* +-20 units, Q8 */
  s->y = unit_rand(7680) - 3840;      /* +-15 */
  s->z = -(int)unit_rand(25600) - 1280; /* -5 .. -105 */
}

static bool sc3_init(void)
{
  fx_seed = 0xDEADBEEFu;
  for (int i = 0; i < N_STARS; i++)
    star_recycle(&A.stars.s[i]);
  A.stars.last = 0;
  return true;
}

static bool star_project(const struct star *s, int z, int *sx, int *sy)
{
  if (-z < 13)
    return false;
  *sx = FX_W / 2 + s->x * PROJ_K / -z;
  *sy = FX_H / 2 - s->y * PROJ_K / -z;
  return true;
}

static void sc3_render(int t)
{
  int n = steps_since(t, &A.stars.last);

  if (t >= FLASH_MS)
  {
    fx_clear(fx_rgb(255, 255, 255));
    return;
  }
  fx_clear(fx_rgb(0, 0, 8));

  int speed = 102; /* 0.4 per step, Q8 */
  if (t >= HYPERSPACE_MS)
  {
    int ramp = (t - HYPERSPACE_MS) * 256 / (FLASH_MS - HYPERSPACE_MS);
    speed += (ramp * ramp >> 8) * 40;
  }
  for (int i = 0; i < N_STARS; i++)
  {
    struct star *s = &A.stars.s[i];
    int prev_z = s->z, sx, sy;

    s->z += speed * n;
    if (s->z > -128)
    {
      star_recycle(s);
      prev_z = s->z;
    }
    if (!star_project(s, s->z, &sx, &sy))
      continue;
    int b = iclamp(255 - -s->z * 255 / 25600, 0, 255);
    fb_data c = fx_rgb(b, b, b);

    if (t < HYPERSPACE_MS)
    {
      fx_pixel(sx, sy, c);
      if (b > 200)
      {
        fx_pixel(sx + 1, sy, c);
        fx_pixel(sx, sy + 1, c);
      }
    }
    else
    {
      int px, py;
      if (star_project(s, prev_z, &px, &py))
        fx_line(px, py, sx, sy, c);
      else
        fx_pixel(sx, sy, c);
    }
  }
}

/* ---- scene 4: the vector parade ---- */

static const int16_t cube_v[8 * 3] = {
  Q(-1), Q(-1), Q(-1), Q(1), Q(-1), Q(-1), Q(1), Q(1), Q(-1), Q(-1), Q(1), Q(-1),
  Q(-1), Q(-1), Q(1), Q(1), Q(-1), Q(1), Q(1), Q(1), Q(1), Q(-1), Q(1), Q(1),
};
static const uint8_t cube_e[] = { 0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7 };
static const Model cube = { cube_v, 8, cube_e, 12 };

static const int16_t octa_v[6 * 3] = {
  Q(1), 0, 0, Q(-1), 0, 0, 0, Q(1), 0, 0, Q(-1), 0, 0, 0, Q(1), 0, 0, Q(-1),
};
static const uint8_t octa_e[] = { 0,2, 0,3, 0,4, 0,5, 1,2, 1,3, 1,4, 1,5, 2,4, 4,3, 3,5, 5,2 };
static const Model octa = { octa_v, 6, octa_e, 12 };

#define PHI 1.6180339887
#define IPH 0.6180339887
static const int16_t ico_v[12 * 3] = {
  0, Q(1), Q(PHI), 0, Q(1), Q(-PHI), 0, Q(-1), Q(PHI), 0, Q(-1), Q(-PHI),
  Q(1), Q(PHI), 0, Q(1), Q(-PHI), 0, Q(-1), Q(PHI), 0, Q(-1), Q(-PHI), 0,
  Q(PHI), 0, Q(1), Q(PHI), 0, Q(-1), Q(-PHI), 0, Q(1), Q(-PHI), 0, Q(-1),
};
static const uint8_t ico_e[] = {
  0,2, 0,4, 0,6, 0,8, 0,10, 1,3, 1,4, 1,6, 1,9, 1,11, 2,5, 2,7, 2,8, 2,10,
  3,5, 3,7, 3,9, 3,11, 4,6, 4,8, 4,9, 5,7, 5,8, 5,9, 6,10, 6,11, 7,10, 7,11, 8,9, 10,11,
};
static const Model ico = { ico_v, 12, ico_e, 30 };

static const int16_t dod_v[20 * 3] = {
  Q(1), Q(1), Q(1), Q(1), Q(1), Q(-1), Q(1), Q(-1), Q(1), Q(1), Q(-1), Q(-1),
  Q(-1), Q(1), Q(1), Q(-1), Q(1), Q(-1), Q(-1), Q(-1), Q(1), Q(-1), Q(-1), Q(-1),
  0, Q(IPH), Q(PHI), 0, Q(IPH), Q(-PHI), 0, Q(-IPH), Q(PHI), 0, Q(-IPH), Q(-PHI),
  Q(IPH), Q(PHI), 0, Q(IPH), Q(-PHI), 0, Q(-IPH), Q(PHI), 0, Q(-IPH), Q(-PHI), 0,
  Q(PHI), 0, Q(IPH), Q(PHI), 0, Q(-IPH), Q(-PHI), 0, Q(IPH), Q(-PHI), 0, Q(-IPH),
};
static const uint8_t dod_e[] = {
  0,8, 0,12, 0,16, 1,9, 1,12, 1,17, 2,10, 2,13, 2,16, 3,11, 3,13, 3,17,
  4,8, 4,14, 4,18, 5,9, 5,14, 5,19, 6,10, 6,15, 6,18, 7,11, 7,15, 7,19,
  8,10, 9,11, 12,14, 13,15, 16,17, 18,19,
};
static const Model dod = { dod_v, 20, dod_e, 30 };

#define CYL_SEG 12
#define DS_LAT 5
#define DS_LON 10

static Model cyl, tie, deathstar, xwing;

static void build_parade(void)
{
  int e;

  /* cylinder: two rings joined by rungs */
  for (int i = 0; i < CYL_SEG; i++)
  {
    int a = i * 1024 / CYL_SEG, c = fx_cos(a) >> 3, s = fx_sin(a) >> 3;
    int16_t *lo = A.parade.cyl + i * 3, *hi = A.parade.cyl + (i + CYL_SEG) * 3;
    lo[0] = hi[0] = c; lo[1] = Q(-1); hi[1] = Q(1); lo[2] = hi[2] = s;
  }
  e = 0;
  for (int i = 0; i < CYL_SEG; i++)
  {
    int n = (i + 1) % CYL_SEG;
    A.parade.cyl_e[e++] = i; A.parade.cyl_e[e++] = n;
    A.parade.cyl_e[e++] = i + CYL_SEG; A.parade.cyl_e[e++] = n + CYL_SEG;
    A.parade.cyl_e[e++] = i; A.parade.cyl_e[e++] = i + CYL_SEG;
  }
  cyl = (Model){ A.parade.cyl, CYL_SEG * 2, A.parade.cyl_e, e / 2 };

  /* TIE fighter: a small octahedron cockpit between two hexagonal panels */
  static const int16_t coc[6 * 3] = {
    Q(.35), 0, 0, Q(-.35), 0, 0, 0, Q(.35), 0, 0, Q(-.35), 0, 0, 0, Q(.35), 0, 0, Q(-.35),
  };
  memcpy(A.parade.tie, coc, sizeof coc);
  for (int p = 0; p < 2; p++)
    for (int i = 0; i < 6; i++)
    {
      int a = i * 1024 / 6;
      int16_t *v = A.parade.tie + (6 + p * 6 + i) * 3;
      v[0] = p ? Q(1.4) : Q(-1.4);
      v[1] = fx_sin(a) * 9 / 10 >> 3;
      v[2] = fx_cos(a) * 9 / 10 >> 3;
    }
  static const uint8_t coc_e[] = { 0,2, 0,3, 0,4, 0,5, 1,2, 1,3, 1,4, 1,5, 2,4, 4,3, 3,5, 5,2 };
  memcpy(A.parade.tie_e, coc_e, sizeof coc_e);
  e = sizeof coc_e;
  for (int p = 0; p < 2; p++)
  {
    int base = 6 + p * 6;
    for (int i = 0; i < 6; i++)
    {
      A.parade.tie_e[e++] = base + i;
      A.parade.tie_e[e++] = base + (i + 1) % 6;
    }
    A.parade.tie_e[e++] = p == 0 ? 1 : 0;
    A.parade.tie_e[e++] = base;
  }
  tie = (Model){ A.parade.tie, 18, A.parade.tie_e, e / 2 };

  /* Death Star: a latitude / longitude wireframe */
  int16_t *v = A.parade.ds;
  v[0] = 0; v[1] = Q(1); v[2] = 0;
  v[3] = 0; v[4] = Q(-1); v[5] = 0;
  int n = 2;
  for (int i = 0; i < DS_LAT; i++)
  {
    int lat = (i + 1) * 512 / (DS_LAT + 1) - 256; /* -90 .. +90 degrees */
    int y = fx_sin(lat) >> 3, r = fx_cos(lat) >> 3;
    for (int j = 0; j < DS_LON; j++, n++)
    {
      int lon = j * 1024 / DS_LON;
      v[n * 3] = r * fx_cos(lon) >> 15;
      v[n * 3 + 1] = y;
      v[n * 3 + 2] = r * fx_sin(lon) >> 15;
    }
  }
  e = 0;
  uint8_t *de = A.parade.ds_e;
  for (int j = 0; j < DS_LON; j++)
  {
    de[e++] = 0; de[e++] = 2 + j;
  }
  int last = 2 + (DS_LAT - 1) * DS_LON;
  for (int j = 0; j < DS_LON; j++)
  {
    de[e++] = 1; de[e++] = last + j;
  }
  for (int i = 0; i < DS_LAT; i++)
    for (int j = 0; j < DS_LON; j++)
    {
      int base = 2 + i * DS_LON;
      de[e++] = base + j; de[e++] = base + (j + 1) % DS_LON;
    }
  for (int i = 0; i < DS_LAT - 1; i++)
    for (int j = 0; j < DS_LON; j++)
    {
      int base = 2 + i * DS_LON;
      de[e++] = base + j; de[e++] = base + DS_LON + j;
    }
  deathstar = (Model){ A.parade.ds, n, de, e / 2 };
}

/* The X-Wing: fuselage box, cockpit and four S-foils; open is 0..256. */
static void build_xwing(int open)
{
  const int fl = Q(1.4), fw = Q(.18), fh = Q(.18);
  int16_t *v = A.parade.xw;
  static const int8_t sx[8] = { -1, 1, 1, -1, -1, 1, 1, -1 }, sy[8] = { -1, -1, 1, 1, -1, -1, 1, 1 },
                      sz[8] = { -1, -1, -1, -1, 1, 1, 1, 1 };
  int lift = Q(.7) * open >> 8, spread = Q(1.6);
  struct { int x, y, z; } w[10] = {
    { Q(-1.4) + Q(.1), fh, 0 }, { Q(-1.4) + Q(.1), -fh, 0 },
    { Q(-1.4) - Q(.6), lift, spread }, { Q(-1.4) - Q(.6), -lift, spread },
    { Q(-1.4) - Q(.6), lift, -spread }, { Q(-1.4) - Q(.6), -lift, -spread },
    { Q(-1.4) - Q(.2), lift, spread }, { Q(-1.4) - Q(.2), -lift, spread },
    { Q(-1.4) - Q(.2), lift, -spread }, { Q(-1.4) - Q(.2), -lift, -spread },
  };

  for (int i = 0; i < 8; i++)
  {
    v[i * 3] = sx[i] * fl;
    v[i * 3 + 1] = sy[i] * fh;
    v[i * 3 + 2] = sz[i] * fw;
  }
  v[24] = fl - Q(.1); v[25] = fh + Q(.18); v[26] = 0; /* cockpit bump */
  for (int i = 0; i < 10; i++)
  {
    v[27 + i * 3] = w[i].x;
    v[28 + i * 3] = w[i].y;
    v[29 + i * 3] = w[i].z;
  }
  static const uint8_t ed[] = {
    0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7,
    8,1, 8,2, 8,5, 8,6,
    9,11, 9,13, 10,12, 10,14, 11,15, 12,16, 13,17, 14,18,
  };
  memcpy(A.parade.xw_e, ed, sizeof ed);
  xwing = (Model){ A.parade.xw, 19, A.parade.xw_e, (int)sizeof ed / 2 };
}

static bool sc4_init(void)
{
  build_parade();
  build_xwing(256);
  return true;
}

/* Camera-space position Q12 from units x1000. */
#define U(m) ((m) * 4096 / 1000)

static void sc4_render(int t)
{
  Xf w;
  int rt = ANG(t); /* 1 rad/s */

  fx_clear(fx_rgb(0, 0, 16));
  if (t < 5000)
  {
    xf_set(&w, 0, 0, U(-5000));
    xf_rot(&w, 1, rt); xf_rot(&w, 0, rt * 7 / 10);
    draw_model(&cube, &w, fx_rgb(0, 255, 255));
    label("CUBE", FX_H - 12, fx_rgb(120, 220, 220));
  }
  else if (t < 10000)
  {
    xf_set(&w, 0, 0, U(-5000));
    xf_rot(&w, 1, rt); xf_rot(&w, 0, ANG(400));
    draw_model(&cyl, &w, fx_rgb(120, 255, 120));
    label("CYLINDER", FX_H - 12, fx_rgb(120, 220, 120));
  }
  else if (t < 14000)
  {
    xf_set(&w, 0, 0, U(-5000));
    xf_rot(&w, 1, rt); xf_rot(&w, 2, rt / 2);
    draw_model(&octa, &w, fx_rgb(255, 200, 80));
    label("OCTAHEDRON", FX_H - 12, fx_rgb(255, 220, 120));
  }
  else if (t < 18000)
  {
    xf_set(&w, 0, 0, U(-7000));
    xf_rot(&w, 1, rt); xf_rot(&w, 0, rt * 6 / 10);
    draw_model(&ico, &w, fx_rgb(180, 180, 255));
    label("ICOSAHEDRON", FX_H - 12, fx_rgb(200, 200, 255));
  }
  else if (t < 23000)
  {
    xf_set(&w, 0, 0, U(-7000));
    xf_rot(&w, 1, rt * 7 / 10); xf_rot(&w, 0, rt * 4 / 10);
    draw_model(&dod, &w, fx_rgb(255, 140, 220));
    label("DODECAHEDRON", FX_H - 12, fx_rgb(255, 180, 230));
  }
  else if (t < 30000)
  {
    int open = t < 25000 ? 0 : t > 28000 ? 256 : (t - 25000) * 256 / 3000;

    build_xwing(open);
    xf_set(&w, 0, 0, U(-5000));
    xf_rot(&w, 1, rt * 4 / 10);
    draw_model(&xwing, &w, fx_rgb(220, 220, 255));
    label(t < 25000 ? "X-WING" : t < 28000 ? "S-FOILS LOCKED" : "X-WING", FX_H - 12, fx_rgb(220, 220, 255));
  }
  else if (t < 37000)
  {
    int dt = t - 30000;           /* ms */
    int pi15 = ANG(471);          /* pi * 0.15 */

    build_xwing(256);
    xf_set(&w, U(-1600), U(300) * fx_sin(ANG(dt * 13 / 10)) >> 15, U(-6000));
    xf_rot(&w, 1, pi15 + (fx_sin(ANG(dt)) * 24 >> 15));
    xf_rot(&w, 2, fx_sin(ANG(dt * 15 / 10)) * 49 >> 15);
    draw_model(&xwing, &w, fx_rgb(220, 220, 255));

    xf_set(&w, U(1800), -(U(400) * fx_sin(ANG(dt * 11 / 10)) >> 15), U(-6000));
    xf_rot(&w, 1, -pi15 + (fx_cos(ANG(dt)) * 24 >> 15));
    draw_model(&tie, &w, fx_rgb(255, 100, 100));

    /* alternating red and green vector shots, every 1.2 s */
    for (int k = 0; k < 2; k++)
    {
      int beat = (dt + k * 600) % 1200;
      if (beat < 250)
      {
        int p0[3] = { k ? U(1000) : U(-1000), 0, U(-5000) }, p1[3] = { k ? U(-2000) : U(2000), 0, U(-5000) };
        int ax, ay, bx, by;
        if (project(p0, &ax, &ay) && project(p1, &bx, &by))
          fx_line(ax, ay, bx, by, k ? fx_rgb(0, 255, 80) : fx_rgb(255, 0, 60));
      }
    }
    label("DOGFIGHT", FX_H - 12, fx_rgb(255, 200, 200));
  }
  else
  {
    int dt = t - 37000;
    int pull = iclamp(dt * 256 / 8000, 0, 256); /* Q8 */

    build_xwing(256);
    xf_set(&w, U(2000) + (U(500) * pull >> 8), U(600), U(-12000) - (U(4000) * pull >> 8));
    xf_rot(&w, 1, ANG(dt * 150 / 1000));
    draw_model(&deathstar, &w, fx_rgb(180, 180, 200));

    xf_set(&w, U(-3000) + (U(4000) * pull >> 8), U(500) - (U(300) * pull >> 8),
           U(-8000) - (U(2000) * pull >> 8));
    xf_rot(&w, 1, ANG(377));
    draw_model(&xwing, &w, fx_rgb(220, 220, 255));

    if (pull < 180)
    {
      xf_set(&w, U(-4500) + (U(4000) * pull >> 8), U(-600), U(-8000) - (U(2000) * pull >> 8));
      xf_rot(&w, 1, -ANG(377));
      draw_model(&tie, &w, fx_rgb(255, 100, 100));
    }
    if (pull > 102)
    {
      int b = 180 * (pull - 102) / 154;
      label("TO BE CONTINUED ...", 8, fx_rgb(b, b, b + 40 > 255 ? 255 : b + 40));
    }
  }
  /* hyperspace delivers us here: fade in from white */
  if (t < 500)
    fade_to_white((500 - t) * 256 / 500);
}

/* ---- scene 5: plasma "breathe" ---- */

static bool sc5_init(void)
{
  int cx = PL_W / 2, cy = PL_H / 2;
  int maxd = fx_isqrt((cx * cx + cy * cy) * 256); /* x16 */

  for (int y = 0; y < PL_H; y++)
    for (int x = 0; x < PL_W; x++)
    {
      int d = fx_isqrt(((x - cx) * (x - cx) + (y - cy) * (y - cy)) * 256);
      A.plasma.radial[y][x] = d * 255 / maxd;
    }
  return true;
}

/* sin(pos * freq + ts * speed) with freq and speed in thousandths. */
static inline int psin(int pos, int freq_k, int t_ms, int speed_k)
{
  return fx_sin(((pos * freq_k * 1000 + t_ms * speed_k) / 1000) * 163 / 1000);
}

static void write2x2(int px, int py, fb_data c)
{
  fb_data *r = fx_fb + py * 2 * FX_W + px * 2;

  r[0] = r[1] = r[FX_W] = r[FX_W + 1] = c;
}

static void sc5_render(int t)
{
  fb_data pal[256];
  int walk = iclamp(t * 256 / 25000, 0, 256);
  int rt = (256 - walk) * 256 + walk * 90, gt = (256 - walk) * 77 + walk * 13, bt = (256 - walk) * 13 + walk * 256;
  int col[PL_W], row[PL_H], diag[PL_W + PL_H], rad[256];

  for (int i = 0; i < 256; i++)
  {
    int in = (fx_sin(i * 4) + 32768) * 255 >> 16;
    pal[i] = fx_rgb(in * rt >> 16, in * gt >> 16, in * bt >> 16);
  }
  /* sin(2x * .05 + ts) etc, tabulated per column, row, diagonal and radius */
  for (int x = 0; x < PL_W; x++)
    col[x] = psin(2 * x, 50, t, 1000);
  for (int y = 0; y < PL_H; y++)
    row[y] = psin(2 * y, 60, t, 700);
  for (int i = 0; i < PL_W + PL_H; i++)
    diag[i] = psin(2 * i, 30, t, 500);
  for (int i = 0; i < 256; i++)
    rad[i] = psin(i, 40, t, 300);

  for (int py = 0; py < PL_H; py++)
    for (int px = 0; px < PL_W; px++)
    {
      int v = col[px] + row[py] + diag[px + py] + rad[A.plasma.radial[py][px]];
      write2x2(px, py, pal[iclamp((v + 131072) >> 10, 0, 255)]);
    }

  if (t >= 8000 && t <= 17000)
  {
    int k = t < 12500 ? (t - 8000) * 256 / 4500 : (17000 - t) * 256 / 4500;
    int b = iclamp(k, 0, 256) * 255 >> 8;
    const char *word = "breathe";

    if (b > 30)
      fx_text((FX_W - fx_text_w(word)) / 2 + 1, FX_H / 2 - 3, word, fx_rgb(0, 0, 0));
    fx_text_center(FX_H / 2 - 4, word, fx_rgb(b, b, b));
  }
}

/* ---- scene 6: metaballs crystallize into a glenz octahedron ---- */

#define N_BLOBS 4
#define BLOB_END_MS 12000
#define CRYSTAL_END_MS 14000

static void render_blobs(int t, int scale /* 0..256 */)
{
  int bx[N_BLOBS], by[N_BLOBS], br2[N_BLOBS]; /* Q8 */
  int walk = iclamp(t * 256 / 25000, 0, 256);
  int rt = (256 - walk) * 256 + walk * 102, gt = (256 - walk) * 128 + walk * 26, bt = walk * 230;

  for (int i = 0; i < N_BLOBS; i++)
  {
    int a = ANG(t * (500 + i * 130) / 1000) + ANG(1700 * i);
    int r = PL_W * 46 + 4 * 256 * fx_sin(ANG(t * 14 / 10) + ANG(1000 * i)) / 32768;

    bx[i] = PL_W * 128 + (fx_cos(a) * (PL_W * 77) >> 15);
    by[i] = PL_H * 128 + (fx_sin(a * 13 / 10 + ANG(700 * i)) * (PL_H * 77) >> 15);
    r = r * scale >> 8;
    br2[i] = r * r >> 8;
  }
  for (int py = 0; py < PL_H; py++)
    for (int px = 0; px < PL_W; px++)
    {
      int sum = 0; /* Q8 */
      for (int i = 0; i < N_BLOBS; i++)
      {
        int dx = px * 256 - bx[i], dy = py * 256 - by[i];
        int d2 = ((dx * dx + dy * dy) >> 8) + 256;
        sum += (br2[i] << 8) / d2;
      }
      int in = sum > 410 ? 255 : sum > 205 ? 60 + (sum - 205) * 195 / 205 : sum * 60 / 205;
      write2x2(px, py, fx_rgb(in * rt >> 16, in * gt >> 16, in * bt >> 16));
    }
}

static const int16_t gl_v[6 * 3] = {
  Q(1), 0, 0, Q(-1), 0, 0, 0, Q(1), 0, 0, Q(-1), 0, 0, 0, Q(1), 0, 0, Q(-1),
};
static const uint8_t gl_f[8][3] = {
  { 0, 2, 4 }, { 0, 4, 3 }, { 0, 3, 5 }, { 0, 5, 2 }, { 1, 4, 2 }, { 1, 3, 4 }, { 1, 5, 3 }, { 1, 2, 5 },
};
static const uint16_t gl_color[8] = { 0xF8C0, 0xFC60, 0xCFE0, 0x07F8, 0x041F, 0x781F, 0xC81F, 0xF810 };

/* Fill a triangle, blending colour c over what is there (alpha 0..256). */
static void tri_alpha(int x0, int y0, int x1, int y1, int x2, int y2, fb_data c, int alpha)
{
  if (alpha <= 0)
    return;
  if (y0 > y1) { int t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
  if (y0 > y2) { int t = y0; y0 = y2; y2 = t; t = x0; x0 = x2; x2 = t; }
  if (y1 > y2) { int t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
  for (int y = y0; y <= y2; y++)
  {
    if (y < 0)
      continue;
    if (y >= FX_H)
      break;
    int xa, xb;
    if (y2 == y0)
    {
      xa = x0 < x1 ? x0 : x1; xa = xa < x2 ? xa : x2;
      xb = x0 > x1 ? x0 : x1; xb = xb > x2 ? xb : x2;
    }
    else
    {
      xa = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
      if (y < y1)
        xb = y1 == y0 ? x0 : x0 + (x1 - x0) * (y - y0) / (y1 - y0);
      else
        xb = y2 == y1 ? x1 : x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    }
    if (xa > xb) { int t = xa; xa = xb; xb = t; }
    xa = xa < 0 ? 0 : xa;
    xb = xb >= FX_W ? FX_W - 1 : xb;
    for (int x = xa; x <= xb; x++)
      fx_fb[y * FX_W + x] = fx_blend(fx_fb[y * FX_W + x], c, alpha);
  }
}

static void render_glenz(int t, int alpha)
{
  Xf w;
  int sx[6], sy[6], z[6];
  bool ok[6];
  int order[8], fz[8];

  xf_set(&w, 0, 0, U(-3500));
  xf_rot(&w, 1, ANG(t) * 6 / 10);
  xf_rot(&w, 0, ANG(t) * 4 / 10);
  for (int v = 0; v < 6; v++)
  {
    int p[3];
    xf_apply(&w, gl_v + v * 3, p);
    z[v] = p[2];
    ok[v] = project(p, &sx[v], &sy[v]);
  }
  for (int f = 0; f < 8; f++)
  {
    fz[f] = (z[gl_f[f][0]] + z[gl_f[f][1]] + z[gl_f[f][2]]) / 3;
    int j = f - 1;
    for (; j >= 0 && fz[order[j]] > fz[f]; j--)
      order[j + 1] = order[j];
    order[j + 1] = f;
  }
  for (int k = 0; k < 8; k++) /* far to near */
  {
    int f = order[k];
    const uint8_t *i = gl_f[f];
    if (ok[i[0]] && ok[i[1]] && ok[i[2]])
      tri_alpha(sx[i[0]], sy[i[0]], sx[i[1]], sy[i[1]], sx[i[2]], sy[i[2]], FX_FROM565(gl_color[f]), alpha);
  }
  for (int f = 0; f < 8; f++)
  {
    const uint8_t *i = gl_f[f];
    if (ok[i[0]] && ok[i[1]] && ok[i[2]])
      for (int e = 0; e < 3; e++)
        fx_line(sx[i[e]], sy[i[e]], sx[i[(e + 1) % 3]], sy[i[(e + 1) % 3]], fx_rgb(255, 255, 255));
  }
}

static void sc6_render(int t)
{
  if (t < BLOB_END_MS)
    render_blobs(t, 256);
  else if (t < CRYSTAL_END_MS)
  {
    int k = (t - BLOB_END_MS) * 256 / (CRYSTAL_END_MS - BLOB_END_MS);

    render_blobs(t, 256 - k);
    render_glenz(t, k * 140 / 255);
  }
  else
  {
    fx_clear(fx_rgb(8, 0, 24));
    render_glenz(t, 140);
  }
}

/* ---- scene 7: rotozoomer with three textures ---- */

#define TEX_HOLD_MS 6500

static void roto_texture(int which)
{
  for (int y = 0; y < TEX; y++)
    for (int x = 0; x < TEX; x++)
    {
      fb_data c;
      if (which == 0)
        c = ((x >> 3) ^ (y >> 3)) & 1 ? fx_rgb(255, 255, 255) : fx_rgb(0, 0, 80);
      else if (which == 1)
      {
        int v = (x ^ y) << 1;
        c = fx_rgb(v, v / 2, 255 - v);
      }
      else
      {
        int dx = x - TEX / 2, dy = y - TEX / 2;
        int band = (int)fx_isqrt(dx * dx + dy * dy) * 12 & 255;
        c = fx_rgb(255 - band, 80, band);
      }
      A.roto.tex[y * TEX + x] = c;
    }
  A.roto.which = which;
}

static bool sc7_init(void)
{
  roto_texture(0);
  return true;
}

static void sc7_render(int t)
{
  int which = (t / TEX_HOLD_MS) % 3;
  int angle = ANG(t) * 6 / 10;
  int zoom = 36045 + 13107 * fx_sin(ANG(t * 4 / 10)) / 32768; /* 1.1 + 0.4 sin, Q15 */
  int c = fx_cos(angle) * zoom >> 14, s = fx_sin(angle) * zoom >> 14; /* Q16 */

  if (which != A.roto.which)
    roto_texture(which);
  for (int py = 0; py < PL_H; py++)
  {
    int yy = py - PL_H / 2;
    int u = -(PL_W / 2) * c - yy * s;
    int v = -(PL_W / 2) * s + yy * c;
    for (int px = 0; px < PL_W; px++)
    {
      write2x2(px, py, A.roto.tex[((v / 65536) & (TEX - 1)) * TEX + ((u / 65536) & (TEX - 1))]);
      u += c;
      v += s;
    }
  }
}

/* ---- scene 8: a swaying silhouette, frames streamed from the SD card ---- */

#define SIL_W 60
#define SIL_H 100
#define SIL_FRAMES 60
#define SIL_BYTES 750

static int sil_fd = -1;

static bool sc8_init(void)
{
  if (sil_fd >= 0)
    return true;
  sil_fd = rb->open(SIL_FILE, O_RDONLY);
  return sil_fd >= 0;
}

static void sc8_render(int t)
{
  uint8_t bits[SIL_BYTES];
  int frame = (t * 20 / 1000) % SIL_FRAMES;
  const int dw = SIL_W * 3 / 2, dh = SIL_H * 3 / 2, x0 = (FX_W - dw) / 2, y0 = (FX_H - dh) / 2;
  const fb_data fg = fx_rgb(255, 80, 200);

  fx_clear(fx_rgb(20, 0, 40));
  if (sil_fd < 0 || rb->lseek(sil_fd, frame * SIL_BYTES, SEEK_SET) < 0
      || rb->read(sil_fd, bits, SIL_BYTES) != SIL_BYTES)
    return;
  for (int y = 0; y < dh; y++)
    for (int x = 0; x < dw; x++)
    {
      int i = (y * 2 / 3) * SIL_W + x * 2 / 3;
      if (bits[i >> 3] & (1 << (i & 7)))
        fx_fb[(y0 + y) * FX_W + x0 + x] = fg;
    }
}

static void sc8_done(void)
{
  if (sil_fd >= 0)
  {
    rb->close(sil_fd);
    sil_fd = -1;
  }
}

/* ---- scene 9: voxel landscape flyover ---- */

static bool sc9_init(void)
{
  for (int y = 0; y < MAP_SZ; y++)
    for (int x = 0; x < MAP_SZ; x++)
    {
      int h = fx_sin(ANG(100 * x)) * 18 + fx_cos(ANG(80 * y)) * 22
              + fx_sin(ANG(50 * (x + y))) * 14 + fx_cos(ANG(70 * (x - y))) * 10;
      A.voxel.h[y][x] = iclamp(64 + h / 32768, 0, 255);
    }
  A.voxel.cam_h = 80 * 16;
  return true;
}

/* Blend two Q8 colour pairs by walk (0 noon .. 256 sunset) into 8 bit. */
static inline int mix(int a, int b, int walk) { return (a * (256 - walk) + b * walk) >> 8; }

static void sc9_render(int t)
{
  const int horizon = 70, depth = 80;
  int walk = iclamp(t * 256 / 30000, 0, 256);
  fb_data pal[256];
  int y_floor[FX_W];

  /* sky: top and horizon colours move from noon to sunset */
  for (int y = 0; y < horizon; y++)
  {
    int k = y * 255 / horizon;
    int rt = mix(102, 51, walk), gt = mix(140, 26, walk), bt = mix(242, 102, walk);
    int rh = mix(217, 255, walk), gh = mix(217, 140, walk), bh = mix(242, 77, walk);
    fx_hline(0, y, FX_W, fx_rgb((rt * (255 - k) + rh * k) / 255, (gt * (255 - k) + gh * k) / 255,
                                 (bt * (255 - k) + bh * k) / 255));
  }
  /* distance haze below the horizon, where no terrain reaches */
  fx_fill_rect(0, horizon, FX_W, FX_H - horizon, fx_rgb(mix(217, 255, walk), mix(217, 140, walk), mix(242, 77, walk)));
  for (int h = 0; h < 256; h++)
  {
    int lit = h; /* 0..255 */
    int r = mix(140 + (115 * lit >> 8), 102 + (140 * lit >> 8), walk);
    int g = mix(128 + (102 * lit >> 8), 51 + (77 * lit >> 8), walk);
    int b = mix(77 + (77 * lit >> 8), 13 + (38 * lit >> 8), walk);
    pal[h] = fx_rgb(iclamp(r, 0, 255), iclamp(g, 0, 255), iclamp(b, 0, 255));
  }

  int cam_x = (int)((int64_t)t * 8 * 65536 / 1000), cam_y = (int)((int64_t)t * 6 * 65536 / 1000); /* Q16 */
  int yaw = fx_sin(ANG(t / 10)) * 49 >> 15; /* sin(ts * 0.1) * 0.3 rad */
  int dir_x = fx_cos(yaw), dir_y = fx_sin(yaw); /* Q15 */

  /* fly above the ground (the upstream camera is a fixed 80 and can end up
   * inside a hill, hiding the whole view) */
  int ground = A.voxel.h[(cam_y >> 16) & (MAP_SZ - 1)][(cam_x >> 16) & (MAP_SZ - 1)];
  A.voxel.cam_h += ((ground + 24 > 80 ? ground + 24 : 80) * 16 - A.voxel.cam_h) / 4;
  int cam_h = A.voxel.cam_h >> 4;

  for (int x = 0; x < FX_W; x++)
    y_floor[x] = FX_H;

  for (int z = 1; z < depth; z++)
  {
    /* left edge of the view at this distance (right = (-dir_y, dir_x)), Q16 */
    int half = FX_W / 2 * z * 65536 / depth;
    int lx = cam_x + (int)(((int64_t)dir_x * z) << 1) + (int)(((int64_t)dir_y * half) >> 15);
    int ly = cam_y + (int)(((int64_t)dir_y * z) << 1) - (int)(((int64_t)dir_x * half) >> 15);
    int dx = -(int)(((int64_t)dir_y * half * 2) >> 15) / FX_W;
    int dy = (int)(((int64_t)dir_x * half * 2) >> 15) / FX_W;
    int mult = 60 * 65536 / z;

    for (int x = 0; x < FX_W; x++, lx += dx, ly += dy)
    {
      int h = A.voxel.h[(ly >> 16) & (MAP_SZ - 1)][(lx >> 16) & (MAP_SZ - 1)];
      int sy = horizon + (int)(((int64_t)(cam_h - h) * mult) >> 16);

      if (sy < y_floor[x])
      {
        fb_data c = pal[h];
        for (int y = sy < 0 ? 0 : sy; y < y_floor[x] && y < FX_H; y++)
          fx_fb[y * FX_W + x] = c;
        y_floor[x] = sy;
      }
    }
  }
}

/* ---- scene 10: twisting tunnel that exits into light ---- */

#define EXIT_BEGIN_MS 22000

/* atan2 in 1024 per turn (-512..511), good to about half a unit. */
static int fx_atan2(int y, int x)
{
  int ax = x < 0 ? -x : x, ay = y < 0 ? -y : y, a;

  if (ax == 0 && ay == 0)
    return 0;
  if (ay <= ax)
  {
    int z = ay * 32768 / ax; /* Q15 */
    a = (int)(((int64_t)z * (128 * 32768 + 44 * (int64_t)(32768 - z))) >> 30);
  }
  else
  {
    int z = ax * 32768 / ay;
    a = 256 - (int)(((int64_t)z * (128 * 32768 + 44 * (int64_t)(32768 - z))) >> 30);
  }
  if (x < 0)
    a = 512 - a;
  return y < 0 ? -a : a;
}

static bool sc10_init(void)
{
  for (int y = 0; y < PL_H; y++)
    for (int x = 0; x < PL_W; x++)
    {
      int dx = 2 * x - FX_W / 2, dy = 2 * y - FX_H / 2;
      int d = fx_isqrt((dx * dx + dy * dy) * 16); /* x4 */
      int u = d < 4 ? 0 : TEX * 32 * 4 / d;
      int v = (fx_atan2(dy, dx) + 512) * TEX / 1024;
      A.tunnel.u[y][x] = u & (TEX - 1);
      A.tunnel.v[y][x] = v & (TEX - 1);
    }
  for (int y = 0; y < TEX; y++)
    for (int x = 0; x < TEX; x++)
    {
      int c = ((x >> 3) ^ (y >> 3)) & 1;
      A.tunnel.tex[y * TEX + x] = fx_rgb(c ? 255 : 60, (y * 4) & 255, c ? 200 : 30);
    }
  return true;
}

static void sc10_render(int t)
{
  int scroll_u = (int)(((int64_t)t * (2621 + t * 262 / 1000)) >> 10) & (TEX - 1);
  int scroll_v = (fx_sin(t * 13 / 100) * 8 / 32768) & (TEX - 1);
  int k = t >= EXIT_BEGIN_MS ? iclamp((t - EXIT_BEGIN_MS) * 256 / (25000 - EXIT_BEGIN_MS), 0, 256) : 0;
  const fb_data white = fx_rgb(255, 255, 255);

  for (int y = 0; y < PL_H; y++)
    for (int x = 0; x < PL_W; x++)
    {
      int u = (A.tunnel.u[y][x] + scroll_u) & (TEX - 1);
      int v = (A.tunnel.v[y][x] + scroll_v) & (TEX - 1);
      fb_data c = A.tunnel.tex[v * TEX + u];
      write2x2(x, y, k ? fx_blend(c, white, k) : c);
    }
}

/* ---- scene 11: synthwave sun and grid ---- */

static void sc11_render(int t)
{
  const int horizon = FX_H * 11 / 20, sun_cx = FX_W / 2, sun_cy = horizon - 8, sun_r = 36;

  for (int y = 0; y < horizon; y++)
    fx_hline(0, y, FX_W, fx_rgb(20 + y * 180 / horizon, y * 30 / horizon, 40 + y * 90 / horizon));
  for (int y = sun_cy - sun_r; y <= sun_cy + sun_r; y++)
  {
    if (y < 0 || y >= horizon)
      continue;
    int dy = y - sun_cy, half = fx_isqrt(sun_r * sun_r - dy * dy);
    int k = (y - (sun_cy - sun_r)) * 256 / (sun_r * 2);
    int g = iclamp(220 - k * 200 / 256, 0, 220), b = iclamp(80 - k * 60 / 256, 0, 255);

    if (((y / 4) & 1) == 0) /* classic outrun stripes */
      fx_hline(sun_cx - half, y, half * 2, fx_rgb(255, g, b));
  }
  fx_fill_rect(0, horizon, FX_W, FX_H - horizon, fx_rgb(10, 0, 20));

  /* grid: rows bunch up towards the horizon, columns fan out from it */
  const int n_horiz = 8, n_vert = 11;
  const fb_data pink = fx_rgb(255, 80, 200);
  int scroll = (t / 30) % 100;

  for (int i = 0; i < n_horiz; i++)
  {
    int r = (i * 100 + scroll) * 256 / (n_horiz * 100); /* 0..255+ */
    if (r >= 256)
      r -= 256;
    int y = horizon + (r * r >> 8) * (FX_H - horizon) / 256;
    if (y >= horizon && y < FX_H)
      fx_hline(0, y, FX_W, pink);
  }
  for (int i = 0; i < n_vert; i++)
  {
    int k = i * 256 / (n_vert - 1) - 128; /* -0.5..0.5, Q8 */
    fx_line(sun_cx + k * 24 / 256, horizon, sun_cx + (k * FX_W * 14 / 10) / 256, FX_H - 1, pink);
  }
  /* chrome-ish "2026" floating above the sun, drawn twice for width */
  int yx = (FX_W - fx_text_w("2026") * 2) / 2, yy = sun_cy - 36;
  for (int rep = 0; rep < 2; rep++)
  {
    fx_text(yx + rep * 6 + 2, yy + 2, "2026", fx_rgb(0, 0, 30));
    fx_text(yx + rep * 6 + 1, yy + 1, "2026", fx_rgb(0, 220, 255));
    fx_text(yx + rep * 6, yy, "2026", fx_rgb(255, 255, 255));
  }
  /* the tunnel's light delivers us here */
  if (t < 500)
    fade_to_white((500 - t) * 256 / 500);
}

/* ---- scene 12: credits ---- */

static void sc12_render(int t)
{
  static const char *const credits[] = {
    "YP3 DEMO", "", "A TOUR THROUGH TIME", "", "CODE & DESIGN", "  MCAREY42 + CLAUDE",
    "", "ROCKBOX PORT", "  YP3 SL6801", "", "INSPIRED BY", "  SPACEBALLS", "  KEFRENS",
    "  SCOOPEX", "  SANITY", "  FUTURE CREW", "  TBL", "", "GREETZ TO ALL THE",
    "DEMOSCENE LEGENDS", "", "2026", "", "", "...AND THE LOOP", "BEGINS AGAIN",
  };
  const int n = sizeof credits / sizeof credits[0], line_h = 12;
  static const int16_t cameo_v[6 * 3] = {
    Q(-2), 0, 0, Q(2), 0, 0, Q(-2), Q(.5), Q(.5), Q(-2), Q(-.5), Q(-.5), Q(-2), Q(.5), Q(-.5), Q(-2), Q(-.5), Q(.5),
  };
  static const uint8_t cameo_e[] = { 0,1, 0,2, 0,3, 0,4, 0,5 };
  static const Model cameo = { cameo_v, 6, cameo_e, 5 };
  Xf w;

  fx_clear(fx_rgb(0, 0, 0));
  int cam_x = U(-8000) + t * 12 / 10 * 4096 / 1000; /* sweeps left to right */
  if (cam_x < U(12000))
  {
    xf_set(&w, cam_x, U(1500), U(-8000));
    xf_rot(&w, 1, ANG(t) * 6 / 10);
    draw_model(&cameo, &w, fx_rgb(60, 60, 90));
  }
  int y0 = FX_H - t * (n * line_h + FX_H) / 25000;
  for (int i = 0; i < n; i++)
  {
    int y = y0 + i * line_h;
    if (y < -8 || y >= FX_H || !credits[i][0])
      continue;
    int x = (FX_W - fx_text_w(credits[i])) / 2;
    fx_text(x + 1, y + 1, credits[i], fx_rgb(0, 0, 0));
    fx_text(x, y, credits[i], fx_rgb(255, 255, 255));
  }
  for (int y = FX_H - 6; y < FX_H; y++)
  {
    int h = (((y - (FX_H - 6)) + t / 30) & 255) / 43 % 6;
    static const uint8_t col[6][3] = { { 200, 0, 0 }, { 200, 100, 0 }, { 0, 200, 0 }, { 0, 200, 200 }, { 0, 0, 200 }, { 200, 0, 200 } };
    fx_hline(0, y, FX_W, fx_rgb(col[h][0], col[h][1], col[h][2]));
  }
}

/* ---- the director ---- */

struct scene
{
  const char *name;
  bool (*init)(void);
  void (*render)(int t);
  void (*done)(void);
  int duration;
  bool fade; /* fade to black afterwards (the other scenes hand over by white) */
};

static const struct scene scenes[] = {
  { "Boot and title", NULL, sc1_render, NULL, 12000, true },
  { "Copper and scroller", NULL, sc2_render, NULL, 25000, true },
  { "Starfield", sc3_init, sc3_render, NULL, 18000, false },
  { "Vector parade", sc4_init, sc4_render, NULL, 45000, true },
  { "Plasma", sc5_init, sc5_render, NULL, 25000, true },
  { "Metaballs and glenz", NULL, sc6_render, NULL, 25000, true },
  { "Rotozoomer", sc7_init, sc7_render, NULL, 20000, true },
  { "Silhouette", sc8_init, sc8_render, sc8_done, 20000, true },
  { "Voxel landscape", sc9_init, sc9_render, NULL, 25000, true },
  { "Tunnel", sc10_init, sc10_render, NULL, 25000, false },
  { "Synthwave", NULL, sc11_render, NULL, 20000, true },
  { "Credits", NULL, sc12_render, NULL, 25000, true },
};
#define N_SCENES ((int)(sizeof scenes / sizeof scenes[0]))
#define FADE_MS 500

static int cur;        /* running scene */
static int t_scene;    /* ms into the scene */
static int fade_ms;    /* >0: fading out the finished scene */
static bool paused;
static bool show_name;
static int name_ms;

static void enter(int i)
{
  int tries = N_SCENES;

  if (scenes[cur].done)
    scenes[cur].done();
  for (; tries > 0; tries--, i++)
  {
    i = (i + N_SCENES) % N_SCENES;
    cur = i;
    t_scene = 0;
    fade_ms = 0;
    if (!scenes[i].init || scenes[i].init())
      break;
  }
  name_ms = 1500;
}

static bool demo_init(void)
{
  cur = 0;
  show_name = true;
  enter(0);
  return true;
}

static void demo_start(void)
{
  rb->lcd_set_viewport(NULL);
  tune_start();
}

static bool demo_frame(unsigned keys, unsigned edge, int dt_ms)
{
  if (edge & BUTTON_RIGHT)
    enter(cur + 1);
  else if (edge & BUTTON_LEFT)
    enter(t_scene > 1500 ? cur : cur - 1);
  if ((edge & BUTTON_UP) && !(keys & BUTTON_SELECT)) /* not the menu chord */
    paused = !paused;
  tune_hold = paused;
  if (edge & BUTTON_DOWN)
    show_name = !show_name;

  const struct scene *s = &scenes[cur];

  if (!paused)
  {
    if (fade_ms > 0)
      fade_ms += dt_ms;
    else
      t_scene += dt_ms;
  }
  if (fade_ms > 0 && fade_ms >= FADE_MS)
    enter(cur + 1);
  else if (fade_ms == 0 && t_scene >= s->duration)
  {
    if (s->fade)
      fade_ms = 1;
    else
      enter(cur + 1);
  }
  s = &scenes[cur];
  s->render(fade_ms > 0 ? s->duration - 1 : t_scene);
  if (fade_ms > 0)
    fade_to_black(iclamp(fade_ms * 256 / FADE_MS, 0, 256));

  if (show_name && name_ms > 0)
  {
    if (!paused)
      name_ms -= dt_ms;
    fx_text_shadow(2, 2, s->name, fx_rgb(255, 255, 255));
  }
  if (paused)
    fx_text_shadow(2, FX_H - 9, "PAUSED", fx_rgb(255, 255, 255));
  return false;
}
