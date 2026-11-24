/* RUMBLO fight logic, see game.h. LGPL-3.0. */
#include "game.h"
#include "assets.h"

struct stats {
    int move_speed, anim_speed, fall_speed, jump_speed, jump_height;
    int weapon_speed, weapon_knock, weapon_strength;
};

static const struct stats stats[2] = {
    { 4, 3, 4, 8, 10, 8, 4, 6 },
    { 8, 2, 4, 8, 15, 6, 4, 4 },
};

struct platform {
    int x0, x1, y;
};

static const struct platform platforms[2][4] = {
    { { 24, 264, 132 }, { 72, 216, 64 }, { -8, 40, 40 }, { 248, 296, 40 } },
    { { -8, 56, 96 }, { 232, 296, 96 }, { 88, 200, 80 }, { 104, 184, 16 } },
};
static const int death_y[2] = { 240, 161 }; /* map 0: y >= 240, map 1: y > 160 */
static const int spawn[2][2][2] = { { { 80, 132 }, { 220, 132 } },
                                    { { 32, 96 }, { 256, 96 } } };
static const int ai_delay_for[3] = { 15, 10, 5 };

#define MAP1_ANIM_SPEED 25

static void respawn(struct game *g, int i)
{
    struct fighter *f = &g->f[i];
    f->x = spawn[g->map][i][0];
    f->y = spawn[g->map][i][1];
    f->flipped = i == 1;
    f->anim = f->anim_count = 0;
    f->shield = false;
    f->grounded = true;
    f->jumping = 0;
    f->knock = 0;
}

void game_start(struct game *g, int map, int char0, int char1, int difficulty)
{
    *g = (struct game){ .map = map, .winner = -1 };
    g->ai_delay = ai_delay_for[difficulty];
    for (int i = 0; i < 2; i++) {
        g->f[i].character = i ? char1 : char0;
        g->f[i].lives = START_LIVES;
        respawn(g, i);
    }
}

int game_screen(const struct game *g)
{
    if (g->map == 0)
        return SCREEN_ARENA0;
    return SCREEN_ARENA1 + g->map_frame;
}

static bool is_grounded(int map, int x, int y)
{
    for (int i = 0; i < 4; i++) {
        if (y == platforms[map][i].y && x >= platforms[map][i].x0 &&
            x <= platforms[map][i].x1)
            return true;
    }
    return false;
}

/* The computer opponent: the decision tree of the original, per arena. */
static void ai(const struct game *g, struct input *in, bool *down)
{
    const struct fighter *h = &g->f[0], *a = &g->f[1];
    int px = h->x, py = h->y, ax = a->x, ay = a->y;
    bool left = false, right = false, jump = false, attack = false;

    if (g->map == 0) {
        if (ax <= 48 && ay == 132) {
            right = jump = true;
        } else if (ax >= 244 && ay == 132) {
            left = jump = true;
        } else if (py == 132 && ay <= 64) {
            if (ax < 160 && ax > 60)
                left = true;
            else if (ax >= 160 && ax < 228)
                right = true;
        } else if (py <= 64 && ay == 132) {
            jump = attack = true;
            if (px < ax) left = true; else right = true;
        } else if (py == 40 && ay < 64) {
            jump = attack = true;
            if (px < ax) left = true; else right = true;
        } else if (py < 64 && ay == 64) {
            if (px < 160 && ax > 84) {
                left = true;
            } else if (px < 160) {
                left = jump = true;
            } else if (px >= 160 && ax < 204) {
                right = true;
            } else {
                right = jump = true;
            }
        } else if (ay == 40 && py < 40) {
            if (ax < 160) right = true; else left = true;
        } else {
            if (px + 20 < ax) left = true;
            else if (px - 20 > ax) right = true;
            if (py > ay) jump = true;
            if (py == ay) attack = true;
        }
    } else {
        if (ax < 92 && ay == 80) {
            right = jump = true;
        } else if (ax > 192 && ay == 80) {
            left = jump = true;
        } else if (ay == 96 && py <= 80) {
            if (ax < 160 && ax < 48) {
                right = true;
            } else if (ax < 160) {
                right = jump = true;
            } else if (ax > 240) {
                left = true;
            } else {
                left = jump = true;
            }
            attack = true;
        } else if (py == 96 && ay == 80) {
            if (px < 160 && ax > 92) {
                left = true;
            } else if (px < 160) {
                left = jump = true;
            } else if (ax < 192) {
                right = true;
            } else {
                right = jump = true;
            }
            attack = true;
        } else if (py <= 16 && ay == 80) {
            if (px < ax) {
                right = jump = attack = true;
            } else if (ax < px) {
                left = jump = attack = true;
            }
        } else if (py > 16 && ay == 16) {
            if (ax < 160 && ax > 96)
                left = true;
            else if (ax < 196)
                right = true;
        } else if (py == 96 && ay == 96) {
            if (px - ax > 50 || px - ax < -50) {
                if (ax < 160 && ax < 48) {
                    right = true;
                } else if (ax < 160) {
                    right = jump = true;
                } else if (ax > 240) {
                    left = true;
                } else {
                    left = jump = true;
                }
                attack = true;
            }
        } else {
            if (px + 20 < ax) left = true;
            else if (px - 20 > ax) right = true;
            if (py > ay) jump = true;
        }
    }

    if (py == ay)
        attack = true;

    *down = false;
    if (a->knock < -4 || a->knock > 4)
        *down = true;
    else if (a->knock < -2 || a->knock > 2)
        left = right = false;

    in->left = left;
    in->right = right;
    in->jump = jump;
    in->attack = attack;
}

