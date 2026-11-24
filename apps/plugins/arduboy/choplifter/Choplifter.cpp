#include <Arduino.h>
#include "src/utils/Arduboy2Ext.h"
#include <ArduboyTones.h>
#include "Enums.h"
#include "Images.h"
#include "src/utils/Stack.h"
#include "src/sounds/Sounds.h"

template< typename IntT > const uint8_t * imageArrayLookup(const uint8_t * const * const array, IntT index);
template<typename T> T absT(const T & v);
void drawBullet(const Bullet & bullet);

void setup();
void loop();
void introduction();
void gameOver();
void play();
void splashScreen_Init();
void splashScreen();
void playerMovements();
void hostageMovements();
void tankMovements();
void render(uint8_t sortie);
void drawExplosion(BulletExplosion *bulletExplosion);
void drawScoreBoard(bool all);
AbsHelicopterStance absHelicopterStance(HelicopterStance helicopterStance);
void drawHorizontalDottedLine(int x1, int x2, int y);
int8_t calcSpeed(int8_t initValue, bool increase);
void drawHelicopter(int8_t x, int8_t y, HelicopterStance image);
void resetSortie();
void resetGame();
void bulletHit(Bullet *bullet, BulletExplosion *explosion, bool playerBullet);
void turnSoundOff();
void turnSoundOn();
const uint8_t * imageArrayLookup(const uint8_t * const * const array);

Arduboy2Ext arduboy;
ArduboyTones sound(arduboy.audio.enabled);

Helicopter heli;

uint8_t frame = 0;
int8_t backgroundXOffset = 0;
uint8_t dead = 0;
uint8_t safe = 0;
uint8_t inHelicopter = 0;
uint8_t sortieNumber = 0;
uint8_t introduction_count = 0;
uint8_t level = LEVEL_EASY;

Stack <HelicopterStance, 10> playerStack;

Hostage hostages[NUMBER_OF_HOSTAGES];
Dormitory dormitories[NUMBER_OF_DORMITORIES];
Tank tanks[NUMBER_OF_TANKS];
Bullet playerBullets[NUMBER_OF_PLAYER_BULLETS];
Bullet tankBullets[NUMBER_OF_TANK_BULLETS];
BulletExplosion playerBulletExplosion;
BulletExplosion tankBulletExplosion;

GameState gameState = GameState::Initialise;


// --------------------------------------------------------------------------------------
//  Setup ..
//
void setup() {

  arduboy.boot();
  arduboy.flashlight();
  arduboy.systemButtons();
  arduboy.audio.begin();
  arduboy.setFrameRate(25);
  arduboy.initRandomSeed();

  resetGame();
  gameState = GameState::SplashScreen_Init;
  
}


// --------------------------------------------------------------------------------------
//  Main Loop ..
//
void loop() {

  if (!(arduboy.nextFrame())) return;
  frame++; if (frame == 4) frame = 0;
  arduboy.pollButtons();
  arduboy.clear();

  switch (gameState) {

    case GameState::SplashScreen_Init:
      splashScreen_Init();
      splashScreen();
      arduboy.display(false);
      break;

    case GameState::SplashScreen:
      splashScreen();
      arduboy.display(false);
      break;

    case GameState::Initialise:
      resetGame();
      sound.tones(rideOfTHeValkyries);
      break;

    case GameState::Introduction:
      introduction();
      break;

    case GameState::Sortie:
      resetSortie();
      render(sortieNumber);
      
      if (arduboy.justPressed(A_BUTTON)) { gameState = GameState::PlayGame; }
      break;

    case GameState::PlayGame:
      play();
      break;

    case GameState::GameOver:
      gameOver();
      break;

  }


  arduboy.display();

}




// --------------------------------------------------------------------------------------
//  Introduction ..
//
void introduction() {

  switch (introduction_count) {

    case 0:
      heli.xPos = 170;
      break;
    
    case 1 ... 64:
      arduboy.drawCompressedMirror(16, 23, choplifter, WHITE, false);
      break;

    case 65 ... 127:
      arduboy.drawCompressedMirror(16, 23, choplifter, WHITE, false);
      arduboy.drawCompressedMirror(36, 54, selection, WHITE, false);
      break;

    case 128:
      arduboy.drawCompressedMirror(0, 0, filmote, WHITE, false);
      arduboy.drawCompressedMirror(32, 54, selection2, WHITE, false);
      heli.xPos = 150;
      break;

    case 129 ... 191:
      arduboy.drawCompressedMirror(0, 0, filmote, WHITE, false);
      arduboy.drawCompressedMirror(32, 54, selection2, WHITE, false);
      break;

    case 192:
      arduboy.drawCompressedMirror(0, 0, keyboard_camper, WHITE, false);
      arduboy.drawCompressedMirror(36, 54, selection, WHITE, false);
      heli.xPos = 150;
      break;

    case 193 ... 254:
      arduboy.drawCompressedMirror(0, 0, keyboard_camper, WHITE, false);
      arduboy.drawCompressedMirror(36, 54, selection, WHITE, false);
      break;

    case 255:
      arduboy.drawCompressedMirror(16, 23, choplifter, WHITE, false);
      heli.xPos = 170;
      introduction_count = 0;
      break;

  }

  if (arduboy.pressed(LEFT_BUTTON)) {
    if (introduction_count % 4 <= 2){
      arduboy.drawCompressedMirror(3, 52, hostage_05, WHITE, false);
    }
    else {
      arduboy.drawCompressedMirror(3, 52, hostage_06, WHITE, false);
    }
  }

  introduction_count++;

  if (heli.xPos > -60) {
    arduboy.drawCompressedMirror(heli.xPos, 12, heli_06_intro_mask, BLACK, false);
    drawHelicopter(heli.xPos, 12, HelicopterStance::Left_Incline_Max);
    heli.xPos = heli.xPos - 3;
  }

  if (arduboy.justPressed(A_BUTTON))      { sound.tones(introduction_sound); level = LEVEL_EASY; gameState = GameState::Sortie; }
  if (arduboy.justPressed(B_BUTTON))      { sound.tones(introduction_sound); level = LEVEL_HARD; gameState = GameState::Sortie; }
  if (arduboy.justPressed(UP_BUTTON))     { turnSoundOn(); }
  if (arduboy.justPressed(DOWN_BUTTON))   { turnSoundOff(); }
}


// --------------------------------------------------------------------------------------
//  Game Over ..
//
void gameOver() {

  arduboy.drawCompressedMirror(45, 21, the_end, WHITE, false);
  drawScoreBoard(false);
  if (arduboy.justPressed(A_BUTTON)) { gameState = GameState::Initialise; }

}


