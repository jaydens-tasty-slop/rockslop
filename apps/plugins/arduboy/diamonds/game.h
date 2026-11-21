#ifndef GAME_H
#define GAME_H

// #define DEBUG 1

#include <Arduboy2.h>
#include <ArduboyPlaytune.h>

typedef uint8_t u8;


#define WALL_FREQ   220
#define PADDLE_FREQ 440
#define WALL_DUR    16
#define PADDLE_DUR    16

extern Arduboy2 arduboy; 
extern ArduboyPlaytune sound;

extern Sprites sprites;

void tick();

#endif // GAME_HPP