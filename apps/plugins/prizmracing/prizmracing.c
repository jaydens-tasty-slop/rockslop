/* PRIZM Racing for Rockbox (YP3 port): a fixed-point textured polygon racer.
 *
 * Game, models and car texture by Duarte Coelho, MIT License (see LICENSE),
 * https://github.com/duarteapcoelho/prizm_racing. This is a C rewrite of the
 * upstream renderer and driving model on 16.16 fixed point: the transforms,
 * near-plane clipping, scanline rasteriser with an 8-bit depth buffer and
 * the car physics follow upstream; malloc, float and the Casio platform
 * layers are gone. The car texture is a 128x128, 32 colour version produced
 * by maketex.py (pr_data.h). */
#include "plugin.h"
#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/playback_control.h"

#define PR_NAME "PRIZM Racing"
#define TRACK_DIR ROCKBOX_DIR "/rocks/games/prizmracing"

struct pr_tri
{
  int16_t p[9];   /* 4.12 view of the three vertices (x, y, z each) */
  int8_t n[3];    /* face normal, /127 */
  uint16_t col;   /* RGB565 (plain, not byte swapped) */
  int16_t uv[6];  /* 4.12 texture coordinates, u then v per vertex */
};

#include "pr_data.h"

typedef int32_t fx;
#define FX1 65536
#define FXC(x) ((fx)((x) * 65536.0 + ((x) < 0 ? -0.5 : 0.5)))

static inline fx fxmul(fx a, fx b) { return (fx)(((int64_t)a * b) >> 16); }
static inline fx fxdiv(fx a, fx b) { return (fx)(((int64_t)a * FX1) / b); }

#if LCD_PIXELFORMAT == RGB565SWAPPED
#define PIX(c) ((fb_data)((((c) >> 8) | ((c) << 8)) & 0xffff))
#elif LCD_PIXELFORMAT == RGB565
#define PIX(c) ((fb_data)(c))
#else
#define PIX(c) FB_RGBPACK((((c) >> 11) << 3), ((((c) >> 5) & 63) << 2), (((c) & 31) << 3))
#endif
#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

#define W LCD_WIDTH
#define H LCD_HEIGHT
#define NEAR FXC(0.1)
#define MAX_POINTS 64
#define MAX_TRACKS 8

/* ------------------------------------------------------------ math -- */

static fx sin_idx(int i)
{
  i &= 4095;
  int r = i & 1023;
  switch (i >> 10)
  {
    case 0: return pr_sintab[r];
    case 1: return pr_sintab[1024 - r];
    case 2: return -pr_sintab[r];
    default: return -pr_sintab[1024 - r];
  }
}

/* angle in radians, 16.16 */
static fx fxsin(fx a) { return sin_idx((int)(((int64_t)a * 42722830) >> 32)); }
static fx fxcos(fx a) { return sin_idx((int)(((int64_t)a * 42722830) >> 32) + 1024); }

static uint32_t isqrt64(uint64_t v)
{
  uint64_t r = 0, bit = (uint64_t)1 << 62;
  while (bit > v)
    bit >>= 2;
  while (bit)
  {
    if (v >= r + bit)
    {
      v -= r + bit;
      r = (r >> 1) + bit;
    }
    else
      r >>= 1;
    bit >>= 2;
  }
  return (uint32_t)r;
}

/* affine transform: p' = m * p + t */
struct mat { fx m[3][3]; fx t[3]; };

static void mat_id(struct mat *a)
{
  rb->memset(a, 0, sizeof *a);
  a->m[0][0] = a->m[1][1] = a->m[2][2] = FX1;
}

static void mat_mul(struct mat *r, const struct mat *a, const struct mat *b)
{
  struct mat o;
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
      o.m[i][j] = fxmul(a->m[i][0], b->m[0][j]) + fxmul(a->m[i][1], b->m[1][j])
                  + fxmul(a->m[i][2], b->m[2][j]);
    o.t[i] = fxmul(a->m[i][0], b->t[0]) + fxmul(a->m[i][1], b->t[1])
             + fxmul(a->m[i][2], b->t[2]) + a->t[i];
  }
  *r = o;
}

static void mat_translate(struct mat *m, fx x, fx y, fx z)
{
  struct mat t;
  mat_id(&t);
  t.t[0] = x;
  t.t[1] = y;
  t.t[2] = z;
  mat_mul(m, m, &t);
}