// --------------------------------------------------------------------------------------
//  Play Choplifter !
//
void play() {


  // Handle player actions ..

  if (playerStack.isEmpty()) { playerMovements(); }

  if (arduboy.justPressed(A_BUTTON) && (absHelicopterStance(heli.stance) <= AbsHelicopterStance::Side_Front_Incline_2 || absHelicopterStance(heli.stance) >= AbsHelicopterStance::Side_Incline_Rotate_3)) {

    switch (absHelicopterStance(heli.stance)) {

      case AbsHelicopterStance::Side_Level ... AbsHelicopterStance::Side_Incline_2:
      case AbsHelicopterStance::Side_Reverse_Incline_1 ... AbsHelicopterStance::Side_Reverse_Incline_5:

        for (int i = 0; i < NUMBER_OF_PLAYER_BULLETS; i++) {
          
          Bullet *bullet = &playerBullets[i];

          if (bullet->xPos == BULLET_INACTIVE_X_VALUE) {

            sound.tones(player_firing);

            bullet->xPos = heli.xPos + ((int8_t)heli.stance < 0 ? -13 : 13);
            bullet->yPos = heli.yPos + 15;
            bullet->yDelta = BULLET_SHOOT_HORIZONTAL;
            bullet->xDelta = ((int8_t)heli.stance < 0 ? 8 : -8);
            bullet->startYPos = bullet->yPos;
            break;

          }

        }

        break;

      default:

        for (int i = 0; i < NUMBER_OF_PLAYER_BULLETS; i++) {
          
          Bullet *bullet = &playerBullets[i];

          if (bullet->xPos == BULLET_INACTIVE_X_VALUE) {

            sound.tones(player_firing);

            bullet->xPos = heli.xPos;
            bullet->yPos = heli.yPos + 19;
            bullet->yDelta = 1;
            bullet->xDelta = heli.xDelta * 2;
            bullet->startYPos = bullet->yPos;
            break;

          }

        }

        break;

    }

  }


  // Update the screen and object positions ..

  if (!playerStack.isEmpty()) {

    if (arduboy.everyXFrames(2)) {

      HelicopterStance oldStance = heli.stance;
      heli.stance = playerStack.pop();

      if (heli.stance == HelicopterStance::Set_Delta_X_Decrease) { heli.xInc = DELTA_X_DECREASE; heli.stance = playerStack.pop(); }
      if (heli.stance == HelicopterStance::Set_Delta_X_Increase) { heli.xInc = DELTA_X_INCREASE; heli.stance = playerStack.pop(); }
      if (heli.stance == HelicopterStance::Set_Delta_X_Zero)     { heli.xInc = 0; heli.xDelta = 0; heli.stance = oldStance; }

    }

  }

  if (arduboy.everyXFrames(4)) {
  
    if (heli.xInc == DELTA_X_INCREASE)     { heli.xDelta = calcSpeed(heli.xDelta, true); }
    if (heli.xInc == DELTA_X_DECREASE)     { heli.xDelta = calcSpeed(heli.xDelta, false); }
    if (heli.xDelta == 0)                  { heli.xInc = DELTA_X_DO_NOTHING; }
  
  }


  // Change the helicopter's position ..

  switch (heli.yDelta) {

    case -8 ... -1:                         // Going up!

      if (heli.yPos < (HELICOPTER_MAXIMUM_HEIGHT + 4) && heli.yDelta == -4) { heli.yDelta = -2; }
      if (heli.yPos < (HELICOPTER_MAXIMUM_HEIGHT + 2) && heli.yDelta == -2) { heli.yDelta = -1; }
      if (heli.yPos < (HELICOPTER_MAXIMUM_HEIGHT + 1) && heli.yDelta == -1) { heli.yDelta = 0; }

      heli.yPos = heli.yPos + heli.yDelta;
      break;

    case 0:
      break;

    case 1 ... 8:                           // We are falling ..

      if (heli.yPos < (HELICOPTER_MINIMUM_HEIGHT - heli.yDelta)) {

        heli.yPos = heli.yPos + heli.yDelta;

      }
      else {                                // Almost about to land or crash!

        switch (absHelicopterStance(heli.stance)) {

          case AbsHelicopterStance::Side_Level ... AbsHelicopterStance::Side_Incline_2:
          case AbsHelicopterStance::Side_Level_Rotate_1 ... AbsHelicopterStance::Side_Front_Level:
          case AbsHelicopterStance::Side_Reverse_Incline_1 ... AbsHelicopterStance::Side_Reverse_Incline_2:
          case AbsHelicopterStance::Side_Front_Incline_1:       
              
            if (heli.yPos > (HELICOPTER_MINIMUM_HEIGHT - 4) && heli.yDelta == 4)  { heli.yDelta = 2; }
            if (heli.yPos > (HELICOPTER_MINIMUM_HEIGHT - 2) && heli.yDelta == 2)  { heli.yDelta = 1; }
            if (heli.yPos > (HELICOPTER_MINIMUM_HEIGHT - 1) && heli.yDelta == 1)  { heli.yDelta = 0; }          

            heli.yPos = heli.yPos + heli.yDelta;
            tankBulletExplosion.yPos = tankBulletExplosion.yPos + heli.yDelta;
            break;

          default:

            if (heli.yDelta == 4 && absT(heli.xDelta) >= 2) {

              sound.tones(exploding);

              heli.yPos = (HELICOPTER_MINIMUM_HEIGHT - heli.yDelta);
              tankBulletExplosion.xPos = heli.xPos + 16;
              tankBulletExplosion.yPos = heli.yPos + 4;
              tankBulletExplosion.explosionType = ExplosionType::Large_1;
              heli.yDelta = 0;
              if (heli.countDown == HELICOPTER_COUNT_DOWN_INACTIVE) heli.countDown = HELICOPTER_COUNT_DOWN_START;


              // Kill any hostages that were in the helicopter ..

              dead = dead + inHelicopter;
              inHelicopter = 0;

              for (int i = 0; i < NUMBER_OF_HOSTAGES; i++) {

                Hostage *hostage = &hostages[i];
                if (hostage->stance == HostageStance::In_Helicopter) hostage->stance = HostageStance::Dead;

              }

            }
            else {
              
              if (heli.yPos > (HELICOPTER_MINIMUM_HEIGHT - 4) && heli.yDelta == 4)  { heli.yDelta = 2; }
              if (heli.yPos > (HELICOPTER_MINIMUM_HEIGHT - 2) && heli.yDelta == 2)  { heli.yDelta = 1; }
              if (heli.yPos > (HELICOPTER_MINIMUM_HEIGHT - 1) && heli.yDelta == 1)  { heli.yDelta = 0; }

              heli.yPos = heli.yPos + heli.yDelta;
              
            }

            break;

        }

      }

      break;

  }



  heli.yPos = heli.yPos + heli.yDelta;
  tankBulletExplosion.yPos = tankBulletExplosion.yPos + heli.yDelta;
  backgroundXOffset = backgroundXOffset - heli.xDelta;
  heli.xPos = heli.xPos - heli.xDelta;
  if (backgroundXOffset < -64) backgroundXOffset = backgroundXOffset + 64;
  if (backgroundXOffset > 0)   backgroundXOffset = backgroundXOffset - 64;



  // Did we just land on hostages?

  if (heli.yPos - heli.yDelta < 38 && heli.yPos > 38) {  // Check to see if we just landed.

    for (int i = 0; i < NUMBER_OF_HOSTAGES; i++) {

      Hostage *hostage = &hostages[i];
      int16_t diff = absT(hostage->xPos - heli.xPos);

      if (hostage->stance >= HostageStance::Running_Left_1 && hostage->stance <= HostageStance::Waving_22) {

        switch (absHelicopterStance(heli.stance)) {

          case AbsHelicopterStance::Side_Level ... AbsHelicopterStance::Side_Incline_Max:
          case AbsHelicopterStance::Side_Reverse_Incline_1 ... AbsHelicopterStance::Side_Reverse_Incline_5:
          case AbsHelicopterStance::Side_Incline_Rotate_3:
      
            if (diff < 7) {

              hostage->stance = HostageStance::Dying_2;
              dead++;

              if (dead + safe == NUMBER_OF_HOSTAGES) {

                heli.countDown = HELICOPTER_END_OF_GAME_START;

              }

            }

            break;

          case AbsHelicopterStance::Side_Level_Rotate_1 ... AbsHelicopterStance::Side_Front_Incline_2:
          case AbsHelicopterStance::Side_Incline_Rotate_1 ... AbsHelicopterStance::Side_Incline_Rotate_2:

            if (diff < 4) {

              hostages->stance = HostageStance::Dying_2;
              dead++;

              if (dead + safe == NUMBER_OF_HOSTAGES) {

                heli.countDown = HELICOPTER_END_OF_GAME_START;

              }

            }

            break;

        }

      }

    }

  }


  // Update hostages ..

  hostageMovements();


  // Update player bullets ..

  for (int i = 0; i < NUMBER_OF_PLAYER_BULLETS; i++) {

    Bullet *bullet = &playerBullets[i];

    if (bullet->xPos != BULLET_INACTIVE_X_VALUE) {


      // Update bullet position ..

      bullet->xPos = bullet->xPos - bullet->xDelta;
      if (absT(bullet->xPos - heli.xPos) > 70) { bullet->xPos = BULLET_INACTIVE_X_VALUE; }

      if (bullet->yDelta != BULLET_SHOOT_HORIZONTAL) {
        bullet->yPos = bullet->yPos + bullet->yDelta;
        bullet->yDelta = calcSpeed(bullet->yDelta, true);
      }



      // Check to see if we hit anything ..

      bulletHit(bullet, &playerBulletExplosion, true);

    }

  }




  // Update tank bullets ..

  for (int i = 0; i < NUMBER_OF_TANK_BULLETS; i++) {

    Bullet *bullet = &tankBullets[i];

    if (bullet->xPos != BULLET_INACTIVE_X_VALUE) {


      // Update bullet position ..

      bullet->xPos = bullet->xPos - bullet->xDelta;
      if (absT(bullet->xPos - heli.xPos) > 70) { bullet->xPos = BULLET_INACTIVE_X_VALUE; }

      bullet->yPos = bullet->yPos + bullet->yDelta;
      bullet->yDelta = calcSpeed(bullet->yDelta, true);


      // Check to see if we hit anything?

      bulletHit(bullet, &tankBulletExplosion, false);

    }

  }


  // Update tank movements ..

  tankMovements();


  // Render screen ..

  render(0);



  // End of game?

  if (dead + safe == NUMBER_OF_HOSTAGES && heli.countDown == HELICOPTER_COUNT_DOWN_INACTIVE) heli.countDown = HELICOPTER_END_OF_GAME_START;
  
  if (heli.countDown >= HELICOPTER_END_OF_GAME_START) {

    heli.countDown++;

    if (heli.countDown == HELICOPTER_END_OF_GAME_END) {

      gameState = GameState::GameOver;

    }

  }


  // heli blades ..

  #ifdef BLADE_SOUNDS
  if (heli.countDown == HELICOPTER_COUNT_DOWN_INACTIVE) {

    if      (absT(heli.xDelta) == 4 || absT(heli.yDelta) == 4 ) { sound.tones(blade_4); }
    else if (absT(heli.xDelta) == 2 || absT(heli.yDelta) == 2 ) { sound.tones(blade_3); }
    else if (absT(heli.xDelta) == 1 || absT(heli.yDelta) == 1 ) { sound.tones(blade_2); }
    else                                                          sound.tones(blade_1);

  }
  #endif

  // -- DEBUG -------------------------------------------------------------------------------

  #ifdef DEBUG

    arduboy.setCursor(1, 10);
    arduboy.print(image);

    arduboy.setCursor(20, 10);
    arduboy.print(deltaX);
    arduboy.setCursor(35, 10);
    arduboy.print(deltaY);
    arduboy.setCursor(50, 10);
    arduboy.print(backgroundXOffset);
    arduboy.setCursor(70, 10);
    arduboy.print(y);

  #endif

  // ----------------------------------------------------------------------------------------
  
 
}




