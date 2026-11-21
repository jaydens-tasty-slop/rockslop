#include <Arduino.h>
#include "src/utils/Arduboy2Ext.h"
#include "src/images/Images.h"
#include "src/fonts/Font4x6.h"
#include "src/utils/Enums.h"
#include "src/utils/Level.h"
#include "src/entity/Entities.h"
#include "src/utils/Utils.h"
#include "src/utils/EEPROM_Utils.h"
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include "src/utils/FadeEffects.h"

#ifdef USE_ARDUBOYTONES
#include <ArduboyTones.h>
#endif

#ifdef USE_ATMLIB
#include "src/songs/song.h"
#include <ATMlib.h>
#endif

#include "src/songs/Songs.h"


Arduboy2Ext arduboy;
Font4x6 font4x6 = Font4x6();
Sprites sprite;

#ifdef USE_ARDUBOYTONES
ArduboyTones sound(arduboy.audio.enabled);
#endif

#ifdef USE_ATMLIB
ATMsynth ATM;
#endif

uint8_t fadeWidth;
uint8_t radioStation = 0;
uint8_t mainCarFrame = 0;
uint8_t previousRadioStation;
uint8_t introDelay = 0;
uint8_t alternate = 0;

GameState gameState = GameState::VSBoot;

Level level;
Player player(PLAYER_INDEX);
OtherCar car0(1);
OtherCar car1(2);
OtherCar car2(3);
OtherCar car3(4);
OtherCar *carControllerOnly[] = { &car0, &car1, &car2, &car3 };
Base *allCars[] = { &car0, &car1, &car2, &car3, &player };

FadeOutEffect fadeOutEffect;
FadeInEffect fadeInEffect;
HighScore highScore;
//uint8_t horizonIncrement = 0;
UQ8x8 horizonIncrement = 0;
CarController carController(carControllerOnly, allCars);
uint8_t carCounterBlink = 0;

void RenderScreen(/*Player *player, Enemy *enemies*/);


// ----------------------------------------------------------------------------
//  Setup
//
/* Arduino IDE auto-prototypes */
void Credits();
void HighScore();
void vsBoot();
void splashScreen();
void radioMusic();
void playGame();
void RenderScreen(uint8_t gear);
uint8_t determineOtherCarArrayIndex(OtherCar *otherCar);
void drawRoadSegment(int16_t x0, int16_t x1, uint8_t y01, int16_t x2, int16_t x3, uint8_t y23);

void setup() {

  arduboy.boot();
  arduboy.flashlight();
  arduboy.systemButtons();
  arduboy.audio.begin();
  arduboy.initRandomSeed();
  arduboy.setFrameRate(75);

  player.setY(39);

  EEPROM_Utils::initEEPROM(false);
  
  fadeOutEffect.reset(0, HEIGHT, 1);

}

void loop() {

  if (!arduboy.nextFrameDEV()) return;

  arduboy.pollButtons();

  switch (gameState) {

    case GameState::VSBoot:
      vsBoot();
      break;

    case GameState::SplashScreen_Init:
      arduboy.setFrameRate(75);
      gameState = GameState::SplashScreen;
      fadeInEffect.reset(0, HEIGHT, 1);
      // break; Fall-through intentional.

    case GameState::SplashScreen:
      splashScreen();
      break;

    case GameState::Credits:
      Credits();
      break;

    case GameState::PlayGame_Init:
      arduboy.setFrameRate(50);
      gameState = GameState::PlayGame_StartOfDay;
      player.setYDelta(0);
      player.setX(64 - PLAYER_WIDTH_HALF);
      player.setXOffset(0);
      player.setCarsPassedInit(50);
      carCounterBlink = 0;
      // break; Fall-through intentional.

    case GameState::PlayGame_StartOfDay:
    case GameState::PlayGame:
    case GameState::PlayGame_EndOfPlay:
      playGame();
      break;

     case GameState::GameOver_Init:
      gameState = GameState::GameOver;
      fadeInEffect.reset(14, 38, 2);
      // break; Fall-through intentional.
   
    case GameState::GameOver:
      playGame();
      break;

    case GameState::SaveScore:
      highScore.reset();
      highScore.setSlotNumber(EEPROM_Utils::saveScore(player.getOdometer(), player.getCarsPassed()));
      gameState = GameState::HighScore;
      fadeInEffect.reset(0, HEIGHT, 1);
      // break; Fall-through intentional.

    case GameState::HighScore:
      HighScore();
      break;

  }

  switch (gameState) {

    case GameState::PlayGame:
    case GameState::PlayGame_StartOfDay:
    case GameState::PlayGame_EndOfPlay:
    case GameState::GameOver_Init:
    case GameState::GameOver:
      arduboy.displayWithBackground(level.getTimeOfDay(), level.getHorizonY(), level.getBand());
      break;

    default:
      arduboy.display(true);
      break;

  }

}

#include "src/utils/Arduboy2Ext.h"

