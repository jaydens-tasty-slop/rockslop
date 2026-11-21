/* main.h */

#pragma once
#include "Pokitto.h"
#include <stdio.h>
#include <stdint.h>

#include "gfx/gfx.h"
#include "structs.h"
#include "sdsave.h"

#define PALETTE_SIZE 16

void startscreen(uint8_t *state);
void history(uint8_t *state);
void game(uint8_t *state);
void gameover(uint8_t *state);
void ending(uint8_t *state);

