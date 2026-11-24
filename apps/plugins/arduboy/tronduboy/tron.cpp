#include <stdlib.h>
#include <math.h>
#include "Arduboy.h"
#include "../lib/ab_math.h"

#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

Arduboy arduboy;
unsigned int score = 0, highScore = 0;
char textBuf[15];

void startScreen() { 
  arduboy.clear();
  arduboy.setCursor(10, 20);
  arduboy.setTextSize(2);
  arduboy.print("TRONDUBOY");

  arduboy.setTextSize(1);
  arduboy.setCursor(6, 53);
  arduboy.print("PRESS FIRE TO START");
  arduboy.display();

  while(! (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON))) {
    delay(15);
  }
}

void newGame() {
  unsigned int playerX, playerY, playerDir, freq = 200;
  short slowDown = 30;

  score = 0;
  
  arduboy.clear();
  arduboy.drawRect(0, 0, 128, 64, 1);

  arduboy.initRandomSeed();
  playerX = random(10, 118);
  playerY = random(10, 54);

  arduboy.drawPixel(playerX, playerY, 1);
  arduboy.display();

  arduboy.tunes.tone(1500, 1000);
  delay(1200);
  arduboy.tunes.tone(1500, 1000);
  delay(1200);
  arduboy.tunes.tone(1500, 1000);
  delay(1200);
  arduboy.tunes.tone(2000, 1500);
  delay(1500);
  
  playerDir = random(0, 3);
  
  while(true) {
    if (arduboy.pressed(UP_BUTTON)) {
      playerDir = DIR_UP;
      freq = 200;
    } else if (arduboy.pressed(DOWN_BUTTON)) {
      playerDir = DIR_DOWN;
      freq = 200;
    } else if (arduboy.pressed(LEFT_BUTTON)) {
      playerDir = DIR_LEFT;
      freq = 200;
    } else if (arduboy.pressed(RIGHT_BUTTON)) {
      playerDir = DIR_RIGHT;
      freq = 200;
    }

    switch(playerDir) {
      case DIR_UP:
        playerY--;
        break;
      case DIR_DOWN:
        playerY++;
        break;
      case DIR_LEFT:
        playerX--;
        break;
      case DIR_RIGHT:
        playerX++;
        break;      
    }

    arduboy.tunes.tone(freq++, 100);

    if (arduboy.getPixel(playerX, playerY)) {
      arduboy.tunes.tone(1500, 100);
      delay(100);
      arduboy.tunes.tone(1000, 100);
      delay(100);
      arduboy.tunes.tone(500, 200);
      delay(200);
      break;
    }

    arduboy.drawPixel(playerX, playerY, 1);
    arduboy.display();
    score++;

    if (score % 100 == 0 && slowDown > 0) {
      slowDown -= 2;
      arduboy.tunes.tone(3000, 100);
    }
    delay(slowDown);
  }
}

void gameOver() { 
  arduboy.clear();
  arduboy.setTextSize(2);
  arduboy.setCursor(7, 20);
  
  if (score > highScore) {
    sprintf(textBuf, "NEW HI:%u", score);
  } else {
    sprintf(textBuf, "SCORE:%u", score);
  }
  
  arduboy.print(textBuf);

  arduboy.setTextSize(1);

  if (score <= highScore) {
    arduboy.setCursor(12, 42);
    sprintf(textBuf, "HI SCORE:%u", highScore);
    arduboy.print(textBuf);
  } else {
    highScore = score;
  }
  
  arduboy.setCursor(6, 53);
  arduboy.print("PRESS FIRE TO START");
  arduboy.display();

  while(! (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON))) {
    delay(15);
  }
}

void startScreen();
void newGame();
void gameOver();

void setup() {
  arduboy.beginNoLogo();
}

void loop() {
  while(true) {
    startScreen(); 
    newGame();  
    gameOver(); 
  }
}