#define CREDITS_RANGE_INC       10
#define CREDITS_RANGE_1_START   0
#define CREDITS_RANGE_1_END     CREDITS_RANGE_INC - 1
#define CREDITS_RANGE_2_START   (CREDITS_RANGE_INC)
#define CREDITS_RANGE_2_END     (CREDITS_RANGE_INC * 2) - 1
#define CREDITS_RANGE_3_START   (CREDITS_RANGE_INC * 2)
#define CREDITS_RANGE_3_END     (CREDITS_RANGE_INC * 3) - 1
#define CREDITS_RANGE_4_START   (CREDITS_RANGE_INC * 3)
#define CREDITS_RANGE_4_END     (CREDITS_RANGE_INC * 4) - 1
#define CREDITS_RANGE_5_START   (CREDITS_RANGE_INC * 4)
#define CREDITS_RANGE_5_END     (CREDITS_RANGE_INC * 5) - 1
#define CREDITS_RANGE_6_START   (CREDITS_RANGE_INC * 5)
#define CREDITS_RANGE_6_END     (CREDITS_RANGE_INC * 6) - 1
#define CREDITS_RANGE_7_START   (CREDITS_RANGE_INC * 6)
#define CREDITS_RANGE_7_END     (CREDITS_RANGE_INC * 7) - 1


// --------------------------------------------------------------------------------------
//  Show credits screen ..
//
void Credits() {

  uint8_t const *filmoteImage = nullptr;
  uint8_t vampiricsImage = 0;


  // Draw logo and names ..

  Sprites::drawOverwrite(24, 13, credits, 0);


  // Display 'Press A' button after a short period of time ..

  introDelay++;

  switch (introDelay) {

    case CREDITS_RANGE_1_START ... CREDITS_RANGE_1_END:
      filmoteImage = filmote1;
      vampiricsImage = 0;
      break;

    case CREDITS_RANGE_2_START ... CREDITS_RANGE_2_END:
      filmoteImage = filmote1;
      vampiricsImage = 1;
      break;

    case CREDITS_RANGE_3_START ... CREDITS_RANGE_3_END:
      filmoteImage = filmote1;
      vampiricsImage = 2;
      break;

    case CREDITS_RANGE_4_START ... CREDITS_RANGE_4_END:
      filmoteImage = filmote2;
      vampiricsImage = 3;
      break;

    case CREDITS_RANGE_5_START ... CREDITS_RANGE_5_END:
      filmoteImage = filmote2;
      vampiricsImage = 4;
      break;

    case CREDITS_RANGE_6_START ... CREDITS_RANGE_6_END:
      filmoteImage = filmote2;
      vampiricsImage = 5;
      break;

    case CREDITS_RANGE_7_START ... CREDITS_RANGE_7_END:
      filmoteImage = filmote1;
      vampiricsImage = 0;
      break;

  }
  
  if (introDelay > CREDITS_RANGE_6_END) {
    introDelay = 0;
  }

  Sprites::drawOverwrite(38, 38, filmoteImage, 0);
  Sprites::drawOverwrite(76, 38, vampiricsBase, 0);
  Sprites::drawSelfMasked(76, 38, vampiricsAnimation, vampiricsImage);
  Sprites::drawOverwrite(32, 50, filmoteText, 0);
  Sprites::drawOverwrite(68, 50, vampriricsText, 0);


  // If 'A' bUtton is pressed move to introduction ..

  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) { 

    gameState = GameState::HighScore; 
    
  }

}

#include "src/utils/Arduboy2Ext.h"

uint8_t clearScores = 0;


// --------------------------------------------------------------------------------------
//  Render High Score screen ..
//

