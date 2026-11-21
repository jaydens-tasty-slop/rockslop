#include <stdlib.h>
#include <math.h>

/*----------------------------------
               JumpBoy2


    Created by:takaiwa@gmail.com
    Game Copyright (C) 2016 takaiwa@gmail.com

  ----------------------------------*/
#include <Arduboy2.h>
#include <ArduboyTones.h>

#include "Bitmaps.h"
#include "Sound.h"
#include "../lib/ab_math.h"

#define NOTE_A1  110  //spring
#define NOTE_C2  131  //miss spring item(-)
#define NOTE_G2  196
#define NOTE_C3  262  //add coins item
#define NOTE_G3  392  //coin
#define NOTE_A3  440  //BOMB
#define NOTE_C4  523  //heart item,spring item(+),flag item
#define NOTE_E4  659  //Fire
#define NOTE_A4  880  //Enemy appears

#define MODE_TITLE  1
#define MODE_START  2
#define MODE_RUNNING  3
#define MODE_GAMECLEAR  4
#define MODE_GAMEOVER  5
#define MODE_CONTINUE  6

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

uint8_t       pMode = MODE_TITLE;

unsigned long lTime;
const byte    FPS = 1000 / 30;

bool          pMissing = false;

unsigned long pWait = 0;
bool          pWaitFlg = false;


float  pASpeed = 1;

uint8_t  boundY = 10;
uint8_t  boundH = 10;

uint8_t lives = 3;
uint8_t stage = 1;
uint8_t record = 1;

int concurrent_coin_max = 1;


/*-------------------------------
    levelStart
  ------------------------------*/
void levelStart(int lvl);
void miss();
void enemyAppears();
void stageClear();
void drawHeader();
void displayTitle();
void initBall(int no);
void waitBall();
void moveBall();
void collisionBall(float playerX, float playerY, int playerW, int playerH);
void initBullet(int no);
void fireBullet(int x,int y);
void moveBullet();
bool collisionBullet(float X, float Y, int W, int H);
void initCoin(uint8_t concurrent_coin_max);
void updateCurrentCoinMax(uint8_t concurrent_coin_max);
void updateCoinNorma(uint8_t norma1,uint8_t norma2);
void resetSpCount();
void moveCoin();
void moveCulcCoin();
void _createCoins();
void _checkCoinsLife();
void collisionCoins(float playerX, float playerY);
uint8_t getCoinQty();
void drawCoin();
void initEnemy(int no);
void activeEnemy();
void moveEnemy();
void moveCulcEnemy();
void actionUpdown();
void actionReflection();
void actionHorizon(int r);
void actionVirtical(int r);
void actionSpeedUpReflection();
void rangeMoveEnemy();
bool rangeMoveEnemyX();
bool rangeMoveEnemyY();
void fireEnemyFireToTarget();
void collisionEnemy(float playerX, float playerY, int playerW, int playerH);
void initEnemyFire(int no);
void fireEnemyFire(int x, int y, int ax, int l);
void moveEnemyFire();
void collisionEnemyFire(float X, float Y, int W, int H);
void initKey();
void resetKey();
void moveKey();
void collisionKey(float playerX, float playerY, int playerW, int playerH);
void initPlayer(float p);
void movePlayer();
void moveCulcPlayer();
void collisionPlayer();
bool duplicatePlayerPosition(float x, float y);
bool collisionPlayerPositionY(float y);
void initPowerUp();
void createPowerUp(uint8_t type);
void movePowerUp();
void moveCulcPowerUp();
void collisionPowerUp(float playerX, float playerY);
void drawPowerUp();
void initSpring();
void setAllOneSpring();
void setAddSpring();
void setAddMaxSpring();
void setDelSpring();
void setOddSpring();
void collisionSpring(float playerX, bool &is_miss);
void moveSpring();
void moveCulcSpring();
void initSpeedScroll(int16_t s);
void speedupScroll();
void drawSpring();

