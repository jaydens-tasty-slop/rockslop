/*
    keykat-it: Turn the computers off and back on again!
    Copyright (C) 2017 Robert Lowe <pngwen@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <Arduboy2.h>
#include <ArduboyPlaytune.h>

#include "assets.h"
Arduboy2 arduboy;
ArduboyPlaytune tunes(arduboy.audio.enabled);

//button state global
bool btnup;
bool btndown;
bool btnleft;
bool btnright;
bool btna;
bool btnb;
bool togglePressed;

//frame status (0-59)
unsigned int frameCount = 0;

//game status
enum {SPLASH, INSTRUCTIONS, PLAY, OVER} game_state;

//instruction screen sprites
sprite instruction_working = computer_working;
sprite instruction_broken = computer_broken;

//keykat status
int kx, ky;  //position
sprite keykat;
bool kmoving;

//computer types and globals
enum compstate {ON, OFF, BROKEN};
struct computer {
  sprite s;          //the computer's sprite
  unsigned int x;    // position x
  unsigned int y;    // position y
  unsigned int ttf;  //time to failure (in frames)
  compstate state;   //the current computer state
};
computer comps[] = {{computer_off, 16, 0, 0, OFF},
                    {computer_off, 28, 0, 0, OFF},
                    {computer_off, 40, 0, 0, OFF},
                    {computer_off, 80, 0, 0, OFF},
                    {computer_off, 92, 0, 0, OFF},
                    {computer_off, 104, 0, 0, OFF},
                    {computer_off, 16, 27, 0, OFF},
                    {computer_off, 28, 27, 0, OFF},
                    {computer_off, 40, 27, 0, OFF},
                    {computer_off, 80, 27, 0, OFF},
                    {computer_off, 92, 27, 0, OFF},
                    {computer_off, 104, 27, 0, OFF}};
const unsigned int STARTMAXTTF = 3000;
const unsigned int MINMAXTTF=600;
const unsigned int SPEEDUP=20;                   
unsigned int ncomps = 12;
unsigned int maxttf = STARTMAXTTF;
unsigned int minttf = 60;


//scoring information
unsigned int nworking=0; // number of working computers
unsigned int score=0;


void splashScreen();
void instructions();
void playGame();
void gameOver();
void initNew();
void renderSprite(int x, int y, sprite &s);
void updateButtons();
void moveKeyKat();
void drawPlay();
void drawKeyKat();
void updateComputers();
void drawComputers();
void drawScore();
void drawGameOver();
void turnComputerOn(computer &comp);
void turnComputerOff(computer &comp);
void breakComputer(computer &comp);
bool collision(int x, int y);
void keyKatToggle();

void setup() {
  arduboy.begin();

  // audio setup
  tunes.initChannel(PIN_SPEAKER_1);
#ifndef AB_DEVKIT
  // if not a DevKit
  tunes.initChannel(PIN_SPEAKER_2);
#else
  // if it's a DevKit
  tunes.initChannel(PIN_SPEAKER_1); // use the same pin for both channels
  tunes.toneMutesScore(true);       // mute the score when a tone is sounding
#endif

  arduboy.clear();
  arduboy.setFrameRate(60);
  arduboy.initRandomSeed();

  initNew();

  //initialize button state
  togglePressed = btnup = btndown = btnleft = btnright = btna = btnb = false;

  game_state = SPLASH;
}


void loop() {
  //wait until the next frame
  if(not arduboy.nextFrame()) return;

  //update the frame counter
  frameCount = (frameCount+1) % 60;

  //update the buttons
  updateButtons();
    
  arduboy.clear();
  switch(game_state) {
    case SPLASH:
      splashScreen();
      break;
    case INSTRUCTIONS:
      instructions();
      break;
    case PLAY:
      playGame();
      break;
    case OVER:
      gameOver();
      break;
  }
  arduboy.display();
}


/* Splash Screen Frame Loop */
void splashScreen() 
{
  arduboy.drawBitmap(0, 0, splash_screen_bitmap, 128, 64, WHITE);  

  //see if we are ready to move into instructions
  if(togglePressed) {
    game_state = INSTRUCTIONS;
    togglePressed = btnup = btndown = btnleft = btnright = btna = btnb = false;
  }
}