void HighScore() {

  uint8_t xOffset = 28;
  uint8_t yOffset = 23;

  Sprites::drawOverwrite(0, 0, highScoreImg, 0);

  for (uint8_t x =0; x < MAX_NUMBER_OF_SCORES; x++) {

    Slot slot;
    EEPROM_Utils::getSlot(x, &slot);

    Sprites::drawOverwrite(xOffset, yOffset, font_images, slot.getChar0());
    Sprites::drawOverwrite(xOffset + 6, yOffset, font_images, slot.getChar1());
    Sprites::drawOverwrite(xOffset + 12, yOffset, font_images, slot.getChar2());


    // Odometer ..
    {
      uint8_t digits[6] = {};
      extractDigits(digits, slot.getOdometer());
    
      for (uint8_t i = 0, x2 = xOffset + 49; i < 6; ++i, x2 -= 5) {
        Sprites::drawOverwrite(x2, yOffset, font_images, (digits[i] + 52));
      }
      
    }


    // Cars Passed ..

    {
      uint8_t digits[3] = {};
      extractDigits(digits, slot.getCarsPassed());
      
      for (uint8_t i = 0, x2 = xOffset + 70; i < 3; ++i, x2 -= 5) {
        Sprites::drawOverwrite(x2, yOffset, font_images, (digits[i] + 52));
      }
      
    }

    yOffset = yOffset + 8;

  }


  if (highScore.getSlotNumber() != DO_NOT_EDIT_SLOT) {

    yOffset = 23;
    alternate++;
  
    if (alternate < 40) {

      Sprites::drawOverwrite(xOffset - 4, yOffset + (highScore.getSlotNumber() * 8) - 1, arrow, 0);
      Sprites::drawOverwrite(xOffset + 76, yOffset + (highScore.getSlotNumber() * 8) - 1, arrow, 0);
    
    }
    else if (alternate > 80) {

      alternate = 0;

    }

    if (highScore.getCharIndex() == 0) {
      arduboy.fillRect(xOffset - 1, yOffset + (highScore.getSlotNumber() * 8) - 1, 6, 8, WHITE);
      Sprites::drawErase(xOffset, yOffset + (highScore.getSlotNumber() * 8), font_images, highScore.getChar(0));
    }
    else {
      Sprites::drawOverwrite(xOffset, yOffset + (highScore.getSlotNumber() * 8), font_images, highScore.getChar(0));
    }

    if (highScore.getCharIndex() == 1) {
      arduboy.fillRect(xOffset + 5, yOffset + (highScore.getSlotNumber() * 8) - 1, 6, 8, WHITE);
      Sprites::drawErase(xOffset + 6, yOffset + (highScore.getSlotNumber() * 8), font_images, highScore.getChar(1));
    }
    else {
      Sprites::drawOverwrite(xOffset + 6, yOffset + (highScore.getSlotNumber() * 8), font_images, highScore.getChar(1));
    }

    if (highScore.getCharIndex() == 2) {
      arduboy.fillRect(xOffset + 11, yOffset + (highScore.getSlotNumber() * 8) - 1, 6, 8, WHITE);
      Sprites::drawErase(xOffset + 12, yOffset + (highScore.getSlotNumber() * 8), font_images, highScore.getChar(2));
    }
    else {
      Sprites::drawOverwrite(xOffset + 12, yOffset + (highScore.getSlotNumber() * 8), font_images, highScore.getChar(2));
    }


    // Handle buttons ..

    uint8_t charIndex = highScore.getCharIndex();

    if (arduboy.justPressed(UP_BUTTON))       { highScore.incChar(charIndex); }
    if (arduboy.justPressed(DOWN_BUTTON))     { highScore.decChar(charIndex); }
    if (arduboy.justPressed(LEFT_BUTTON))     { highScore.decCharIndex(); } 
    if (arduboy.justPressed(RIGHT_BUTTON))    { highScore.incCharIndex(); } 

    if (arduboy.justPressed(A_BUTTON)) { 
      
      EEPROM_Utils::writeChars(highScore.getSlotNumber(), &highScore);
      highScore.disableEditting(); 
      
    }

  }
  else {


    // Clear scores ..

    if (arduboy.pressed(UP_BUTTON) && arduboy.pressed(DOWN_BUTTON)) {

      clearScores++;

      switch (clearScores) {

        case 21 ... 60:
          arduboy.setRGBled(128 - (clearScores * 2), 0, 0);
          break;

        case 61:
          clearScores = 0;
          arduboy.setRGBled(0, 0, 0);
          EEPROM_Utils::initEEPROM(true);
          sound.tone(NOTE_C6, 100);
          return;

      }

    }
    else {

      if (clearScores > 0) {
      
        arduboy.setRGBled(0, 0, 0);
        clearScores = 0;

      }
      
    }

    if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) { 
    
      arduboy.setRGBled(0, 0, 0);
      gameState = GameState::SplashScreen_Init; 
      
    }
    
  }

}

#include "src/utils/Arduboy2Ext.h"


// // ----------------------------------------------------------------------------
// //  Reset the fade effect .. 
// //
// void resetFade() {
//   fadeWidth = 0;
// }

// // ----------------------------------------------------------------------------
// //  Reset the fade in effect .. 
// //
// void resetFadeIn() {
//   fadeWidth = WIDTH;
// }


// // ----------------------------------------------------------------------------
// //  Fade In function .. 
// //
// bool fadeIn() {

//   for (uint8_t i = 0; i < (HEIGHT / 2); ++i)
//   {
//     arduboy.drawFastHLine(0, (i * 2), fadeWidth, BLACK);
//     arduboy.drawFastHLine((WIDTH - fadeWidth), (i * 2) + 1, fadeWidth, BLACK);
//   }

//   // If fade isn't complete, decrease the counter
//   if (fadeWidth > 0) 
//   {
//     fadeWidth = fadeWidth - 4;
//     return false;
//   }
//   else 
//   {
//     return true;
//   }

// }


// // ----------------------------------------------------------------------------
// //  Fade Out function .. 
// //
// bool fadeOut() 
// {

//   for (uint8_t i = 0; i < (HEIGHT / 2); ++i) 
//   {
//     arduboy.drawFastHLine(0, (i * 2), fadeWidth, BLACK);
//     arduboy.drawFastHLine((WIDTH - fadeWidth), (i * 2) + 1, fadeWidth, BLACK);
//   }

//   // If fade isn't complete, increase the counter
//   if(fadeWidth < WIDTH) 
//   {
//     ++fadeWidth;
//     ++fadeWidth;
//     return false;
//   }
//   else 
//   {
//     return true;
//   }
  
