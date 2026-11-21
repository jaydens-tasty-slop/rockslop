#pragma once
#include <stdint.h>
#include "types.h"

#define HOUSE_LINE_CO 11
#define CRANE_LINE_CO 1
/* Rockbox: upstream said 2 but get_ground() fills one line; the other was
 * uninitialised heap that drew huge lines (stutter, hangs). */
#define GROUND_LINE_CO 1

typedef struct {
    uint8_t count;
    // We have to free this later!
    Line* lines;
} Block;

Block get_house();
Block get_crane();
Block get_ground();