static constexpr uint8_t ppotFrames = (1.6 *25); // 1.6s animation loop

// ----------------------------------------------------------------------------
//  Initialise state ..
//
void splashScreen_Init() {

    gameState = GameState::SplashScreen;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void splashScreen() { 

    if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
        
        gameState = GameState::Initialise; 

    }

    arduboy.drawCompressedMirror(32, 16, PPOT, WHITE, false);

    uint8_t p = 0x11; // Default pixel position 0 is hidden in the image

    switch (arduboy.frameCount % ppotFrames) {

        // 2nd quarter of frames
        case (ppotFrames*1/4) ... (ppotFrames*2/4)-1:
            p = 0x31; // Move pixel down to position 1
            /*-fallthrough*/

        // 1st quarter of frames
        case 0 ... (ppotFrames*1/4)-1:
            // Flash 'Play' arrow by clearing the image
            // Overwrite using filled rectangle
            arduboy.fillRect(91, 24, 5, 7, BLACK);
            break;

        // 3rd quarter of frames
        case (ppotFrames*2/4) ... (ppotFrames*3/4)-1:
            p = 0x51; // Move pixel down to position 2
            break;

        // 4th quarter of frames
        default:
            p = 0x91; // Move pixel down to position 3
            break;

    }

    // Draw pixel to represent the tape spooling
    // Render directly to the screen buffer 
    arduboy.sBuffer[52 +(3*WIDTH)] = p; // Values 0x11, 0x31, 0x51, 0x91

    // On even steps of pixel movement, update the spindle image
    if ((p & 0xA0) == 0) {
        // Render directly to the screen buffer
        arduboy.sBuffer[45 +(3*WIDTH)] = 0xA5;
        arduboy.sBuffer[46 +(3*WIDTH)] = 0x95;
    }

}


/* ----------------------------------------------------------------------------
 *  Hanlde player movements ..
 */
