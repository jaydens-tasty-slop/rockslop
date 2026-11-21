#include <stdlib.h>
#include <math.h>
/*----------------------------------
               JumpBoy


    Created by:takaiwa@gmail.com
    Game Copyright (C) 2016 takaiwa@gmail.com

  ----------------------------------*/
#include <Arduboy2.h>
#include <ArduboyTones.h>

#include "Bitmaps.h"
#include "../lib/ab_math.h"

#define NOTE_A1  110  //spring
#define NOTE_C2  131  //miss spring item(-)
#define NOTE_G2  196
#define NOTE_C3  262  // add coins item
#define NOTE_G3  392  //coin
#define NOTE_C4  523  //heart item,spring item(+),flag item


#define MODE_TITLE  1
#define MODE_START  2
#define MODE_RUNNING  3
#define MODE_GAMECLEAR  4
#define MODE_GAMEOVER  5

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

uint8_t       pMode = MODE_TITLE;

unsigned long lTime;
const byte    FPS = 1000 / 30;

unsigned long pWait = 0;
bool          pWaitFlg = false;

uint8_t  gray = 0;
float    frame = 0;
float    animSpeed = 0.2;
uint8_t  totalFrames = 4;

int      pX = 20;
int      pY = 0;
float  pSpeed = 0;
float  pASpeed = 2;
float  savepASpeed = 2;

int      prevPX = 20;
int      prevPY = 0;
int      pDir = 270;

int      pYY = 1;

uint8_t  pDirectionY = 1;
uint8_t  pSpeedYInit = 1;
uint8_t  pSpeedY = pSpeedYInit;

uint8_t  boundY = 10;
uint8_t  boundH = 10;

const uint8_t MAXSPD = 4;
const uint8_t MAXLIVES = 12;

uint8_t  lives = 3;
uint8_t  level = 1;

int stage = 1;
int record = 0;

int concurrent_coin_max = 1;

/*-------------------------------
    levelStart
  ------------------------------*/
void levelStart(int lvl);
void miss();
void stageClear();
void drawHeader();
void drawPlayer();
void displayTitle();
void initCoin(int concurrent_coin_max,bool is_puzzle);
void updateCurrentCoinMax(int concurrent_coin_max);
void resetSpCount();
void moveCoin();
boolean _checkPlayerPosition(int x,int y);
void _createCoins();
void _checkCoinsLife();
void _collisionCoins();
uint8_t getCoinQty();
void drawCoin();
void initPowerUp();
void createPowerUp(uint8_t type);
void movePowerUp();
void collisionPowerUp();
void drawPowerUp();
void initSpring();
void setAllOneSpring();
void setAddSpring();
void setAddMaxSpring();
void setDelSpring();
void setOddSpring();
void collisionSpring(bool &is_miss);
void moveSpring();
void initSpeedScroll();
void speedupScroll();
void drawSpring();

void levelStart(int lvl)
{
  stage = 1;
  pASpeed = 2;
  savepASpeed = 2;

  arduboy.clear();

  if (lvl == 1) {
    stage = 1;
    lives = 3;
    initSpeedScroll();
    arduboy.drawSlowXYBitmap(17, 10, bStart, 96, 48, 1);
  }

  arduboy.display();

  delay(2000);

  initSpring();
  
  initCoin(concurrent_coin_max,false);
  
  initPowerUp();

  level = lvl;
  pY = 24;

  pMode = MODE_RUNNING;
}


/*-------------------------------
    miss
  ------------------------------*/
void miss()
{
  pSpeed = 0;
  sound.tone(NOTE_C2, 160); 
  initPowerUp();
  lives--;
  if (lives <= 0) {
    delay(3000);
    pMode = MODE_TITLE;
  }
}


/*-------------------------------
    stageClear
  ------------------------------*/
void stageClear()
{
  record = stage;

//  pASpeed++;
  pASpeed+=0.5;
  speedupScroll();

  if (pASpeed > 4) {
    pASpeed = 2;
  }

  savepASpeed = pASpeed;
  stage++;

  initSpring();
  initCoin(concurrent_coin_max,false);

  arduboy.clear();
  arduboy.drawSlowXYBitmap(17, 10, bClear, 96, 48, 1);
  arduboy.display();

  delay(2000);
}


