#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "globals.h"

//###### MEZZO ######
void mezzo() {
  arduboy.drawBitmap(posx, posy, MEZZO, 36, 16);
  if (rotate == 4) rotate = 0;

  switch (rotate) {
    case 0:

      sprites.drawOverwrite(posx + 1, posy + 8 - bump3, RUOTA, 0);
      sprites.drawOverwrite(posx + 10, posy + 8 - bump2, RUOTA, 1);
      sprites.drawOverwrite(posx + 25, posy + 8 - bump1, RUOTA, 2);
      break;

    case 1:
      sprites.drawOverwrite(posx + 1, posy + 8 - bump3, RUOTA, 1);
      sprites.drawOverwrite(posx + 10, posy + 8 - bump2, RUOTA, 2);
      sprites.drawOverwrite(posx + 25, posy + 8 - bump1, RUOTA, 3);
      break;

    case 2:
      sprites.drawOverwrite(posx + 1, posy + 8 - bump3, RUOTA, 2);
      sprites.drawOverwrite(posx + 10, posy + 8 - bump2, RUOTA, 3);
      sprites.drawOverwrite(posx + 25, posy + 8 - bump1, RUOTA, 0);
      break;

    case 3:
      sprites.drawOverwrite(posx + 1, posy + 8 - bump3, RUOTA, 3);
      sprites.drawOverwrite(posx + 10, posy + 8 - bump2, RUOTA, 0);
      sprites.drawOverwrite(posx + 25, posy + 8 - bump1, RUOTA, 1);
      break;
  }
}

//###### PERCORSO ######
void percorso() {


  ////// Check tyre's bounce ///////

  if (ster[(posx + 36) / 8] == 1) {
    bump1 = 1;
  } else bump1 = 0;

  if (ster[(posx + 16) / 8 ] == 1) {
    bump2 = 1;
  } else bump2 = 0;

  if (ster[(posx + 8) / 8] == 1) {
    bump3 = 1;
  } else bump3 = 0;


  ////// Draw terrain //////

  for (int nn = 0; nn <= 16; nn++) {
    sprites.drawSelfMasked(terx + (nn * 8), 57, TERRENO, ster[nn]);
  }

  if (terx == -8) {
    for (int cc = 0; cc < 16; cc++) {
      ster[cc] = ster[cc + 1];
    }
    ster[16] = random(4);
    if (ster[12] + ster[13] + ster[14] + ster[15] >= 2) {
      ster[16] = random(2);
    }



    /////// Score 10 pt every  10 terrain's tiles ///////

    terx = 0;
    scTmp++;

    if (scTmp == 10) {
      score += 10;
      scTmp = 0;
    }
    if (ster[posx / 8] == 2 && hole == false) {
      score += 50;
      sound.tone(NOTE_C4, 50);
      shot++;
    }
    if (ster[posx / 8] == 3 && hole == false) {
      score += 80;
      sound.tone(NOTE_A3, 50);
    }


    /////// Check if front and rear tyre hit a terrain's hole //////

    if (ster[(posx + 32) / 8] == 2 && jump == false && hole == false) {
      score -= 200;
      bump1 = -2;
      shake = true;
      hole = true;
      sound.tone(NOTE_C2, 100);

    }

    if (ster[(posx + 8) / 8] == 2 && jump == false && hole == false) {
      score -= 200;
      bump1 = -2;
      shake = true;
      hole = true;
      sound.tone(NOTE_C2, 100);
    }

    if (ster[(posx + 32) / 8] == 3 && jump == false && hole == false) {
      score -= 300;
      bump1 = -3;
      shake = true;
      hole = true;
      sound.tone(NOTE_C2, 100);
    }

    if (ster[(posx + 8) / 8] == 3 && jump == false && hole == false) {
      score -= 200;
      bump1 = -2;
      shake = true;
      hole = true;
      sound.tone(NOTE_C2, 100);
    }

  } else if (arduboy.everyXFrames(10 - veloc)) {
    terx--;
    rotate = terx / -2;
    if (rokon) rockx--;

  }
}