static void mat_rotx(struct mat *m, fx a)
{
  struct mat t;
  mat_id(&t);
  t.m[1][1] = t.m[2][2] = fxcos(a);
  t.m[2][1] = fxsin(a);
  t.m[1][2] = -t.m[2][1];
  mat_mul(m, m, &t);
}

static void mat_roty(struct mat *m, fx a)
{
  struct mat t;
  mat_id(&t);
  t.m[0][0] = t.m[2][2] = fxcos(a);
  t.m[0][2] = fxsin(a);
  t.m[2][0] = -t.m[0][2];
  mat_mul(m, m, &t);
}

static void mat_apply(const struct mat *m, const fx v[3], fx o[3])
{
  for (int i = 0; i < 3; i++)
    o[i] = fxmul(m->m[i][0], v[0]) + fxmul(m->m[i][1], v[1]) + fxmul(m->m[i][2], v[2]) + m->t[i];
}

/* ------------------------------------------------------- rasteriser -- */

static unsigned char depth_buf[W * H];
static fb_data *fb;
static fx proj_k; /* pixels per unit at z = 1, 16.16 */

static fb_data lit_pal[16][PR_PAL]; /* palette at 16 brightness levels */

static void build_lit_palettes(void)
{
  for (int l = 0; l < 16; l++)
    for (int i = 0; i < PR_PAL; i++)
    {
      unsigned c = pr_palette[i];
      unsigned r = ((c >> 11) & 31) * (l + 1) / 16, g = ((c >> 5) & 63) * (l + 1) / 16,
               b = (c & 31) * (l + 1) / 16;
      lit_pal[l][i] = PIX((r << 11) | (g << 5) | b);
    }
}

struct fill
{
  bool depth;
  bool tex;
  fb_data col;
  const fb_data *pal;
};

static void raster(const struct fill *f, int x0, int y0, int x1, int y1, int x2, int y2,
                   fx u0, fx v0, fx u1, fx v1, fx u2, fx v2, unsigned char z)
{
#define SWAP(a, b, t) do { t = a; a = b; b = t; } while (0)
  int ti;
  fx tf;
  if (y0 > y1) { SWAP(x0, x1, ti); SWAP(y0, y1, ti); SWAP(u0, u1, tf); SWAP(v0, v1, tf); }
  if (y1 > y2) { SWAP(x1, x2, ti); SWAP(y1, y2, ti); SWAP(u1, u2, tf); SWAP(v1, v2, tf); }
  if (y0 > y1) { SWAP(x0, x1, ti); SWAP(y0, y1, ti); SWAP(u0, u1, tf); SWAP(v0, v1, tf); }
  if (y2 < 0 || y0 >= H || y0 == y2)
    return;

  int64_t dx02 = ((int64_t)(x2 - x0) * FX1) / (y2 - y0);
  int64_t dx01 = y1 != y0 ? ((int64_t)(x1 - x0) * FX1) / (y1 - y0) : 0;
  int64_t dx12 = y2 != y1 ? ((int64_t)(x2 - x1) * FX1) / (y2 - y1) : 0;
  int64_t dudx = 0, dudy = 0, dvdx = 0, dvdy = 0;

  if (f->tex)
  {
    int ax = x1 - x0, ay = y1 - y0, bx = x2 - x0, by = y2 - y0;
    int64_t d = (int64_t)ax * by - (int64_t)ay * bx;
    if (!d)
      return;
    int64_t du1 = u1 - u0, du2 = u2 - u0, dv1 = v1 - v0, dv2 = v2 - v0;
    dudx = (du1 * by - du2 * ay) / d;
    dudy = (du2 * ax - du1 * bx) / d;
    dvdx = (dv1 * by - dv2 * ay) / d;
    dvdy = (dv2 * ax - dv1 * bx) / d;
  }

  int ys = y0 < 0 ? 0 : y0, ye = y2 >= H ? H - 1 : y2;
  for (int y = ys; y <= ye; y++)
  {
    int64_t xa = ((int64_t)x0 * FX1) + (int64_t)(y - y0) * dx02;
    int64_t xb = y < y1 ? ((int64_t)x0 * FX1) + (int64_t)(y - y0) * dx01
                        : ((int64_t)x1 * FX1) + (int64_t)(y - y1) * dx12;
    int64_t lo = (xa < xb ? xa : xb) >> 16, hi = (xa < xb ? xb : xa) >> 16;
    if (hi < 0 || lo >= W)
      continue;
    int xs = lo < 0 ? 0 : (int)lo, xe = hi >= W ? W - 1 : (int)hi;
    fb_data *row = fb + y * W;
    unsigned char *dp = depth_buf + y * W;

    if (!f->tex)
    {
      if (!f->depth)
        for (int x = xs; x <= xe; x++)
          row[x] = f->col;
      else
        for (int x = xs; x <= xe; x++)
          if (z < dp[x])
          {
            dp[x] = z;
            row[x] = f->col;
          }
      continue;
    }

    int64_t u = u0 + dudx * (xs - x0) + dudy * (y - y0);
    int64_t v = v0 + dvdx * (xs - x0) + dvdy * (y - y0);
    for (int x = xs; x <= xe; x++, u += dudx, v += dvdx)
    {
      if (f->depth)
      {
        if (z >= dp[x])
          continue;
        dp[x] = z;
      }
      int ui = (int)(u >> 16), vi = (int)(v >> 16);
      if ((unsigned)ui >= PR_TEX)
        ui = ui < 0 ? 0 : PR_TEX - 1;
      if ((unsigned)vi >= PR_TEX)
        vi = vi < 0 ? 0 : PR_TEX - 1;
      row[x] = f->pal[pr_texture[vi * PR_TEX + ui]];
    }
  }
}