/* display some instructions */
void instructions()
{
  //Instruction Text
  arduboy.setCursor(0,0);
  arduboy.print(F("Turn broken"));
  arduboy.setCursor(0,8);
  arduboy.print(F("computers off &"));
  arduboy.setCursor(0,16);
  arduboy.print(F("back on again."));
  arduboy.setCursor(0,40);
  arduboy.print(F("Keep at least"));
  arduboy.setCursor(0, 48);
  arduboy.print(F("25% up at"));
  arduboy.setCursor(0, 56);
  arduboy.print(F("all times!"));

  //show the example systems
  renderSprite(103, 4, instruction_broken);

  renderSprite(103, 40, instruction_working);
  
  //see if we are ready to move into gameplay
  if(togglePressed) {
    game_state = PLAY;
  }
}


/* play the game */
void playGame()
{
  //update status
  moveKeyKat();
  updateComputers();

  //draw the stuff
  drawPlay();

  //check for game over
  if(nworking < 3) {
    //draw the game over message and display it
    drawGameOver();
    arduboy.display();

    //wait a bit so the game over message is seen
    arduboy.delayShort(3000);

    game_state = OVER;
  }
}


/* 
 *  The game is over!  Alert the user.
 */
void gameOver()
{
  //draw the stuff
  drawPlay();

  //display game over message
  drawGameOver();

  if(togglePressed) {
    //start a new game
    initNew();
    game_state = PLAY;
  }
}


/*
 * Initialize for a new game
 */
void initNew()
{
  //initialize keykat
  keykat = keykat_standing;
  kx = 60;
  ky = 15;
  kmoving = false;

  //initialize maximum time to fail
  maxttf = STARTMAXTTF;

  //turn all the computers on
  for(int i=0; i<ncomps; i++) {
    turnComputerOn(comps[i]);
  }

  //initialize the score
  score = 0;
}

/*
 * Render the sprite frame, updating its index as needed.
 */
void renderSprite(int x, int y, sprite &s)
{
  if(frameCount % s.fdelay == 0) {
    s.fi = (s.fi+1) % s.frames;
  }
  
  //display the bitmap
  arduboy.drawBitmap(x, y, s.bitmap + s.fi*s.height, s.width, s.height, WHITE); 
}



/*
 * Update which button(s) is/are pressed
 */
void updateButtons()
{
  bool a, b;
  
  //get the state of each button
  btnup = arduboy.pressed(UP_BUTTON);
  btndown = arduboy.pressed(DOWN_BUTTON);
  btnleft = arduboy.pressed(LEFT_BUTTON);
  btnright = arduboy.pressed(RIGHT_BUTTON);
  a = arduboy.pressed(A_BUTTON);
  b = arduboy.pressed(B_BUTTON);

  //detect toggle event
  if((not a and btna) or (not b and btnb)) {
    togglePressed = true;
  }

  //record btna and btnb
  btna = a;
  btnb = b;
}

/*
 * Allow keykat to move.
 */
void moveKeyKat()
{
  bool nowmoving = btnup or btndown or btnleft or btnright or btna or btnb;
  
  //handle start moving
  if((not kmoving) and nowmoving) {
    keykat = keykat_walking;
  }

  //remember walking state
  kmoving = nowmoving;

  //handle standing
  if(not kmoving) {
    keykat = keykat_standing;
  }

  //update coordinates (if no collision occur)
  if(btnup) {
    if(not collision(kx, ky-1)){
      ky--;
    }
  } else if(btndown) {
    if(not collision(kx, ky+1)) {
      ky++;
    }
  } 
  
  if(btnleft) {
    if(not collision(kx-1, ky)) {
      kx--;
    }
  } else if(btnright) {
    if(not collision(kx+1, ky)) {
      kx++;
    }
  }

  //handle action buttons
  if(togglePressed) {
    keyKatToggle();
    togglePressed = false;
  }
}


/*
 * Draw the play screen
 */