//####### SFONDO ########
void sfondo() {
  for (byte ids = 0; ids < 7; ids++) {
    arduboy.drawLine(sfx[ids], sfy[ids], sfx[ids + 1], sfy[ids + 1], WHITE);
  }
  if (arduboy.everyXFrames(10 - (veloc / 2)) && veloc > 2 ) {

    for (byte mov = 0; mov < 8; mov++) {
      sfx[mov]--;
    }

    if (sfx[2] == 0) {
      for (byte ret = 0; ret < 6 ; ret++) {
        sfx[ret] = sfx[ret + 2];
        sfy[ret] = sfy[ret + 2];
        up = !up;
      }
      sfx[6] = random(165, 175);
      sfx[7] = random(190, 200);
      if (up) {
        sfy[6] = random(30, 40);
        sfy[7] = random(10, 20);
      } else {
        sfy[6] = random(10, 20);
        sfy[7] = random(30, 40);
      }
    }
  }
}

//######## JUMP ########
void salto() {
  if (jump == true && arduboy.everyXFrames(3) && idjump < 17) {
    posy += jumph[idjump];
    idjump++;
    bump1 = 0;
    bump2 = 0;
    bump3 = 0;
  }
  if (idjump > 16) {
    idjump = 0;
    jump = false;
  }
}


//###### FIRE ######

void fire() {
  if (bfire == true) {
    arduboy.fillCircle(xfire - 1, yfire, speedFire / 3, WHITE);
    arduboy.drawLine(xfire, yfire, xfire + 4, yfire);
    xfire += speedFire;
    if (xfire >= 125) {
      bfire = false;
      speedFire = 0;
    }
    if (xfire < 125 && xfire - rockx > -3 && yfire > 50 && rockx > posx + 34) {
      rokon = false;
      bfire = false;
      score += 40;
      rockExp = true;
    }

    if (bombon == true && xfire > bombx && yfire < bomby + 5) {
      bombon = false;
      bfire = false;
      score += 60;
      bombExp = true;
      shot++;
    }
  }
  if (cfire == true) {
    arduboy.fillCircle(xfire2 , yfire2 - 1, speedFire2 / 4, WHITE);
    arduboy.drawLine(xfire2, yfire2, xfire2, yfire2 + 3);
    yfire2 -= speedFire2;
    if (yfire2 < alieny + 12 && xfire2 > alienx && xfire2 < alienx + 12 && alienon == true && alienExp == false) {
      cfire = false;
      score += 50;
      alienExp = true;
      speedFire2 = 0;
    }
  }
  if (arduboy.everyXFrames(5)) {
    if (bfire == true) speedFire++;
    if (cfire == true) speedFire2++;
  }
  if (yfire2 < 1) {
    cfire = false;
    speedFire2 = 0;
  }
}



//####### ALIENS ########
void aliens() {
  if (alienon == false && alienExp == false) {
    dir = random(2);
    if (dir) {
      alienx = random(-32, -16);
    } else {
      alienx = random(128, 133);
    }
    alieny = random (0, maxAlienY - 10);
    alienon = random(2);
  }

  if (alienon) {
    if (arduboy.everyXFrames(random(40, 70))) {
      alienyTarget = random(-8, 9);
      nextY = alienyTarget;
    }
    sprites.drawSelfMasked(alienx , alieny, Ali, sal[contSal]);
    if (arduboy.everyXFrames(4)) {
      contSal++;
    }
    if (contSal == 4) contSal = 0;

    if (arduboy.everyXFrames(2) && dir == true) {
      alienx++ ;
      if (nextY > 0) {
        alieny++;
        nextY--;
      } else {
        alieny--;
        nextY++;
      }
      if (alieny > maxAlienY) alieny--;
      if (alieny < 0) alieny++;
    }
  }
  if (arduboy.everyXFrames(2) && dir == false) {
    alienx--;
    if (nextY > 0) {
      alieny++;
      nextY--;
    } else {
      alieny--;
      nextY++;
    }
    if (alieny > maxAlienY) alieny--;
    if (alieny < 0) alieny++;
  }

  if (alienx == 134 || alienx == -33) alienon = false;

  if (alienExp) {
    arduboy.fillCircle(alienx + random(2, 12) , alieny + random(2, 12), random (3, 6), WHITE);
    alienon = false;
    alienxptime++;
    if (alienxptime == 30) {
      alienxptime = 0;
      alienExp = false;
      shot++;
    }
  }

  if (alienx >= posx && alienx <= posx + 20 && alieny + 15 >= posy && alienExp == false) {
    score -= 200;
    shake = true;
    alienExp = true;
  }


  if (level > 3 && alienx > 70 - maxAlienY && alienx < 114 && bombon == false) {
    byte prob = random(255);
    if (prob < 2 * hard) {
      bombon = true;
      bombx = alienx + 6;
      bomby = alieny + 6;
    }
  }
  if (bombon == true) {
    sprites.drawSelfMasked(bombx, bomby, Bomb, bomby % 2);
    if (bombx >= posx && bombx <= posx + 28 && bomby >= posy) {
      score -= 200;
      bombon = false;
      bombExp = true;
      shake = true;
    }
    if (arduboy.everyXFrames(bombSpeed)) {
      bomby ++;
      bombSpeed--;
      if (dir && bombSpeed > 8) {
        bombx ++;
      } else if (!dir && bombSpeed > 8)bombx --;
    }



    if (bombSpeed < 2)bombSpeed = 2;
    if (bomby > 56) {
      ster[bombx / 8 + 1] = 3;
      bombon = false;
      bombSpeed = 10;
      bombExp = true;
    }

  }

  if (bombExp) {
    arduboy.fillCircle(bombx + random(2, 8) , bomby - random(2, 8), random (2, 5), WHITE);
    bombExpTime--;
    if (bombExpTime == 0) {
      bombExpTime = 20;
      bombExp = false;
    }
  }


}