/*--------------------------
     Draw Header
  -------------------------*/
void drawHeader()
{
  arduboy.fillRect(0, 0, 127, 10, 1);
  for (int i = 0; i < lives; i++) {
    arduboy.drawBitmap(3 + (i * 10) , 1 , bHeart, 8, 8, 0);
  }
  arduboy.setCursor(72, 1);
  arduboy.write(' ');
  arduboy.write('C');
  arduboy.write('O');
  arduboy.write('I');
  arduboy.write('N');
  arduboy.write(' ');
  arduboy.print(getCoinQty());
  arduboy.write(' ');
}


/*-------------------------
    Draw Player
  ------------------------ */
void drawPlayer()
{
    bool is_miss = false;
    if (pY >= 46) {
      collisionSpring(is_miss);
  
      if (!is_miss) {
        pDir = 270;
        pYY = -1 * pASpeed;
      } else {
        //Serial.println("miss...");
        pY = 12;
  
        // check gameover
        miss();
        
        pWait = millis() + 1000 * 3;
        savepASpeed = pASpeed;
        pASpeed = 0;
        pWaitFlg = true;
  
        //Serial.println(pWait);
        return;
      }
    }
    if (pY <= 12) {
      pDir == 90;
      pYY = 1 * pASpeed;
    }
    pY += pYY;

  arduboy.drawSlowXYBitmap(pX , pY, bMan, 8, 8, 1);
}


/*-------------------------
    displayTitle
  ------------------------ */
void displayTitle()
{
  int flash = 0;
  
  while (true) {
    delay( 30 );
    arduboy.clear();

    arduboy.setCursor(36, 1);
    arduboy.print("JUMP BOY");
    
    arduboy.drawSlowXYBitmap(14, 12, bTitle, 104, 36, 1);

    flash++;
    flash %= 50;

    if (record > 0) {
      arduboy.setCursor(30, 44);
      arduboy.print(" record: ");
      arduboy.print(record);
      arduboy.print(" ");         
    }

    if (flash < 25) {
      arduboy.setCursor(30, 54);
      arduboy.print("Press A or B");
    }

    arduboy.display();

    if (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)) {
      pMode = MODE_START;
      break;
    }
  }
}


/*-------------------------
    setup
  ------------------------ */
void setup()
{
  //Serial.print("Entering Setup");
  arduboy.begin();
  arduboy.audio.on();

  displayTitle();
  lTime = millis();
  arduboy.initRandomSeed();
}


/*-------------------------
    LOOP
  ------------------------ */
void loop()
{ 
  if (pMode == MODE_TITLE) {
    displayTitle();
    return;
  }

  if (pMode == MODE_START) {
    levelStart(1);
    return;
  }
  
  if (pWaitFlg) {
    if (millis() > pWait) {
      //Serial.println("wait brake....");
      pWaitFlg = false;
      return;
    } else {
      //Serial.print("waiting....\n");
      //Serial.println(millis());
      return;
    }
  }

  //Serial.print("Loop");
 
  if (millis() > lTime + FPS) {
    if (pSpeed == 0) {
      frame = 0;
    }

    prevPX = pX;
    prevPY = pY;
    pSpeed = 0;

    if (pASpeed == 0) {
      if (arduboy.buttonsState()) {
        pASpeed = 1;
        pSpeed = pASpeed;
      }
    } else {
      if (arduboy.pressed(UP_BUTTON)) {
        pDir = 90;
        pASpeed = savepASpeed;
        pSpeed = pASpeed;
      } else if (arduboy.pressed(DOWN_BUTTON)) {
        pDir = 270;
        pASpeed = savepASpeed;
        pSpeed = pASpeed;
      } else if (arduboy.pressed(LEFT_BUTTON)) {
        pDir = 180;
        pASpeed = savepASpeed;
        pSpeed = pASpeed;
      } else if (arduboy.pressed(RIGHT_BUTTON)) {
        pDir = 0;
        pASpeed = savepASpeed;
        pSpeed = pASpeed;
      }
    }

    if (pSpeed > 0) {
      switch (pDir) {
        case 180:
          pX -= pSpeed;
          break;
        case 0:
          pX += pSpeed;
          break;
      }
    }

    if (pY > 56 - boundH) {
      pY = 56 - boundH;
    }

    if (pY < boundY) {
      pY = boundY;
    }

    if (pX > 120) {
      pX = 120;
    }

    if (pX < 8) {
      pX = 8;
    }


    if ( prevPX != pX || prevPY != pY ) {
      frame += animSpeed * pASpeed;
    }
    if (frame > 4) {
      frame = 0;
    }

    movePowerUp();
    moveCoin();
    moveSpring();

    arduboy.clear();
    lTime = millis();

    drawHeader();
    drawSpring();

    drawPlayer();
    drawPowerUp();
    drawCoin();

    /*----------------------------
        Drawing - Display
      ---------------------------*/
    arduboy.display();
  }
}
/*----------------------
    Coins:
  --------------------- */