void playerMovements() {


  // Up and down movements ..

  if (arduboy.pressed(UP_BUTTON)) {

    if (heli.yPos > HELICOPTER_MAXIMUM_HEIGHT) {

      heli.yDelta = calcSpeed(heli.yDelta, false);  

    }

  } 
  
  else {
    
    if (heli.yPos < HELICOPTER_MINIMUM_HEIGHT) {

      if (level == LEVEL_HARD) {
        heli.yDelta = calcSpeed(heli.yDelta, true);  
      }
      else {
        
        if (heli.yDelta < 1) {
          heli.yDelta = calcSpeed(heli.yDelta, true);  
        }

      }

    }

  }


  // Turn the helicoper ..

  if (arduboy.pressed(B_BUTTON)) {

    if (heli.xDelta == 0) {

      switch (heli.stance) {

        case HelicopterStance::Left_Level:
          playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Left_Level_Rotate_3);
          playerStack.push(HelicopterStance::Left_Level_Rotate_2, HelicopterStance::Left_Level_Rotate_1);
          heli.prevTurn = PREV_TURN_FROM_LEFT;
          break;

        case HelicopterStance::Right_Level:
          playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Right_Level_Rotate_3);
          playerStack.push(HelicopterStance::Right_Level_Rotate_2, HelicopterStance::Right_Level_Rotate_1);
          heli.prevTurn = PREV_TURN_FROM_RIGHT;
          break;

        case HelicopterStance::Front_Level:

          if (heli.prevTurn == PREV_TURN_FROM_LEFT) {
            playerStack.push(HelicopterStance::Right_Level, HelicopterStance::Right_Level_Rotate_1);
            playerStack.push(HelicopterStance::Right_Level_Rotate_2, HelicopterStance::Right_Level_Rotate_3);
          }
          else {
            playerStack.push(HelicopterStance::Left_Level, HelicopterStance::Left_Level_Rotate_1);
            playerStack.push(HelicopterStance::Left_Level_Rotate_2, HelicopterStance::Left_Level_Rotate_3);
          }

          break;

          default: break;
      }

    }
    else {

      switch (heli.stance) {

        case HelicopterStance::Right_Reverse_Incline_5:
          playerStack.push(HelicopterStance::Left_Incline_Max, HelicopterStance::Left_Incline_Rotate_3);
          playerStack.push(HelicopterStance::Left_Incline_Rotate_2, HelicopterStance::Left_Incline_Rotate_1);
          break;

        case HelicopterStance::Right_Front_Incline_2:
          playerStack.push(HelicopterStance::Right_Incline_Max, HelicopterStance::Right_Incline_Rotate_3);
          break;

        case HelicopterStance::Left_Front_Incline_2:
          playerStack.push(HelicopterStance::Left_Incline_Max, HelicopterStance::Left_Incline_Rotate_3);
          break;
      
        case HelicopterStance::Left_Reverse_Incline_5:
          playerStack.push(HelicopterStance::Right_Incline_Max, HelicopterStance::Right_Incline_Rotate_3);
          playerStack.push(HelicopterStance::Right_Incline_Rotate_2, HelicopterStance::Right_Incline_Rotate_1);
          break;

        default: break;

      }

    }

  }


  // Left and right movements ..

  if (arduboy.pressed(LEFT_BUTTON)) {

    if (heli.yPos < HELICOPTER_MINIMUM_HEIGHT || arduboy.pressed(UP_BUTTON)) {

      switch (heli.stance) {

        case HelicopterStance::Left_Level:
          playerStack.push(HelicopterStance::Left_Incline_Max, HelicopterStance::Left_Incline_4, HelicopterStance::Left_Incline_3);
          playerStack.push(HelicopterStance::Left_Incline_2, HelicopterStance::Left_Incline_1);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Front_Level:
          playerStack.push(HelicopterStance::Left_Front_Incline_2, HelicopterStance::Left_Front_Incline_1);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Left_Reverse_Incline_5:
          playerStack.push(HelicopterStance::Left_Incline_Max, HelicopterStance::Left_Incline_4, HelicopterStance::Left_Incline_3);
          playerStack.push(HelicopterStance::Left_Incline_2, HelicopterStance::Left_Incline_1, HelicopterStance::Left_Level);
          playerStack.push(HelicopterStance::Left_Reverse_Incline_1, HelicopterStance::Left_Reverse_Incline_2);
          playerStack.push(HelicopterStance::Left_Reverse_Incline_3, HelicopterStance::Left_Reverse_Incline_4);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Right_Level:
          playerStack.push(HelicopterStance::Right_Reverse_Incline_5, HelicopterStance::Right_Reverse_Incline_4, HelicopterStance::Right_Reverse_Incline_3);
          playerStack.push(HelicopterStance::Right_Reverse_Incline_2, HelicopterStance::Right_Reverse_Incline_1);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Right_Incline_Max:
          playerStack.push(HelicopterStance::Right_Reverse_Incline_5, HelicopterStance::Right_Reverse_Incline_4, HelicopterStance::Right_Reverse_Incline_3);
          playerStack.push(HelicopterStance::Right_Reverse_Incline_2, HelicopterStance::Right_Reverse_Incline_1);
          playerStack.push(HelicopterStance::Right_Level, HelicopterStance::Right_Incline_1, HelicopterStance::Right_Incline_2);
          playerStack.push(HelicopterStance::Right_Incline_3, HelicopterStance::Right_Incline_4);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Right_Front_Incline_2:
          playerStack.push(HelicopterStance::Left_Front_Incline_2, HelicopterStance::Left_Front_Incline_1);
          playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Right_Front_Incline_1);
          heli.xInc = DELTA_X_INCREASE;          
          break;

        default:
          heli.xInc = DELTA_X_DECREASE;          
          break;
        
      }

    }
    else {    // We are on the ground.

      switch (heli.stance) {

        case HelicopterStance::Left_Level:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Left_Level, HelicopterStance::Left_Incline_1, HelicopterStance::Left_Incline_2);
          playerStack.push(HelicopterStance::Left_Incline_2, HelicopterStance::Set_Delta_X_Increase);
          playerStack.push(HelicopterStance::Left_Incline_2, HelicopterStance::Left_Incline_2, HelicopterStance::Left_Incline_1);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Left_Incline_Max:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Left_Level, HelicopterStance::Left_Incline_1);
          playerStack.push(HelicopterStance::Left_Incline_2, HelicopterStance::Left_Incline_3, HelicopterStance::Left_Incline_4);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Front_Level:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Left_Front_Incline_1, HelicopterStance::Left_Front_Incline_1);
          playerStack.push(HelicopterStance::Set_Delta_X_Increase);
          playerStack.push(HelicopterStance::Left_Front_Incline_2, HelicopterStance::Left_Front_Incline_1, HelicopterStance::Left_Front_Incline_1);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Left_Front_Incline_2:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Left_Front_Incline_1, HelicopterStance::Left_Front_Incline_1);
          playerStack.push(HelicopterStance::Set_Delta_X_Increase);
          playerStack.push(HelicopterStance::Left_Front_Incline_1);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Right_Reverse_Incline_5:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Right_Level);
          playerStack.push(HelicopterStance::Right_Reverse_Incline_2, HelicopterStance::Right_Reverse_Incline_1);
          playerStack.push(HelicopterStance::Right_Reverse_Incline_4, HelicopterStance::Right_Reverse_Incline_3);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        case HelicopterStance::Right_Level:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Right_Level, HelicopterStance::Right_Reverse_Incline_1);
          playerStack.push(HelicopterStance::Right_Reverse_Incline_2, HelicopterStance::Right_Reverse_Incline_3);
          playerStack.push(HelicopterStance::Set_Delta_X_Increase);
          playerStack.push(HelicopterStance::Right_Reverse_Incline_3, HelicopterStance::Right_Reverse_Incline_2, HelicopterStance::Right_Reverse_Incline_1);
          heli.xInc = DELTA_X_DECREASE;          
          break;

        default: break;

      }

    }

  } 

  else if (arduboy.pressed(RIGHT_BUTTON)) {

    if (heli.yPos < HELICOPTER_MINIMUM_HEIGHT || arduboy.pressed(UP_BUTTON)) {

      switch (heli.stance) {

        case HelicopterStance::Right_Level:
          playerStack.push(HelicopterStance::Right_Incline_Max, HelicopterStance::Right_Incline_4, HelicopterStance::Right_Incline_3);
          playerStack.push(HelicopterStance::Right_Incline_2, HelicopterStance::Right_Incline_1);
          heli.xInc = DELTA_X_INCREASE; 
          break;

        case HelicopterStance::Front_Level:
          playerStack.push(HelicopterStance::Right_Front_Incline_2, HelicopterStance::Right_Front_Incline_1);
          heli.xInc = DELTA_X_INCREASE; 
          break;

        case HelicopterStance::Right_Reverse_Incline_5:
          playerStack.push(HelicopterStance::Right_Incline_Max, HelicopterStance::Right_Incline_4, HelicopterStance::Right_Incline_3);
          playerStack.push(HelicopterStance::Right_Incline_2, HelicopterStance::Right_Incline_1, HelicopterStance::Right_Level);
          playerStack.push(HelicopterStance::Right_Reverse_Incline_1, HelicopterStance::Right_Reverse_Incline_2);
          playerStack.push(HelicopterStance::Right_Reverse_Incline_3, HelicopterStance::Right_Reverse_Incline_4);
          heli.xInc = DELTA_X_INCREASE; 
          break;

        case HelicopterStance::Left_Level:
          playerStack.push(HelicopterStance::Left_Reverse_Incline_5, HelicopterStance::Left_Reverse_Incline_4, HelicopterStance::Left_Reverse_Incline_3);
          playerStack.push(HelicopterStance::Left_Reverse_Incline_2, HelicopterStance::Left_Reverse_Incline_1);
          heli.xInc = DELTA_X_INCREASE;       
          break;

        case HelicopterStance::Left_Incline_Max:
          playerStack.push(HelicopterStance::Left_Reverse_Incline_5, HelicopterStance::Left_Reverse_Incline_4, HelicopterStance::Left_Reverse_Incline_3);
          playerStack.push(HelicopterStance::Left_Reverse_Incline_2, HelicopterStance::Left_Reverse_Incline_1);
          playerStack.push(HelicopterStance::Left_Level, HelicopterStance::Left_Incline_1, HelicopterStance::Left_Incline_2);
          playerStack.push(HelicopterStance::Left_Incline_3, HelicopterStance::Left_Incline_4);
          heli.xInc = DELTA_X_INCREASE;       
          break;

        case HelicopterStance::Left_Front_Incline_2:
          playerStack.push(HelicopterStance::Right_Front_Incline_2, HelicopterStance::Right_Front_Incline_1);
          playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Left_Front_Incline_1);
          heli.xInc = DELTA_X_DECREASE;       
          break;

        default:
          heli.xInc = DELTA_X_INCREASE; 
          break;

      }

    }
    else {    // We are on the ground.

      switch (heli.stance) {

        case HelicopterStance::Right_Level:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Right_Level, HelicopterStance::Right_Incline_1, HelicopterStance::Right_Incline_2);
          playerStack.push(HelicopterStance::Right_Incline_2, HelicopterStance::Set_Delta_X_Decrease);
          playerStack.push(HelicopterStance::Right_Incline_2, HelicopterStance::Right_Incline_2, HelicopterStance::Right_Incline_1);
          heli.xInc = DELTA_X_INCREASE;          
          break;

        case HelicopterStance::Right_Incline_Max:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Right_Level, HelicopterStance::Right_Incline_1);
          playerStack.push(HelicopterStance::Right_Incline_2, HelicopterStance::Right_Incline_3, HelicopterStance::Right_Incline_4);
          heli.xInc = DELTA_X_INCREASE;          
          break;

        case HelicopterStance::Front_Level:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Right_Front_Incline_1, HelicopterStance::Right_Front_Incline_1);
          playerStack.push(HelicopterStance::Set_Delta_X_Decrease);
          playerStack.push(HelicopterStance::Right_Front_Incline_2, HelicopterStance::Right_Front_Incline_1, HelicopterStance::Right_Front_Incline_1);
          heli.xInc = DELTA_X_INCREASE;          
          break;

        case HelicopterStance::Right_Front_Incline_2:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Right_Front_Incline_1);
          playerStack.push(HelicopterStance::Set_Delta_X_Decrease);
          playerStack.push(HelicopterStance::Right_Front_Incline_1, HelicopterStance::Right_Front_Incline_1);
          heli.xInc = DELTA_X_INCREASE;          
          break;

        case HelicopterStance::Left_Reverse_Incline_5:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Left_Level);
          playerStack.push(HelicopterStance::Left_Reverse_Incline_2, HelicopterStance::Left_Reverse_Incline_1);
          playerStack.push(HelicopterStance::Left_Reverse_Incline_4, HelicopterStance::Left_Reverse_Incline_3);
          heli.xInc = DELTA_X_INCREASE;          
          break;

        case HelicopterStance::Left_Level:
          playerStack.push(HelicopterStance::Set_Delta_X_Zero);
          playerStack.push(HelicopterStance::Left_Level, HelicopterStance::Left_Reverse_Incline_1);
          playerStack.push(HelicopterStance::Left_Reverse_Incline_2, HelicopterStance::Left_Reverse_Incline_3);
          playerStack.push(HelicopterStance::Set_Delta_X_Decrease);
          playerStack.push(HelicopterStance::Left_Reverse_Incline_3, HelicopterStance::Left_Reverse_Incline_2, HelicopterStance::Left_Reverse_Incline_1);
          heli.xInc = DELTA_X_INCREASE;          
          break;

        default: break;

      }

    }

  } 
  else {  // Return from current position to upright ..

    switch (heli.stance) {

      case HelicopterStance::Left_Front_Incline_2:
        playerStack.push(HelicopterStance::Set_Delta_X_Zero);
        playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Left_Front_Incline_1);
        if (heli.xDelta != 0) heli.xInc = (heli.xDelta > 0 ? DELTA_X_DECREASE : DELTA_X_INCREASE);
        break;

      case HelicopterStance::Right_Front_Incline_2:
        playerStack.push(HelicopterStance::Set_Delta_X_Zero);
        playerStack.push(HelicopterStance::Front_Level, HelicopterStance::Right_Front_Incline_1);
        if (heli.xDelta != 0) heli.xInc = (heli.xDelta > 0 ? DELTA_X_DECREASE : DELTA_X_INCREASE);
        break;

      case HelicopterStance::Left_Incline_Max:
        playerStack.push(HelicopterStance::Set_Delta_X_Zero);
        playerStack.push(HelicopterStance::Left_Level, HelicopterStance::Left_Incline_1, HelicopterStance::Left_Incline_2);
        playerStack.push(HelicopterStance::Left_Incline_3, HelicopterStance::Left_Incline_4);
        if (heli.xDelta != 0) heli.xInc = (heli.xDelta > 0 ? DELTA_X_DECREASE : DELTA_X_INCREASE);
        break;

      case HelicopterStance::Right_Incline_Max:
        playerStack.push(HelicopterStance::Set_Delta_X_Zero);
        playerStack.push(HelicopterStance::Right_Level, HelicopterStance::Right_Incline_1, HelicopterStance::Right_Incline_2);
        playerStack.push(HelicopterStance::Right_Incline_3, HelicopterStance::Right_Incline_4);
        if (heli.xDelta != 0) heli.xInc = (heli.xDelta > 0 ? DELTA_X_DECREASE : DELTA_X_INCREASE);
        break;
      
      case HelicopterStance::Left_Reverse_Incline_5:
        playerStack.push(HelicopterStance::Set_Delta_X_Zero);
        playerStack.push(HelicopterStance::Left_Level, HelicopterStance::Left_Reverse_Incline_1, HelicopterStance::Left_Reverse_Incline_2);
        playerStack.push(HelicopterStance::Left_Reverse_Incline_3, HelicopterStance::Left_Reverse_Incline_4);
        if (heli.xDelta != 0) heli.xInc = (heli.xDelta > 0 ? DELTA_X_DECREASE : DELTA_X_INCREASE);
        break;

      case HelicopterStance::Right_Reverse_Incline_5:
        playerStack.push(HelicopterStance::Set_Delta_X_Zero);
        playerStack.push(HelicopterStance::Right_Level, HelicopterStance::Right_Reverse_Incline_1, HelicopterStance::Right_Reverse_Incline_2);
        playerStack.push(HelicopterStance::Right_Reverse_Incline_3, HelicopterStance::Right_Reverse_Incline_4);
        if (heli.xDelta != 0) heli.xInc = (heli.xDelta > 0 ? DELTA_X_DECREASE : DELTA_X_INCREASE); //inc
        break;

      default: break;
      
    }

  }

}


