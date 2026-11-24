#pragma once

#include <Arduboy2.h>
#include <ArduboyTones.h>
#include "MenuState.h"
#include "GameState.h"
#include "Font3x5.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_HEIGHT_UNITS 640
#define HALF_SCREEN_HEIGHT_UNITS 320
#define HALF_SCREEN_WIDTH 64
#define HALF_SCREEN_HEIGHT 32
#define HALF_SCREEN_WIDTH_UNITS 640
#define WORLD_WIDTH_PIXELS 1024
#define WORLD_WIDTH_UNITS 10240
#define HALF_WORLD_WIDTH_PIXELS 512
#define HALF_WORLD_WIDTH_UNITS 5120

#define STATE_MENU		0
#define STATE_GAME		1
#define STATE_GAME_OVER 2

extern Arduboy2 arduboy;
extern GameState* pGameState;
extern ArduboyTones sound;
#define ARDBITMAP_SBUF arduboy.getBuffer()
#include "ArdBitmap.h"

extern Sprites sprites;
extern Font3x5 smallFont;
extern void setState(uint8_t state);
extern bool isFlagSet(uint8_t flags, uint8_t flagToCheck);
extern void setFlag(uint8_t* pFlags, uint8_t flagToSet);
extern void unsetFlag(uint8_t* pFlags, byte flagToUnset);
extern void setFlag(uint8_t* pFlags, uint8_t flagToSet, bool setValue);
extern bool renderSprite(const uint8_t* spriteData, Vector2Int screenPos, uint8_t mirror = 0, uint8_t frame = 0);
bool renderSpriteIfVisible(const uint8_t* spriteData, uint8_t* pFlags, Vector2Int screenPos, uint8_t mirror = 0, uint8_t frame = 0);
extern void explodeObject(uint8_t* pFlags, Vector2Int worldPos, uint8_t type);
extern void fireAtPlayer(PlayerShip* pPlayerShip, Vector2Int startPos);
extern void verticalWrap(Vector2Int* pos);
extern unsigned long getHighScore();
extern void setHighScore(unsigned long score);
