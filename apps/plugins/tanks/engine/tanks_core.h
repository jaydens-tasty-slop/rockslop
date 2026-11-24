/* C interface of the Tanks! engine for the Rockbox front end (MIT). */
#ifndef TANKS_CORE_H
#define TANKS_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#define PLAY_W 128 /* playfield pixels (16 x 17 tiles of 8) */
#define PLAY_H 136

enum { TS_RUNNING, TS_NEXT_LEVEL, TS_LOST_LIFE, TS_GAME_OVER };

/* The front end provides these (all coordinates in playfield pixels). */
void tv_fill(int x, int y, int w, int h, unsigned rgb);
void tv_line(int x1, int y1, int x2, int y2, unsigned rgb);
unsigned tanks_random(void);

void tanks_seed(unsigned seed);
int tanks_level_count(void);
const char *tanks_level_name(int level);
void tanks_new_game(void);
void tanks_begin_level(int level);
void tanks_retry_level(void);
/* move: bit 0 up, 1 down, 2 left, 3 right. turn -1/0/1. */
int tanks_step(int move, int turn, int fire, int mine, int aim);
void tanks_draw(int tick);
int tanks_lives(void);
int tanks_kills(void);
int tanks_enemies(void);
int tanks_level(void);

#ifdef __cplusplus
}
#endif

#endif