// }

void vsBoot() {

  // Vsoft logo display
  arduboy.drawBitmap(0, 0, bootlogo, 128, 64, WHITE);

  if (!fadeOutEffect.isComplete()) {

    fadeOutEffect.draw(arduboy);
    fadeOutEffect.update();

  }

  if (fadeOutEffect.isComplete()) gameState = GameState::SplashScreen_Init;

}

void splashScreen() {

  arduboy.drawBitmap(0, 0, SplashScreen, 128, 64, WHITE);

  font4x6.setCursor(74, 5);
  radioMusic();

  switch (radioStation) {

    case 0:
      font4x6.print(F("RADIO OFF"));
      break;

    case 1:
      font4x6.print(F("ARDX 89.9"));
      arduboy.drawLine(103, 25, 103, 28, WHITE);
      break;

    case 2:
      font4x6.print(F("ROCK 94.6"));
      arduboy.drawLine(113, 25, 113, 28, WHITE);
      break;

  }
  
  if (!fadeInEffect.isComplete()) {

    fadeInEffect.draw(arduboy);
    fadeInEffect.update();

  }

  if (arduboy.justPressed(LEFT_BUTTON) && radioStation > 0) {
    radioStation--;
    #ifdef USE_ARDUBOYTONES
    sound.tone(NOTE_C3,50, NOTE_D2,50, NOTE_E1,50);
    #endif
  }

  if (arduboy.justPressed(RIGHT_BUTTON) && radioStation < 2) {
    radioStation++;
    #ifdef USE_ARDUBOYTONES
    sound.tone(NOTE_C3,50, NOTE_D2,50, NOTE_E1,50);
    #endif
  }

  if (arduboy.justPressed(A_BUTTON)) { gameState = GameState::PlayGame_Init; }
  if (arduboy.justPressed(B_BUTTON)) { gameState = GameState::Credits; }
  
}

void radioMusic() 
{
  if(previousRadioStation != radioStation)
  {
    if (radioStation == 1)
    {
      #ifdef USE_ATMLIB
      ATM.play(music1);
      #endif
      #ifdef USE_ARDUBOYTONES
      sound.tones(song2small); 
      #endif
    }
    if (radioStation == 2)
    {
      #ifdef USE_ATMLIB
      ATM.play(music2);
      #endif
      #ifdef USE_ARDUBOYTONES
      sound.tones(song2); 
      #endif

    }
    if (radioStation == 0)
    {
      #ifdef USE_ATMLIB
      ATM.stop();
      #endif
    }
    previousRadioStation = radioStation;
  }
}


#include "src/utils/Arduboy2Ext.h"

bool allowChangeGears = true;
bool showGauges = true;
uint8_t clutchCounter = 0;

