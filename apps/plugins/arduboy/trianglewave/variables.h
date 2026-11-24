// Triangle Wave: Arcade Game for the Arduboy
// 2020 (C) SergioFLS
// This game is licensed under the MIT License.
// For more info read the LICENSE file.

#ifndef VARIABLES_H
#define VARIABLES_H

#define TITLE_SCREEN 0
#define IN_GAME 1
#define GAME_OVER 2

uint8_t game_state = TITLE_SCREEN;

uint8_t backgroundshift = 0;
uint8_t sawbladetimer = 0;

unsigned score = 0;

struct ObjectPlayer {
  int8_t x = 10;
  int8_t y = 26;
  bool started = 0;
} obj_player;

struct ObjectBullet {
  uint8_t x;
  uint8_t y;
  uint8_t yspd;
  bool active = false;
} obj_bullet[3];

struct ObjectSawblade {
  int8_t x;
  uint8_t y;
  bool active = false;
} obj_sawblade[10];

void reset() {
  game_state = IN_GAME;
  sawbladetimer = 0;
  score = 0;
  obj_player.x = 10;
  obj_player.y = 26;
  obj_player.started = 0;
  for (int i=0; i < 3; i++) {
    obj_bullet[i].active = 0;
  }
  for (int i=0; i < 10; i++) {
    obj_sawblade[i].active = 0;
  }
}
#endif