struct vtx { fx x, y, z, u, v; };

/* clip a triangle against the near plane; returns 0, 3 or 4 vertices */
static int clip_near(const struct vtx in[3], struct vtx out[4])
{
  int n = 0;
  for (int i = 0; i < 3; i++)
  {
    const struct vtx *a = &in[i], *b = &in[(i + 1) % 3];
    bool ia = a->z >= NEAR, ib = b->z >= NEAR;
    if (ia)
      out[n++] = *a;
    if (ia != ib)
    {
      fx t = fxdiv(NEAR - a->z, b->z - a->z);
      out[n].x = a->x + fxmul(b->x - a->x, t);
      out[n].y = a->y + fxmul(b->y - a->y, t);
      out[n].z = NEAR;
      out[n].u = a->u + fxmul(b->u - a->u, t);
      out[n].v = a->v + fxmul(b->v - a->v, t);
      n++;
    }
  }
  return n;
}

static void project(const struct vtx *p, int *sx, int *sy)
{
  *sx = W / 2 + (int)(((int64_t)p->x * proj_k / p->z) >> 16);
  *sy = H / 2 + (int)(((int64_t)p->y * proj_k / p->z) >> 16);
}

struct mesh_cfg
{
  bool depth;
  bool shaded;
  bool tex;
  bool clip; /* clip against the near plane; otherwise drop such triangles */
};

static struct mat view;

static void draw_mesh(const struct pr_tri *tris, int count, const struct mat *model,
                      const struct mesh_cfg *cfg)
{
  struct mat vm;
  mat_mul(&vm, &view, model);

  for (int i = 0; i < count; i++)
  {
    const struct pr_tri *t = &tris[i];
    fx nrm[3] = { t->n[0] * (FX1 / 127), t->n[1] * (FX1 / 127), t->n[2] * (FX1 / 127) };
    fx nv = fxmul(vm.m[2][0], nrm[0]) + fxmul(vm.m[2][1], nrm[1]) + fxmul(vm.m[2][2], nrm[2]);
    if (nv > FXC(0.3))
      continue; /* back face */

    struct vtx v[3], c[4];
    bool behind = false;
    for (int j = 0; j < 3; j++)
    {
      fx p[3] = { t->p[j * 3] * 16, t->p[j * 3 + 1] * 16, t->p[j * 3 + 2] * 16 }, o[3];
      mat_apply(&vm, p, o);
      v[j].x = o[0];
      v[j].y = o[1];
      v[j].z = o[2];
      v[j].u = t->uv[j * 2] * (PR_TEX - 1) * 16;
      v[j].v = t->uv[j * 2 + 1] * (PR_TEX - 1) * 16;
      if (o[2] < NEAR)
        behind = true;
    }
    if (behind && !cfg->clip)
      continue;
    int n = clip_near(v, c);
    if (n < 3)
      continue;

    struct fill f = { cfg->depth, cfg->tex, 0, NULL };
    int level = 15;
    if (cfg->shaded)
    {
      fx mn[3] = { fxmul(model->m[0][0], nrm[0]) + fxmul(model->m[0][1], nrm[1]) + fxmul(model->m[0][2], nrm[2]),
                   fxmul(model->m[1][0], nrm[0]) + fxmul(model->m[1][1], nrm[1]) + fxmul(model->m[1][2], nrm[2]),
                   fxmul(model->m[2][0], nrm[0]) + fxmul(model->m[2][1], nrm[1]) + fxmul(model->m[2][2], nrm[2]) };
      fx b = fxmul(mn[0] - mn[1] - mn[2], FXC(0.57735 * 0.5)) + FXC(0.5);
      b = b < 0 ? 0 : (b > FX1 ? FX1 : b);
      level = (b * 15) >> 16;
    }
    if (cfg->tex)
      f.pal = lit_pal[level];
    else
    {
      unsigned r = ((t->col >> 11) & 31) * (level + 1) / 16, g = ((t->col >> 5) & 63) * (level + 1) / 16,
               bl = (t->col & 31) * (level + 1) / 16;
      f.col = PIX((r << 11) | (g << 5) | bl);
    }

    int sx[4], sy[4];
    fx zsum = 0;
    for (int j = 0; j < n; j++)
    {
      project(&c[j], &sx[j], &sy[j]);
      zsum += c[j].z;
    }
    int z8 = (int)(((int64_t)(zsum / n) * 5) >> 16) + 1;
    if (z8 > 255)
      z8 = 255;
    for (int j = 1; j + 1 < n; j++)
      raster(&f, sx[0], sy[0], sx[j], sy[j], sx[j + 1], sy[j + 1],
             c[0].u, c[0].v, c[j].u, c[j].v, c[j + 1].u, c[j + 1].v, (unsigned char)z8);
  }
}