void playGame() {

  uint8_t speed = player.getYDelta().getInteger();
//  uint8_t frameDelay = player.getFrameDelay();
  UQ8x8 frameDelay = player.getFrameDelay();

  if (clutchCounter > 0 || speed > 1 || (speed == 1 && arduboy.pressed(A_BUTTON))) { 

    if (horizonIncrement >= frameDelay) {

      level.incHorizonY(&player);
      horizonIncrement = horizonIncrement - frameDelay;
      
    }

    horizonIncrement = horizonIncrement + 1;


    // Move car to outside of curve ..
    
    if (arduboy.isFrameCount(frameDelay.getInteger() * 4, 0)) {

      int8_t curve0 = level.getCurve(0);
      int8_t curve6 = level.getCurve(6);

      if (curve0 != 0 && curve6 != 0) {

        if (curve0 > 0) player.decX();
        if (curve0 < 0) player.incX();

      }

    }

  }


  // Update positions of other cars ..

  if (arduboy.isFrameCount(3, 0)) {

    carController.updatePositions(&player, speed);

  }


  RenderScreen(speed);


  // Move road and horizon scenery horizontally .. 

  if (arduboy.isFrameCount(FRAME_COUNT_HORIZON_X * (5 - speed))) {
    level.moveHorizontally(&arduboy, speed);
  }


  // Launch another car?
  
  if (gameState == GameState::PlayGame) {

    OtherCar *otherCar = carController.getInactiveCar();

    if (otherCar != nullptr) {

      if (frameDelay < 2) {
        
        OtherCar *otherCarLowestY = carController.getActiveCar_LowestY();

        if (otherCarLowestY == nullptr || otherCarLowestY->getY() > 20) {
          
          otherCar->setActive(true);
          otherCar->setX(random(OTHER_CAR_X_MIN, OTHER_CAR_X_MAX + 1));
          otherCar->setYDelta(SQ7x8::fromInternal(random((int16_t)(LAUNCH_SLOWER_SPEED * 256), (int16_t)(LAUNCH_FASTER_SPEED * 256))));
          otherCar->setXDelta(static_cast<Direction>(random(static_cast<int8_t>(Direction::Left), static_cast<int8_t>(Direction::Right) + 1)));
          otherCar->setTurnLength(random(0, OTHER_CAR_TURN_LENGTH_MAX + 1));

          switch (level.getTimeOfDay()) {
            
            case TimeOfDay::Dusk:
            case TimeOfDay::Dawn:
              otherCar->setY(DIST_6_BEGIN_DUSK);
              break;

            case TimeOfDay::Day:
              otherCar->setY(DIST_6_BEGIN_DAY);
              break;

            case TimeOfDay::Night:
              otherCar->setY(DIST_6_BEGIN_NIGHT);
              break;

          }

        }

      }
      else {

        OtherCar *otherCarHighestY = carController.getActiveCar_HighestY();

        if (otherCarHighestY == nullptr || otherCarHighestY->getY() < 65) {

          OtherCar *otherCar = carController.getInactiveCar();
          otherCar->setActive(true);
          otherCar->setYDelta(SQ7x8::fromInternal(random((int16_t)(LAUNCH_SLOWER_SPEED * 256), (int16_t)(LAUNCH_FASTER_SPEED * 256))));
          otherCar->setXDelta(static_cast<Direction>(random(static_cast<int8_t>(Direction::Left), static_cast<int8_t>(Direction::Right) + 1)));
          otherCar->setTurnLength(random(0, OTHER_CAR_TURN_LENGTH_MAX + 1));
          otherCar->setY(80);

          int8_t playerXCentered = player.getXCentered();

          int randMin = 0;
          int randMax = 0;

          switch (playerXCentered) {

            case -37 ... -17:
              randMin = -17 + PLAYER_WIDTH;
              randMax = OTHER_CAR_X_MAX;
              break;

            case -16 ... 0:
              randMin = PLAYER_WIDTH;
              randMax = OTHER_CAR_X_MAX;
              break;

            case 1 ... 16:
              randMin = OTHER_CAR_X_MIN;
              randMax = -PLAYER_WIDTH;
              break;

            case 17 ... 37:
              randMin = OTHER_CAR_X_MIN;
              randMax = 17 - PLAYER_WIDTH;
              break;

          }

          otherCar->setX(random(randMin, randMax + 1));

        }

      }

    }

    //}

  }
  

  // Handle player's keypresses ..

  if (gameState == GameState::PlayGame || gameState == GameState::PlayGame_StartOfDay) {
      
    if (arduboy.pressed(A_BUTTON) && speed > 0) { 

      if (arduboy.everyXFrames(2)) {
        mainCarFrame = !mainCarFrame;
      }
      allowChangeGears = true;

    }

    if (arduboy.pressed(LEFT_BUTTON) && speed > 0)     { player.decX(); }
    if (arduboy.pressed(RIGHT_BUTTON) && speed > 0)    { player.incX(); }

    if (allowChangeGears && !arduboy.pressed(A_BUTTON) && arduboy.justPressed(UP_BUTTON)) { 
      if (player.incYDelta()) horizonIncrement = 0; 
      allowChangeGears = false; 
      clutchCounter = 30; 
    }

    if (allowChangeGears && !!arduboy.pressed(A_BUTTON) && arduboy.justPressed(DOWN_BUTTON)) { 
      
      if (player.decYDelta()) horizonIncrement = 0; 
      allowChangeGears = false; 
      clutchCounter = 30;  
    
    }

    if (!arduboy.pressed(A_BUTTON) && clutchCounter == 0 && arduboy.everyXFrames(FRAME_RATE_16)) { 
      
      if (!player.decelerate()) allowChangeGears = true; 
      
    }

    if (arduboy.justPressed(B_BUTTON)) showGauges = !showGauges;

    if (clutchCounter > 0) clutchCounter--;

  }
  else {

    if (arduboy.everyXFrames(FRAME_RATE_16)) { 
      
      player.decelerate();
      
    }

    if (gameState == GameState::GameOver && arduboy.justPressed(A_BUTTON)) { gameState = GameState::SaveScore; }

  }



  // Increase start of day counter ..

  if (arduboy.isFrameCount(24, 0)) {

    if (gameState == GameState::PlayGame && level.getTimeLow()) {
     
      carCounterBlink++;

    }

    if (gameState == GameState::PlayGame_StartOfDay) {
     
      carCounterBlink++;
      if (carCounterBlink == 10) gameState = GameState::PlayGame;

    }

    if (gameState == GameState::PlayGame_EndOfPlay) {

      carCounterBlink++;
      if (carCounterBlink == 20) {
        
        gameState = GameState::GameOver_Init;

      }

    }

  }


  // Increase time of day ..  if a new day has

  if (gameState != GameState::PlayGame_EndOfPlay && level.incTime()) {

    if (player.getCarsPassed() > 0) {
      
      gameState = GameState::PlayGame_EndOfPlay;

    }

  }

}

#include "src/utils/Arduboy2Ext.h"


#define RENDER_CAR_COUNTER_LEFT     104
#define RENDER_CAR_COUNTER_TOP      3

#define RENDER_GEARBOX_RHS_LEFT     109
#define RENDER_GEARBOX_RHS_TOP      38