void levelStart(int lvl)
{
  if (pMode == MODE_START) {
    lives = 3;
    stage = 1;
  }
  if (pMode == MODE_CONTINUE) {
    lives = 3;
  }

  if (lvl == 1) {
    arduboy.clear();
    arduboy.drawSlowXYBitmap(17, 10, bStart, 96, 48, 1);
    arduboy.display();
  }
  delay(2000);

  record = stage;
  initSpeedScroll(lvl % 10 + 1);

  pASpeed = 1 + ((lvl-1) % 10 * 0.2);
  resetKey();
  initPlayer(pASpeed);
  initSpring();
  initCoin(concurrent_coin_max);
  initPowerUp();
  initEnemy(lvl);
  initEnemyFire(1);
  initBall(lvl);
  initBullet(1);

  uint8_t n1 = 5 + lvl/10;
  uint8_t n2 = 10 + lvl/10;
  updateCoinNorma(n1, n2);

  pMode = MODE_RUNNING;
}


/*-------------------------------
    miss
  ------------------------------*/
void miss()
{
  if (!pMissing) {
    arduboy.display();
    pMissing = true;
    sound.tone(NOTE_C2, 160);
    delay(3000);

    initPowerUp();
    initSpring();
    lives--;
    if (lives <= 0) {
      pMode = MODE_TITLE;
    }
    initPlayer(pASpeed);
    pMissing = false;
  }
}


/*-------------------------------
    enemyAppears
  ------------------------------*/
void enemyAppears()
{
  arduboy.setCursor(5, 30);
  arduboy.setTextSize(1);
  arduboy.print("Enemy appears!");
  arduboy.display();
  initPlayer(pASpeed);

  // too hard
  /*
    if (stage >= 6) {
      initBall(stage);
    }
  */

  //  sound.tone(NOTE_A4, 200);
  sound.tones(songEnemyAppear);
  delay(4000);
}


/*-------------------------------
    stageClear
  ------------------------------*/
void stageClear()
{
  if (stage % 10 == 0) {
    sound.tones(songStageClear2);
  } else {
    sound.tones(songStageClear);
  }
  delay(3000);

  arduboy.clear();
  arduboy.setTextSize(1);
  arduboy.setCursor(36, 1);
  arduboy.print("Stage:");
  arduboy.print(stage);
  arduboy.drawSlowXYBitmap(17, 10, bClear, 96, 48, 1);
  arduboy.display();

  stage += 1;
  levelStart(stage);
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
  arduboy.setTextSize(1);
  arduboy.setCursor(60, 1);
  arduboy.write(' ');
  arduboy.write('S');
  arduboy.write(':');
  arduboy.print(stage);
  arduboy.write(' ');
  arduboy.write('C');
  arduboy.write(':');
  arduboy.print(getCoinQty());
  arduboy.write(' ');
}


/*-------------------------
    displayTitle
  ------------------------ */