/* ------------------------------------------------------------ track -- */

struct seg { fx x[4], z[4]; }; /* p0 = pos - perp, p1 = pos + perp, p2, p3 next */

static struct seg segs[MAX_POINTS];
static fx pts_x[MAX_POINTS], pts_z[MAX_POINTS];
static int npts;
static fx half_width = FXC(10.0);
static int cur_track; /* 0 = built in */
static char track_names[MAX_TRACKS][24];
static int ntracks;

/* built-in track: the upstream figure of 28 points, x and z in 1/1000 of
 * 40 game units */
static const int16_t builtin_track[28][2] = {
  {0, 0}, {2000, 0}, {2707, 293}, {3000, 1000}, {3293, 1707}, {4000, 2000}, {4707, 1707},
  {5000, 1000}, {5293, 293}, {6000, 0}, {7000, 0}, {7707, -293}, {8000, -1000},
  {7707, -1707}, {7000, -2000}, {5000, -2000}, {4293, -2293}, {4000, -3000}, {3707, -3707},
  {3000, -4000}, {2293, -3707}, {2000, -3000}, {1707, -2293}, {1000, -2000}, {0, -2000},
  {-707, -1707}, {-1000, -1000}, {-707, -293},
};

static void track_build(void)
{
  for (int i = 0; i < npts; i++)
  {
    int n1 = (i + 1) % npts, n2 = (i + 2) % npts;
    fx dx = pts_x[n1] - pts_x[i], dz = pts_z[n1] - pts_z[i];
    fx ex = pts_x[n2] - pts_x[n1], ez = pts_z[n2] - pts_z[n1];
    /* isqrt64 of a 32.32 value is a 16.16 length */
    fx l1 = (fx)isqrt64((uint64_t)((int64_t)dx * dx + (int64_t)dz * dz));
    fx l2 = (fx)isqrt64((uint64_t)((int64_t)ex * ex + (int64_t)ez * ez));
    fx px = l1 ? fxmul(fxdiv(dz, l1), half_width) : 0, pz = l1 ? fxmul(fxdiv(-dx, l1), half_width) : 0;
    fx qx = l2 ? fxmul(fxdiv(ez, l2), half_width) : 0, qz = l2 ? fxmul(fxdiv(-ex, l2), half_width) : 0;
    struct seg *s = &segs[i];
    s->x[0] = pts_x[i] - px;  s->z[0] = pts_z[i] - pz;
    s->x[1] = pts_x[i] + px;  s->z[1] = pts_z[i] + pz;
    s->x[2] = pts_x[i] - qx + dx;  s->z[2] = pts_z[i] - qz + dz;
    s->x[3] = pts_x[i] + qx + dx;  s->z[3] = pts_z[i] + qz + dz;
  }
}

static void track_builtin(void)
{
  npts = 28;
  for (int i = 0; i < npts; i++)
  {
    pts_x[i] = (fx)((int64_t)builtin_track[i][0] * 40 * FX1 / 1000);
    pts_z[i] = (fx)((int64_t)builtin_track[i][1] * 40 * FX1 / 1000);
  }
  half_width = FXC(10.0);
  track_build();
}

