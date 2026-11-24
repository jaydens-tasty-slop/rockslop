#ifndef OCTOPUS_SUB_SPRITE
#define OCTOPUS_SUB_SPRITE

byte frameSub = 0;

void die() {
  moveSubButton = false;
  for (size_t index = 0; index < 4; ++index)
    showOctopusLeg3[index] = true;
  diverToShow = 6;
  if ((frameSub == 0) || (frameSub == 2) || (frameSub == 4) || (frameSub == 6)) {
    diverArmToShow = 4;
    diverBagToShow = 6;
  }
  if (( frameSub == 1) || (frameSub == 3) || (frameSub == 5) || (frameSub == 7)) {
    diverArmToShow = 5;
    diverBagToShow = 7;
  }
}

void animSubGold() {
  if (subGold == false) {
    if (animFrame == 0) {
      diverToShow = 5;
      diverArmToShow = 1;
      diverBagToShow = 0;
    }
    if (animFrame == 1) {
      diverToShow = 5;
      diverArmToShow = 2;
      diverBagToShow = 0;
    }
    if (animFrame == 2) {
      diverToShow = 5;
      diverArmToShow = 3;
      diverBagToShow = 0;
    }
    if (animFrame == 3) {
      ++score;
      ++getPoint;
      // gb.sound.play("OR.WAV");
      gb.sound.playOK();
      subGold = true;
      subMove = 5;
      animFrame = 1;
      catchGold = false;
    }
  } else if (subGold == true) {
    if (animFrame == 0) {
      diverToShow = 5;
      diverArmToShow = 1;
      diverBagToShow = 5 ;
    }
    if (animFrame == 1) {
      diverToShow = 5;
      diverArmToShow = 2;
      diverBagToShow = 5 ;
    }
    if (animFrame == 2) {
      diverToShow = 5;
      diverArmToShow = 3;
      diverBagToShow = 5 ;
    }
    if (animFrame == 3) {
      ++score;
      ++getPoint;
      // gb.sound.play("OR.WAV");
      gb.sound.playOK();
      subGold = true;
      subMove = 5;
      animFrame = 0;
      catchGold = false;
    }
  }
  if ((moveTick % speedAnim) == 0) {
    animFrame++;
  }
}

void Sub() {
  // If diger hasn't been hited
  if (hited == false) {
    if (subGold == false) {
      // if diver has no bag
      diverToShow = subMove;
      diverBagToShow = 0;
      if (subMove == 0) diverArmToShow = 0;
    } else {
      // if diver has a gold bag
      if (subMove == 0) {
        diverToShow = 0;
        diverBagToShow = 0;
        diverArmToShow = 0;
      } else {
        diverToShow = subMove;
        diverBagToShow = 5 - subMove;
      }
    }
  }
  if (hited == true && life == 3) {
    animBarca = 0;
    if (animBarca == 0) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = true;
      showSpriteDiver1Arm2 = false;
      showSpriteDiver2 = false;
      showSpriteDiver3 = true;
    }

    if (animBarca == 1) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = true;
      showSpriteDiver1Arm2 = false;
      showSpriteDiver2 = true;
      showSpriteDiver3 = false;
    }
    if (moveTick % speedAnim == 0) {
      animBarca++;
    }
    die();
    if (frameSub == 8) {
      barcaState = 3;
      hited = false;
      subMove = 0;
      inGold = false;
      subGold = false;
      --life;
      frameSub = 0;
      moveSubButton = true;
    }
    if ((moveTick % speedAnim) == 0) {
      frameSub++;
      gb.sound.playTick();
    }
  } else if (hited == true && life == 2) {
    animBarca = 0;
    if (animBarca == 0) {
      showSpriteDiver1 = false;
      showSpriteDiver1Arm1 = false;
      showSpriteDiver1Arm2 = true;
      showSpriteDiver2 = false;
      showSpriteDiver3 = false;
    }
    if (animBarca == 1) {
      showSpriteDiver1 = true;
      showSpriteDiver1Arm1 = true;
      showSpriteDiver1Arm2 = false;
      showSpriteDiver2 = false;
      showSpriteDiver3 = false;
    }
    if ((moveTick % speedAnim) == 0) {
      animBarca++;
    }
    die();
    if (frameSub == 8) {
      barcaState = 6;
      hited = false;
      subMove = 0;
      inGold = false;
      subGold = false;
      --life;
      frameSub = 0;
      moveSubButton = true;
    }
    if ((moveTick % speedAnim) == 0) {
      frameSub++;
      gb.sound.playTick();
    }
  } else if (hited == true && life == 1) {
    moveSubButton = false;
    showSpriteDiver1 = false;
    showSpriteDiver1Arm1 = false;
    showSpriteDiver1Arm2 = false;
    showSpriteDiver2 = false;
    showSpriteDiver3 = false;
    die();
    if (frameSub == 8) {
      barcaState = 10;
      hited = false;
      subMove = 0;
      inGold = false;
      subGold = false;
      --life;
      frameSub = 0;
      moveSubButton = true;
    }
    if ((moveTick % speedAnim) == 0) {
      frameSub++;
      gb.sound.playTick();
    }
  }
}


#endif