/* ----------------------------------------------------------------------------
 *  Handle hostage movements ..
 */
void hostageMovements() {


  // Update hostages ..

  HostageStance randomLowerLimit = HostageStance::Running_Left_1;
  HostageStance randomTopLimit = HostageStance::Running_Right_4;

  for (uint8_t i = 0; i < NUMBER_OF_HOSTAGES; i++) {

    Hostage *hostage = &hostages[i];


    // Collect hostages ..

    if (inHelicopter < HELICOPTER_HOSTAGE_CAPACITY &&
        hostage->stance != HostageStance::Dead && hostage->stance > HostageStance::In_Dorm && hostage->stance <= HostageStance::Leaving_Dorm &&
        heli.countDown == HELICOPTER_COUNT_DOWN_INACTIVE && heli.xPos > 50 && heli.yPos >= 40 && absT(hostage->xPos - heli.xPos) < 5
        ) {    

      sound.tones(collect_hostage);
      
      inHelicopter++;
      hostage->stance = HostageStance::In_Helicopter;
      hostage->countDown = (14 * inHelicopter);

    }


    // Deposit hostages at base ..

    else if (hostage->stance == HostageStance::In_Helicopter && 
             heli.yPos >= 40 &&                                                                                               
             ((heli.stance == HelicopterStance::Left_Level && heli.xPos < 26 && heli.xPos > -6) || 
              (heli.stance == HelicopterStance::Right_Level && heli.xPos < 24 && heli.xPos > 2) || 
              (heli.stance == HelicopterStance::Front_Level && heli.xPos < 24 && heli.xPos > -14))) {                

      hostage->countDown--;

      if (hostage->countDown == 0) {

        hostage->stance = HostageStance::Running_Right_1;
        hostage->countDown = 255;
        hostage->xPos = heli.xPos;
        inHelicopter--;

      }

    }

    // Hostages are safe ..
    
    else if (hostage->stance != HostageStance::Safe && hostage->xPos < -40) {                                                                                   

        hostage->stance = HostageStance::Safe;
        sound.tones(safety);
        safe++;

        if (dead + safe == NUMBER_OF_HOSTAGES) {

          heli.countDown = HELICOPTER_END_OF_GAME_START;

        }

    }

    // Hostages run wild!

    else {                                                                                                            

      switch (hostage->stance) {

        case HostageStance::Running_Left_1 ... HostageStance::Running_Left_3:

          hostage->stance = (HostageStance)((uint8_t)hostage->stance + 1);
          hostage->countDown--;
          hostage->xPos++;
          randomTopLimit = HostageStance::Waving_22;
          break;

        case HostageStance::Running_Left_4:

          hostage->stance = HostageStance::Running_Left_1;
          hostage->countDown--;
          hostage->xPos++;
          randomTopLimit = HostageStance::Waving_22;
          break;

        case HostageStance::Running_Right_1 ... HostageStance::Running_Right_3:

          hostage->stance = (HostageStance)((uint8_t)hostage->stance + 1);
          hostage->countDown--;
          hostage->xPos--;
          randomTopLimit = HostageStance::Waving_22;
          break;

        case HostageStance::Running_Right_4:

          hostage->stance = HostageStance::Running_Right_1;
          hostage->countDown--;
          hostage->xPos--;
          randomTopLimit = HostageStance::Waving_22;
          break;

        case HostageStance::Waving_11 ... HostageStance::Waving_12:

          hostage->stance = (HostageStance)((uint8_t)hostage->stance + 1);
          hostage->countDown--;
          randomTopLimit = HostageStance::Running_Right_4;
          break;

        case HostageStance::Waving_21 ... HostageStance::Waving_22:

          hostage->stance = (HostageStance)((uint8_t)hostage->stance + 1);
          if (hostage->stance > HostageStance::Waving_22) hostage->stance = HostageStance::Waving_11;
          hostage->countDown--;
          randomTopLimit = HostageStance::Running_Right_4;
          break;

        case HostageStance::Leaving_Dorm:

          hostage->stance = (HostageStance)((uint8_t)hostage->stance - 1);
          if (hostage->countDown == 0) {
            randomTopLimit = HostageStance::Running_Right_4;
          }
          break;

        default: break;

      }


      if (hostage->countDown == 0) {

        if (absT(hostage->xPos - heli.xPos) < 70) {

          if (random(0, 15) < 13) {

            hostage->stance = (hostage->xPos > heli.xPos ? (hostage->xPos > HOSTAGE_FAR_RIGHT_POS ? HostageStance::Running_Right_1 : HostageStance::Running_Left_1) : HostageStance::Running_Left_1);
            hostage->countDown = random(5, 15);

          }
          else {

            hostage->stance = HostageStance::Waving_11;
            hostage->countDown = random(2, 5);

          }

        }
        else {

          if (hostage->xPos < HOSTAGE_FAR_RIGHT_POS) { randomTopLimit = HostageStance::Running_Left_4; }  // Prevent hostage from crossing fence ..      
          if (hostage->xPos > HOSTAGE_FAR_LEFT_POS)  { randomLowerLimit = HostageStance::Running_Right_1; }  // Prevent hostage from crossing fence ..      

          hostage->stance = (HostageStance)random((uint8_t)randomLowerLimit, (uint8_t)randomTopLimit + 1);

          switch (hostage->stance) {

            case HostageStance::Running_Left_1 ... HostageStance::Running_Right_4:
              hostage->countDown = random(5, 25);
              break;

            case HostageStance::Waving_11 ... HostageStance::Waving_22:
              hostage->countDown = random(2, 5);
              break;

            default: break;

          }

        }

      }

    }

  }

}


/* ----------------------------------------------------------------------------
 *  Handle tank movements ..
 */
void tankMovements() {


  // Update tank ..

  for (uint8_t i=0; i < NUMBER_OF_TANKS; i++) {

    Tank *tank = &tanks[i];

    int16_t tankDif = tank->xPos - heli.xPos;

    if (tank->state > TankState::Dead3) {

      switch (tank->state) {

        case TankState::Move_Left:
          tank->xPos+=2;
          break;

        case TankState::Move_Right:

          if (tank->xPos > TANK_FAR_RIGHT_POS) {
            tank->xPos-=2;
          }
          else {
            tank->state = TankState::Stationary;
          }
          break;

        default: break;

      }


      // Move turrent ..

      switch (tankDif) {

        case -999 ... -40:
          tank->turrentDirection = TurrentDirection::Left_Low;
          break;

        case -39 ... -13:
          tank->turrentDirection = TurrentDirection::Left_Mid;
          break;

        case -12 ... 12:
          tank->turrentDirection = TurrentDirection::Upright;
          break;

        case 13 ... 39:
          tank->turrentDirection = TurrentDirection::Right_Mid;
          break;

        case 40 ... 999:
          tank->turrentDirection = TurrentDirection::Right_Low;
          break;

      }

      
      tank->countDown--;


      // Tank has completed the last move.  Chose a new random move ..
      
      if (tank->countDown == 0) {

        switch (random(0, 3)) {

          case 0:

            if (tankDif < -30 && tank->xPos < tank->startingXPos + DORMITORY_SPACING_HALF - TANK_WIDTH) {

              tank->state = TankState::Move_Left;

            }
            else if (tankDif > 30 && tank->xPos > tank->startingXPos - DORMITORY_SPACING_HALF + TANK_WIDTH) {

              tank->state = TankState::Move_Right;
              
            }
            else {

              tank->state = TankState::Stationary;
              
            }

            break;

          case 1:

            if (tankDif > 30 && tank->xPos > tank->startingXPos - DORMITORY_SPACING_HALF + TANK_WIDTH) {

              tank->state = TankState::Move_Right;

            }
            else if (tankDif < -30 && tank->xPos < tank->startingXPos + DORMITORY_SPACING_HALF - TANK_WIDTH) {

              tank->state = TankState::Move_Left;
              
            }
            else {

              tank->state = TankState::Stationary;
              
            }

            break;

          case 2:

            tank->state = TankState::Stationary;
            
            break;

        }

        tank->countDown = random(4, 12);

      }


      // Are we in a position to fire ?

      if (absT(tankDif) < 45) {

        if (random(0, (level == LEVEL_EASY ? 40 : 25) ) == 0) {

          for (uint8_t i = 0; i < NUMBER_OF_TANK_BULLETS; i++) {
            
            Bullet *bullet = &tankBullets[i];

            if (bullet->xPos == BULLET_INACTIVE_X_VALUE) {

            sound.tones(tank_firing); 

              bullet->yPos = 40;
              bullet->yDelta = -8;
              bullet->startYPos = bullet->yPos;

              switch (tank->turrentDirection) {

                case TurrentDirection::Left_Low:
                  bullet->xPos = tank->xPos + 11;
                  bullet->xDelta = -4;
                  break;

                case TurrentDirection::Left_Mid:
                  bullet->xPos = tank->xPos + 9;
                  bullet->xDelta = -2;
                  break;

                case TurrentDirection::Upright:
                  bullet->xPos = tank->xPos;
                  bullet->xDelta = random(-1, 2);
                  break;

                case TurrentDirection::Right_Mid:
                  bullet->xPos = tank->xPos - 9;
                  bullet->xDelta = 2;
                  break;

                case TurrentDirection::Right_Low:
                  bullet->xPos = tank->xPos - 11;
                  bullet->xDelta = 4;
                  break;

              }

              break;

            }

          }

        }

      }

    }

  }

}