/* Track file: text, "#" comments, an optional "width N" line, then one
 * "x z" pair per line in tenths of a game unit. The car starts at the first
 * point heading +x. */
static bool track_load(const char *name)
{
  char path[MAX_PATH];
  static char buf[1536];
  rb->snprintf(path, sizeof path, TRACK_DIR "/%s", name);
  int fd = rb->open(path, O_RDONLY);
  if (fd < 0)
    return false;
  int len = rb->read(fd, buf, sizeof buf - 1);
  rb->close(fd);
  if (len <= 0)
    return false;
  buf[len] = 0;

  int n = 0;
  fx width = FXC(10.0);
  char *p = buf;
  while (*p && n < MAX_POINTS)
  {
    char *e = p;
    while (*e && *e != '\n')
      e++;
    char save = *e;
    *e = 0;
    if (*p == 'w')
    {
      int w = rb->atoi(p + 5);
      if (w >= 20 && w <= 400)
        width = w * (FX1 / 10);
    }
    else if (*p == '-' || (*p >= '0' && *p <= '9'))
    {
      char *q;
      int x = (int)rb->strtol(p, &q, 10);
      int z = (int)rb->strtol(q, NULL, 10);
      pts_x[n] = x * (FX1 / 10);
      pts_z[n] = z * (FX1 / 10);
      n++;
    }
    *e = save;
    p = save ? e + 1 : e;
  }
  if (n < 3)
    return false;
  npts = n;
  half_width = width;
  track_build();
  return true;
}

static void track_scan(void)
{
  ntracks = 0;
  DIR *d = rb->opendir(TRACK_DIR);
  if (!d)
    return;
  struct dirent *e;
  while ((e = rb->readdir(d)) && ntracks < MAX_TRACKS - 1)
  {
    int l = rb->strlen(e->d_name);
    if (l > 4 && l < (int)sizeof track_names[0] && !rb->strcasecmp(e->d_name + l - 4, ".trk"))
      rb->strlcpy(track_names[ntracks++], e->d_name, sizeof track_names[0]);
  }
  rb->closedir(d);
}

static void track_select(int idx)
{
  cur_track = idx;
  if (idx == 0 || !track_load(track_names[idx - 1]))
  {
    cur_track = 0;
    track_builtin();
  }
}

static bool track_inside(fx px, fx pz)
{
  int64_t best = -1;
  for (int i = 0; i < npts; i++)
  {
    int j = (i + 1) % npts;
    int64_t ax = pts_x[i], az = pts_z[i];
    int64_t bx = pts_x[j] - ax, bz = pts_z[j] - az;
    int64_t qx = px - ax, qz = pz - az;
    int64_t l2 = (bx * bx + bz * bz) >> 16;
    int64_t t = l2 ? ((qx * bx + qz * bz) >> 16) * FX1 / l2 : 0; /* 16.16 */
    if (t < 0) t = 0;
    if (t > FX1) t = FX1;
    int64_t dx = qx - ((bx * t) >> 16), dz = qz - ((bz * t) >> 16);
    int64_t d = (dx * dx + dz * dz) >> 16;
    if (best < 0 || d < best)
      best = d;
  }
  return best < ((int64_t)half_width * half_width >> 16);
}

static const struct mesh_cfg cfg_cone = { true, true, false, false };
static const struct mesh_cfg cfg_car = { true, true, true, true };

static void track_render(fx cx, fx cz)
{
  struct fill f = { false, false, 0, NULL };
  const fb_data road = PIX(C565(50, 50, 50));
  f.col = road;

  /* road: two triangles per segment, no depth */
  for (int i = 0; i < npts; i++)
  {
    const struct seg *s = &segs[i];
    int64_t dx = s->x[0] - cx, dz = s->z[0] - cz;
    if ((dx * dx + dz * dz) >> 16 > (int64_t)250 * 250 * FX1)
      continue;
    static const int idx[2][3] = { { 0, 1, 2 }, { 1, 2, 3 } };
    for (int k = 0; k < 2; k++)
    {
      struct vtx v[3], c[4];
      bool any = false;
      for (int j = 0; j < 3; j++)
      {
        fx p[3] = { s->x[idx[k][j]], 0, s->z[idx[k][j]] }, o[3];
        mat_apply(&view, p, o);
        v[j].x = o[0]; v[j].y = o[1]; v[j].z = o[2]; v[j].u = v[j].v = 0;
        any |= o[2] >= NEAR;
      }
      if (!any)
        continue;
      int n = clip_near(v, c), sx[4], sy[4];
      if (n < 3)
        continue;
      for (int j = 0; j < n; j++)
        project(&c[j], &sx[j], &sy[j]);
      for (int j = 1; j + 1 < n; j++)
        raster(&f, sx[0], sy[0], sx[j], sy[j], sx[j + 1], sy[j + 1], 0, 0, 0, 0, 0, 0, 255);
    }
  }

  /* cones along both edges at every second point, nearby only */
  struct mat m;
  for (int i = 0; i < npts; i += 2)
  {
    const struct seg *s = &segs[i];
    int64_t dx = pts_x[i] - cx, dz = pts_z[i] - cz;
    if ((dx * dx + dz * dz) >> 16 > (int64_t)150 * 150 * FX1)
      continue;
    for (int k = 0; k < 2; k++)
    {
      mat_id(&m);
      mat_translate(&m, s->x[k], 0, s->z[k]);
      draw_mesh(pr_cone_tris, PR_CONE_N, &m, &cfg_cone);
    }
  }
}

