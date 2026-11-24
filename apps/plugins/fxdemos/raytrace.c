/* Progressive ray tracer for Rockbox: a Cornell-box style scene with a
 * reflective and a diffuse sphere, a textured floor, soft radiosity lighting
 * and dithered output. Ported from TheScienceElf's TI-84 CE Raytracing
 * (https://github.com/TheScienceElf/TI-84-CE-Raytracing, MIT): the eZ80
 * 24-bit fixed point types and assembly are replaced by 32-bit 12.12 fixed
 * point, the display access by direct framebuffer writes, and the render is
 * done in progressively finer passes so it can be watched and interrupted. */
#include "plugin.h"

#include "fx_common.h"

#define FX_NAME "Raytracer"
#define FX_SETTINGS rt_settings
#define FX_PAUSE_MS 1000 /* SELECT+UP moves the camera forward */
#define FX_MIN_TICKS 1
#define FX_HELP "Raytracer:", "a small scene is", "traced in passes, from", "coarse to fine.", "", \
  "D-pad: move the camera", "or the light.", "SELECT: render / stop.", \
  "SELECT+UP/DOWN: move", "forward / back.", "SELECT+LEFT: switch", \
  "camera / light.", "Hold SELECT+UP", "1 s: menu.", "", "A full render takes", "a while; a light move", "recomputes radiosity."
#define FX_ABOUT "Raytracer", "after TI-84 CE", "Raytracing by", "TheScienceElf", \
  "github.com/TheScienceElf/", "TI-84-CE-Raytracing", "MIT licence."

#include "fx_main.h"

#include "rt_tables.h"
#include "rt_wood.h"

/* ---- 12.12 fixed point ---- */

typedef int32_t fx;
#define POINT 12
#define ONE (1 << POINT)
#define FXC(f) ((fx)((f) * ONE))

static inline fx fmul(fx a, fx b) { return (fx)(((int64_t)a * b) >> POINT); }
static inline fx fsqr(fx a) { return fmul(a, a); }

static inline fx fdiv(fx a, fx b)
{
  if (b == 0)
    return a < 0 ? -0x3fffffff : 0x3fffffff;
  if (a > -(1 << 19) && a < (1 << 19))
    return (a * ONE) / b;
  return (fx)(((int64_t)a * ONE) / b);
}

static fx fsqrt(fx a)
{
  if (a <= 0)
    return 0;
  if (a < (1 << 20))
    return fx_isqrt((unsigned long)a << POINT);
  return fx_isqrt(a) << (POINT / 2);
}

static inline fx clamp01(fx x) { return x < 0 ? 0 : x > ONE ? ONE : x; }
static inline fx clamp0(fx x) { return x < 0 ? 0 : x; }

/* arcsin(x) / (pi / 2) from a table */
static inline fx fasin(fx x)
{
  int i = (x + ONE) >> 7;
  return asin_table[i < 0 ? 0 : i > 64 ? 64 : i];
}

/* Angle of (x, y) around the origin divided by pi (0..2). */
static fx fatan2(fx x, fx y)
{
  fx rad = fsqrt(fsqr(x) + fsqr(y));
  fx nx = fdiv(x, rad);
  nx = nx < -ONE ? -ONE : nx > ONE ? ONE : nx;
  fx a = (ONE + fasin(-nx)) >> 1;
  return y > 0 ? a : 2 * ONE - a;
}

typedef struct { fx x, y, z; } Vec3;