//####### ROCK #######
void rock() {
  if (arduboy.everyXFrames(60) && rockx == 128 && ster[16] < 2) {
    rokon = random(2);
  }

  if (rokon) {
    arduboy.fillTriangle(rockx, 57, rockx + 7, 57, rockx + 4, 50, WHITE);
    arduboy.fillTriangle(rockx + 3, 57, rockx + 10, 57, rockx + 7, 48, WHITE);

    if (rockx == -10) {
      rockx = 128;
      shot++;
      rokon = false;
    }
  }

  if (rockExp) {
    arduboy.fillCircle(rockx + random(-2, 3), yfire + random(-2, 3), random (3, 6), WHITE);
    rockexptime++;
    if (rockexptime == 30) {
      rockexptime = 0;
      rockx = 128;
      rockstep = 2;
      rockExp = false;
      shot++;
    }
  }
  if (rockx < posx + 36 && rockx > posx && jump == false && rockExp == false) {
    rockcol = true;
    shake = true;
  }

  if (rockcol && arduboy.everyXFrames(20)) {
    score -= 100;
    posx -= 3;
    rockstep--;
    if (posx < 3)posx = 3;

    if (rockstep == 0) rockcol = false;
    rockExp = true;
    rokon = false;
  }
}



//####### SCORE ########
void showScore() {
  if (score > hiScore) hiScore = score;
  if (score < 0) gamestate = 2;

  if (shake && contShake < 60) {
    xShake = random(10);
    contShake++;
  } else {
    shake = false;
    contShake = 0;
    hole = false;
  }

  if (score < 10) arduboy.setCursor(122 - xShake, 0);
  if (score >= 10 && score < 100) arduboy.setCursor(116 - xShake, 0);
  if (score >= 100 && score < 1000) arduboy.setCursor(110 - xShake, 0);
  if (score >= 1000 && score < 10000) arduboy.setCursor(104 - xShake, 0);
  if (score > 1000 && level < 2) {
    level = 2;
    scaleFont = 60;
  }
  if (score >= 3000 && level < 3) {
    level = 3;
    scaleFont = 60;
  }
  if (score >= 5000 && level < 4) {
    level = 4;
    scaleFont = 60;
  }
  if (score >= 10000 && level < 5) {
    level = 5;
    scaleFont = 60;
    hard = 2;
  }
  if (score >= 15000 && level < 6) {
    level = 6;
    scaleFont = 60;
    hard = 3;
    maxAlienY = 30;
  }
  if (score >= 10000 && score < 30000) arduboy.setCursor(98 - xShake, 0);

  if (score >= 20000) gamestate = 3;

  arduboy.print(score);
  arduboy.setCursor(0 , 2);
  arduboy.print("L.");
  if (scaleFont > 0) {
    arduboy.setTextSize(2);
    scaleFont--;
    arduboy.setCursor(6 , 0);
  }
  arduboy.print(level);
  arduboy.setTextSize(1);
  arduboy.setCursor(50, 0);
  arduboy.print("S:"); arduboy.print(shot);

}


#endif