#define RENDER_GEARBOX_LHS_LEFT     1
#define RENDER_GEARBOX_LHS_TOP      39

#define RENDER_ODOMETER_LHS_LEFT    32
#define RENDER_ODOMETER_RHS_LEFT    120
#define RENDER_ODOMETER_TOP         56

bool drawOnLHS = true;


// Render the horizon ..

const uint8_t PROGMEM horizon[8] =            { 0, 2, 6, 12, 20, 30, 42, 56 };
const int16_t PROGMEM road_outside_left[8] =  { 53, 50, 46, 38, 29, 17, 2, -14 };
const int16_t PROGMEM road_outside_right[8] = { 74, 77, 82, 89, 98, 110, 125, 142 };
const int16_t PROGMEM road_marking_left[8] =  { 61, 60, 59, 57, 54, 51, 47, 42 };
const int16_t PROGMEM road_marking_right[8] = { 67, 67, 69, 71, 73, 77, 81, 85 };
const int8_t  PROGMEM curve_offset[8] =       { 32, 22, 14, 8, 4, 2, 1, 0 };

const int8_t  PROGMEM curve_offset1[8][32] = { 
                                      { 0,0,0,0,           // 0-7         Line 0
                                        0,0,0,0,           // 8-15
                                        0,0,0,0,           // 16-23
                                        0,0,0,0,           // 24-31
                                        0,0,0,0,           // 32-39
                                        0,0,0,0,           // 40-47
                                        0,0,0,0,           // 48-55
                                        0,0,0,0,},         // 56-63

                                      { 1,1,1,1,           // 0-7         Line 1
                                        1,1,1,2,           // 8-15
                                        0,0,0,0,           // 16-23
                                        0,0,0,0,           // 24-31
                                        0,0,0,0,           // 32-39
                                        0,0,0,0,           // 40-47
                                        0,0,0,0,           // 48-55
                                        0,0,0,0,},         // 56-63

                                      { 1,1,1,1,           // 0-7         Line 2
                                        1,1,1,1,           // 8-15
                                        0,0,0,0,           // 16-23
                                        0,0,0,0,           // 24-31
                                        0,0,0,0,           // 32-39
                                        0,0,0,0,           // 40-47
                                        0,0,0,0,           // 48-55
                                        0,0,0,0,},         // 56-63

                                      { 2,2,2,2,           // 0-7         Line 3
                                        2,2,2,2,           // 8-15
                                        2,2,0,0,           // 16-23
                                        0,0,0,0,           // 24-31
                                        0,0,0,0,           // 32-39
                                        0,0,0,0,           // 40-47
                                        0,0,0,0,           // 48-55
                                        0,0,0,0,},         // 56-63

                                      { 25,23,21,18,       // 0-7         Line 4
                                        16,14,12,11,       // 8-15
                                        9,8,7,6,           // 16-23
                                        5,5,4,4,           // 24-31
                                        4,4,3,3,           // 32-39
                                        3,2,2,2,           // 40-47
                                        2,2,2,1,           // 48-55
                                        0,0,0,0,},         // 56-63

                                      { 25,23,21,18,       // 0-7         Line 5
                                        16,14,12,11,       // 8-15
                                        9,8,7,6,           // 16-23
                                        5,5,4,4,           // 24-31
                                        3,3,3,3,           // 32-39
                                        3,2,2,2,           // 40-47
                                        2,2,2,1,           // 48-55
                                        0,0,0,0,},         // 56-63

                                      { 25,23,21,18,       // 0-7         Line 6
                                        16,14,12,11,       // 8-15
                                        9,8,7,6,           // 16-23
                                        5,5,4,4,           // 24-31
                                        3,3,3,3,           // 32-39
                                        3,2,2,2,           // 40-47
                                        2,2,2,1,           // 48-55
                                        0,0,0,0,},         // 56-63

                                      { 25,23,21,18,       // 0-7         Line 7
                                        16,14,12,11,       // 8-15
                                        9,8,7,6,           // 16-23
                                        5,5,4,4,           // 24-31
                                        3,3,3,3,           // 32-39
                                        3,2,2,2,           // 40-47
                                        2,2,2,1,           // 48-55
                                        0,0,0,0,}          // 56-63

                                    };