static void fire(struct fighter *f)
{
    for (int i = 0; i < MAX_WEAPONS; i++) {
        if (!f->w[i].on) {
            f->w[i].on = true;
            f->w[i].flipped = f->flipped;
            f->w[i].x = f->flipped ? f->x : f->x + 26;
            f->w[i].y = f->y + 17;
            return;
        }
    }
}

static void steer(struct fighter *f, const struct stats *s, const struct input *in,
                  bool down)
{
    f->shield = false;
    if (down) {
        f->jumping = 0;
        f->shield = f->grounded;
    }
    for (int dir = 0; dir < 2; dir++) {
        bool go = dir ? in->right : in->left;
        if (!go || f->shield)
            continue;
        f->x += dir ? s->move_speed : -s->move_speed;
        if (f->flipped == (dir == 1)) { /* turning round */
            f->flipped = dir == 0;
            f->anim = f->anim_count = 0;
        }
        if (++f->anim_count == s->anim_speed) {
            f->anim = f->anim != 3 ? f->anim + 1 : 0;
            f->anim_count = 0;
        }
    }
    if (in->jump && f->grounded)
        f->jumping = s->jump_height;
}

static void hits(struct fighter *from, struct fighter *to)
{
    const struct stats *s = &stats[from->character];
    for (int i = 0; i < MAX_WEAPONS; i++) {
        struct weapon *w = &from->w[i];
        if (!w->on || w->y + 8 < to->y || w->y > to->y + FIGHTER_H)
            continue;
        if (w->x + 8 >= to->x && w->x <= to->x + FIGHTER_W) {
            if (!to->shield)
                to->knock += w->flipped ? -s->weapon_knock : s->weapon_knock;
            w->on = false;
        }
    }
}

void game_step(struct game *g, const struct input *in1)
{
    struct input in2 = { 0 };
    bool down2;
    bool down1 = in1->down;

    if (g->winner >= 0)
        return;

    if (g->map == 1) {
        if (++g->map_count == MAP1_ANIM_SPEED) {
            g->map_count = 0;
            g->map_frame = g->map_frame < 2 ? g->map_frame + 1 : 0;
        }
    }

    ai(g, &in2, &down2);
    steer(&g->f[0], &stats[g->f[0].character], in1, down1);
    if (in1->attack)
        fire(&g->f[0]);
    steer(&g->f[1], &stats[g->f[1].character], &in2, down2);
    if (in2.attack && --g->ai_count <= 0) {
        g->ai_count = g->ai_delay;
        fire(&g->f[1]);
    }

    for (int i = 0; i < 2; i++) {
        struct fighter *f = &g->f[i];
        f->grounded = is_grounded(g->map, f->x, f->y);
        if (f->y >= death_y[g->map]) {
            if (--f->lives <= 0) {
                g->winner = 1 - i;
                return;
            }
            respawn(g, i);
        }
    }

    for (int i = 0; i < 2; i++) {
        struct fighter *f = &g->f[i], *o = &g->f[1 - i];
        const struct stats *s = &stats[f->character];
        if (f->jumping > 0) {
            f->jumping--;
            f->y -= s->jump_speed;
        } else if (!f->grounded) {
            f->y += s->fall_speed;
        }
        /* knockback is applied with the strength of the opponent's weapon */
        if (f->knock < 0) {
            f->knock++;
            f->x -= stats[o->character].weapon_strength;
        } else if (f->knock > 0) {
            f->knock--;
            f->x += stats[o->character].weapon_strength;
        }
    }

    for (int i = 0; i < 2; i++) {
        struct fighter *f = &g->f[i];
        const struct stats *s = &stats[f->character];
        for (int k = 0; k < MAX_WEAPONS; k++) {
            struct weapon *w = &f->w[k];
            if (!w->on)
                continue;
            w->x += w->flipped ? -s->weapon_speed : s->weapon_speed;
            if (w->x <= -8 || w->x >= ARENA_W)
                w->on = false;
        }
        if (!f->grounded) {
            f->anim = f->anim_count = 0;
        }
    }

    hits(&g->f[0], &g->f[1]);
    hits(&g->f[1], &g->f[0]);
}