const uint8_t * const renderSortieLookup[] PROGMEM = { sortie_01, sortie_02, sortie_03 };

const uint8_t * const renderTankLookup[] PROGMEM = { tank_turrent_00, tank_turrent_01, tank_turrent_02, tank_turrent_01, tank_turrent_00 };

/* ----------------------------------------------------------------------------
 *  Render the screen ..
 * 
 *  If a sortie number greater than zero is supplied, the sortie number is 
 *  rendered otherwise the notmal screen os drawn.
 */
void render(uint8_t sortie) {


  // Render background ..

  for (int16_t i = backgroundXOffset; i < backgroundXOffset + 150; i = i + (int16_t)64) {
    arduboy.drawCompressedMirror(i, 43, background, WHITE, false);
  }

  drawHorizontalDottedLine(backgroundXOffset % 2, WIDTH, 51);


  // Draw base ..

  if (heli.xPos > -158 && heli.xPos < 105) {

    arduboy.fillRect(heli.xPos + 90, 43, 42, 9, BLACK);
    arduboy.drawCompressedMirror(heli.xPos + 26, 32, base, WHITE, false);

  }


  // Draw dormitories ..
  
  if (sortie > 0) {
  
    arduboy.drawCompressedMirror(38, 22, imageArrayLookup(&renderSortieLookup[sortie - 1]), WHITE, false);

  }
  else {

    for (uint8_t i = 0; i < NUMBER_OF_DORMITORIES; i++) {

      Dormitory *dormitory = &dormitories[i];

      if ((dormitory->xPos > heli.xPos - 144) && (dormitory->xPos < heli.xPos + 144)) {

        const uint8_t * mask = (dormitory->state == DormitoryState::Intact) ? dormitory_01_mask : dormitory_02_mask;
        const uint8_t * image =(dormitory->state == DormitoryState::Intact) ? dormitory_01 : dormitory_02;
      
        arduboy.drawCompressedMirror(heli.xPos - dormitory->xPos + 64 - 15, 40, mask, BLACK, false);
        arduboy.drawCompressedMirror(heli.xPos - dormitory->xPos + 64 - 15, 40, image, WHITE, false);
          

      }

    }


    // Draw hostages ..

    for (uint8_t i = 0; i < NUMBER_OF_HOSTAGES; i++) {
      
      Hostage *hostage = &hostages[i];

      if ((hostage->xPos > heli.xPos - 133) && (hostage->xPos < heli.xPos + 133)) {

        const uint8_t * image = nullptr;
        bool flag = false;

        switch (hostage->stance) {

          case HostageStance::Running_Left_1 ... HostageStance::Running_Left_4:
            image = hostage_images[(uint8_t)hostage->stance - 1];
            break;

          case HostageStance::Running_Right_1 ... HostageStance::Running_Right_4:
            image = hostage_images[(uint8_t)hostage->stance - 5];
            flag = true;
            break;

          case HostageStance::Waving_11 ... HostageStance::Waving_12:
            image = hostage_05;
            break;

          case HostageStance::Waving_21 ... HostageStance::Waving_22:
            image = hostage_06;
            break;

          case HostageStance::Dying_1:
          case HostageStance::Dying_2:
            image = hostage_07;
            hostage->stance = (HostageStance)((uint8_t)hostage->stance - 1);
            break;

          default: break;

        }

        if(image != nullptr)
          arduboy.drawCompressedMirror(heli.xPos - hostage->xPos + 64 - 3, 48, image, WHITE, flag);

      }

    }


    // Draw helicopter ..

    if (heli.countDown < HELICOPTER_COUNT_DOWN_MID || heli.countDown >= HELICOPTER_END_OF_GAME_START) {

      drawHelicopter(45, heli.yPos, heli.stance);

    }
    else {

      heli.countDown++;
      if (heli.countDown == HELICOPTER_COUNT_DOWN_END) {

        sortieNumber++;

        if (sortieNumber <= NUMBER_OF_SORTIES && dead + safe < NUMBER_OF_HOSTAGES) {
          gameState = GameState::Sortie;
        }
        else {
          gameState = GameState::GameOver;
        }

      }

    }


    // Draw tank ..

    for (int i = 0; i < NUMBER_OF_TANKS; i++) {

      Tank *tank = &tanks[i];
  
      if (tank->state > TankState::Dead1 && (tank->xPos > heli.xPos - 144) && (tank->xPos < heli.xPos + 144)) {

        arduboy.drawCompressedMirror(heli.xPos - tank->xPos + 64 - 15, 47, tank_00_mask, BLACK, false);
        arduboy.drawCompressedMirror(heli.xPos - tank->xPos + 64 - 15, 47, (tank->track ? tank_00 : tank_01), WHITE, false);

        const uint8_t * image = imageArrayLookup(&renderTankLookup[(uint8_t)tank->turrentDirection]);
        bool flag = ((uint8_t)tank->turrentDirection < (uint8_t)TurrentDirection::Upright);

        arduboy.drawCompressedMirror(heli.xPos - tank->xPos + 64 - 15, 47, image, WHITE, flag);

        if (tank->state == TankState::Move_Left || tank->state == TankState::Move_Right) tank->track = !tank->track;

        if (tank->state <= TankState::Dead3) { tank->state = (TankState)((uint8_t)tank->state - 1); }

      }

    }



    // Draw player bullets ..

    for (int i = 0; i < NUMBER_OF_PLAYER_BULLETS; i++) {

      drawBullet(playerBullets[i]);

    }



    // Draw tank bullets ..

    for (int i = 0; i < NUMBER_OF_TANK_BULLETS; i++) {

      drawBullet(tankBullets[i]);

    }



    // Draw explosion ..

    if (playerBulletExplosion.xPos != BULLET_INACTIVE_X_VALUE) {

      drawExplosion(&playerBulletExplosion);

    }



    // Draw tank explosion ..


    if (tankBulletExplosion.xPos != BULLET_INACTIVE_X_VALUE) {

      drawExplosion(&tankBulletExplosion);
      if (heli.countDown > HELICOPTER_COUNT_DOWN_INACTIVE) heli.countDown++;

    }



    // Draw fences ..

    switch (heli.xPos) {

      // Right fence ..

      case 120 ... 159:
        arduboy.drawCompressedMirror(-24 + (heli.xPos - 150), 2, fence_00_mask, BLACK, true);
        arduboy.drawCompressedMirror(-24 + (heli.xPos - 150), 2, fence_00, WHITE, true);
        break;

      case 160 ... 169:
        arduboy.drawCompressedMirror(-16 + (heli.xPos - 160), 2, fence_01_mask, BLACK, true);
        arduboy.drawCompressedMirror(-16 + (heli.xPos - 160), 2, fence_01, WHITE, true);
        break;

      case 170 ... 179:
        arduboy.drawCompressedMirror(-8 + (heli.xPos - 170), 2, fence_02_mask, BLACK, true);
        arduboy.drawCompressedMirror(-8 + (heli.xPos - 170), 2, fence_02, WHITE, true);
        break;

      case 180 ... 189:
        arduboy.drawCompressedMirror(60 + (heli.xPos - 180), 2, fence_03_mask, BLACK, false);
        arduboy.drawCompressedMirror(60 + (heli.xPos - 180), 2, fence_03, WHITE, false);
        break;

      case 190 ... 199:
        arduboy.drawCompressedMirror(68 + (heli.xPos - 190), 2, fence_02_mask, BLACK, false);
        arduboy.drawCompressedMirror(68 + (heli.xPos - 190), 2, fence_02, WHITE, false);
        break;

      case 200 ... 209:
        arduboy.drawCompressedMirror(76 + (heli.xPos - 200), 2, fence_01_mask, BLACK, false);
        arduboy.drawCompressedMirror(76 + (heli.xPos - 200), 2, fence_01, WHITE, false);
        break;

      case 210 ... 249:
        arduboy.drawCompressedMirror(84 + (heli.xPos - 210), 2, fence_00_mask, BLACK, false);
        arduboy.drawCompressedMirror(84 + (heli.xPos - 210), 2, fence_00, WHITE, false);
        break;
      

      // Left fence ..

      case 2220 ... 2259:
        arduboy.drawCompressedMirror(-24 + (heli.xPos - 2250), 2, fence_00_mask, BLACK, true);
        arduboy.drawCompressedMirror(-24 + (heli.xPos - 2250), 2, fence_00, WHITE, true);
        break;

      case 2260 ... 2269:
        arduboy.drawCompressedMirror(-16 + (heli.xPos - 2260), 2, fence_01_mask, BLACK, true);
        arduboy.drawCompressedMirror(-16 + (heli.xPos - 2260), 2, fence_01, WHITE, true);
        break;

      case 2270 ... 2279:
        arduboy.drawCompressedMirror(-8 + (heli.xPos - 2270), 2, fence_02_mask, BLACK, true);
        arduboy.drawCompressedMirror(-8 + (heli.xPos - 2270), 2, fence_02, WHITE, true);
        break;

      case 2280 ... 2289:
        arduboy.drawCompressedMirror(60 + (heli.xPos - 2280), 2, fence_03_mask, BLACK, false);
        arduboy.drawCompressedMirror(60 + (heli.xPos - 2280), 2, fence_03, WHITE, false);
        break;

      case 2290 ... 2299:
        arduboy.drawCompressedMirror(68 + (heli.xPos - 2290), 2, fence_02_mask, BLACK, false);
        arduboy.drawCompressedMirror(68 + (heli.xPos - 2290), 2, fence_02, WHITE, false);
        break;

      case 2300 ... 2309:
        arduboy.drawCompressedMirror(76 + (heli.xPos - 2300), 2, fence_01_mask, BLACK, false);
        arduboy.drawCompressedMirror(76 + (heli.xPos - 2300), 2, fence_01, WHITE, false);
        break;

      case 2310 ... 2349:
        arduboy.drawCompressedMirror(84 + (heli.xPos - 2310), 2, fence_00_mask, BLACK, false);
        arduboy.drawCompressedMirror(84 + (heli.xPos - 2310), 2, fence_00, WHITE, false);
        break;

    }


    // Draw scoreboard ..

    drawScoreBoard(true);

  }


}