void RenderScreen(uint8_t gear) {

//  uint32_t m = micros();

  int8_t xPlayerOffset = player.getXOffset();

  for (uint8_t col = 0; col < HORIZON_COL_COUNT; col++) {

    int8_t curve0 = level.getCurve(col);
    int8_t curve1 = level.getCurve(col + 1);

    int8_t curveOffset0 = pgm_read_byte(&curve_offset[HORIZON_COL_COUNT - absT(curve0)]);
    if (curve0 < 0) curveOffset0 = -curveOffset0;
    curveOffset0  += xPlayerOffset;

    int8_t curveOffset1 = pgm_read_byte(&curve_offset[HORIZON_COL_COUNT - absT(curve1)]);
    if (curve1 < 0) curveOffset1 = -curveOffset1;
    curveOffset1  += xPlayerOffset;

    int16_t x1 = pgm_read_word_near(&road_outside_left[col]) + curveOffset0;
    uint8_t y1 = pgm_read_byte(&horizon[col]) + HORIZON_OFFSET;
    
    int16_t x2 = pgm_read_word_near(&road_outside_left[col + 1]) + curveOffset1;
    uint8_t y2 = pgm_read_byte(&horizon[col + 1]) + HORIZON_OFFSET;

    int16_t x3 = pgm_read_word_near(&road_outside_right[col]) + curveOffset0;
    int16_t x4 = pgm_read_word_near(&road_outside_right[col + 1]) + curveOffset1;


    drawRoadSegment(x1, x3, y1, x2, x4, y2);


    if (col > 0 && (col % 2 == level.getBand())) {

      const int16_t x5 = pgm_read_word_near(&road_marking_left[col]) + curveOffset0;
      const int16_t x6 = pgm_read_word_near(&road_marking_left[col + 1]) + curveOffset1;
      const int16_t x7 = pgm_read_word_near(&road_marking_right[col]) + curveOffset0;
      const int16_t x8 = pgm_read_word_near(&road_marking_right[col + 1]) + curveOffset1;

      const uint8_t col2 = col / 2;
      if (col2 > 0 && col2 < 4) {

        arduboy.drawLine(x5, y1, x6, y2);
        arduboy.drawLine(x7, y1, x8, y2);

      }

    }

  }


  // uint32_t n = micros();

  // Serial.print(", time:");
  // Serial.println(n - m);


  // Render horizon

  arduboy.fillRect(0, 0, WIDTH, pgm_read_byte(&horizon_heights[level.getTimeOfDayImageIndex()]), BLACK);
  Sprites::drawSelfMasked(level.getHorizonX(), 0, horizons, level.getTimeOfDayImageIndex());
  Sprites::drawSelfMasked(level.getHorizonX() + 128, 0, horizons, level.getTimeOfDayImageIndex());


  // Render other cars ..

  carController.sortCars();

  for (uint8_t i = 0; i < NUMBER_OF_CARS_INC_PLAYER; i++) {

    Base *baseCar = carController.getCarBase(carController.getSortedIndex(i));

    if (baseCar->getId() == PLAYER_INDEX) {

        Sprites::drawPlusMask(player.getX(), 39, mainCar, mainCarFrame);
        uint8_t dirtCloud = player.getDirtCloud();

        if (dirtCloud > 0) {

          Sprites::drawPlusMask(player.getX(), 57, dirt_cloud, (dirtCloud % 3));
          player.decDirtCloud();

        }

    }
    else {

      OtherCar *otherCar = (OtherCar *)baseCar;
      TimeOfDay timeOfDay = level.getTimeOfDay();

      if (otherCar->isActive() && otherCar->isVisible(timeOfDay)) {
        
        uint8_t mask = (timeOfDay == TimeOfDay::Night ? 1 : 0);

        uint8_t otherCarY = otherCar->getYDisplay();
        int8_t otherCarX = otherCar->getX();
        uint8_t colIndex = determineOtherCarArrayIndex(otherCar);
        int8_t curveIndex = level.getCurve(colIndex);

        int8_t offset = pgm_read_byte(&curve_offset1[absT(curveIndex)][otherCarY / 2]);
        if (curveIndex < 0) offset = -offset;

        uint8_t w = otherCar->getImageWidthHalf();
        int16_t x = 64 + ((otherCarX * otherCarY) / 39) + offset + xPlayerOffset;

        uint8_t index = static_cast<uint8_t>(otherCar->getImageSize());
        Sprites::drawPlusMask(x - w, otherCarY, opp_cars[index], mask);

      }

    }

  }


  // Render car counter ..
  
  if (showGauges && gameState != GameState::GameOver) {
      
    Sprites::drawPlusMask(RENDER_CAR_COUNTER_LEFT, RENDER_CAR_COUNTER_TOP, CarCounter, 0);

//    if (gameState == GameState::PlayGame || (gameState == GameState::PlayGame_StartOfDay && carCounterBlink % 2 == 0)) {
    if (carCounterBlink % 2 == 0) {

      uint8_t carsPassed = player.getCarsPassed();
      arduboy.setRGBled(0, 0, 0);

      Sprites::drawOverwrite(RENDER_CAR_COUNTER_LEFT +  2, RENDER_CAR_COUNTER_TOP + 2, numbers_white, carsPassed / 100);                           
      carsPassed = carsPassed % 100;
      Sprites::drawOverwrite(RENDER_CAR_COUNTER_LEFT +  8, RENDER_CAR_COUNTER_TOP + 2, numbers_white, carsPassed / 10);                           
      Sprites::drawOverwrite(RENDER_CAR_COUNTER_LEFT + 14, RENDER_CAR_COUNTER_TOP + 2, numbers_white, carsPassed % 10);

    }
    else {

      arduboy.fillRect(RENDER_CAR_COUNTER_LEFT + 2, RENDER_CAR_COUNTER_TOP + 2, 18, 8, BLACK);

      if (gameState == GameState::PlayGame) {
        arduboy.setRGBled(32, 0, 0);
      }
      else {
        arduboy.setRGBled(0, 32, 0);
      }

    }



    // Render gearbox and odometer ..
    {
      uint8_t xPos = 0;
      uint8_t yPos = 0; 

      switch (player.getX()) {

        case 0 ... 39:
          xPos = RENDER_GEARBOX_RHS_LEFT;
          yPos = RENDER_GEARBOX_RHS_TOP;
          drawOnLHS = false;
          break;

        case 40 ... 42:
          if (!drawOnLHS) {
            xPos = RENDER_GEARBOX_RHS_LEFT;
            yPos = RENDER_GEARBOX_RHS_TOP;
            drawOnLHS = false;
          }
          else {
            xPos = RENDER_GEARBOX_LHS_LEFT;
            yPos = RENDER_GEARBOX_LHS_TOP;
            drawOnLHS = true;
          }
          break;

        case 43 ... 46:
          if (!drawOnLHS) {
            xPos = RENDER_GEARBOX_RHS_LEFT;
            yPos = RENDER_GEARBOX_RHS_TOP;
            drawOnLHS = false;
          }
          else {
            xPos = RENDER_GEARBOX_LHS_LEFT;
            yPos = RENDER_GEARBOX_LHS_TOP;
            drawOnLHS = true;
          }
          break;

        case 47 ... 85:
          xPos = RENDER_GEARBOX_LHS_LEFT;
          yPos = RENDER_GEARBOX_LHS_TOP;
          drawOnLHS = true;
          break;

      }

      Sprites::drawPlusMask(xPos, yPos, gearbox, 0);

      static const uint8_t gearLookup[] PROGMEM = { 2, 3, 4, 3, };
	        
      switch (gear) {

        case 0:
          {
            const uint8_t index = (arduboy.getFrameCount(16) / 4);
            xPos += pgm_read_byte(&gearLookup[index]);
            yPos += 6;
          }
          break;
        
        case 1:                                      break;
        case 2:                           yPos+= 11; break;
        case 3:               xPos+= 6;              break;
        case 4:               xPos+= 6;   yPos+= 11; break;

      }

      Sprites::drawPlusMask(xPos, yPos, gearbox_knob, 0);


      // Score ..
      {
        uint8_t digits[5] = {};
        extractDigits(digits, player.getOdometer() / 10);
        
        for (uint8_t i = 0, x2 = (drawOnLHS ? RENDER_ODOMETER_LHS_LEFT : RENDER_ODOMETER_RHS_LEFT) - 7; i < 5; ++i, x2 -= 6) {
          Sprites::drawOverwrite(x2, RENDER_ODOMETER_TOP, numbers_white, digits[i]);
        }

        Sprites::drawOverwrite((drawOnLHS ? RENDER_ODOMETER_LHS_LEFT : RENDER_ODOMETER_RHS_LEFT), RENDER_ODOMETER_TOP, numbers_black, player.getOdometer() % 10);
        
      }

    }

  }



  // Display start of day logo?

  if (gameState == GameState::PlayGame_StartOfDay) {

    arduboy.fillRect(15, 23, 98, 18, BLACK);
    arduboy.drawFastHLine(16, 24, 96, WHITE);
    arduboy.drawFastHLine(16, 40, 96, WHITE);

    font4x6.setCursor(18, 26);
    font4x6.print(F("Day 00 Target 050"));

  }

  if (gameState == GameState::GameOver) {

    Sprites::drawOverwrite(0, 14, GameOverBanner, 0);

    if (!fadeInEffect.isComplete()) {

      fadeInEffect.draw(arduboy);
      fadeInEffect.update();

    }    

  }

}