static inline Vec3 V(fx x, fx y, fx z) { return (Vec3){ x, y, z }; }
static inline Vec3 vadd(Vec3 a, Vec3 b) { return V(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline Vec3 vsub(Vec3 a, Vec3 b) { return V(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline Vec3 vscale(Vec3 a, fx s) { return V(fmul(a.x, s), fmul(a.y, s), fmul(a.z, s)); }
static inline fx dot(Vec3 a, Vec3 b) { return fmul(a.x, b.x) + fmul(a.y, b.y) + fmul(a.z, b.z); }
static inline fx norm_sq(Vec3 a) { return dot(a, a); }
static inline fx norm(Vec3 a) { return fsqrt(norm_sq(a)); }

typedef struct { Vec3 origin, dir; } Ray;

static inline Vec3 ray_at(const Ray *r, fx t) { return vadd(r->origin, vscale(r->dir, t)); }

/* ---- colours ---- */

/* sRGB colours are 5 bits per channel: (r << 11) | (g << 6) | b */
typedef uint16_t Color;
#define FROMRGB(r, g, b) ((Color)(((r) << 11) | ((g) << 6) | (b)))
#define LERP_BITS 5
#define LERP_MASK ((1 << LERP_BITS) - 1)
#define LERP_HALF (1 << (LERP_BITS - 1))

static Color lerp_color(Color c1, Color c2, int t)
{
  int r1 = (c1 >> 11) & 31, g1 = (c1 >> 6) & 31, b1 = c1 & 31;
  int r2 = (c2 >> 11) & 31, g2 = (c2 >> 6) & 31, b2 = c2 & 31;

  return FROMRGB(r1 + (r2 - r1) * t / LERP_MASK, g1 + (g2 - g1) * t / LERP_MASK,
                 b1 + (b2 - b1) * t / LERP_MASK);
}

/* Linear light, Q12 per channel. */
typedef struct { fx r, g, b; } Spectrum;

static inline Spectrum spec_add(Spectrum a, Spectrum b)
{
  return (Spectrum){ a.r + b.r, a.g + b.g, a.b + b.b };
}

static inline Spectrum spec_scale(Spectrum a, fx s)
{
  return (Spectrum){ fmul(a.r, s), fmul(a.g, s), fmul(a.b, s) };
}

static inline Spectrum spec_gray(fx f) { return (Spectrum){ f, f, f }; }

static Spectrum spec_from_color(Color c)
{
  return (Spectrum){ degamma_lut[(c >> 11) & 31], degamma_lut[(c >> 6) & 31],
                     degamma_lut[c & 31] };
}

/* Invert gamma_lut by binary search: linear Q12 -> 0..255 sRGB. */
static int gamma8(fx x)
{
  unsigned low = 0, high = 255;

  if (x <= 0)
    return 0;
  while (low < high)
  {
    unsigned mid = (low + high) >> 1;
    if (gamma_lut[mid] == x)
      return mid;
    if (gamma_lut[mid] > x)
      high = mid - 1;
    else
      low = mid + 1;
  }
  return low;
}

static Spectrum spec_lerp(Spectrum a, Spectrum b, int t)
{
  return (Spectrum){ a.r + (b.r - a.r) * t / LERP_MASK, a.g + (b.g - a.g) * t / LERP_MASK,
                     a.b + (b.b - a.b) * t / LERP_MASK };
}

static Spectrum spec_bilinear(Spectrum t00, Spectrum t10, Spectrum t01, Spectrum t11, int sx, int sy)
{
  return spec_lerp(spec_lerp(t00, t10, sx), spec_lerp(t01, t11, sx), sy);
}

/* ---- light maps: 8x8 patches of radiance on each surface ---- */

#define MAP_BITS 3
#define MAP_SIZE (1 << MAP_BITS)
#define MAP_MASK (MAP_SIZE - 1)
#define STEP_SIZE (2 * ONE / MAP_SIZE)

typedef struct
{
  Spectrum bitmap[MAP_SIZE][MAP_SIZE];    /* total emission for rendering */
  Spectrum emissive[MAP_SIZE][MAP_SIZE];  /* outgoing radiance */
  Spectrum emissive2[MAP_SIZE][MAP_SIZE]; /* incoming radiance */
} LightMap;

static const int sphere_x[8] = { 3784, 1567, -1567, -3784, -3784, -1567, 1567, 3784 };
static const int sphere_y[8] = { 1567, 3784, 3784, 1567, -1567, -3784, -3784, -1567 };
static const int sphere_elev[8] = { -4017, -3405, -2275, -799, 799, 2275, 3405, 4017 };
static const int sphere_rad[8] = { 799, 2275, 3405, 4017, 4017, 3405, 2275, 799 };

static void lm_clear(LightMap *m)
{
  memset(m->bitmap, 0, sizeof m->bitmap);
}

/* Add this round's incoming light to the total and make it the new emission. */
static void lm_copy(LightMap *m)
{
  for (int y = 0; y < MAP_SIZE; y++)
    for (int x = 0; x < MAP_SIZE; x++)
    {
      m->bitmap[x][y] = spec_add(m->bitmap[x][y], m->emissive2[x][y]);
      m->emissive[x][y] = m->emissive2[x][y];
    }
}

static void lm_from_bitmap(LightMap *m)
{
  memcpy(m->emissive, m->bitmap, sizeof m->emissive);
}

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static Spectrum lm_fetch(const LightMap *m, int tx, int ty, int sx, int sy)
{
  int x0 = clampi(tx, 0, MAP_MASK), x1 = clampi(tx + 1, 0, MAP_MASK);
  int y0 = clampi(ty, 0, MAP_MASK), y1 = clampi(ty + 1, 0, MAP_MASK);

  return spec_bilinear(m->bitmap[x0][y0], m->bitmap[x1][y0], m->bitmap[x0][y1],
                       m->bitmap[x1][y1], sx, sy);
}

#define LM_DIGITS (LERP_BITS + MAP_BITS - 1)

/* Sample an axis-aligned plane's map; hit is relative to the plane origin. */
static Spectrum lm_sample_plane(const LightMap *m, Vec3 hit, Vec3 normal)
{
  int tile_x = (hit.x >> (POINT - LM_DIGITS)) - LERP_HALF;
  int tile_y = (hit.y >> (POINT - LM_DIGITS)) - LERP_HALF;
  int tile_z = (hit.z >> (POINT - LM_DIGITS)) - LERP_HALF;
  int tx = 0, ty = 0, sx = 0, sy = 0;

  if (normal.x)
  {
    tx = tile_z >> LERP_BITS; ty = tile_y >> LERP_BITS;
    sx = tile_z & LERP_MASK; sy = tile_y & LERP_MASK;
  }
  if (normal.y)
  {
    tx = tile_x >> LERP_BITS; ty = tile_z >> LERP_BITS;
    sx = tile_x & LERP_MASK; sy = tile_z & LERP_MASK;
  }
  if (normal.z)
  {
    tx = tile_x >> LERP_BITS; ty = tile_y >> LERP_BITS;
    sx = tile_x & LERP_MASK; sy = tile_y & LERP_MASK;
  }
  return lm_fetch(m, tx, ty, sx, sy);
}

/* Sample a sphere's map by surface normal. */
static Spectrum lm_sample_sphere(const LightMap *m, Vec3 normal)
{
  fx theta = fatan2(normal.x, normal.z);
  fx phi = ONE + fasin(normal.y);
  int tile_x = (theta >> (POINT - LM_DIGITS)) - LERP_HALF;
  int tile_y = (phi >> (POINT - LM_DIGITS)) - LERP_HALF;

  return lm_fetch(m, (int8_t)(tile_x >> LERP_BITS), (int8_t)(tile_y >> LERP_BITS),
                  tile_x & LERP_MASK, tile_y & LERP_MASK);
}

/* ---- scene ---- */

#define TEX_BITS 7
#define TEX_MASK ((1 << TEX_BITS) - 1)
#define TEX_DIGITS (LERP_BITS + TEX_BITS - 1)

typedef struct
{
  Vec3 point, normal;
  Spectrum albedo;
  LightMap lm;
  bool textured;     /* the wood floor */
  fx numerator;      /* camera-dependent part of the ray intersection */
} Plane;

typedef struct
{
  Vec3 point;
  fx radius;
  bool reflective;
  LightMap lm;
  Vec3 offset;       /* camera-dependent precomputation */
  fx c;
} Sphere;

static Plane planes[5];
static Sphere spheres[2];
static Vec3 light, cam;
static const fx exposure = 2 * ONE;

static void scene_reset(void)
{
  static const struct { Vec3 p, n; Color c; bool tex; } defs[5] = {
    { { -ONE, -ONE, 2 * ONE }, { 0, ONE, 0 }, FROMRGB(29, 24, 18), true },
    { { -ONE, ONE, 2 * ONE }, { 0, -ONE, 0 }, FROMRGB(24, 24, 24), false },
    { { -ONE, -ONE, 2 * ONE }, { ONE, 0, 0 }, FROMRGB(24, 9, 9), false },
    { { ONE, -ONE, 2 * ONE }, { -ONE, 0, 0 }, FROMRGB(9, 9, 26), false },
    { { -ONE, -ONE, 4 * ONE }, { 0, 0, -ONE }, FROMRGB(24, 24, 24), false },
  };

  for (int i = 0; i < 5; i++)
  {
    planes[i].point = defs[i].p;
    planes[i].normal = defs[i].n;
    planes[i].albedo = spec_from_color(defs[i].c);
    planes[i].textured = defs[i].tex;
  }
  spheres[0] = (Sphere){ .point = V(FXC(-.33f), FXC(-.6f), FXC(3.052f)), .radius = FXC(.4f), .reflective = true };
  spheres[1] = (Sphere){ .point = V(FXC(.43f), FXC(-.6f), FXC(2.43f)), .radius = FXC(.4f), .reflective = false };
  light = V(0, FXC(.5f), 3 * ONE);
  cam = V(0, 0, 0);
}

/* Camera-dependent precomputation for the primary rays. */
static void register_camera(void)
{
  for (int i = 0; i < 5; i++)
    planes[i].numerator = dot(vsub(planes[i].point, cam), planes[i].normal);
  for (int i = 0; i < 2; i++)
  {
    spheres[i].offset = vsub(cam, spheres[i].point);
    spheres[i].c = norm_sq(spheres[i].offset) - fsqr(spheres[i].radius);
  }
}

/* Restrict plane hits to the 2x2x2 scene region. */
static inline bool in_box(Vec3 h)
{
  const fx hi = FXC(2.01f), lo = FXC(-.01f);
  return h.x <= hi && h.y <= hi && h.z <= hi && h.x >= lo && h.y >= lo && h.z >= lo;
}

static fx plane_hit(const Plane *p, const Ray *r, bool fast)
{
  fx num = fast ? p->numerator : dot(vsub(p->point, r->origin), p->normal);
  fx den = dot(r->dir, p->normal);

  if (den == 0)
    return -ONE;
  fx t = fdiv(num, den);
  if (t <= 1)
    return -ONE;
  return in_box(vsub(ray_at(r, t), p->point)) ? t : -ONE;
}

static fx sphere_hit(const Sphere *s, const Ray *r, bool fast)
{
  Vec3 off = fast ? s->offset : vsub(r->origin, s->point);
  fx c = fast ? s->c : norm_sq(off) - fsqr(s->radius);
  fx a = norm_sq(r->dir);
  fx b2 = dot(off, r->dir);
  fx d = fsqr(b2) - fmul(a, c);

  if (d < 0)
    return -ONE;
  return fdiv(-b2 - fsqrt(d), a);
}

/* Is the light blocked by a sphere? Only spheres cast shadows. */
static bool in_shadow(const Ray *r)
{
  for (int i = 0; i < 2; i++)
  {
    Vec3 off = vsub(r->origin, spheres[i].point);
    fx a = norm_sq(r->dir);
    fx b2 = dot(off, r->dir);
    fx c = norm_sq(off) - fsqr(spheres[i].radius);
    fx d = fsqr(b2) - fmul(a, c);

    if (d >= 0 && -b2 - fsqrt(d) < a)
      return true;
  }
  return false;
}

/* ---- radiosity ---- */

static Vec3 sample_pos_plane(const Plane *p, int x, int y)
{
  Vec3 sx = V(0, 0, 0), sy = V(0, 0, 0);

  if (p->normal.x)
  {
    sx = V(0, 0, STEP_SIZE);
    sy = V(0, STEP_SIZE, 0);
  }
  if (p->normal.y)
  {
    sx = V(STEP_SIZE, 0, 0);
    sy = V(0, 0, STEP_SIZE);
  }
  if (p->normal.z)
  {
    sx = V(STEP_SIZE, 0, 0);
    sy = V(0, STEP_SIZE, 0);
  }
  /* half a step in from the corner, then x and y steps */
  return V(p->point.x + (sx.x + sy.x) / 2 + x * sx.x + y * sy.x,
           p->point.y + (sx.y + sy.y) / 2 + x * sx.y + y * sy.y,
           p->point.z + (sx.z + sy.z) / 2 + x * sx.z + y * sy.z);
}

/* Patch positions on the unit sphere. */
static Vec3 sample_pos_sphere(int x, int y)
{
  fx scl = sphere_rad[y];

  return V(fmul(sphere_x[x], scl), sphere_elev[y], fmul(sphere_y[x], scl));
}

static void compute_shadows(Plane *p)
{
  for (int y = 0; y < MAP_SIZE; y++)
    for (int x = 0; x < MAP_SIZE; x++)
    {
      Vec3 o = sample_pos_plane(p, x, y);
      Ray sr = { o, vsub(light, o) };
      fx n = norm(sr.dir);
      fx att = clamp0(fdiv(dot(p->normal, sr.dir), fmul(n, norm_sq(sr.dir))));

      p->lm.emissive[x][y] = spec_scale(p->albedo, att);
      for (int i = 0; i < 2; i++)
        if (sphere_hit(&spheres[i], &sr, false) > 0)
        {
          p->lm.emissive[x][y] = (Spectrum){ 0, 0, 0 };
          break;
        }
    }
}

/* Light arriving at in_pos from the patches of one plane. */
static Spectrum incident_radiosity(const Plane *p, Vec3 in_pos, Vec3 normal)
{
  Spectrum inc = { 0, 0, 0 };

  for (int y = 0; y < MAP_SIZE; y++)
    for (int x = 0; x < MAP_SIZE; x++)
    {
      Vec3 dir = vsub(sample_pos_plane(p, x, y), in_pos);
      fx att = fmul(dot(normal, dir), -dot(p->normal, dir));

      att = clamp0(fdiv(att, fsqr(norm_sq(dir))));
      if (att > 2 * ONE) /* a patch almost touching the receiver */
        att = 2 * ONE;
      inc = spec_add(inc, spec_scale(p->lm.emissive[x][y], att));
    }
  return spec_scale(inc, ONE / (MAP_SIZE * MAP_SIZE));
}

static void plane_radiosity(Plane *p)
{
  for (int y = 0; y < MAP_SIZE; y++)
    for (int x = 0; x < MAP_SIZE; x++)
    {
      Vec3 in_pos = sample_pos_plane(p, x, y);
      Spectrum inc = { 0, 0, 0 };

      for (int i = 0; i < 5; i++)
        if (&planes[i] != p)
          inc = spec_add(inc, incident_radiosity(&planes[i], in_pos, p->normal));

      /* divide by pi */
      inc = spec_scale(inc, FXC(1.0 / 3.14159265));
      p->lm.emissive2[x][y] = (Spectrum){ fmul(inc.r, p->albedo.r), fmul(inc.g, p->albedo.g),
                                          fmul(inc.b, p->albedo.b) };
    }
}

static void sphere_radiosity(Sphere *s)
{
  for (int y = 0; y < MAP_SIZE; y++)
    for (int x = 0; x < MAP_SIZE; x++)
    {
      Vec3 normal = sample_pos_sphere(x, y);
      Vec3 point = vadd(vscale(normal, s->radius), s->point);
      Spectrum inc = { 0, 0, 0 };

      for (int i = 0; i < 5; i++)
        inc = spec_add(inc, incident_radiosity(&planes[i], point, normal));
      /* not physically accurate, but it emphasises colour bleeding */
      s->lm.bitmap[x][y] = spec_scale(inc, FXC(3.14159265));
    }
}

/* Direct light, then two bounces, then the spheres. */
static void compute_lighting(void)
{
  for (int i = 0; i < 5; i++)
  {
    compute_shadows(&planes[i]);
    lm_clear(&planes[i].lm);
  }
  for (int i = 0; i < 2; i++)
    lm_clear(&spheres[i].lm);

  for (int round = 0; round < 2; round++)
  {
    for (int i = 0; i < 5; i++)
      plane_radiosity(&planes[i]);
    for (int i = 0; i < 5; i++)
      lm_copy(&planes[i].lm);
  }
  for (int i = 0; i < 5; i++)
    lm_from_bitmap(&planes[i].lm);
  for (int i = 0; i < 2; i++)
    if (!spheres[i].reflective)
      sphere_radiosity(&spheres[i]);
}

/* ---- shading ---- */

static Spectrum compute_ray(const Ray *ray, bool from_cam, int depth);

static Color wood_sample(Vec3 hit, Vec3 normal)
{
  int tile_x = (hit.x >> (POINT - TEX_DIGITS)) - LERP_HALF;
  int tile_y = (hit.y >> (POINT - TEX_DIGITS)) - LERP_HALF;
  int tile_z = (hit.z >> (POINT - TEX_DIGITS)) - LERP_HALF;
  int tx = 0, ty = 0, sx = 0, sy = 0;

  if (normal.x)
  {
    tx = tile_z >> LERP_BITS; ty = tile_y >> LERP_BITS;
    sx = tile_z & LERP_MASK; sy = tile_y & LERP_MASK;
  }
  if (normal.y)
  {
    tx = tile_x >> LERP_BITS; ty = tile_z >> LERP_BITS;
    sx = tile_x & LERP_MASK; sy = tile_z & LERP_MASK;
  }
  if (normal.z)
  {
    tx = tile_x >> LERP_BITS; ty = tile_y >> LERP_BITS;
    sx = tile_x & LERP_MASK; sy = tile_y & LERP_MASK;
  }
  int x0 = clampi(tx, 0, TEX_MASK), x1 = clampi(tx + 1, 0, TEX_MASK);
  int y0 = clampi(ty, 0, TEX_MASK), y1 = clampi(ty + 1, 0, TEX_MASK);

  return lerp_color(lerp_color(rt_wood[y0][x0], rt_wood[y0][x1], sx),
                    lerp_color(rt_wood[y1][x0], rt_wood[y1][x1], sx), sy);
}

static Spectrum shade_plane(const Plane *p, Vec3 hit)
{
  Vec3 offset = vsub(light, hit);
  Ray sr = { hit, offset };
  Vec3 sample = vsub(hit, p->point);
  Spectrum color = lm_sample_plane(&p->lm, sample, p->normal);

  if (!in_shadow(&sr))
  {
    fx att = dot(p->normal, offset);
    att = clamp01(fdiv(att, fmul(norm(offset), norm_sq(offset))));
    color = spec_add(color, spec_scale(p->albedo, att));
  }
  if (p->textured)
  {
    /* swap the flat albedo for the texel */
    Spectrum tex = spec_from_color(wood_sample(sample, p->normal));
    color = (Spectrum){ fmul(fdiv(color.r, p->albedo.r), tex.r), fmul(fdiv(color.g, p->albedo.g), tex.g),
                        fmul(fdiv(color.b, p->albedo.b), tex.b) };
  }
  return color;
}

static Spectrum shade_sphere(const Sphere *s, Vec3 hit, const Ray *ray, int depth)
{
  Vec3 normal = vsub(hit, s->point);
  Vec3 to_light = vsub(light, hit);

  normal = V(fdiv(normal.x, s->radius), fdiv(normal.y, s->radius), fdiv(normal.z, s->radius));
  if (s->reflective)
  {
    Vec3 refl = vsub(ray->dir, vscale(normal, 2 * dot(ray->dir, normal)));
    Ray rr = { vadd(hit, vscale(refl, FXC(.01f))), refl };

    return depth > 0 ? compute_ray(&rr, false, depth - 1) : (Spectrum){ 0, 0, 0 };
  }
  Spectrum color = lm_sample_sphere(&s->lm, normal);
  fx att = clamp01(fdiv(dot(normal, to_light), fmul(norm(to_light), norm_sq(to_light))));

  return spec_add(color, spec_gray(att));
}

static Spectrum compute_ray(const Ray *ray, bool from_cam, int depth)
{
  Spectrum color = { 0, 0, 0 };
  fx min_t = 2000 * ONE;
  int hit_plane = -1, hit_sphere = -1;

  for (int i = 0; i < 5; i++)
  {
    fx t = plane_hit(&planes[i], ray, from_cam);
    if (t > 1 && t < min_t)
    {
      min_t = t;
      hit_plane = i;
      hit_sphere = -1;
    }
  }
  for (int i = 0; i < 2; i++)
  {
    fx t = sphere_hit(&spheres[i], ray, from_cam);
    if (t > 1 && t < min_t)
    {
      min_t = t;
      hit_sphere = i;
      hit_plane = -1;
    }
  }
  if (min_t < 2000 * ONE)
  {
    Vec3 hit = ray_at(ray, min_t);
    color = hit_plane >= 0 ? shade_plane(&planes[hit_plane], hit)
                           : shade_sphere(&spheres[hit_sphere], hit, ray, depth);
  }
  return spec_scale(color, exposure);
}

/* ---- rendering ---- */

static int fx_div_h(int v) { return v * ONE / FX_H; }

/* One pixel (block) of the picture: linear light -> 8 bit sRGB. */
static void trace_pixel(int x, int y, int grain, uint8_t rgb[3])
{
  Ray ray;

  ray.origin = cam;
  ray.dir = V(fx_div_h(2 * x + grain - FX_W) / 2, fx_div_h(FX_H - 2 * y - grain) / 2, ONE);
  Spectrum s = compute_ray(&ray, true, 3);
  rgb[0] = gamma8(s.r);
  rgb[1] = gamma8(s.g);
  rgb[2] = gamma8(s.b);
}

static fb_data pack5(int r, int g, int b)
{
  return fx_rgb(r & 0xf8, g & 0xf8, b & 0xf8);
}

enum state { PREVIEW_DUE, IDLE, RENDERING };

static enum state state = PREVIEW_DUE;
static bool lit;               /* radiosity matches the light position */
static bool move_light;
static int grain_final = 1;
static fb_data *saved;         /* copy of the last finished render, if memory allows */
static bool have_img;
static int pass_grain, pass_row;
static int err_r, err_g, err_b;  /* dither error carried along a row */

static void render_rows(int grain, int y, int rows, bool dither)
{
  for (int by = y; by < y + rows && by < FX_H; by += grain)
  {
    err_r = err_g = err_b = 0;
    for (int x = 0; x < FX_W; x += grain)
    {
      uint8_t c[3];
      trace_pixel(x, by, grain, c);
      fb_data p;
      if (dither)
      {
        int r = c[0] + err_r, g = c[1] + err_g, b = c[2] + err_b;
        r = r > 255 ? 255 : r;
        g = g > 255 ? 255 : g;
        b = b > 255 ? 255 : b;
        err_r = r & 7;
        err_g = g & 7;
        err_b = b & 7;
        p = pack5(r, g, b);
      }
      else
        p = pack5(c[0], c[1], c[2]);
      for (int j = 0; j < grain && by + j < FX_H; j++)
        for (int i = 0; i < grain && x + i < FX_W; i++)
          fx_fb[(by + j) * FX_W + x + i] = p;
    }
  }
}

static void draw_hud(void)
{
  fx_text_shadow(2, FX_H - 9, move_light ? "LIGHT" : "CAMERA", fx_rgb(255, 255, 255));
  fx_text_shadow(2, FX_H - 18, "Hold SELECT+UP: menu", fx_rgb(255, 255, 255));
}

static void ensure_lit(void)
{
  if (!lit)
  {
    fx_boost(true);
    fx_clear(fx_rgb(0, 0, 0));
    fx_text_center(FX_H / 2 - 3, "Computing light...", fx_rgb(255, 255, 255));
    rb->lcd_update();
    compute_lighting();
    lit = true;
  }
}

static void start_render(void)
{
  ensure_lit();
  fx_boost(true);
  register_camera();
  pass_grain = 16;
  pass_row = 0;
  state = RENDERING;
}

static void rt_settings(void)
{
  MENUITEM_STRINGLIST(menu, "Settings", NULL, "Move", "Detail", "Reset scene");
  static const struct opt_items moves[2] = { { "Camera", -1 }, { "Light", -1 } };
  static const struct opt_items details[3] = { { "Full (1x1)", -1 }, { "Medium (2x2)", -1 }, { "Draft (4x4)", -1 } };
  int sel = 0, v;

  while (true)
  {
    switch (rb->do_menu(&menu, &sel, NULL, false))
    {
      case 0:
        v = move_light;
        rb->set_option("Move", &v, RB_INT, moves, 2, NULL);
        move_light = v;
        break;
      case 1:
        v = grain_final == 1 ? 0 : grain_final == 2 ? 1 : 2;
        rb->set_option("Detail", &v, RB_INT, details, 3, NULL);
        grain_final = 1 << v;
        have_img = false;
        break;
      case 2:
        scene_reset();
        lit = false;
        have_img = false;
        state = PREVIEW_DUE;
        rb->splash(HZ / 2, "Scene reset");
        break;
      default:
        return;
    }
  }
}

static bool demo_init(void)
{
  size_t size;
  void *buf = rb->plugin_get_buffer(&size);

  if (buf && size >= sizeof(fb_data) * FX_W * FX_H)
    saved = buf;
  scene_reset();
  return true;
}

static void demo_start(void)
{
  rb->lcd_set_viewport(NULL);
  ensure_lit();
  if (have_img)
  {
    memcpy(fx_fb, saved, sizeof(fb_data) * FX_W * FX_H); /* the menu overdrew it */
    state = IDLE;
  }
  else
    state = PREVIEW_DUE;
}

#define MOVE_STEP FXC(0.1f)

static void move(unsigned dirs, bool depth)
{
  Vec3 *v = move_light ? &light : &cam;
  fx lim = FXC(.8f);

  if (depth)
  {
    if (dirs & BUTTON_UP) v->z += MOVE_STEP;
    if (dirs & BUTTON_DOWN) v->z -= MOVE_STEP;
  }
  else
  {
    if (dirs & BUTTON_LEFT) v->x -= MOVE_STEP;
    if (dirs & BUTTON_RIGHT) v->x += MOVE_STEP;
    if (dirs & BUTTON_UP) v->y += MOVE_STEP;
    if (dirs & BUTTON_DOWN) v->y -= MOVE_STEP;
  }
  v->x = v->x < -lim ? -lim : v->x > lim ? lim : v->x;
  v->y = v->y < -lim ? -lim : v->y > lim ? lim : v->y;
  v->z = v->z < FXC(-.8f) ? FXC(-.8f) : v->z > FXC(3.4f) ? FXC(3.4f) : v->z;
  if (move_light)
    lit = false;
  have_img = false;
  state = PREVIEW_DUE;
}

static bool demo_frame(unsigned keys, unsigned edge, int dt_ms)
{
  (void)dt_ms;
  bool sel = keys & BUTTON_SELECT;

  if (sel)
  {
    if (edge & BUTTON_LEFT)
    {
      move_light = !move_light;
      state = PREVIEW_DUE;
    }
    if (edge & (BUTTON_UP | BUTTON_DOWN))
      move(edge, true);
  }
  else if (edge & FX_DIRS)
    move(edge, false);
  else if (state == IDLE)
    fx_boost(false);

  if ((edge & BUTTON_SELECT) && state != RENDERING)
    start_render();
  else if (edge & BUTTON_SELECT)
    state = IDLE;

  fx_y0 = fx_y1 = 0;
  if (state == PREVIEW_DUE)
  {
    fx_boost(true);
    register_camera();
    for (int g = 16; g >= 4; g /= 2)
      render_rows(g, 0, FX_H, false);
    draw_hud();
    state = IDLE;
    fx_y1 = FX_H;
  }
  else if (state == RENDERING)
  {
    unsigned long t0 = *rb->current_tick;
    int lo = pass_row, hi = pass_row;
    bool wrapped = false;

    do
    {
      int g = pass_grain;
      render_rows(g, pass_row, g, g == 1);
      pass_row += g;
      hi = pass_row;
      if (pass_row >= FX_H)
      {
        pass_row = 0;
        pass_grain /= 2;
        wrapped = true;
        if (pass_grain < grain_final)
        {
          state = IDLE;
          if (saved)
          {
            memcpy(saved, fx_fb, sizeof(fb_data) * FX_W * FX_H);
            have_img = true;
          }
        }
        break;
      }
    } while (*rb->current_tick - t0 < 4);

    fx_y0 = wrapped ? 0 : lo;
    fx_y1 = wrapped || hi > FX_H ? FX_H : hi;
  }
  return false;
}