void drawBullet(const Bullet & bullet)
{
    if (bullet.xPos != BULLET_INACTIVE_X_VALUE) {
  
        if ((bullet.xPos > heli.xPos - 144) && (bullet.xPos < heli.xPos + 144)) {

          arduboy.fillRect(heli.xPos - bullet.xPos + 64 - 1, bullet.yPos, 2, 2, WHITE);

        }
    
    }
}

void drawExplosion(BulletExplosion *bulletExplosion) {

    switch (bulletExplosion->explosionType) {

      case ExplosionType::Small:
      case ExplosionType::Medium:

      case ExplosionType::Both_Small:
      case ExplosionType::Both_Medium:

        if ((bulletExplosion->xPos > heli.xPos - 144) && (bulletExplosion->xPos < heli.xPos + 144)) {

          arduboy.drawCompressedMirror(heli.xPos - bulletExplosion->xPos + 64 - 4, bulletExplosion->yPos, explosions[(uint8_t)bulletExplosion->explosionType], WHITE, false);
          bulletExplosion->explosionType = (ExplosionType)((uint8_t)bulletExplosion->explosionType - 1);
          if (bulletExplosion->explosionType == ExplosionType::None || bulletExplosion->explosionType == ExplosionType::Both_None) bulletExplosion->xPos = BULLET_INACTIVE_X_VALUE;

        }

        break;

      case ExplosionType::Large_1 ... ExplosionType::Large_4:

        if ((bulletExplosion->xPos > heli.xPos - 144) && (bulletExplosion->xPos < heli.xPos + 144)) {

          arduboy.drawCompressedMirror(heli.xPos - bulletExplosion->xPos + 64 - 12, bulletExplosion->yPos, explosions_masks[(uint8_t)bulletExplosion->explosionType], BLACK, false);
          arduboy.drawCompressedMirror(heli.xPos - bulletExplosion->xPos + 64 - 12, bulletExplosion->yPos, explosions[(uint8_t)bulletExplosion->explosionType], WHITE, false);

          bulletExplosion->explosionType = (ExplosionType)((uint8_t)bulletExplosion->explosionType + 1);
          if (bulletExplosion->explosionType == ExplosionType::Large_End) bulletExplosion->xPos = BULLET_INACTIVE_X_VALUE;

        }

        break;

      default: break;

    }
    
}


/* ----------------------------------------------------------------------------
 *  Draw the scoreboard in the top left hand corner ..
 */
void drawScoreBoard(bool all) {

  arduboy.drawCompressedMirror(-1, 0, digit_Cross, WHITE, false);
  arduboy.drawCompressedMirror(4, 0, imageArrayLookup(&digits[dead / 10]), WHITE, false);
  arduboy.drawCompressedMirror(9, 0, imageArrayLookup(&digits[dead % 10]), WHITE, false);

  if (all) {
    
    arduboy.drawCompressedMirror(14, 0, digit_Diamond, WHITE, false);
    arduboy.drawCompressedMirror(21, 0, imageArrayLookup(&digits[inHelicopter / 10]), WHITE, false);
    arduboy.drawCompressedMirror(26, 0, imageArrayLookup(&digits[inHelicopter % 10]), WHITE, false);
    
    arduboy.drawCompressedMirror(31, 0, digit_Heart, WHITE, false);
    arduboy.drawCompressedMirror(38, 0, imageArrayLookup(&digits[safe / 10]), WHITE, false);
    arduboy.drawCompressedMirror(43, 0, imageArrayLookup(&digits[safe % 10]), WHITE, false);

  }
  else {

    arduboy.drawCompressedMirror(14, 0, digit_Heart, WHITE, false);
    arduboy.drawCompressedMirror(21, 0, imageArrayLookup(&digits[safe / 10]), WHITE, false);
    arduboy.drawCompressedMirror(26, 0, imageArrayLookup(&digits[safe % 10]), WHITE, false);

  }

}

/* ----------------------------------------------------------------------------
 *  A better absolute!
 */
template<typename T> T absT(const T & v) {
  
  return (v < 0) ? -v : v;

}


AbsHelicopterStance absHelicopterStance(HelicopterStance helicopterStance) {

  return (AbsHelicopterStance)absT((int8_t)helicopterStance);

}

/* ----------------------------------------------------------------------------
 *  Draw a horizontal dotted line. 
 *  
 *  So much nicer than a solid line!
 */
void drawHorizontalDottedLine(int x1, int x2, int y) {
    
  for (int z = x1; z <= x2; z+=2) {
    arduboy.drawPixel(z, y, WHITE);
  }
  
}


/* ----------------------------------------------------------------------------
 *  Slow the helicopter down or speed it up expotentially ..
 */
int8_t calcSpeed(int8_t initValue, bool increase) {
      
  switch (initValue) {

    case 8:
      if (!increase) return 4;
    
    case 4: 
      if (increase)  return 4;
      if (!increase) return 2;
    
    case 2:
      if (increase)  return 4;
      if (!increase) return 1;

    case 1:
      if (increase)  return 2;
      if (!increase) return 0;

    case 0:
      if (increase)  return 1;
      if (!increase) return -1;

    case -1:
      if (increase)  return 0;
      if (!increase) return -2;

    case -2:
      if (increase)  return -1;
      if (!increase) return -4;

    case -4:
      if (increase)  return -2;
      if (!increase) return -4;

    case -8:
      if (increase)  return -4;

    default: return 0;

  }

}


/* ----------------------------------------------------------------------------
 *  Draw the helicopter ..
 */
void drawHelicopter(int8_t x, int8_t y, HelicopterStance image) {

  uint8_t index = (uint8_t)absHelicopterStance(image);
  bool mirror = ((int8_t)image < 0);

  arduboy.drawCompressedMirror(x, y, helicopter[((index - 1) * 6) + 1], BLACK, mirror);
  arduboy.drawCompressedMirror(x, y, helicopter[(index - 1) * 6], WHITE, mirror);
  arduboy.drawCompressedMirror(x, y, helicopter[((index - 1) * 6) + (frame) + 2], WHITE, mirror);

}


/* ----------------------------------------------------------------------------
 *  Reset the helicopter ready for the next sortie ..
 */
void resetSortie() {

  heli.stance = HelicopterStance::Left_Level;
  heli.xDelta = 0;
  heli.yDelta = 0;
  heli.xInc = DELTA_X_DO_NOTHING;
  heli.xPos = 0;
  heli.yPos = 40;
  heli.prevTurn = PREV_TURN_FROM_LEFT; 
  heli.hits = 0; 
  heli.countDown = HELICOPTER_COUNT_DOWN_INACTIVE;

  tankBulletExplosion.xPos = BULLET_INACTIVE_X_VALUE;
  playerBulletExplosion.xPos = BULLET_INACTIVE_X_VALUE;
  backgroundXOffset = 0;
  playerStack.clear();

}


/* ----------------------------------------------------------------------------
 *  Reset the all variables ready for the next game ..
 */