/* -------------------------------------------------------------- car -- */

static fx car_x, car_z, car_vx, car_vz;
static fx wheel_speed, target_dir, car_dir;
static fx cam_x, cam_z, cam_vx, cam_vz, cam_angle;
static fx fov_deg = 70 * FX1;

static void game_reset(void)
{
  car_x = pts_x[0];
  car_z = pts_z[0];
  car_vx = car_vz = 0;
  wheel_speed = target_dir = car_dir = 0;
  cam_x = car_x;
  cam_z = car_z;
  cam_vx = cam_vz = 0;
  cam_angle = 0;
}

/* One fixed step of 4/128 s; the constants are upstream's per-delta rates
 * multiplied by 4. */
static void game_step(unsigned keys)
{
  if (keys & BUTTON_RIGHT)
    target_dir += FXC(0.05);
  if (keys & BUTTON_LEFT)
    target_dir -= FXC(0.05);
  if (keys & BUTTON_UP)
    wheel_speed += FXC(0.008);
  if (keys & BUTTON_DOWN)
    wheel_speed -= FXC(0.02);
#ifdef HAVE_BLUETOOTH_HID
  if (keys & BUTTON_PAD_A)
    wheel_speed += FXC(0.008);
  if (keys & BUTTON_PAD_B)
    wheel_speed -= FXC(0.02);
#endif
  if (wheel_speed > FX1)
    wheel_speed = FX1;
  if (wheel_speed < 0)
    wheel_speed = 0;

  bool on = track_inside(car_x, car_z);
  wheel_speed = fxdiv(wheel_speed, on ? FXC(1.004) : FXC(1.008));
  fx acc = fxmul(wheel_speed, FXC(0.016));
  car_vx += fxmul(acc, fxcos(car_dir));
  car_vz += fxmul(acc, fxsin(-car_dir));
  car_vx = fxdiv(car_vx, on ? FXC(1.04) : FXC(1.08));
  car_vz = fxdiv(car_vz, on ? FXC(1.04) : FXC(1.08));
  car_x += car_vx * 4;
  car_z += car_vz * 4;
  car_dir += fxmul(target_dir - car_dir, FXC(0.2));

  cam_angle += fxmul(car_dir - cam_angle, FXC(0.08));

  /* smoothDamp (smooth time 5, step 4) towards the car on the ground plane */
  fx cx = cam_x - car_x, cz = cam_z - car_z;
  fx tx = fxmul(cam_vx + fxmul(cx, FXC(0.4)), 4 * FX1);
  fx tz = fxmul(cam_vz + fxmul(cz, FXC(0.4)), 4 * FX1);
  cam_vx = fxmul(cam_vx - fxmul(tx, FXC(0.4)), FXC(0.2088));
  cam_vz = fxmul(cam_vz - fxmul(tz, FXC(0.4)), FXC(0.2088));
  fx ox = car_x + fxmul(cx + tx, FXC(0.2088)), oz = car_z + fxmul(cz + tz, FXC(0.2088));
  if ((int64_t)(car_x - cam_x) * (ox - car_x) + (int64_t)(car_z - cam_z) * (oz - car_z) > 0)
  {
    ox = car_x;
    oz = car_z;
    cam_vx = cam_vz = 0;
  }
  cam_x = ox;
  cam_z = oz;
}

static int speed_kmh(void)
{
  uint64_t l2 = (uint64_t)((int64_t)car_vx * car_vx + (int64_t)car_vz * car_vz);
  fx len = (fx)isqrt64(l2); /* sqrt of a 32.32 value: 16.16 */
  return (int)(((int64_t)len * 461) >> 16);
}

