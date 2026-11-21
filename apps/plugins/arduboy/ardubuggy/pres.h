#ifndef PRES_H
#define PRES_H

#include "globals.h"

void pres() {
  sound.noTone();
  if (prey <= 7) {
    arduboy.drawBitmap(35, prey, Ardu, 69, 22);
    prey++;
    arduboy.display();
  }
  if (prey >= 7 && prex > 55) {
    arduboy.drawBitmap(35, 7, Ardu, 69, 22);
    arduboy.drawBitmap(prex, 28, Buggy, 67, 24);
    prex -= 2;
    arduboy.display();
  }

  if (prex <= 55) {
    delay(350);
    arduboy.drawBitmap(35, 7, Ardu, 69, 22);
    arduboy.drawBitmap(56, 28, Buggy, 67, 24);
    arduboy.drawBitmap(6, 25, Dpad, 32, 32);
    if (hiScore > 0 && win == false) {
      arduboy.setCursor(48, 56);
      arduboy.print("HI:");
      arduboy.print(hiScore);
    }
    if (win == true) {
      arduboy.setCursor(38, 56);
      arduboy.print("BEST ");
      arduboy.print(minutes);
      arduboy.print(":");
      if (seconds < 10)arduboy.print("0");
      arduboy.print(seconds);
    }
    arduboy.display();

    if (arduboy.pressed(B_BUTTON) || arduboy.pressed(A_BUTTON)) {
      arduboy.clear();
      arduboy.display();
      delay(500);
      gamestate = 1;
      tempo = millis();
    }
  }
}

#endif