void resetGame() {

  resetSortie();

  heli.xPos = 160;
  dead = 0;
  inHelicopter = 0;
  safe = 0;

  introduction_count = 0;
  sortieNumber = 1;

  dormitories[0] = { DormitoryState::Open,    DORMITORY_SPACING,       0 };
  dormitories[1] = { DormitoryState::Intact,  (DORMITORY_SPACING * 2), 0 };
  dormitories[2] = { DormitoryState::Intact,  (DORMITORY_SPACING * 3), 0 };
  dormitories[3] = { DormitoryState::Intact,  (DORMITORY_SPACING * 4), 0 };

  tanks[0] = {TankState::Stationary, TurrentDirection::Left_Low, 5, (1 * (int16_t)DORMITORY_SPACING) + (int16_t)random(-DORMITORY_SPACING_HALF, DORMITORY_SPACING_HALF), (1 * (int16_t)DORMITORY_SPACING), true, 0};
  tanks[1] = {TankState::Stationary, TurrentDirection::Left_Low, 5, (2 * (int16_t)DORMITORY_SPACING) + (int16_t)random(-DORMITORY_SPACING_HALF, DORMITORY_SPACING_HALF), (2 * (int16_t)DORMITORY_SPACING), true, 0};
  tanks[2] = {TankState::Stationary, TurrentDirection::Left_Low, 5, (3 * (int16_t)DORMITORY_SPACING) + (int16_t)random(-DORMITORY_SPACING_HALF, DORMITORY_SPACING_HALF), (3 * (int16_t)DORMITORY_SPACING), true, 0};
  tanks[3] = {TankState::Stationary, TurrentDirection::Left_Low, 5, (4 * (int16_t)DORMITORY_SPACING) + (int16_t)random(-DORMITORY_SPACING_HALF, DORMITORY_SPACING_HALF), (4 * (int16_t)DORMITORY_SPACING), true, 0};
  tanks[4] = {TankState::Stationary, TurrentDirection::Left_Low, 5, (5 * (int16_t)DORMITORY_SPACING) + (int16_t)random(-DORMITORY_SPACING_HALF, DORMITORY_SPACING_HALF), (5 * (int16_t)DORMITORY_SPACING), true, 0};
 

  for (uint8_t i = 0; i < NUMBER_OF_HOSTAGES; i++) {

      hostages[i] = { (i < 16 ? HostageStance::Leaving_Dorm : HostageStance::In_Dorm), (uint8_t)(((i % 16) * 15) + 10), dormitories[i / 16].xPos };

  }

  for (uint8_t i = 0; i < NUMBER_OF_PLAYER_BULLETS; i++) {
     
      playerBullets[i] = { BULLET_INACTIVE_X_VALUE, 0, 0, 0, 0 };

  }

  for (uint8_t i = 0; i < NUMBER_OF_TANK_BULLETS; i++) {
     
      tankBullets[i] = { BULLET_INACTIVE_X_VALUE, 0, 0, 0, 0 };

  }

  gameState = GameState::Introduction;

}


/* ----------------------------------------------------------------------------
 *  Has the nominated bullet hot anything ?
 */
void bulletHit(Bullet *bullet, BulletExplosion *explosion, bool playerBullet) {


  // Check to see if a tank bullet hit the helicopter (it can be at any height) ..
  
  {
    Rect hitZone = {50, 0, 0, 0};

    if (!playerBullet) {

      switch (absHelicopterStance(heli.stance)) {

        case AbsHelicopterStance::Side_Level ... AbsHelicopterStance::Side_Incline_2:
        case AbsHelicopterStance::Side_Reverse_Incline_1 ... AbsHelicopterStance::Side_Reverse_Incline_5:
        case AbsHelicopterStance::Side_Incline_Rotate_3:
    
          hitZone.x = 50;
          hitZone.y = heli.yPos + 5;
          hitZone.width = 28;
          hitZone.height = 12;

          break;

        case AbsHelicopterStance::Side_Incline_3 ... AbsHelicopterStance::Side_Incline_Max:
    
          hitZone.x = 50;
          hitZone.y = heli.yPos + 7;
          hitZone.width = 28;
          hitZone.height = 12;

          break;

        case AbsHelicopterStance::Side_Level_Rotate_1 ... AbsHelicopterStance::Side_Front_Incline_2:
        case AbsHelicopterStance::Side_Incline_Rotate_1 ... AbsHelicopterStance::Side_Incline_Rotate_2:

          hitZone.x = 57;
          hitZone.y = heli.yPos + 5;
          hitZone.width = 14;
          hitZone.height = 13;

          break;

      }

      Point pt = { 64 + bullet->xPos - heli.xPos, bullet->yPos };

      if (arduboy.collide(pt, hitZone)) {

        explosion->xPos = bullet->xPos;
        explosion->yPos = hitZone.y - 4;
        explosion->explosionType = ExplosionType::Both_Medium;
        bullet->xPos = BULLET_INACTIVE_X_VALUE;

        heli.hits++;

        if (heli.hits == HELICOPTER_BULLET_NUMBER_OF_HITS) {

          sound.tones(exploding);

          explosion->explosionType = ExplosionType::Large_1;
          if (heli.countDown == HELICOPTER_COUNT_DOWN_INACTIVE) {
            
            heli.countDown++;
            sound.tones(exploding);

          }
          

          // Kill any hostages that were in the helicopter ..

          dead = dead + inHelicopter;
          inHelicopter = 0;

          for (int i = 0; i < NUMBER_OF_HOSTAGES; i++) {

            Hostage *hostage = &hostages[i];
            if (hostage->stance == HostageStance::In_Helicopter) hostage->stance = HostageStance::Dead;

          }

        }

      }

    }

  }



  // Check to see if we hit anything ..

  switch (bullet->yPos) {

    case 40 ... 46:               // Hitting a dormitory?

      for (uint8_t i = 0; i < NUMBER_OF_DORMITORIES; i++) {

        Dormitory *dormitory = &dormitories[i];
        uint16_t diff = absT(dormitory->xPos - bullet->xPos);
 
        if (dormitory->state == DormitoryState::Intact && diff < 16) {

            dormitory->numberOfHits++;
            
            explosion->xPos = bullet->xPos;
            explosion->yPos = 36 + (diff / 3);
            explosion->explosionType = ExplosionType::Medium;

            if (dormitory->numberOfHits == DORMITORY_HITS_REQUIRED) {

              sound.tones(exploding);

              dormitory->state = DormitoryState::Open;
              explosion->xPos = bullet->xPos;
              explosion->yPos = 36 + (diff / 3);
              explosion->explosionType = ExplosionType::Large_1;


              // Release the hostages ..

              uint8_t count = 0;

              for (uint8_t i = 0; i < NUMBER_OF_HOSTAGES; i++) {

                Hostage *hostage = &hostages[i];

                if (hostage->stance == HostageStance::In_Dorm) {
                  
                  hostage->stance = HostageStance::Leaving_Dorm;
                  hostage->xPos = dormitory->xPos + 16;
                  hostage->countDown = (count * 15) + 10;
                  count++;

                  if (count == DORMITORY_HOSTAGE_CAPACITY) { break; }

                }

              }

            } 

            bullet->xPos = BULLET_INACTIVE_X_VALUE;

        }

      }

      break;

    case 47 ... 52:               // Hitting a hostage or the tank?
      {
        bool hit = false;

        for (uint8_t i = 0; i < NUMBER_OF_HOSTAGES; i++) {

          Hostage *hostage = &hostages[i];
          uint16_t diff = hostage->xPos - bullet->xPos;

          if (hostage->stance >= HostageStance::Running_Left_1 && 
              hostage->stance <= HostageStance::Waving_22 && 
              diff < 3) {

            sound.tones(kill_hostage);

            explosion->xPos = bullet->xPos;
            explosion->yPos = 52;
            explosion->explosionType = ExplosionType::Medium;
            bullet->xPos = BULLET_INACTIVE_X_VALUE;

            hostage->stance = HostageStance::Dying_2;
            dead++;

            if (dead + safe == NUMBER_OF_HOSTAGES) {

              heli.countDown = HELICOPTER_END_OF_GAME_START;

            }

            hit = true;
            break;

          }

        }

        if (!hit && playerBullet) {

          for (uint8_t i = 0; i < NUMBER_OF_TANKS; i++) {

            Tank *tank = &tanks[i];
            uint16_t diff = absT(tank->xPos - bullet->xPos);

            if (bullet->startYPos > TANK_BULLET_MIN_Y_VALUE && diff < 16) {  // Bullets that hit tank must be fired from down low ..

              explosion->xPos = bullet->xPos;
              explosion->yPos = (diff < 8 ? 46 : 51);
              explosion->explosionType = ExplosionType::Medium;
              bullet->xPos = BULLET_INACTIVE_X_VALUE;

              tank->numberOfHits++;

              if (tank->numberOfHits == (level == LEVEL_EASY ? TANK_BULLET_NUMBER_OF_HITS_EASY : TANK_BULLET_NUMBER_OF_HITS_HARD) ) {

                sound.tones(exploding);

                explosion->yPos = 50;
                explosion->explosionType = ExplosionType::Large_1;
                tank->state = TankState::Dead3;

              }

            }

          }

        }

      }
      break;

    case 61 ... 74:               //  Hitting the ground ..

      sound.tones(miss); 

      explosion->xPos = bullet->xPos;
      explosion->yPos = 60;
      explosion->explosionType = ExplosionType::Small;
      bullet->xPos = BULLET_INACTIVE_X_VALUE;
      break;

  }

}

/* ----------------------------------------------------------------------------
 *  Turn sound off and commit to the EEPROM.
 */
void turnSoundOff() {

  arduboy.audio.off(); 
  arduboy.audio.saveOnOff();
    
}



/* ----------------------------------------------------------------------------
 *  Turn sound on and commit to the EEPROM.
 */
void turnSoundOn() {

  arduboy.audio.on(); 
  arduboy.audio.saveOnOff();
    
}

/* ----------------------------------------------------------------------------
 *  Helper function for using image lookup arrays
 */
const uint8_t * imageArrayLookup(const uint8_t * const * const array) {

    return reinterpret_cast<const uint8_t *>(pgm_read_ptr(array));

}

/* ----------------------------------------------------------------------------
 *  Alternate helper function for using image lookup arrays
 */
template< typename IntT > const uint8_t * imageArrayLookup(const uint8_t * const * const array, IntT index) {

    return reinterpret_cast<const uint8_t *>(pgm_read_ptr(&array[index]));

}