void drawPlay()
{
  drawComputers();
  drawKeyKat();
  drawScore();
}


/*
 * Update and draw KeyKat
 */
void drawKeyKat()
{
  //draw keykat
  renderSprite(kx, ky, keykat);
}


/*
 * Update the status of all computers and count the
 * ones that are working.
 */
void updateComputers()
{
  nworking = 0;

  for(int i=0; i<ncomps; i++) {
    //count down to failure
    if(comps[i].ttf > 0) {
      comps[i].ttf--;

      //BOOOM
      if(comps[i].ttf == 0 and comps[i].state == ON) {
        breakComputer(comps[i]);
      }
    }

    //count the working
    if(comps[i].state == ON) {
      nworking++;
    }
  }
}

/*
 * Update and draw all the computers.  (One frame worth)
 */
void drawComputers()
{
  for(int i=0; i<ncomps; i++) {
    //draw the machine
    renderSprite(comps[i].x, comps[i].y, comps[i].s);
  }
}


/*
 * Draw the score and the number of working computers
 */
void drawScore()
{
  arduboy.setCursor(0, 56);
  arduboy.print(F("Score: "));
  arduboy.print(score);
  arduboy.setCursor(80, 56);
  arduboy.print(F("Up: "));
  arduboy.print(100*nworking/ncomps);
  arduboy.print("%");
}


/*
 * Draw the game over indication
 */
void drawGameOver()
{
  arduboy.setCursor(37, 18);
  arduboy.print(F("GAME OVER"));
}


/*
 * Turn the computer on (which lasts for a temporary number of frames)
 */
void turnComputerOn(computer &comp)
{
  //set the machine to working
  comp.state = ON;
  comp.s = computer_working;

  //determine how long the computer will work
  comp.ttf = random(minttf, maxttf);
  tunes.playScore(snd_on);
}


/*
 * Turn the computer off
 */
void turnComputerOff(computer &comp)
{
  //was it working before?
  if(comp.state != ON) {
    score += 1;
  }
  
  //turn the machine off
  comp.state = OFF;
  comp.s = computer_off;
  tunes.playScore(snd_off);
}


/*
 * Do what user's do best!
 */
void breakComputer(computer &comp)
{
  //break the machine
  comp.state = BROKEN;
  comp.s = computer_broken;
  tunes.playScore(snd_fail);
}


/* 
 * Returns true if KeyKat would be in a collision with something at
 * the specified point.
 */
bool collision(int x, int y)
{
  int kxmax, kymax;
  int cxmax, cymax;
  
  //edge collision
  if(x < 0 or x > 120 or y < 0 or y > 48) {
    return true;
  } 

  //check the computers
  kxmax = x + 7;
  kymax = y + 15;
  for(int i=0; i<ncomps; i++) {
    //computer bounding boxes (allowing for keyboard traversal)
    cxmax = comps[i].x + 7;
    cymax = comps[i].y + 5;

    //just simple axis aligned bounding box
    if(x <= cxmax && comps[i].x <= kxmax && y <= cymax && comps[i].y <= kymax) {
      return true;
    }
  }
  
  //all passed! no collision
  return false;
}


/*
 * This function has KeyKat toggle all computers within her reach.
 * (She has to be touching the keyboards)
 */
void keyKatToggle()
{
  int kxmax, kymax;
  int cxmax, cymax;

  //check the computers
  kxmax = kx + 7;
  kymax = ky + 15;
  for(int i=0; i<ncomps; i++) {
    //computer bounding boxes (for keyboard touching)
    cxmax = comps[i].x + 7;
    cymax = comps[i].y + 6;

    //just simple axis aligned bounding box
    if(kx <= cxmax && comps[i].x <= kxmax && ky <= cymax && comps[i].y <= kymax) {
      //toggle the machine
      if(comps[i].state != OFF) {
        turnComputerOff(comps[i]);
        if(maxttf > MINMAXTTF) {
          maxttf -= SPEEDUP; //mwahaha
        }
      } else {
        turnComputerOn(comps[i]);
      }
    }
  }
}

