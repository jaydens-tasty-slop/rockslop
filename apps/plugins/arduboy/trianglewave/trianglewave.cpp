#include <stdlib.h>
#include <math.h>
// Triangle Wave: Arcade Game for the Arduboy
// 2020 (C) SergioFLS
// This game is licensed under the MIT License.
// For more info read the LICENSE file.

#include <Arduboy2.h>
#include "sprites.h"
#include "variables.h"
Arduboy2 arduboy;

void createBullet();
void handleCollision();
void sawbladeGen();
void moveBulSaw();
void drawBackground();
void drawPlayerObjs();
void drawSawblades();
void titlescreen();
void ingame();
void gameover();

void setup() {
  arduboy.setFrameRate(60);
  arduboy.initRandomSeed();
  arduboy.begin();
  arduboy.clear();
}

// update functions
void createBullet() {
  for (int i=0; i < 3; i++) {
    if (! obj_bullet[i].active) {
      obj_bullet[i].x = obj_player.x + 9;
      obj_bullet[i].y = obj_player.y;
      if (arduboy.pressed(UP_BUTTON)) {
        obj_bullet[i].yspd = -1;
      } else {
        obj_bullet[i].y += 9;
        obj_bullet[i].yspd = 1;
      }
      obj_bullet[i].active = 1;
      i = 4;
    }
  }
}

void handleCollision() {
  // collision detection between bullets and sawblades
  for (int i=0; i < 3; i++) {
    if (obj_bullet[i].active) {
      for (int j=0; j < 10; j++) {
        if (obj_sawblade[j].active) {
          if (obj_bullet[i].x == obj_sawblade[j].x + 8 && obj_sawblade[j].y < obj_bullet[i].y + 8 && obj_sawblade[j].y + 8 > obj_bullet[i].y) {
            score++;
            obj_bullet[i].active = 0;
            obj_sawblade[j].active = 0;
          }
        }
      }
    }
  }

  // collision detection between sawblades and the player
  for (int i=0; i < 10; i++) {
      if (obj_sawblade[i].active) {
        if (obj_player.x + 8 == obj_sawblade[i].x && obj_sawblade[i].y < obj_player.y + 8 && obj_sawblade[i].y + 8 > obj_player.y) {
          game_state=GAME_OVER;
        }
      }
  }

  // if the ship touches floor/ceil, game over
  if (obj_player.y < 1 || obj_player.y > 53) {
    game_state=GAME_OVER;
  }
}

void sawbladeGen() {
  if (obj_player.started ) { 
    if (sawbladetimer == 29) {
      for (int i=0; i < 10; i++) {
        if (!obj_sawblade[i].active) {
          obj_sawblade[i].x=127;
          obj_sawblade[i].y=random(5,50);
          obj_sawblade[i].active=1;
          i = 11;
        }
      }
    }

    sawbladetimer++;
    sawbladetimer%=30;
  }
}

void moveBulSaw() {
  // move bullets
  for (int i=0; i < 3; i++) {
    if (obj_bullet[i].active) {
      obj_bullet[i].x++;
      obj_bullet[i].y+=obj_bullet[i].yspd;
      if (obj_bullet[i].x > 129) {
        obj_bullet[i].active = 0;
      }

      if (obj_bullet[i].y < 2 || obj_bullet[i].y > 62) {
        obj_bullet[i].yspd*=-1;
      }
    }
  }

  // move sawblades
  for (int i=0; i < 10; i++) {
    if (obj_sawblade[i].active) {
      obj_sawblade[i].x--;
      if (obj_sawblade[i].x < -8) {
        obj_sawblade[i].active = 0;
      }
    }
  }
}

// draw functions

void drawBackground() {
  for (int backgroundx = 0; backgroundx < 136; backgroundx+=8) {
    Sprites::drawOverwrite(backgroundx + (backgroundshift * -1), 0, spr_groundup, 0);
    Sprites::drawOverwrite(backgroundx + (backgroundshift * -1), 56, spr_grounddown, 0);
  }
  backgroundshift++;
  backgroundshift%=8;
}

void drawPlayerObjs() {
  if (obj_player.started) {
    // draw player
    if (arduboy.pressed(UP_BUTTON)) {
      Sprites::drawOverwrite(obj_player.x, obj_player.y, spr_shipup, 0);
    } else {
      Sprites::drawOverwrite(obj_player.x, obj_player.y, spr_shipdown, 0);
    }
  } else {
    Sprites::drawOverwrite(obj_player.x, obj_player.y, spr_shipmiddle, 0);
    arduboy.setCursor(20,10);
    arduboy.print("Hold Up to Fly!");
  }

  // draw bullets
  for (int i=0; i < 3; i++) {
    if (obj_bullet[i].active) {
      arduboy.fillRect(obj_bullet[i].x, obj_bullet[i].y, 1, 1, WHITE);
    }
  }
}

void drawSawblades() {
  for (int i=0; i < 10; i++) {
    if (obj_sawblade[i].active) {
      Sprites::drawOverwrite(obj_sawblade[i].x, obj_sawblade[i].y, spr_sawblade, 0);
    }
  }
}

void titlescreen() {
  arduboy.clear();
  arduboy.setCursor(0,0);
  arduboy.print("Triangle Wave\nPress A to start");
  if (arduboy.pressed(A_BUTTON)) {
    reset(); // upstream never initialises the player before the first game
    game_state = IN_GAME;
  }
  arduboy.display();
}

void ingame() {
  if (!arduboy.nextFrame()) {
    return;
  }
  arduboy.pollButtons();

  // make player float
  if (obj_player.started) {
    if (arduboy.pressed(UP_BUTTON)) {
      obj_player.y--;
    } else {
      obj_player.y++;
    }
  } else {
    if (arduboy.pressed(UP_BUTTON)) {
      obj_player.started = 1;
    }
  }

  // create bullet
  if (arduboy.justPressed(A_BUTTON)) {
    createBullet();
  }
  
  // handle generation of sawblades
  sawbladeGen();

  // move bullets
  moveBulSaw();

  handleCollision();

  //draw functions
  arduboy.clear();
  // draws moving floor and ceiling background
  drawBackground();
  // draw player, bullets and "hold up" message
  drawPlayerObjs();
  drawSawblades();

  arduboy.setCursor(0,0);
  arduboy.print(score);
  arduboy.display();
}

void gameover() {
  if (arduboy.pressed(B_BUTTON)) {
    reset();
    game_state=IN_GAME;
  }
  arduboy.clear();
  arduboy.setCursor(0,0);
  arduboy.print("Game Over!\nYour score is:\n");
  arduboy.print(score);
  arduboy.display();
}

void loop(){
  switch (game_state) {
    case TITLE_SCREEN:
      titlescreen();
      break;
    case IN_GAME:
      ingame();
      break;
    case GAME_OVER:
      gameover();
      break;
    default:
      game_state = TITLE_SCREEN;
      break;
  }
}