static void set_fov(fx deg)
{
  /* d = 2 * tan(fov / 2); projection scale is W / d pixels per unit */
  int i = (int)(((int64_t)deg * 4096) / (720 * (int64_t)FX1));
  fx s = sin_idx(i), c = sin_idx(i + 1024);
  fx d = 2 * fxdiv(s, c);
  proj_k = (fx)(((int64_t)W << 32) / d);
}

static void sky_and_ground(int floor_y)
{
  const uint32_t sky = PIX(C565(70, 180, 220)), grass = PIX(C565(32, 79, 19));
  uint32_t *p = (uint32_t *)fb;
  if (floor_y < 0) floor_y = 0;
  if (floor_y > H) floor_y = H;
  for (int i = 0; i < floor_y * W / 2; i++)
    p[i] = sky | (sky << 16);
  for (int i = floor_y * W / 2; i < W * H / 2; i++)
    p[i] = grass | (grass << 16);
}

static bool show_fps = true;
static int fps;

static void render(void)
{
  fx len = (fx)isqrt64((uint64_t)((int64_t)car_vx * car_vx + (int64_t)car_vz * car_vz));
  fx fv = 70 * FX1 + 50 * len;
  fov_deg = fv > 110 * FX1 ? 110 * FX1 : fv;
  set_fov(fov_deg);

  mat_id(&view);
  mat_rotx(&view, FXC(1.570796327 * 0.1));
  mat_translate(&view, 0, 2 * FX1, 6 * FX1);
  mat_roty(&view, -cam_angle - FXC(1.570796327));
  mat_translate(&view, -cam_x, 0, -cam_z);

  /* horizon: tan(-0.157) / fov_d * W + H / 2 */
  fx tn = fxdiv(fxsin(-FXC(0.157079)), fxcos(FXC(0.157079)));
  int floor_y = H / 2 + (int)(((int64_t)tn * proj_k) >> 32);
  sky_and_ground(floor_y);
  rb->memset(depth_buf, 255, sizeof depth_buf);

  struct mat m;
  static const struct mesh_cfg cfg_sun = { false, false, false, true };
  mat_id(&m);
  mat_translate(&m, 20 * FX1, -6 * FX1, -20 * FX1);
  mat_translate(&m, cam_x, 0, cam_z);
  mat_roty(&m, cam_angle + FXC(1.570796327));
  draw_mesh(pr_sun_tris, PR_SUN_N, &m, &cfg_sun);

  track_render(car_x, car_z);

  mat_id(&m);
  mat_translate(&m, car_x, -FX1 / 2, car_z);
  mat_roty(&m, car_dir);
  draw_mesh(pr_car_tris, PR_CAR_N, &m, &cfg_car);

  rb->lcd_set_drawmode(DRMODE_FG);
  rb->lcd_set_foreground(LCD_WHITE);
  if (show_fps)
    rb->lcd_putsxyf(0, 0, (const unsigned char *)"FPS: %d", fps);
  rb->lcd_putsxyf(0, H - 10, (const unsigned char *)"SPEED: %d", speed_kmh());
  rb->lcd_update();
}

/* --------------------------------------------------------- Rockbox -- */

static bool usb_connected;

static bool show_text(char **words, int count)
{
  if (display_text(count, words, NULL, NULL, true))
  {
    usb_connected = true;
    return true;
  }
  return false;
}

static void settings(void)
{
  static const struct opt_items onoff[2] = { { "Off", -1 }, { "On", -1 } };
  char names[MAX_TRACKS][24];
  struct opt_items items[MAX_TRACKS];
  MENUITEM_STRINGLIST(menu, "Settings", NULL, "Track", "Show FPS");
  int sel = 0;

  while (true)
  {
    int r = rb->do_menu(&menu, &sel, NULL, false);
    if (r < 0)
      return;
    if (r == 0)
    {
      rb->strlcpy(names[0], "Built-in", sizeof names[0]);
      items[0].string = names[0];
      items[0].voice_id = -1;
      for (int i = 0; i < ntracks; i++)
      {
        rb->strlcpy(names[i + 1], track_names[i], sizeof names[0]);
        names[i + 1][rb->strlen(names[i + 1]) - 4] = 0;
        items[i + 1].string = names[i + 1];
        items[i + 1].voice_id = -1;
      }
      int t = cur_track;
      rb->set_option("Track", &t, RB_INT, items, ntracks + 1, NULL);
      if (t != cur_track)
        track_select(t);
    }
    else
    {
      int f = show_fps;
      rb->set_option("Show FPS", &f, RB_INT, onoff, 2, NULL);
      show_fps = f;
    }
  }
}