const uint8_t CONCURRENT_COIN_MAX = 10;

struct Coin
{
  uint8_t spCount = 0;
  uint8_t qty = 0;
  uint8_t norma = 20;
  uint8_t concurrent_coin_max = CONCURRENT_COIN_MAX;
  bool is_puzzle = false;
  bool is_puzzle_prepare = false;  
} coin;


struct Coins
{
  float x = 64;
  float y = 16;
  bool active = false;
  uint8_t spCount = 0;
};
Coins coins[CONCURRENT_COIN_MAX];



void initCoin(int concurrent_coin_max,bool is_puzzle)
{
  coin.spCount = 0;
  coin.qty = 0;
  
  coin.is_puzzle = is_puzzle;
  coin.is_puzzle_prepare = false;

  coin.norma = (coin.is_puzzle) ? concurrent_coin_max : 20;
  coin.concurrent_coin_max = (concurrent_coin_max > CONCURRENT_COIN_MAX ) ? CONCURRENT_COIN_MAX : concurrent_coin_max;
  
  for (int i = 0; i < CONCURRENT_COIN_MAX; i++) {
    coins[i].active = false;
  }
}




void updateCurrentCoinMax(int concurrent_coin_max)
{
  coin.concurrent_coin_max = (concurrent_coin_max > CONCURRENT_COIN_MAX ) ? CONCURRENT_COIN_MAX : concurrent_coin_max;  
}


void resetSpCount()
{
  coin.spCount = 0;
}

void moveCoin()
{

  if (coin.is_puzzle && !coin.is_puzzle_prepare) {
    coin.is_puzzle_prepare = true;
    _createCoins();
  } else {



//アイテムの有効期限チェック
    coin.spCount++;
    if (coin.spCount == 200)
    {
      coin.spCount = 0;
      updateCurrentCoinMax(1);
    }
   
 
      _createCoins();
  }
  
  _checkCoinsLife();
  _collisionCoins();
}


boolean _checkPlayerPosition(int x,int y)
{
  if (pX - 8 * 2 < x && x < pX + 8 * 3 &&
      pY - 8 * 2 < y && y < pY + 8 * 3 ) {
    return false;
  }
  return true;
}


void _createCoins()
{
  for (int i = 0; i < coin.concurrent_coin_max; i++) {
    if (coins[i].active == false) {

      if (coin.is_puzzle) {
        // 高さは同じで横に等間隔にコインを配置
        coins[i].x = 10 + i * 10;
        coins[i].y = 10 + 30;
      }

      int x = 10 + random(110);
      int y = 10 + random(34);
      if (_checkPlayerPosition(x,y)) {
        coins[i].x = x;
        coins[i].y = y;
        coins[i].spCount = 40 + random(80);
//        coins[i].spCount = 50;
        coins[i].active = true;
      }
    }
  }
}


void _checkCoinsLife()
{
  for (int i = 0; i < coin.concurrent_coin_max; i++) {
    coins[i].spCount--;
    if (coins[i].spCount <= 0) {
      coins[i].active = false;
    }
  }
}