void displayTitle()
{
  //sound.tones(songMain);

  int flash = 0;
  while (true) {
    delay( 30 );
    arduboy.clear();
    arduboy.setTextSize(2);
    arduboy.setCursor(3, 1);
    arduboy.print("JUMP BOY2");

    arduboy.drawSlowXYBitmap(14, 10, bTitle, 104, 36, 1);

    flash++;
    flash %= 50;


    if (flash < 25) {
      arduboy.setTextSize(1);
      arduboy.setCursor(30, 48);
      arduboy.print("Press A or B");
    }

    if (record > 0) {
      arduboy.setTextSize(1);
      arduboy.setCursor(1, 56);
      arduboy.print("Stage:");
      arduboy.print(record);
      arduboy.print(" ");
      arduboy.print("B:CONTINUE");
    }

    arduboy.display();

    if (arduboy.pressed(A_BUTTON)) {
      pMode = MODE_START;
      break;
    }
    if (arduboy.pressed(B_BUTTON)) {
      pMode = MODE_CONTINUE;
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

  if (pMode == MODE_CONTINUE) {
    stage = record;
    levelStart(stage);
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
    arduboy.clear();
    lTime = millis();

    drawHeader();

    moveSpring();
    moveCoin();
    movePowerUp();
    moveEnemy();
    moveEnemyFire();
    moveKey();
    moveBall();
    moveBullet();
    movePlayer();

    arduboy.display();
  }
}
/*----------------------
    Balls
  --------------------- */
const uint8_t CONCURRENT_BALL_MAX = 3;

struct Balls
{
  uint8_t inispCount = 0;
  uint8_t spCount = 0;
  int w = 8;
  int h = 8;
  float x = 64;
  float y = 16;
  int ax = -1;
  int no = 1;
  bool active = false;
  bool wait = false;
};
Balls balls[CONCURRENT_BALL_MAX];


void initBall(int no) {
  if (no > 3) {
    no = no % 3 + 1;
  }
  for (int i = 0; i < CONCURRENT_BALL_MAX; i++) {
    balls[i].inispCount = 100 + random(300);
    balls[i].no = no;
    balls[i].spCount = 0;
    balls[i].active = false;
    balls[i].wait = false;
    balls[i].x = 128;
    balls[i].y = 10 + random(34);
  }
}


void waitBall() {
  for (int i = 0; i < CONCURRENT_BALL_MAX; i++) {
    balls[i].wait = true;
    balls[i].active = false;
  }
}


void moveBall()
{
  for (int i = 0; i < CONCURRENT_BALL_MAX; i++) {
    if (balls[i].wait) {
      return;
    }
    balls[i].spCount++;
    if (balls[i].active) {
      balls[i].x += balls[i].ax;
      if (balls[i].x < 0) {
        balls[i].active = false;
      }
      if (collisionBullet(balls[i].x, balls[i].y, balls[i].w, balls[i].h)) {
        balls[i].active = false;
      }
      arduboy.drawSlowXYBitmap(balls[i].x, balls[i].y, bBird, balls[i].w, balls[i].h, 1);
    } else {
      if (balls[i].inispCount < balls[i].spCount) {
        balls[i].spCount = 0;
        balls[i].ax = -1 * random(balls[i].no) - 1;
        balls[i].inispCount = 100 + random(300);
        balls[i].x = 128;
        balls[i].active = true;
      }
    }
  }
}


void collisionBall(float playerX, float playerY, int playerW, int playerH)
{
  for (int i = 0; i < CONCURRENT_BALL_MAX; i++) {
    if (balls[i].active) {
      if (balls[i].x < playerX + playerW / 2 && playerX + playerW / 2 < balls[i].x + balls[i].w &&
          balls[i].y < playerY + playerH / 2 && playerY + playerH / 2 < balls[i].y + balls[i].h)
      {
        miss();
        balls[i].active = false;
      }
    }
  }
}
/*----------------------
    Bullets
  --------------------- */
const uint8_t CONCURRENT_BULLET_MAX = 1;

struct Bullets
{
  int w = 4;
  int h = 4;
  float x = 64;
  float y = 16;
  int ax = 4;
  int no = 1;
  bool active = false;
};
Bullets bullets[CONCURRENT_BULLET_MAX];


void initBullet(int no) {
  for (int i = 0; i < CONCURRENT_BULLET_MAX; i++) {
    bullets[i].no = no;
    bullets[i].active = false;
    bullets[i].x = 0;
    bullets[i].y = 0;
  }
}


void fireBullet(int x,int y){
  for (int i = 0; i < CONCURRENT_BULLET_MAX; i++) {
    if (!bullets[i].active) {
      sound.tone(NOTE_E4, 40);
      bullets[i].active = true;
      bullets[i].x = x;
      bullets[i].y = y;
      return;
    }
  }
}


void moveBullet()
{
  for (int i = 0; i < CONCURRENT_BULLET_MAX; i++) {
    if (bullets[i].active) {
      bullets[i].x += bullets[i].ax;
      if (bullets[i].x < 0 || bullets[i].x > 128) {
        bullets[i].active = false;
      }
      arduboy.drawSlowXYBitmap(bullets[i].x, bullets[i].y, bBullet, bullets[i].w, bullets[i].h, 1);
    }
  }
}


bool collisionBullet(float X, float Y, int W, int H)
{
  for (int i = 0; i < CONCURRENT_BULLET_MAX; i++) {
    if (bullets[i].active) {
      float bx = bullets[i].x + bullets[i].w / 2;
      float by = bullets[i].y + bullets[i].h / 2;
      if (X < bx && bx < X + W && Y < by && by < Y + H) {        
        bullets[i].active = false;
        sound.tone(NOTE_A3, 80);
        return true;
      }
    }
  }
  return false;
}
/*----------------------
    Coins:
  --------------------- */
const uint8_t CONCURRENT_COIN_MAX = 10;

struct Coin
{
  uint8_t spCount = 0;
  uint8_t qty = 0;
  uint8_t norma1 = 5;
  uint8_t norma2 = 10;
  uint8_t concurrent_coin_max = CONCURRENT_COIN_MAX;
} coin;


struct Coins
{
  float x = 64;
  float y = 16;
  bool active = false;
  uint8_t spCount = 0;
};
Coins coins[CONCURRENT_COIN_MAX];


void initCoin(uint8_t concurrent_coin_max)
{
  coin.spCount = 0;
  coin.qty = 0;
  coin.concurrent_coin_max = (concurrent_coin_max > CONCURRENT_COIN_MAX ) ? CONCURRENT_COIN_MAX : concurrent_coin_max;

  for (int i = 0; i < CONCURRENT_COIN_MAX; i++) {
    coins[i].active = false;
  }
}


void updateCurrentCoinMax(uint8_t concurrent_coin_max)
{
  coin.concurrent_coin_max = (concurrent_coin_max > CONCURRENT_COIN_MAX ) ? CONCURRENT_COIN_MAX : concurrent_coin_max;
}

void updateCoinNorma(uint8_t norma1,uint8_t norma2)
{
  coin.norma1 = norma1;
  coin.norma2 = norma2;
}

void resetSpCount()
{
  coin.spCount = 0;
}


void moveCoin()
{
  moveCulcCoin();
  drawCoin();
}


void moveCulcCoin()
{
  coin.spCount++;
  if (coin.spCount == 200)
  {
    coin.spCount = 0;
    updateCurrentCoinMax(1);
  }
  _createCoins();
  _checkCoinsLife();
}


void _createCoins()
{
  for (int i = 0; i < coin.concurrent_coin_max; i++) {
    if (coins[i].active == false) {
      int x = 10 + random(100);
      int y = 10 + random(34);

      if (!duplicatePlayerPosition(x, y)) {
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


void collisionCoins(float playerX, float playerY)
{
  for (int i = 0; i < coin.concurrent_coin_max; i++) {
    if (coins[i].active) {
      if (coins[i].x < playerX + 8 &&
          coins[i].x + 8 > playerX &&
          coins[i].y < playerY + 8 &&
          8 + coins[i].y > playerY)
      {
        sound.tone(NOTE_G3, 80);
        coin.qty++;
        coins[i].active = false;
        if (coin.qty == coin.norma2) {
          initKey();
        } else if (coin.qty == coin.norma1) {
          waitBall();
          enemyAppears();
          initSpring();
          activeEnemy();
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
    Enemy
  --------------------- */

struct Enemy
{
  int w = 16;
  int h = 16;
  float x = 128;
  float y = 32;
  int ax = 1;
  int ay = 1;
  int no = 1;
  bool active = false;
  float xspd = 0;
  float yspd = 0;
  uint8_t spCount = 0;
} enemy;


void initEnemy(int no) {
  if (no > 10) {
    no = no % 10;
  }
  enemy.no = no;
  enemy.spCount = 0;
  enemy.x = 100;
  enemy.y = 10 + random(34);
  enemy.active = false;
}


void activeEnemy() {
  enemy.active = true;
}


void moveEnemy()
{
  if (enemy.active) {
    enemy.spCount++;
    moveCulcEnemy();
    if (collisionBullet(enemy.x, enemy.y, enemy.w, enemy.h)) {
      enemy.spCount = 201;
    }
  }
}


void moveCulcEnemy()
{
  switch (enemy.no) {
    case 1:
      actionVirtical(1);
      break;
    case 2:
      actionHorizon(1);
      break;
    case 3:
      actionUpdown();
      break;
    case 4:
    case 5:
    case 6:
      actionReflection();
    case 7:
    case 8:
    case 9:
      actionSpeedUpReflection();
      break;
    case 10:
      actionUpdown();
      fireEnemyFireToTarget();
      break;
    default:
      actionUpdown();
      break;
  }
  arduboy.drawSlowXYBitmap(enemy.x, enemy.y, bEnemy[enemy.no - 1], enemy.w, enemy.h, 1);
}


void actionUpdown() {
  if (enemy.spCount % 2 != 0) {
    //stop
  }
  else if (enemy.spCount > 200) {
    //stop
  }
  else if (enemy.spCount > 400) {
    //restart
    enemy.spCount = 0;
  }
  else {
    int r = 1;
    enemy.y += r * enemy.ay;
    rangeMoveEnemy();
  }
}


void actionReflection() {
  if (enemy.spCount > 200) {
    //stop
  }
  else if (enemy.spCount > 350) {
    //restart
    enemy.spCount = 0;
  }
  else {
    int r = 1;
    enemy.x += r * enemy.ax;
    enemy.y += r * enemy.ay;
    rangeMoveEnemy();
  }
}


void actionHorizon(int r) {
  if (enemy.spCount > 200) {
    //stop
  }
  else if (enemy.spCount > 350) {
    //restart
    enemy.spCount = 0;
  }
  else {
    enemy.x += r * enemy.ax;
    if (rangeMoveEnemyX()) {
      enemy.y += r * enemy.ay;
    }
    rangeMoveEnemyY();
  }
}


void actionVirtical(int r) {
  if (enemy.spCount > 200) {
    //stop
  }
  else if (enemy.spCount > 350) {
    //restart
    enemy.spCount = 0;
  }
  else {
    enemy.y += r * enemy.ay;
    if (rangeMoveEnemyY()) {
      enemy.x += r * enemy.ax;
    }
    rangeMoveEnemyX();
  }
}


void actionSpeedUpReflection() {
  if (enemy.spCount > 200) {
    //stop
    fireEnemyFire(enemy.x, enemy.y + enemy.h/2, 4, 5);
  }
  else if (enemy.spCount > 350) {
    //restart
    enemy.spCount = 0;
  }
  else {
    int r = random(2);
    enemy.x += r * enemy.ax;
    enemy.y += r * enemy.ay;
    rangeMoveEnemy();
  }
}


void rangeMoveEnemy() {
  rangeMoveEnemyX();
  rangeMoveEnemyY();
}


bool rangeMoveEnemyX() {
  if (enemy.x < 80) {
    enemy.ax = 1;
    return true;
  }
  if (enemy.x > 128 - enemy.w - 8) {
    enemy.ax = -1;
    return true;
  }
  return false;
}


bool rangeMoveEnemyY() {
  if (enemy.y < 10) {
    enemy.ay = 1;
    return true;
  }
  if (enemy.y > 64 - 10 - enemy.h) {
    enemy.ay = -1;
    return true;
  }
  return false;
}


void fireEnemyFireToTarget() {
  if (collisionPlayerPositionY(enemy.y)) {    
    fireEnemyFire(enemy.x, enemy.y + enemy.h/2, 2, 50);
  }
}


void collisionEnemy(float playerX, float playerY, int playerW, int playerH)
{
  if (enemy.active) {
    if (enemy.x < playerX + playerW / 2 && playerX + playerW / 2 < enemy.x + enemy.w &&
        enemy.y < playerY + playerH / 2 && playerY + playerH / 2 < enemy.y + enemy.h)
    {
      miss();
    }
  }
}
/*----------------------
    EnemyFire
  --------------------- */
const uint8_t CONCURRENT_ENEMY_FIRE_MAX = 1;

struct EnemyFire
{
  int w = 4;
  int h = 4;
  float x = 64;
  float y = 16;
  int ax = -2;
  int no = 1;
  int xrange = 0;
  bool active = false;
};
EnemyFire enemy_fire[CONCURRENT_ENEMY_FIRE_MAX];


void initEnemyFire(int no) {
  for (int i = 0; i < CONCURRENT_ENEMY_FIRE_MAX; i++) {
    enemy_fire[i].no = no;
    enemy_fire[i].active = false;
    enemy_fire[i].ax = -2;
    enemy_fire[i].xrange = 0;
    enemy_fire[i].x = 0;
    enemy_fire[i].y = 0;
  }
}


void fireEnemyFire(int x, int y, int ax, int l) {
  for (int i = 0; i < CONCURRENT_ENEMY_FIRE_MAX; i++) {
    if (!enemy_fire[i].active) {
      sound.tone(NOTE_C3, 40);
      enemy_fire[i].ax = -1 * ax;
      enemy_fire[i].active = true;
      enemy_fire[i].x = x;
      enemy_fire[i].y = y;
      enemy_fire[i].xrange = x + enemy_fire[i].ax * l;
      return;
    }
  }
}


void moveEnemyFire()
{
  for (int i = 0; i < CONCURRENT_ENEMY_FIRE_MAX; i++) {
    if (enemy_fire[i].active) {
      enemy_fire[i].x += enemy_fire[i].ax;
      if (enemy_fire[i].x < 0 || enemy_fire[i].x > 128) {
        enemy_fire[i].active = false;
      }

      if (enemy_fire[i].x < enemy_fire[i].xrange) {
        enemy_fire[i].active = false;
      }

      if (enemy_fire[i].x < 0 || enemy_fire[i].x > 128) {
        enemy_fire[i].active = false;
      }

      arduboy.drawSlowXYBitmap(enemy_fire[i].x, enemy_fire[i].y, bEnemyFire, enemy_fire[i].w, enemy_fire[i].h, 1);
    }
  }
}


void collisionEnemyFire(float X, float Y, int W, int H)
{
  for (int i = 0; i < CONCURRENT_ENEMY_FIRE_MAX; i++) {
    if (enemy_fire[i].active) {
      float bx = enemy_fire[i].x + enemy_fire[i].w / 2;
      float by = enemy_fire[i].y + enemy_fire[i].h / 2;
      if (X < bx && bx < X + W && Y < by && by < Y + H) {
        enemy_fire[i].active = false;
        miss();
        return;
      }
    }
  }
  return;
}
/*----------------------
    Key
  --------------------- */

struct Key
{
  int w = 8;
  int h = 8;
  float x = 120;
  float y = 32;
  bool active = false;
} key;


void initKey() {
  key.x = 10 + 110;
  key.y = 10 + random(34);
  key.active = true;
}


void resetKey() {
  key.active = false;
}


void moveKey()
{
  if (key.active) {
    arduboy.drawSlowXYBitmap(key.x, key.y, bKey, 8, 8, 1);
  }
}


void collisionKey(float playerX, float playerY, int playerW, int playerH)
{
  if (key.active) {
    if (key.x < playerX + playerW / 2 && playerX + playerW / 2 < key.x + key.w &&
        key.y < playerY + playerH / 2 && playerY + playerH / 2 < key.y + key.h)
    {
      stageClear();
      key.active = false;
    }
  }
}
/*----------------------
    Player
  --------------------- */

struct Player
{
  bool active = false;
  int w = 8;
  int h = 8;
  float x = 128;
  float y = 32;
  float ax = 1;
  float ay = 1;
} player;


void initPlayer(float p) {
  player.x = 10;
  player.y = 20;
  player.ax = 1.5;
  player.ay = p;
  player.active = true;
}


void movePlayer()
{
  if (player.active) {
    moveCulcPlayer();
    arduboy.drawSlowXYBitmap(player.x, player.y, bMan, player.w, player.h, 1);
    collisionPlayer();
  }
}


void moveCulcPlayer()
{
  if (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)) {
    fireBullet(player.x, player.y);
  }

  if (arduboy.pressed(LEFT_BUTTON)) {
    player.x -= player.ax;
  } else if (arduboy.pressed(RIGHT_BUTTON)) {
    player.x += player.ax;
  }
  // Drop.
  player.y += player.ay;

  // Range Check.
  if (player.y  > 56 - player.h) {
    player.y  = 56 - player.h;
  }
  if (player.y < boundY) {
    player.y  = boundY;
  }
  if (player.x > 120) {
    player.x = 120;
  }
  if (player.x < 8) {
    player.x = 8;
  }
}


void collisionPlayer()
{
  bool is_miss = false;
  if (player.y >= 56 - player.h) {
    player.ay = -1 * player.ay;
    collisionSpring(player.x, is_miss);
    if (is_miss) {
      miss();
    }
  }
  if (player.y <= 12) {
    player.ay = -1 * player.ay;
  }
  collisionPowerUp(player.x, player.y);
  collisionCoins(player.x, player.y);
  collisionKey(player.x, player.y, player.w, player.h);
  collisionEnemy(player.x, player.y, player.w, player.h);
  collisionEnemyFire(player.x, player.y, player.w, player.h);
  collisionBall(player.x, player.y, player.w, player.h);
}


bool duplicatePlayerPosition(float x, float y)
{
  return (player.x - player.w < x && x < player.x + player.w * 2);
}

bool collisionPlayerPositionY(float y)
{
  return (player.y - player.h < y && y < player.y + player.h * 2);  
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
  powerup.x = 10 + random(110);
  powerup.y = 10 + random(34);
  if (duplicatePlayerPosition(powerup.x, powerup.y)) {
    return;
  }
  powerup.xspd = 1;
  powerup.yspd = 1.5;
  powerup.type = type;
  powerup.active = true;
}


void movePowerUp()
{
  moveCulcPowerUp();
  drawPowerUp();
}


void moveCulcPowerUp()
{
  powerup.spCount++;
  if (powerup.spCount == 50) {
    powerup.active = false;
  }
  if (powerup.spCount == 200) {
    powerup.spCount = 0;
    uint8_t r = random(100);
    uint8_t item[] = {5, 20, 30, 45, 50};
    for (int i = 0; i < 5; i++) {
      if (r <= item[i]) {
        createPowerUp(i);
        break;
      }
    }
  }
}


void collisionPowerUp(float playerX, float playerY)
{
  if (powerup.active) {
    if (powerup.x < playerX + 8 &&
        powerup.x + 8 > playerX &&
        powerup.y < playerY + 8 &&
        8 + powerup.y > playerY)
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
void collisionSpring(float playerX, bool &is_miss)
{
  sound.tone(NOTE_A1, 80);

  is_miss = false;
  int i = 0;
  for (i = 0; i < 16; i++)
  {
    int16_t x = pScroll - (i * 8);
    if (x < 0) {
      x = 128 + x;
    }
    x += 4;
    if (playerX - 4 <= x && x <= playerX + 12 && pSprings[i] > 0) {
      pSprings[i] -= 1;
      is_miss = false;
      break;
    }
  }
  if (i == 16) {
    is_miss = true;
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
  moveCulcSpring();
  drawSpring();
}

/*-----------------------
    moveCulcSpring
  ----------------------*/
void moveCulcSpring()
{
  pScroll -= pScrollSpeed;
  if (pScroll < 0) {
    pScroll = 127;
  }
}

/*-----------------------
    initSpeedScroll
  ----------------------*/
void initSpeedScroll(int16_t s)
{
  pScrollSpeed = s;
}

/*-----------------------
    speedupScroll
  ----------------------*/
void speedupScroll()
{
  pScrollSpeed++;
  if (pScrollSpeed > 5) {
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
