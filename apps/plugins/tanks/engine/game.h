/* Game state and mission loop of Tanks! CE for Rockbox (MIT, see ../LICENSE).
 * The mission loop is the original's play_mission() split into a step
 * function; levels come from levels.cpp instead of an appvar. */
#ifndef TANKS_GAME_H
#define TANKS_GAME_H

#include "tank.h"

enum {
    GAME_RUNNING = 0, NEXT_LEVEL, LOSE_LIFE, LOSE
};

struct level {
    uint8_t num_tanks; //Number of tanks in the level
};

struct game {
    struct level level; //level currently being played
    uint8_t mission; //The mission number (index into the level table)
    tile_t tiles[LEVEL_SIZE_Y][LEVEL_SIZE_X]; //Currently active tilemap data
    uint8_t lives; //Number of remaining tanks, including the one in use
    uint8_t total_kills; //Number of enemy tanks destroyed.
    uint8_t kills[NUM_TANK_TYPES];
    uint8_t num_tanks;
    bool alive_tanks[MAX_NUM_TANKS];
    Tank *player;
    uint24_t tick;
};

extern struct game game;

struct input_state {
    uint8_t move;       /* UP | DOWN | LEFT | RIGHT */
    int8_t turn;        /* barrel: -1, 0, +1 */
    bool fire, mine;
};

int level_count(void);
void game_new(void);                 /* lives, kills */
void game_begin_level(int mission);  /* all tanks alive, then game_start_mission */
/* Create the tanks and map of the mission (also used to retry it). */
void game_start_mission(int mission);
int game_step(const struct input_state *in);

/* Point the player's barrel at the nearest enemy tank. */
void game_aim_assist(void);

#endif //TANKS_GAME_H