void _collisionCoins()
{
  for (int i = 0; i < coin.concurrent_coin_max; i++) {
    if (coins[i].active) {
      if (coins[i].x < pX + 8 &&
          coins[i].x + 8 > pX &&
          coins[i].y < pY + 8 &&
          8 + coins[i].y > pY)
      {
        sound.tone(NOTE_G3, 80);
        coin.qty++;
        coins[i].active = false;
        if (coin.qty >= coin.norma) {
          stageClear();
          return;
        }
      }
    }
  }
}


uint8_t getCoinQty()
{
  return coin.qty;
}


void drawCoin()
{
  for (int i = 0; i < coin.concurrent_coin_max; i++) {
    if (coins[i].active) {
      arduboy.drawBitmap(coins[i].x, coins[i].y, bCoin, 8, 8, 1);
    }    
  }
}
/*----------------------
    PowerUps
  --------------------- */

struct PowerUps
{
  float x = 128;
  float y = 32;
  uint8_t type = 0;
  bool active = false;
  float xspd = 0;
  float yspd = 0;
  uint8_t spCount = 0;
} powerup;


void initPowerUp() {
  powerup.spCount = 0;
  powerup.active = false;
}


void createPowerUp(uint8_t type)
{
  powerup.xspd = 1;
  powerup.yspd = 1.5;

  powerup.x = 10 + random(110);
  powerup.y = 10 + random(34);

  //
  if (pX - 8 < powerup.x && powerup.x < pX + 16) {
    return;
  }

  powerup.type = type;
  powerup.active = true;
}

void movePowerUp()
{
  powerup.spCount++;

  if (powerup.spCount == 50) {
    powerup.active = false;
  }

  if (powerup.spCount == 200) {
    powerup.spCount = 0;

    uint8_t r = random(100);
    uint8_t item[] = {5, 20, 30, 45, 50};
    for (int i = 0;i < 5;i++) {
      if (r <= item[i]) {
        createPowerUp(i);
        break;
      }
    }
  }

  collisionPowerUp();
}


void collisionPowerUp()
{
  if (powerup.active) {
    if (powerup.x < pX + 8 &&
        powerup.x + 8 > pX &&
        powerup.y < pY + 8 &&
        8 + powerup.y > pY)
    {
      if (powerup.type == 0) // heart
      {
        sound.tone(NOTE_C4, 80);
        lives++;
        powerup.active = false;
        powerup.x = 128;
      }
      if (powerup.type == 1) // spring=+1
      {
        setAddSpring();
        sound.tone(NOTE_C4, 80);
        powerup.active = false;
      }
      if (powerup.type == 2) // spring=-1
      {
        // setAllOneSpring();
        // setOddSpring();
        setDelSpring();
        sound.tone(NOTE_C2, 80);
        powerup.active = false;
      }

      if (powerup.type == 3) // coin add item
      {
        resetSpCount();
        updateCurrentCoinMax(3);
        sound.tone(NOTE_C3, 80);
        powerup.active = false;
      }

      if (powerup.type == 4) // flag item
      {
        setAddMaxSpring();
        sound.tone(NOTE_C4, 80);
        powerup.active = false;
      }
    }
  }
}
/*------------------
    Draw Power Up
  ----------------- */
void drawPowerUp()
{
  if (powerup.active) {
    if (powerup.type == 0) {
      arduboy.drawBitmap(powerup.x, powerup.y, bHeart, 8, 8, 1);
    }
    if (powerup.type == 1) {
      arduboy.drawSlowXYBitmap(powerup.x, powerup.y, bItemPlus, 8, 8, 1);
    }
    if (powerup.type == 2) {
      arduboy.drawSlowXYBitmap(powerup.x, powerup.y, bItemMinus, 8, 8, 1);
    }
    if (powerup.type == 3) {
      arduboy.drawSlowXYBitmap(powerup.x, powerup.y, bItemAddCoins, 8, 8, 1);
    }
    if (powerup.type == 4) {
      arduboy.drawSlowXYBitmap(powerup.x, powerup.y, bItemFlag, 8, 8, 1);
    }
  }
}
/*----------------------
    spring:
  --------------------- */
