/* Level table of the Rockbox port (MIT, see ../LICENSE). The original loads
 * level packs from a calculator appvar that is not part of its repository;
 * these missions are new, laid out for the 16x17 playfield. */
#ifndef TANKS_LEVELS_H
#define TANKS_LEVELS_H

#include "level.h"

struct level_def {
    const char *name;
    /* '.' floor, '#' wall, 'o' destructible block, 'h' hole, 'P' player,
     * '1'..'9' enemy tank types (physicsbody tank.h: brown, grey, turquoise,
     * yellow, red, green, purple, white, black) */
    const char *map[LEVEL_SIZE_Y - 2];
};

#define LEVEL_COUNT 10
extern const struct level_def levels[LEVEL_COUNT];

tile_t tile_from_char(char c);
int tank_from_char(char c); /* tank type, or -1 */

#endif
