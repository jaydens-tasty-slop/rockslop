#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduboy2.h>
#include <ArduboyTones.h>
#include "gfx.h"

Arduboy2 arduboy;
Sprites sprites;
ArduboyTones sound(arduboy.audio.enabled);

byte gamestate = 0;
byte level = 1;
int prex = 128;
int prey = -22;
byte posx = 3;
byte posy = 42;
int rotate = 0;
byte veloc = 0;
int terx = 0;
byte idjump = 0;
int score = 0;
byte scTmp = 0;
byte xfire = posx + 38;
byte yfire = posy;
byte xfire2 = posx + 6;
int yfire2 = posy;
byte speedFire = 0;
byte speedFire2 = 0;
boolean bfire = false;
boolean cfire = false;
int bump1 = 0;
int bump2 = 0;
int bump3 = 0;
boolean jump = false;
boolean up = false;
boolean shake = false;
byte contShake = 0;
byte xShake = 0;
boolean hole = false;
byte contSal = 0;
boolean rokon = false;
int rockx = 128;
boolean rockExp = false;
byte rockexptime = 0;
boolean alienon = false;
int alienx;
int alieny;
int alienyTarget;
int nextY = 1;
boolean dir = false;
boolean alienExp = false;
byte alienxptime = 0;
boolean bombon = false;
byte bombx;
byte bomby;
boolean rockcol = false;
byte rockstep = 2;
byte hard = 1;
byte bombSpeed = 10;
boolean bombExp = false;
int bombExpTime = 20;
byte scaleFont;
long hiScore = 0;
int shot = 5;
byte maxAlienY = 25;
unsigned long tempo = 0;
unsigned long tempo2 = 0;
unsigned long tempo3 = 0;
byte seconds = 0;
byte minutes = 0;
boolean win = false;



int ster[] = {0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0};
int jumph[] = { -4, -3, -2, -1, -1, 0, 0 , 0 , 0 , 1, 1, 2, 3, 4, 1, -0, -1};
int sfx[] = {0, 30, 50, 90, 120, 150, 170, 200, };
byte sfy[] = {40, 10, 30, 10, 40, 10, 30, 20, };
byte sal[] = {0, 1, 2, 3,};

#endif
