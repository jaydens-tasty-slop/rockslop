/* RUMBLO rendering, see view.h. LGPL-3.0. Pixels are sampled from the 320x240
 * artwork with nearest neighbour at 2/5 scale. */
#include "view.h"
#include "assets.h"

static pixel_t pal[RUMBLO_COLORS];

void view_init(pixel_t (*pack)(int r, int g, int b))
{
    for (int i = 0; i < RUMBLO_COLORS; i++)
        pal[i] = pack(rumblo_palette[3 * i], rumblo_palette[3 * i + 1],
                      rumblo_palette[3 * i + 2]);
}

/* arena coordinate of the centre of a view pixel */
static inline int world(int v)
{
    return (v * 5 + 2) >> 1;
}

void view_screen(pixel_t *dst, int screen)
{
    const unsigned char *map = rumblo_screens[screen];
    for (int y = 0; y < VIEW_H; y++) {
        int wy = world(y);
        for (int x = 0; x < VIEW_W; x++) {
            int wx = world(x);
            const unsigned char *t = rumblo_tiles[map[(wy >> 4) * 20 + (wx >> 4)]];
            *dst++ = pal[t[(wy & 15) * 16 + (wx & 15)]];
        }
    }
}

static void sprite(pixel_t *dst, const unsigned char *px, int w, int h, int x,
                   int y, bool flip)
{
    int x0 = (x * 2 + 4) / 5, x1 = ((x + w) * 2 + 4) / 5;
    int y0 = (y * 2 + 4) / 5, y1 = ((y + h) * 2 + 4) / 5;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > VIEW_W) x1 = VIEW_W;
    if (y1 > VIEW_H) y1 = VIEW_H;
    for (int dy = y0; dy < y1; dy++) {
        int sy = world(dy) - y;
        if (sy < 0 || sy >= h)
            continue;
        for (int dx = x0; dx < x1; dx++) {
            int sx = world(dx) - x;
            if (sx < 0 || sx >= w)
                continue;
            unsigned char c = px[sy * w + (flip ? w - 1 - sx : sx)];
            if (c)
                dst[dy * VIEW_W + dx] = pal[c];
        }
    }
}

static void walk_frame(const struct fighter *f, int *frame)
{
    static const int cycle[4] = { 0, 1, 2, 1 };
    *frame = f->grounded ? cycle[f->anim] : 3;
}

void view_fighters(pixel_t *dst, const struct game *g)
{
    for (int i = 0; i < 2; i++) {
        const struct fighter *f = &g->f[i];
        int frame;
        walk_frame(f, &frame);
        sprite(dst, rumblo_char[f->character][frame], FIGHTER_W, FIGHTER_H, f->x,
               f->y, f->flipped);
        if (f->shield)
            sprite(dst, rumblo_shield, 56, 56, f->x - 12, f->y - 4, false);
        for (int k = 0; k < MAX_WEAPONS; k++) {
            if (f->w[k].on)
                sprite(dst, rumblo_weapon[f->character], 8, 8, f->w[k].x,
                       f->w[k].y, f->w[k].flipped);
        }
    }
}
