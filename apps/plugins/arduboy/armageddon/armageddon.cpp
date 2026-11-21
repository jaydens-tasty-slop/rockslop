#include <stdlib.h>
#include <math.h>
#include <SPI.h>
//Use for PROGMEM
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <Gamebuino.h>
#include "../lib/ab_math.h"
Gamebuino gb;

const byte armageddon[] PROGMEM = {64,36,
0b11111111,0b11110000,0b00000000,0b00000000,0b00000000,0b00000000,0b00001111,0b11111111,
0b11111100,0b00001111,0b11111111,0b11111111,0b11111111,0b11111111,0b11110000,0b00111111,
0b11100011,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11000111,
0b10011111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111001,
0b01110111,0b10011110,0b11101110,0b11111001,0b11000010,0b01111001,0b11110111,0b10111010,
0b01110111,0b10101110,0b01001110,0b11110110,0b11011110,0b10111010,0b11101011,0b10011010,
0b01101011,0b10001110,0b01001101,0b01110111,0b11000110,0b11011011,0b01011101,0b10101010,
0b01100011,0b10101110,0b10101100,0b01110100,0b11011110,0b11011011,0b01011101,0b10110010,
0b01101011,0b10100110,0b11101101,0b01110110,0b11011110,0b10111010,0b11101011,0b10111010,
0b01001001,0b10110110,0b11101001,0b00111001,0b11000010,0b01111001,0b11110111,0b10111010,
0b10111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111101,
0b11011111,0b11111111,0b11100000,0b11111111,0b11111111,0b00000111,0b11111111,0b11111011,
0b11100011,0b11111110,0b00011111,0b00011111,0b11111000,0b11111000,0b01111111,0b11000111,
0b11111000,0b11100001,0b11111111,0b11101111,0b11110111,0b11111111,0b10000111,0b00011111,
0b11111110,0b00001111,0b11111111,0b11110111,0b11101111,0b11111111,0b11110000,0b01111111,
0b11111111,0b11111111,0b11111111,0b11110111,0b11101111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11111011,0b11011111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11111011,0b11011111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111110,0b00000011,0b11000000,0b01111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11100001,0b11111011,0b11011111,0b10000111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11011111,0b11110011,0b11001111,0b11111011,0b11111111,0b11111111,
0b11111111,0b11111111,0b11011111,0b11110011,0b11001111,0b11111011,0b11111111,0b11111111,
0b11111111,0b11111111,0b11011111,0b11110011,0b11001111,0b11111011,0b11111111,0b11111111,
0b11111111,0b11111111,0b11100001,0b11110011,0b11001111,0b10000111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111110,0b00010111,0b11101000,0b01111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11100000,0b00000111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11110111,0b11101111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11100111,0b11100111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11100111,0b11100111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11101111,0b11110111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11101111,0b11110111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11101111,0b11110111,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b11011111,0b11111011,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111111,0b10111111,0b11111101,0b11111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111110,0b01111111,0b11111110,0b01111111,0b11111111,0b11111111,
0b11111111,0b11111111,0b11111101,0b11111111,0b11111111,0b10111111,0b11111111,0b11111111,
};

/*const byte city[] PROGMEM = {8,8,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00100010,
0b01110110,
0b01111110,
0b11111111,
};*/

const byte city[] PROGMEM = {8,8,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b01100100,
0b01101110,
0b01111111,
0b11111111,
};

const byte deadcity[] PROGMEM = {8,8,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00100000,
0b01100010,
};

const byte launcher[] PROGMEM = {8,8,
0b00011000,
0b00011000,
0b00011000,
0b00100100,
0b00100100,
0b01000010,
0b01000010,
0b10000001,
};

const byte deadlauncher[] PROGMEM = {8,8,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b01100110,
0b01011010,
0b10000001,
};

#define SOUND_PLAUNCH 0
#define SOUND_ELAUNCH 1
#define SOUND_DETONATE 2
#define SOUND_SCORE 3
#define SOUND_DEAD 4
#define SOUND_LOSE 5

