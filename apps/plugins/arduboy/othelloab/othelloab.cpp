#include <stdlib.h>
#include <math.h>
#include "data.h"
#include "logic.h"
#include "visual.h"
#include <Arduboy2.h>

static Board B;
static Operation P;
static uint8_t T = 0;
static Arduboy2 arduboy;

static void intro(void) {
  for (char i = -8; i < 28; i = i + 2) {
    arduboy.clear();
    arduboy.setCursor(22, i);
    arduboy.print("Pocket Othello");
    delay(20);
    arduboy.display();
  }
  delay(1000);
}

static uint8_t input(void) {
  if (!P.px && arduboy.pressed(RIGHT_BUTTON)) {
    P.px = true;
    return RIGHT_BUTTON;
  } else if (!P.px && arduboy.pressed(LEFT_BUTTON)) {
    P.px = true;
    return LEFT_BUTTON;
  } else if (!P.py && arduboy.pressed(UP_BUTTON)) {
    P.py = true;
    return UP_BUTTON;
  } else if (!P.py && arduboy.pressed(DOWN_BUTTON)) {
    P.py = true;
    return DOWN_BUTTON;
  } else if (P.px && arduboy.notPressed(RIGHT_BUTTON + LEFT_BUTTON)) {
    P.px = false;
  } else if (P.py && arduboy.notPressed(UP_BUTTON + DOWN_BUTTON)) {
    P.py = false;
  }
  if (!P.pa && arduboy.pressed(A_BUTTON)) {
    P.pa = true;
    return A_BUTTON;
  } else if (!P.pb && arduboy.pressed(B_BUTTON)) {
    P.pb = true;
    return B_BUTTON;
  } else if (P.pa && arduboy.notPressed(A_BUTTON)) {
    P.pa = false;
  } else if (P.pb && arduboy.notPressed(B_BUTTON)) {
    P.pb = false;
  }
  return 0;
}

static void inputMenu(void) {
  switch (input()) {
    case UP_BUTTON:
    case DOWN_BUTTON:
      P.chosen = P.chosen == 0 ? 1 : 0;
      break;
    case A_BUTTON:
      B.clear();
      B.init();
      P.reset();
      P.state = GS_MAIN;
      break;
  }
}

static void drawMenu(void) {
  arduboy.setCursor(0, 0);
  arduboy.print("Play as:");
  arduboy.setCursor(49, 24);
  arduboy.print("BLACK");
  arduboy.setCursor(49, 32);
  arduboy.print("WHITE");
  arduboy.setCursor(42, P.chosen == 0 ? 32 : 24);
  arduboy.print(">");
}

static void inputPaused(void) {
  switch (input()) {
    case RIGHT_BUTTON:
    case LEFT_BUTTON:
    case UP_BUTTON:
    case DOWN_BUTTON:
    case A_BUTTON:
      P.state = GS_MAIN;
      break;
    case B_BUTTON:
      P.state = GS_MENU;
      break;
  }
}

static void drawPaused(void) {
  arduboy.setCursor(19, 28);
  arduboy.print("PRESS B TO EXIT");
}

static bool inputMain(void) {
  switch (input()) {
    case RIGHT_BUTTON:
      if (P.x == 7) P.x = 0;
      else ++P.x;
      break;
    case LEFT_BUTTON:
      if (P.x == 0) P.x = 7;
      else --P.x;
      break;
    case UP_BUTTON:
      if (P.y == 0) P.y = 7;
      else --P.y;
      break;
    case DOWN_BUTTON:
      if (P.y == 7) P.y = 0;
      else ++P.y;
      break;
    case A_BUTTON:
      if (place(&B, P.currentSide(), P.x, P.y, true) != 0)
        return true;
      break;
    case B_BUTTON:
      P.state = GS_PAUSED;
      break;
  }
  return false;
}