/* Start menu, also the pause menu. Returns true to race, false to quit. */
static bool menu(bool started)
{
  MENUITEM_STRINGLIST(start_menu, PR_NAME, NULL, "Start", "Settings", "Help", "About",
                      "Playback Control", "Quit");
  MENUITEM_STRINGLIST(pause_menu, PR_NAME, NULL, "Resume", "Settings", "Help", "About",
                      "Playback Control", "Quit");
  static char *help[] = { "PRIZM Racing.", "", "LEFT/RIGHT: steer.", "UP: accelerate.",
                          "DOWN: brake.", "Stay on the grey road;", "the grass slows you.",
                          "Hold SELECT+UP: menu.", "", "Extra tracks: put .trk", "files in",
                          "rocks/games/prizmracing", "and pick one in", "Settings." };
  static char *about[] = { "PRIZM Racing", "by Duarte Coelho,", "MIT License.",
                           "github.com/", "duarteapcoelho/", "prizm_racing", "",
                           "Rockbox port: YP3." };
  int sel = 0;

  rb->lcd_set_viewport(NULL);
  rb->lcd_set_background(LCD_BLACK);
  rb->lcd_clear_display();

  while (true)
  {
    int r = rb->do_menu(started ? &pause_menu : &start_menu, &sel, NULL, false);
    if (r == MENU_ATTACHED_USB)
    {
      usb_connected = true;
      return false;
    }
    switch (r)
    {
      case 0:
        return true;
      case 1:
        settings();
        break;
      case 2:
        if (show_text(help, ARRAYLEN(help)))
          return false;
        break;
      case 3:
        if (show_text(about, ARRAYLEN(about)))
          return false;
        break;
      case 4:
        playback_control(NULL);
        break;
      default:
        return false;
    }
  }
}

/* Returns true to open the menu, false on USB. */
static bool race(void)
{
  unsigned latched = 0;
  unsigned long hold_start = 0;
  unsigned long last = *rb->current_tick, fps_t = last;
  int acc = 0, frames = 0;
  const unsigned dirs = BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT;

  fb = (*rb->screens[SCREEN_MAIN]->current_viewport)->buffer->fb_ptr;
  rb->button_clear_queue();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
  rb->cpu_boost(true);
#endif

  while (true)
  {
    int ev;
    while ((ev = rb->button_get(false)) != BUTTON_NONE)
    {
      if (!(ev & BUTTON_REL))
        latched |= ev & (dirs | BUTTON_SELECT);
      /* SELECT is also the power key: a power-off request is dropped
       * while racing */
      if (ev == SYS_POWEROFF)
        continue;
#ifdef HAVE_BLUETOOTH_HID
      if ((ev & (BUTTON_PAD_MENU | BUTTON_PAD_HOME)) && !(ev & BUTTON_REL))
      {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif
        return true;
      }
#endif
      if (rb->default_event_handler(ev) == SYS_USB_CONNECTED)
      {
        usb_connected = true;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif
        return false;
      }
    }

    unsigned keys = rb->button_status() | latched;
    latched = 0;
    /* Hold SELECT+UP for 500 ms: pause menu */
    if ((keys & (BUTTON_SELECT | BUTTON_UP)) == (BUTTON_SELECT | BUTTON_UP))
    {
      if (!hold_start)
        hold_start = *rb->current_tick;
      else if (TIME_AFTER(*rb->current_tick, hold_start + HZ / 2))
      {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif
        return true;
      }
    }
    else
      hold_start = 0;

    unsigned long now = *rb->current_tick;
    acc += (int)(now - last) * 128;
    last = now;
    if (acc > 6 * 400)
      acc = 6 * 400;
    while (acc >= 400)
    {
      acc -= 400;
      game_step(keys);
    }

    render();

    frames++;
    if (now - fps_t >= HZ)
    {
      fps = frames * HZ / (int)(now - fps_t);
      frames = 0;
      fps_t = now;
    }
    rb->yield();
  }
}

enum plugin_status plugin_start(const void *parameter)
{
  (void)parameter;

  backlight_ignore_timeout();
  playback_control_init(NULL);
  build_lit_palettes();
  track_scan();
  track_builtin();
  game_reset();

  bool started = false;
  while (menu(started))
  {
    started = true;
    if (!race())
      break;
  }

  backlight_use_settings();
  return usb_connected ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
