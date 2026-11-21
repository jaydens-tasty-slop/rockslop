#ifndef OCTOPUS_BARCA
#define OCTOPUS_BARCA

#include "Global.h"
#include "Sprites.h"

byte animBarca = 0;

void Barca() {
  if (barcaState == 0 && hited == false) {
    showSpriteDiver1 = true;
    showSpriteDiver1Arm1 = true;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = true;
    showSpriteDiver3 = true;
  } else if (barcaState == 1 && hited == false) {
    showSpriteDiver1 = false;
    showSpriteDiver1Arm1 = false;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = true;
    showSpriteDiver3 = true;
  } else if (barcaState == 2 && hited == false) {
    if (animBarca == 0 || animBarca == 2 ) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = true;
      showSpriteDiver1Arm2 = false;
      showSpriteDiver2 = true;
      showSpriteDiver3 = true;
    } else if (animBarca == 1 || animBarca == 3) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = false;
      showSpriteDiver1Arm2 = true;
      showSpriteDiver2 = true;
      showSpriteDiver3 = true;
    } else if (animBarca == 4) {
      score = score + getPoint;
      subGold = false;
      animBarca = 0;
      barcaState = 0;
    }
    if  ((moveTick % speedAnim) == 0) {
      animBarca++;
      gb.sound.playTick();
    }
  } else if (barcaState == 3 && hited == false) {
    showSpriteDiver1 = true;
    showSpriteDiver1Arm1 = true;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = true;
    showSpriteDiver3 = false;
  } else if (barcaState == 4 && hited == false) {
    showSpriteDiver1 = false;
    showSpriteDiver1Arm1 = false;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = true;
    showSpriteDiver3 = false;
  } else if (barcaState == 5 && hited == false) {
    if (animBarca == 0 || animBarca == 2) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = true;
      showSpriteDiver1Arm2 = false;
      showSpriteDiver2 = true;
      showSpriteDiver3 = false;
    }
    if (animBarca == 1 || animBarca == 3) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = false;
      showSpriteDiver1Arm2 = true;
      showSpriteDiver2 = true;
      showSpriteDiver3 = false;
    }
    if (animBarca == 4) {
      score = score + getPoint;
      subGold = false;
      animBarca = 0;
      barcaState = 3;
    }
    if ((moveTick % speedAnim) == 0) {
      animBarca++;
      gb.sound.playTick();
    }
  } else if (barcaState == 6 && hited == false) {
    showSpriteDiver1 = true;
    showSpriteDiver1Arm1 = true;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = false;
    showSpriteDiver3 = false;
  } else if (barcaState == 7 && hited == false) {
    showSpriteDiver1 = false;
    showSpriteDiver1Arm1 = false;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = false;
    showSpriteDiver3 = false;
  } else if (barcaState == 8 && hited == false) {
    if (animBarca == 0 || animBarca == 2) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = true;
      showSpriteDiver1Arm2 = false;
      showSpriteDiver2 = false;
      showSpriteDiver3 = false;
    }
    if (animBarca == 1 || animBarca == 3) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = false;
      showSpriteDiver1Arm2 = true;
      showSpriteDiver2 = false;
      showSpriteDiver3 = false;
    }
    if (animBarca == 4) {
      score = score + getPoint;
      subGold = false;
      animBarca = 0;
      barcaState = 9;
    }
    if ((moveTick % speedAnim) == 0) {
      animBarca++;
      gb.sound.playTick();
    }
  } else if (barcaState == 9 && hited == false) {
    showSpriteDiver1 = true;
    showSpriteDiver1Arm1 = true;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = false;
    showSpriteDiver3 = false;
  } else if (barcaState == 10) {
    showSpriteDiver1 = false;
    showSpriteDiver1Arm1 = false;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = false;
    showSpriteDiver3 = false;
  }
}

#endif
