/* RUMBLO fight logic (LGPL-3.0, see ../LICENSE).
 * Rewritten from the fight loop of RUMBLO by Techcrafter: same arenas, speeds,
 * jump, knockback and AI rules, for two fighters held in arrays. Coordinates
 * are those of the 320x240 calculator screen. */
#ifndef RUMBLO_GAME_H
#define RUMBLO_GAME_H

#include <stdbool.h>

#define ARENA_W 320
#define ARENA_H 240
#define FIGHTER_W 32
#define FIGHTER_H 48
#define MAX_WEAPONS 3
#define START_LIVES 3

struct weapon {
    bool on;
    bool flipped;
    int x, y;
};

struct fighter {
    int x, y;
    bool flipped;
    int anim, anim_count;      /* walk cycle 0..3 */
    int jumping;               /* frames of upward movement left */
    bool grounded, shield;
    int knock;                 /* pending horizontal knockback steps */
    int lives;
    int character;             /* 0 or 1 */
    struct weapon w[MAX_WEAPONS];
};

struct input {
    bool left, right, down;
    bool jump, attack;         /* edge triggered: set for one step */
};

struct game {
    struct fighter f[2];       /* 0: the player, 1: the computer */
    int map;                   /* 0 or 1 */
    int map_frame, map_count;  /* arena animation */
    int ai_delay, ai_count;
    int winner;                /* -1 playing, 0 player, 1 computer */
};

/* difficulty 0..2: easy, normal, hard */
void game_start(struct game *g, int map, int char0, int char1, int difficulty);
void game_step(struct game *g, const struct input *in);
/* Screen index (assets.h) of the arena in its current animation frame. */
int game_screen(const struct game *g);

#endif