#define MODE_PREGAME 0
#define MODE_GAME 1 
#define MODE_LULL 2
#define MODE_DEAD 3
#define MODE_POSTDEAD 4

#define TARGET_SPEED 3

#define LAUNCHER_ONE 1
#define LAUNCHER_TWO 2
#define MAX_PMISSILES 10
#define PSPEED 4
#define PRADIUS 7
#define EXPAND 0
#define SHRINK 1

#define MAX_EMISSILES 10
#define MAX_CHANCE 50

// State variables
uint8_t mode = 0;
uint8_t counter = 0;
uint8_t flash = 0;
uint8_t stage = 0; //Maximum of 255 stages
uint32_t score = 0; //Score needs more space than other vars

uint8_t lullMissiles = 0;
uint8_t lullCities[8] = {0,0,0,0,0,0,0,0};

uint8_t cities[8] = {1,1,1,1,1,1,1,1}; //Whether the cities or launchers are alive

uint8_t targetX = 84/2;
uint8_t targetY = 48/2;
uint8_t pammo[2] = {10,10};
uint8_t pDests[MAX_PMISSILES][2] = {{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{100,100}};
float pMissiles[MAX_PMISSILES][3] = {{100,100,0},{100,100,0},{100,100,0},{100,100,0},{100,100,0},{100,100,0},{100,100,0},{100,100,0},{100,100,0},{100,100,0}};
uint8_t pDetonations[MAX_PMISSILES][4] = {{100,100,0,0},{100,100,0,0},{100,100,0,0},{100,100,0,0},{100,100,0,0},{100,100,0,0},{100,100,0,0},{100,100,0,0},{100,100,0,0},{100,100,0,0}};

uint8_t etotal = 5;
uint8_t echance = 1;
float espeed = 0.2;
uint8_t eDests[MAX_EMISSILES] = {100,100,100,100,100,100,100,100,100,100};
float eMissiles[MAX_EMISSILES][4] = {{100,100,100,100},{100,100,100,100},{100,100,100,100},{100,100,100,100},{100,100,100,100},{100,100,100,100},{100,100,100,100},{100,100,100,100},{100,100,100,100},{100,100,100,100}};

void nextStage();
void nextLull();
void drawScore();
void drawTargets();
void drawCities();
void drawAmmo();
void drawMissiles();
void drawDetonations();
void launchMissile(uint8_t launcher);
void tryLaunchEnemy();
void stepMissiles();
void stepDetonations();
void stepCollision();
void checkMenu();
void checkWin();
void checkLose();
void stepGame();
void drawLull();
void stepLull();
void stepDead();
void stepPregame();
void loadHighscores();
uint8_t isHighscore(uint32_t score);
void saveHighscore(uint32_t score, char* who);
void drawHighscores();
void initSound();
void playSound(uint8_t i);

void setup() {
  gb.begin();
  loadHighscores();
  gb.titleScreen(armageddon);
  gb.battery.show = false;
  gb.pickRandomSeed();
}

void nextStage(){
  stage++;
  //Reset cities (since we cleared them for the visual effect
  for( uint8_t i = 0; i < 8; i++ ){
    cities[i] = lullCities[i];
  }
  //Reset launchers
  cities[2] = 1;
  cities[5] = 1;
  //Reset missiles
  pammo[0] = 10;
  pammo[1] = 10;
  //Reset in-flight missiles
  for( uint8_t i = 0; i < 10; i++ ){
    pDests[i][0] = 100;
    pDetonations[i][0] = 100;
    eDests[i] = 100;
  }

  etotal = stage > 10 ? 20 : 10+stage; //Max of 20 missiles per stage, start at 10
  //echance = stage > 5 ? 5 : stage; //Max of 5/100 chance per frame
  espeed = stage > 18 ? 2 : 0.2+(stage*0.1); //Max speed of 2, up 0.1 per stage, starts at 0.2
}

void nextLull(){
  //Reset lullMissiles
  lullMissiles = 0;
  //Reset lullCities
  for( uint8_t i = 0; i < 8; i++ ){
    lullCities[i] = 0;
  }
}

void drawScore(){
  gb.display.cursorX = 84/2 - 4*3;
  gb.display.cursorY = 0;

  if( score < 100000 ){
    gb.display.print(0);
  }
  if( score < 10000 ){
    gb.display.print(0);
  }
  if( score < 1000 ){
    gb.display.print(0);
  }
  if( score < 100 ){
    gb.display.print(0);
  }
  if( score < 10 ){
    gb.display.print(0);
  }
  gb.display.print(score);
}

void drawTargets(){
  gb.display.drawFastHLine(targetX-1,targetY,3);
  gb.display.drawFastVLine(targetX,targetY-1,3);

  for(uint8_t i = 0; i < MAX_PMISSILES; i++){
    //Check for a valid destination without a current detonation
    if( pDests[i][0] <= 84 && pDetonations[i][0] > 84 ){
      gb.display.drawPixel(pDests[i][0], pDests[i][1]);
    }
  }
}

void drawCities(){
  //uint8_t alldead = 1;
  for(uint8_t i = 0; i < 8; i++){
      if( i == 2 || i == 5 ){
        if( cities[i] ){
          gb.display.drawBitmap(i*10+2,40,launcher);
        }else{
          gb.display.drawBitmap(i*10+2,40,deadlauncher);
        }
      }else{
        if( cities[i] ){
          //alldead = 0;
          gb.display.drawBitmap(i*10+2,40,city);
        }else{
          gb.display.drawBitmap(i*10+2,40,deadcity);
        }
      }
    }

    /*
    if( alldead ){
      gb.display.cursorX = 84/2 - 5*3;
      gb.display.cursorY = 48/2 - 5;
      gb.display.print("THE END");
    }
    */
}

void drawAmmo(){
  for( uint8_t i = 0; i < 2; i++ ){
    uint8_t xcoord = i == 0 ? 25 : 55;
    uint8_t ycoord = 47;
    if( cities[i*3+2] ){ //Is launcher alive?
      for( uint8_t j = 0; j < pammo[i]; j++ ){
        gb.display.drawPixel(xcoord,ycoord);
        if( xcoord % 2 == 0 ){
          xcoord--;
          ycoord--;
        }else{
          xcoord++;
        }
      }
    }
  }
}

void drawMissiles(){
  //Player Missiles
  for(uint8_t i = 0; i < MAX_PMISSILES; i++){
    //Check for a valid destination without a current detonation
    if( pDests[i][0] <= 84 && pDetonations[i][0] > 84 ){
      if(pMissiles[i][2] == LAUNCHER_ONE){
        gb.display.drawLine(25,40,pMissiles[i][0], pMissiles[i][1]);
      }else{
        gb.display.drawLine(56,40,pMissiles[i][0], pMissiles[i][1]);
      }
    }
  }

  //Enemy Missiles
  for(uint8_t i = 0; i < MAX_EMISSILES; i++){
    //Check for a valid destination
    if( eDests[i] <= 84 ){
        gb.display.drawLine(eMissiles[i][0],eMissiles[i][1],eMissiles[i][2], eMissiles[i][3]);
    }
  }
}

void drawDetonations(){
  for(uint8_t i = 0; i < MAX_PMISSILES; i++){
    if( pDetonations[i][0] <= 84 ){
      gb.display.drawCircle(pDetonations[i][0],pDetonations[i][1],pDetonations[i][2]);
    }
  }
}

void launchMissile(uint8_t launcher){
  //If there is no free slot in pDests, do not launch
  for(uint8_t i = 0; i < MAX_PMISSILES; i++){
    if( pDests[i][0] > 84 ){
      // Which launcher?             Alive?       Has ammo?
      if(launcher == LAUNCHER_ONE && cities[2] && pammo[0]){
        pDests[i][0] = targetX;
        pDests[i][1] = targetY;
        pMissiles[i][0] = 25; //X-coord of left launcher
        pMissiles[i][2] = LAUNCHER_ONE; //Launched from launcher one
        pammo[0]--;
        playSound(SOUND_PLAUNCH);
      }else if(launcher == LAUNCHER_TWO && cities[5] && pammo[1]){
        pDests[i][0] = targetX;
        pDests[i][1] = targetY;
        pMissiles[i][0] = 56; //X-coord of right launcher
        pMissiles[i][2] = LAUNCHER_TWO; //Launched from launcher two
        pammo[1]--;
        playSound(SOUND_PLAUNCH);
      }
      pMissiles[i][1] = 40; //Y-coord of both launchers
      break;
    }
  }
}

void tryLaunchEnemy(){
  uint8_t someActive = 0;

  if( etotal > 0 ){
  
    // Check whether there are any active missiles
    // If none are active, always spawn one to avoid
    // long pauses without any enemy missiles
    for(uint8_t i = 0; i < MAX_EMISSILES; i++){
      if( eDests[i] <= 84 ){
        someActive = 1;
        break;
      }
    }
    
    if( (!someActive || echance >= random(100)) ){ //echance of 100
      for(uint8_t i = 0; i < MAX_EMISSILES; i++){
        if( eDests[i] > 84 ){
          etotal--;
          eDests[i] = random(8); //Target one of the 6 cities or 2 launch sites
          eMissiles[i][0] = random(84); //Screen width
          eMissiles[i][1] = 0; //Top of screen
          eMissiles[i][2] = eMissiles[i][0]; //Start and end are same
          eMissiles[i][3] = 0; //Top of screen
          playSound(SOUND_ELAUNCH);
          break; //Only spawn one
        }
      }
    }

  }
}

void stepMissiles(){
  //Player Missiles
  for(uint8_t i = 0; i < MAX_PMISSILES; i++){
    //Check for a valid destination without a current detonation
    if( pDests[i][0] <= 84 && pDetonations[i][0] > 84 ){
      //If the missile is close enough to the destination, detonate
      if( abs( pDests[i][0] - pMissiles[i][0] ) < PSPEED && abs( pDests[i][1] - pMissiles[i][1] ) < PSPEED ){
        pDetonations[i][0] = pDests[i][0];
        pDetonations[i][1] = pDests[i][1];
        pDetonations[i][2] = 0; //Start detonation at radius of 0
        pDetonations[i][3] = EXPAND; //Detonation is increasing in size
      //Otherwise, keep moving towards destination
      }else{
        float dir = atan2(pDests[i][1]-pMissiles[i][1], pDests[i][0]-pMissiles[i][0]);
        pMissiles[i][0] += PSPEED * cos(dir);
        pMissiles[i][1] += PSPEED * sin(dir);
      }
    }
  }

  //Enemy Missiles
  for(uint8_t i = 0; i < MAX_EMISSILES; i++){
    //Check for a valid destination
    if( eDests[i] <= 84 ){
       //If enemy missile is close enough to the destination, detonate
      if( abs( (eDests[i]*10+6) - eMissiles[i][2] ) < PSPEED && abs( 44 - eMissiles[i][3] ) < PSPEED ){
        cities[eDests[i]] = 0; //Destroy city/launcher
        
        //If launcher, remove its ammo
        if( eDests[i] == 2 ){
          pammo[0] = 0;
        }
        if( eDests[i] == 5 ){
          pammo[1] = 0;
        }
        
        eDests[i] = 100; //Reset enemy missile
        playSound(SOUND_DEAD);
      //Otherwise, keep moving towards destination
      }else{
        float dir = atan2( 44-eMissiles[i][3], (eDests[i]*10+6)-eMissiles[i][2] );
        eMissiles[i][2] += espeed * cos(dir);
        eMissiles[i][3] += espeed * sin(dir);
      }
    }
  }
}

void stepDetonations(){
  if( counter%2 == 0 ){
    for(uint8_t i = 0; i < MAX_PMISSILES; i++){
      if( pDetonations[i][0] <= 84 ){
        if( pDetonations[i][3] == EXPAND ){ //If detonation is expanding
          if( pDetonations[i][2] < PRADIUS ){
            pDetonations[i][2]++;
          }else{
            pDetonations[i][3] = SHRINK; //Start shrinking
          }
        }
        //Check this now instead of using else because it may have just
        //set to SHRINK; immediately checking will remove a delay at full
        //size.  If a delay is desired, this if can be switched to an else.
        if( pDetonations[i][3] == SHRINK ){ //If detonation is shrinking
          if( pDetonations[i][2] > 0 ){
            pDetonations[i][2]--;
          }else{
            pDetonations[i][0] = 100; //Detonation is complete, remove it
            pDests[i][0] = 100; //Remove this destination
          }
        }
      }
    }
  }
}

void stepCollision(){
  //Player Detonations with Enemy Missiles
  for(uint8_t i = 0; i < MAX_PMISSILES; i++){
      if( pDetonations[i][0] <= 84 ){
        for(uint8_t j = 0; j < MAX_EMISSILES; j++){
          if( eDests[j] <= 84 && pDetonations[i][2] >= sqrt( (eMissiles[j][2]-pDetonations[i][0])*(eMissiles[j][2]-pDetonations[i][0]) + (eMissiles[j][3]-pDetonations[i][1])*(eMissiles[j][3]-pDetonations[i][1]) ) ){
            eDests[j] = 100; //Remove enemy missile
            score+=25;
            playSound(SOUND_DETONATE);
          }
        }
      }
  }
}


void checkMenu(){
  if( gb.buttons.pressed(BTN_C) ){
    gb.titleScreen(armageddon);
    gb.battery.show = false;
  }
}

void checkWin(){
  if( etotal == 0 ){
    for(uint8_t i = 0; i < MAX_EMISSILES; i++){
      //Check for a valid destination
      if( eDests[i] <= 84 ){
        return;
      }
    }
    //If we get here, all enemy missiles are destroyed and they have no more
    nextLull();
    mode = MODE_LULL;
  }
}

void checkLose(){
  for(uint8_t i = 0; i < 8; i++){
    if( i != 2 && i != 5 ){
      if( cities[i] ){
        return; // A city remains alive
      }
    }
  }
  //If we get here, all cities are dead.  It is the end of days.
  counter = 0;
  mode = MODE_DEAD;
  playSound(SOUND_LOSE);
}

void stepGame(){
  //Player input
  if( gb.buttons.pressed(BTN_A) ){
    launchMissile(LAUNCHER_ONE);
  }
  if( gb.buttons.pressed(BTN_B) ){
    launchMissile(LAUNCHER_TWO);
  }
  
  if( gb.buttons.repeat(BTN_LEFT,1) ){
    targetX = targetX-TARGET_SPEED > 0 ? targetX-TARGET_SPEED : 0;
  }
  if( gb.buttons.repeat(BTN_RIGHT,1) ){
    targetX = targetX+TARGET_SPEED < 84 ? targetX+TARGET_SPEED : 84;
  }
  if( gb.buttons.repeat(BTN_UP,1) ){
    targetY = targetY-TARGET_SPEED > 0 ? targetY-TARGET_SPEED : 0;
  }
  if( gb.buttons.repeat(BTN_DOWN,1) ){
    targetY = targetY+TARGET_SPEED < 48 ? targetY+TARGET_SPEED : 48;
  }

  //Game logic
  tryLaunchEnemy();
  stepMissiles();
  stepDetonations();
  stepCollision();

  //Drawing
  drawScore();
  drawTargets();
  drawCities();
  drawAmmo();
  drawMissiles();
  drawDetonations();

  checkWin();
  checkLose();
}

void drawLull(){
  uint8_t cityCount = 0;
  gb.display.cursorX = 84/2 - 4*6;
  gb.display.cursorY = 48/2 - 5*3;
  gb.display.print(F("BONUS POINTS"));

  gb.display.cursorX = 84/2 - 4*8;
  gb.display.cursorY += 5*2;
  gb.display.print(lullMissiles);

  for(uint8_t i = 0; i < lullMissiles; i++){
    gb.display.drawPixel(84/2 - 4*6 + i*2,48/2 - 3);
  }

  gb.display.cursorX = 84/2 - 4*8;
  gb.display.cursorY += 5*2;
  for( uint8_t i; i < 8; i++ ){
    if( lullCities[i] ) cityCount++;
  }
  gb.display.print(cityCount);

  for(uint8_t i = 0; i < cityCount; i++){
    gb.display.drawBitmap(84/2 - 4*6 + i*9,48/2+2, city);
  }

  drawScore();
  drawCities();
  drawAmmo();
}

void stepLull(){
  
  if( counter % 4 == 0 && (pammo[0] > 0 || pammo[1] > 0) ){
    lullMissiles++;
    score+=10;
    if( pammo[0] > 0 ) pammo[0]--;
    else pammo[1]--;
    playSound(SOUND_SCORE);
  }

  //If we have already iterated through the missiles
  if( counter % 8 == 0 && pammo[0] == 0 && pammo[1] == 0 ){
    for( uint8_t i = 0; i < 9; i++ ){
      if( i == 8 ){ //We have iterated through all live cities
        nextStage();
        mode = MODE_GAME;
        return;
      }
      //If not a launcher and the city is alive
      if( i != 2 && i != 5 && cities[i] != 0 ){
        lullCities[i] = 1;
        score+=100;
        cities[i] = 0;
        playSound(SOUND_SCORE);
        break;
      }
    }
  }

  drawLull();
}

void stepDead(){
  gb.display.cursorX = 84/2 - 5*3;
  gb.display.cursorY = 48/2 - 5;
  gb.display.print(F("THE END"));

  if( mode == MODE_DEAD && counter%20 == 0 ){
    mode = MODE_POSTDEAD;
  }else if( mode == MODE_POSTDEAD ){
    if( counter%8 == 0 ){
      flash ^= 255;
    }
    if( flash ){
      gb.display.cursorX = 84/2 - 5*3;
      gb.display.cursorY = 48 - 9;
      gb.display.print(F("PRESS \25"));

      if( isHighscore(score) ){
        gb.display.cursorX = 84/2 - 5*5 - 2;
        gb.display.cursorY = 48 - 15;
        gb.display.print(F("NEW HIGHSCORE"));
      }
    }

    if( gb.buttons.pressed(BTN_A) ){
      if( isHighscore(score) ){
        char tmp_name[11];
        gb.getDefaultName(tmp_name);
        gb.keyboard(tmp_name, 11);
        saveHighscore(score,tmp_name);
      }
      score = 0;
      mode = MODE_PREGAME;
    }
  }
}

void stepPregame(){
  drawHighscores();

  if( counter%8 == 0 ){
    flash ^= 255;
  }
  if( flash ){
    gb.display.cursorX = 84/2 - 5*3;
    gb.display.cursorY = 48 - 9;
    gb.display.print(F("PRESS \25"));
  }

  if( gb.buttons.pressed(BTN_A) ){
    stage = 255;
    for( uint8_t i = 0; i < 8; i++ ){
      lullCities[i] = 1;
    }
    nextStage(); //Reset to stage 0
    mode = MODE_GAME;
  }
}

void loop() {
  if(gb.update()){

    switch( mode ){
      case MODE_GAME:
        stepGame();
        break;

      case MODE_LULL:
        stepLull();
        break;

      case MODE_DEAD:
      case MODE_POSTDEAD:
        stepDead();
        break;

      case MODE_PREGAME:
        stepPregame();
        break;
    }

    checkMenu();

    counter++;
  }
}
#define NUM_HIGHSCORES 5
#define NAME_SIZE 10
#define ENTRY_SIZE 15

uint32_t highscores[NUM_HIGHSCORES];
char names[NUM_HIGHSCORES][NAME_SIZE+1];

void loadHighscores(){
  for( uint8_t entry = 0; entry < NUM_HIGHSCORES; entry++ ){
    for( uint8_t offset = 0; offset < ENTRY_SIZE; offset++ ){
      if( offset < NAME_SIZE ){
        names[entry][offset] = EEPROM.read( ENTRY_SIZE * entry + offset );
      }else{
        uint8_t* addr = (uint8_t*) &highscores[entry];
        addr+=offset-NAME_SIZE;
        *addr = EEPROM.read( ENTRY_SIZE * entry + offset );
      }
    }
  }
}

uint8_t isHighscore(uint32_t score){
  if( score > highscores[NUM_HIGHSCORES-1] ){
    return 1;
  }
  return 0;
}

void saveHighscore(uint32_t score, char* who){
  uint8_t found = 0;
  uint32_t tmp_score = 0;
  char tmp_name[10];
  for( uint8_t entry = 0; entry < NUM_HIGHSCORES; entry++ ){
    if( score > highscores[entry] ){
      found = 1;
    }
    if( found ){
      tmp_score = highscores[entry];
      strcpy(tmp_name,names[entry]);

      highscores[entry] = score;
      strcpy(names[entry], who);

      score = tmp_score;
      strcpy(who, tmp_name);
    }
  }
  for( uint8_t entry = 0; entry < NUM_HIGHSCORES; entry++ ){
    for( uint8_t offset = 0; offset < ENTRY_SIZE; offset++ ){
      if( offset < NAME_SIZE ){
        EEPROM.write( ENTRY_SIZE * entry + offset, names[entry][offset] );
      }else{
        uint8_t* addr = (uint8_t*) &highscores[entry];
        addr+=offset-NAME_SIZE;
        EEPROM.write( ENTRY_SIZE * entry + offset, *addr );
      }
    }
  }
}

void drawHighscores(){
  gb.display.cursorX = 84/2 - 5*4;
  gb.display.cursorY = 5;
  gb.display.print(F("HIGHSCORES"));

  for( uint8_t entry = 0; entry < NUM_HIGHSCORES; entry++ ){
    gb.display.cursorX = 5;
    gb.display.cursorY = 12 + 5*entry;
    gb.display.print(names[entry]);
    gb.display.print(F(" "));
    gb.display.cursorX = 84 - 5*6;
    if( highscores[entry] < 100000 ){
      gb.display.print(0);
    }
    if( highscores[entry] < 10000 ){
      gb.display.print(0);
    }
    if( highscores[entry] < 1000 ){
      gb.display.print(0);
    }
    if( highscores[entry] < 100 ){
      gb.display.print(0);
    }
    if( highscores[entry] < 10 ){
      gb.display.print(0);
    }
    gb.display.print(highscores[entry]);
  }
}

// Wave, pitch, duration, arpeggio step duration, arpeggio step size
const int8_t sounds[][5] = {
  { 0, 20, 5, 1, 1 }, //Player launch
  { 0, 25, 5, 1, -1 }, //Enemy launch
  { 1, 10, 5, 1, -1 }, //Detonating enemy missile
  { 1, 10, 2, 0, 0 }, //Score pips
  { 1, 2, 10, 1, -1 }, //A city dies
  { 0, 20, 14, 3, -1 }, //Lose
};

void initSound(){
  gb.sound.command(CMD_VOLUME, 5, 0, 0);
  gb.sound.command(CMD_SLIDE, 0, 0, 0);
}

void playSound(uint8_t i){
  gb.sound.command(CMD_VOLUME, 5, 0, 0);
  gb.sound.command(CMD_SLIDE, 0, 0, 0);
  gb.sound.command(CMD_ARPEGGIO, sounds[i][3], sounds[i][4], 0); 
  gb.sound.command(CMD_INSTRUMENT, sounds[i][0], 0, 0);
  gb.sound.playNote(sounds[i][1], sounds[i][2], 0);

  //gb.sound.playNote(20, 5, 0);
}