uint8_t pSprings[]   = {1, 2, 3, 4, 3, 4, 3, 2, 1, 2, 1, 2, 3, 3, 4, 3};
uint8_t pSprings_1[] = {1, 2, 3, 4, 3, 4, 3, 2, 1, 2, 1, 2, 3, 3, 4, 3};
int16_t pScroll = 128;
int16_t pScrollSpeed = 1;

/*-----------------------
    initSpring
  ----------------------*/
//TODO:引数でN面を対応する
void initSpring() {
  for (int i = 0; i < 16; i++) {
    pSprings[i] = pSprings_1[i];
  }
}

/*-----------------------
    setAllOneSpring
  ----------------------*/
void setAllOneSpring() {
  for (int i = 0; i < 16; i++) {
    pSprings[i] = 1;
  }
}

/*-----------------------
    setAddSpring
  ----------------------*/
void setAddSpring() {
  for (int i = 0; i < 16; i++) {
    if (pSprings[i] < 4) {
      pSprings[i] += 1;
    }
  }
}

/*-----------------------
    setAddMaxSpring
  ----------------------*/
void setAddMaxSpring() {
  for (int i = 0; i < 16; i++) {
    pSprings[i] = 4;
  }
}



/*-----------------------
    setDelSpring
  ----------------------*/
void setDelSpring() {
  for (int i = 0; i < 16; i++) {
    if (pSprings[i] > 0) {
      pSprings[i] -= 1;
    }
  }
}


/*-----------------------
    setAllOneSpring
  ----------------------*/
void setOddSpring() {
  for (int i = 0; i < 16; i++) {
    if (i % 2 == 0) {
      pSprings[i] = 0;
    } else {
      pSprings[i] = 1;
    }
  }
}


/*-------------------------
    collisionSpring
  ------------------------ */
void collisionSpring(bool &is_miss)
{
  sound.tone(NOTE_A1, 80);

  is_miss = true;
  for (int i = 0; i < 16; i++)
  {
    //
    int16_t x = pScroll - (i * 8);
    if (x < 0) {
      x = 128 + x;
    }
    x+=4;
    if (pX - 4 <= x && x <= pX + 12 && pSprings[i] > 0) {
      pSprings[i] -= 1;
      is_miss = false;
      break;
    }
  }

  // 全部なくなったらリセット
  int s = 0;
  for (int i = 0; i < 16; i++) {
    s += pSprings[i];
  }
  if (s == 0) {
    initSpring();
  }
}

/*-----------------------
    moveSpring
  ----------------------*/
void moveSpring()
{
////  pScroll--;
  pScroll-=pScrollSpeed;
  if (pScroll < 0) {
    pScroll = 127;
  }
}

/*-----------------------
    initSpeedScroll
  ----------------------*/
void initSpeedScroll()
{
  pScrollSpeed = 1;
}

/*-----------------------
    speedupScroll
  ----------------------*/
void speedupScroll()
{
  pScrollSpeed++;
  if (pScrollSpeed > 5){
    pScrollSpeed = 1;
  }
}

/*-----------------------
    drawSpring
  ----------------------*/
void drawSpring()
{
  arduboy.fillRect(8, 56, 127, 64, 1);
  for (int i = 0; i < 16; i++)
  {
    //
    int16_t x = pScroll - (i * 8);
    if (x < 0) {
      x = 128 + x;
    }

    switch ( pSprings[i] )
    {
      case 0:
        arduboy.drawSlowXYBitmap(x, 56, bEmpty, 8, 8, 0);
        break;
      case 1:
        arduboy.drawSlowXYBitmap(x, 56, bSpring1, 8, 8, 0);
        break;
      case 2:
        arduboy.drawSlowXYBitmap(x, 56, bSpring2, 8, 8, 0);
        break;
      case 3:
        arduboy.drawSlowXYBitmap(x, 56, bSpring3, 8, 8, 0);
        break;
      case 4:
        arduboy.drawSlowXYBitmap(x, 56, bSpring4, 8, 8, 0);
        break;
      default:
        arduboy.drawSlowXYBitmap(x, 56, bEmpty, 8, 8, 0);
        break;
    }
  }
}