static void drawMain(void) {
  uint8_t xoff = 7;
  uint8_t yoff = 7;
  arduboy.drawBitmap(0, 0, SPRITE_BOARD, 64, 64, WHITE);
  for (uint8_t i = 0; i < 8; ++i) {
    for (uint8_t j = 0; j < 8; ++j) {
      GridStates s = B.get(i, j);
      if (s == GS_WHITE)
        arduboy.drawBitmap(xoff + 7 * i, yoff + 7 * j, SPRITE_WHITE, 8, 8, WHITE);
      else if (s == GS_BLACK)
        arduboy.drawBitmap(xoff + 7 * i, yoff + 7 * j, SPRITE_BLACK, 8, 8, WHITE);
    }
  }
  if (T >= 15 && P.state == GS_MAIN && P.chosen == P.playing)
    arduboy.drawBitmap(xoff + 7 * P.x, yoff + 7 * P.y, SPRITE_CURSOR, 8, 8, WHITE);
  arduboy.drawBitmap(64 + 23, 8, SPRITE_BLACK, 8, 8, WHITE);
  arduboy.setCursor(64 + 32, 8);
  arduboy.print(P.c1);
  arduboy.drawBitmap(64 + 23, 16, SPRITE_WHITE, 8, 8, WHITE);
  arduboy.setCursor(64 + 32, 16);
  arduboy.print(P.c0);
  arduboy.setCursor(64 + 6, 32);
  arduboy.print("WON");
  arduboy.setCursor(64 + 6, 40);
  arduboy.print(P.won);
  arduboy.setCursor(64 + 6, 48);
  arduboy.print("SUM");
  arduboy.setCursor(64 + 6, 56);
  arduboy.print(P.total);
}

static void inputOver(void) {
  switch (input()) {
    case A_BUTTON:
      B.clear();
      B.init();
      P.reset();
      P.state = GS_MAIN;
      break;
    case B_BUTTON:
      P.state = GS_MENU;
      break;
  }
}

static void drawOver(void) {
  drawMain();
  arduboy.fillRect(50, 26, 28, 12, BLACK);
  switch (P.state) {
    case GS_WIN:
      arduboy.setCursor(55, 28);
      arduboy.print("WIN");
      break;
    case GS_LOSE:
      arduboy.setCursor(52, 28);
      arduboy.print("LOSE");
      break;
    case GS_DRAW:
      arduboy.setCursor(52, 28);
      arduboy.print("DRAW");
      break;
  }
}

void setup() {
  arduboy.begin();
  arduboy.initRandomSeed();
  arduboy.setFrameRate(30);
  intro();
  B.init();
  shuffle();
  load(&P);
}

void loop() {
  if (!(arduboy.nextFrame()))
    return;

  if (++T >= 30) T = 0;

  arduboy.clear();

  switch (P.state) {
    case GS_MENU:
      inputMenu();
      drawMenu();
      break;
    case GS_MAIN:
      if (P.playing == P.chosen) {
        if (inputMain()) {
          if (turn(&B, &P)) {
            if (P.chosen == 1 && P.c1 > P.c0) { P.state = GS_WIN; ++P.total; ++P.won; save(&P); }
            else if (P.chosen == 1 && P.c1 < P.c0) { P.state = GS_LOSE; ++P.total; save(&P); }
            else if (P.chosen == 0 && P.c0 > P.c1) { P.state = GS_WIN; ++P.total; ++P.won; save(&P); }
            else if (P.chosen == 0 && P.c0 < P.c1) { P.state = GS_LOSE; ++P.total; save(&P); }
            else { P.state = GS_DRAW; ++P.total; save(&P); }
          }
        }
      } else {
        think(&B, &P);
        if (turn(&B, &P)) {
          if (P.chosen == 1 && P.c1 > P.c0) { P.state = GS_WIN; ++P.total; ++P.won; save(&P); }
          else if (P.chosen == 1 && P.c1 < P.c0) { P.state = GS_LOSE; ++P.total; save(&P); }
          else if (P.chosen == 0 && P.c0 > P.c1) { P.state = GS_WIN; ++P.total; ++P.won; save(&P); }
          else if (P.chosen == 0 && P.c0 < P.c1) { P.state = GS_LOSE; ++P.total; save(&P); }
          else { P.state = GS_DRAW; ++P.total; save(&P); }
        }
      }
      drawMain();
      break;
    case GS_PAUSED:
      inputPaused();
      drawPaused();
      break;
    case GS_WIN:
    case GS_LOSE:
    case GS_DRAW:
      inputOver();
      drawOver();
      break;
  }

  arduboy.display();
}