uint8_t determineOtherCarArrayIndex(OtherCar *otherCar) {

  for (uint8_t i = 0; i < HORIZON_COL_COUNT; i ++) {

    if (otherCar->getY() < horizon[i] + HORIZON_OFFSET) {

      return i;

    }

  }

  return 0;

}

void drawRoadSegment(int16_t x0, int16_t x1, uint8_t y01, int16_t x2, int16_t x3, uint8_t y23) {
	
 	const int8_t dy = y23 - y01;
 	const int8_t dx02 = x2 - x0;
 	const int8_t dx13 = x3 - x1;
	
	const SQ7x8 xRatio = SQ7x8(dx02) / SQ7x8(dy);
	const SQ7x8 yRatio = SQ7x8(dx13) / SQ7x8(dy);
	
	SQ7x8 sb = 0;
	SQ7x8 sc = 0;

	for (int16_t y = y01; y <= y23; ++y) {

		if (y >= 64) break;
	
		const int8_t a = x0 + static_cast<int8_t>(sb);
		const int8_t d = x1 + static_cast<int8_t>(sc);
		sb += xRatio;
		sc += yRatio;

		arduboy.drawFastHLine(a, y, 2, WHITE);  
		arduboy.drawFastHLine(a + 2, y, d - a - 3, BLACK);  
		arduboy.drawFastHLine(d - 1, y, 2, WHITE); 
		
	}

}

