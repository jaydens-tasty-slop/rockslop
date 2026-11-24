#pragma once
#include <Arduboy2.h>

void soundInit(Arduboy2 &arduboy);
void soundToggle(Arduboy2 &arduboy);

void soundPlay_lock();
void soundPlay_lineClear(uint8_t lines);   // lines: 1〜4
void soundPlay_hardDrop();
void soundPlay_levelUp();
void soundPlay_gameOver();
