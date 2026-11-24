#ifndef CONTROLS_H
#define CONTROLS_H

#include "globals.h"

void speedBuggy() {
  veloc = posx / 3 + 1 ;
  if (veloc > 9)veloc = 9;
}


void buttons() {
  if (arduboy.pressed(RIGHT_BUTTON) && arduboy.everyXFrames(4) && posx < 50) {
    posx++;
    if (posx > 50)posx = 50;
  }

  if (arduboy.notPressed(RIGHT_BUTTON) && arduboy.everyXFrames(15) && (posx > 3)) {
    if (arduboy.everyXFrames(5))veloc--;
    posx--;
  }
  if (arduboy.pressed(LEFT_BUTTON) && arduboy.everyXFrames(5) && posx > 3 ) {
    posx--;
  }


  if (arduboy.justPressed(B_BUTTON) && jump == false) {
    jump = true;

  }
  if (arduboy.pressed(UP_BUTTON))arduboy.audio.on();
  if (arduboy.pressed(DOWN_BUTTON))arduboy.audio.off();
  if (arduboy.pressed(A_BUTTON) && bfire == false && cfire == false && rockExp == false && alienExp == false && shot > 0) {
    xfire = posx + 38;
    yfire = posy + 10;
    xfire2 = posx + 6;
    yfire2 = posy + 2;
    bfire = true;
    cfire = true;
    shot--;

  }


}

#endif
