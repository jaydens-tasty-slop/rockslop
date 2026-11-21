#include <Arduino.h>
#include "src/Utils/Arduboy2Ext.h"

#include <ArduboyTones.h>
#include "src/Images/Images.h"
#include "src/Entities/Entities.h"
#include "src/Utils/Utils.h"
#include "src/Utils/Constants.h"
#include "src/Utils/EEPROM_Utils.h"
#include "src/Utils/FadeEffects.h"
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include "src/Fonts/Font4x6.h"
#include "src/Utils/HighScoreEditor.h"
#include "src/Sounds/Themes.h"

Arduboy2Ext arduboy;
ArduboyTones sound(arduboy.audio.enabled);
Sprites sprite;

Player player;
Customer customer;

Font4x6 font4x6 = Font4x6(0);

FadeOutEffect fadeOutEffect;
FadeInEffect fadeInEffect;
HighScore highScore;


// Global Variables now --------------------------------------------------------

GameState state = GameState::PPOT_Init;
GameState nextState = GameState::PPOT_Init;

uint8_t levelNumber = 0;
uint8_t thrusterFrame = 0;
int16_t backdropx = 0;
uint8_t alternate = 0;
uint8_t fareCount = 0;
uint8_t dollarsCount = 0;
uint8_t flashingCounter = 0;
uint8_t bootCounter = 50;
uint8_t gotoCounter = 0;
uint8_t ouchCounter = 0;
uint8_t gateToRender = 1; // For introduction.
bool starsInitialised = false;

Level level;


//------------------------------------------------------------------------------
//  Setup 
//------------------------------------------------------------------------------

/* Arduino IDE auto-prototypes */
void gameoverScreen();
void renderHighScore(HighScore &highScore);
void titleScreen(Level &level, Player &player, Customer &customer);
bool canMoveLeft(Level &level, uint16_t playerXPosition, uint16_t playerYPosition);
bool canMoveRight(Level &level, uint16_t playerXPosition, uint16_t playerYPosition);
bool canMoveUp(Level &level, uint16_t playerXPosition, uint16_t playerYPosition);
bool canMoveDown(Level &level, uint16_t playerXPosition, uint16_t playerYPosition, uint8_t size);
void moveCab(Level &level, Player &player, Customer &customer);
void handleInput(Player &player);
void launchCustomer(Level &level, Customer &customer, uint8_t defaultStartPosition, uint8_t defaultEndingPosition);
void checkCollisionWithCustomer(Level &level, Player &player, Customer &customer);
void checkCollisionWithLevelElements(Level &level, Player &player);
void checkCollisionWithLevelElements_TestElement(Level &level, Player &player, uint8_t x, uint8_t y, uint8_t tile);
void updateStatus(Player &player, Customer &customer);
void inGame(Font4x6 &font4x6, Level &level, Player &player, Customer &customer);
void drawLevel_Intro(Level &level, uint8_t gateToRender);
void drawLevel(Level &level);
void drawHUD(Font4x6 &font4x6, Player &player, Customer &customer);
void scrollingBackground(bool scrollLeft);
void playerDisplay(Player &player);
void customerDisplay(Level &level, Player &player, Customer &customer, int16_t customerXVal, int16_t customerYVal);
void drawDollars(Player &player);
void drawGoto(Level &level, Player &player, Customer &customer, int16_t customerXVal, int16_t customerYVal);
void drawOuch(Customer &customer, int16_t customerXVal, int16_t customerYVal);
void splashScreen_Init();
void splashScreen();
void initialiseStars(void);
void resetStars(void);
void updateStarfield(void);
void drawStarfield();
void drawLevelStart(Font4x6 &font4x6, Level &level);
void drawMaxLevelCompleted(Font4x6 &font4x6);

void setup() {
  
  arduboy.boot();
  arduboy.display();
  arduboy.flashlight();
  arduboy.systemButtons();
  arduboy.audio.begin();
  arduboy.clear();
  arduboy.setFrameRate(60);

  EEPROM_Utils::initEEPROM(false);
    
}

void loop() {
  
  if (!(arduboy.nextFrame())) return;

  arduboy.pollButtons();

  switch (state) {

    case GameState::PPOT_Init:
      splashScreen_Init();
      break;

    case GameState::PPOT:
      splashScreen();
      break;

    case GameState::SplashScreen_Init:
      gotoCounter = 0;
      ouchCounter = 0;
      levelNumber = 0;
      gateToRender = 1;
      initLevel(level, player, customer, levelNumber);
      launchCustomer(level, customer, gateToRender, GO_TO_GATE);
      player.setFrame(0);
      state = GameState::SplashScreen;
      fadeInEffect.reset(0, HEIGHT, 1);
      
      // break;  -- Fall through intentional.

    case GameState::SplashScreen:
      titleScreen(level, player, customer);
      break;

    case GameState::PlayGame_InitGame:
      levelNumber = 1;
      initGame(player);
      state = GameState::PlayGame_LevelIntroduction_Init;
      // break;  -- Fall through intentional.

    case GameState::PlayGame_LevelIntroduction_Init:
      level.reset(levelNumber);
      sound.tones(LevelTransition_Theme);
      state = GameState::PlayGame_LevelIntroduction;
      // break;  -- Fall through intentional.

    case GameState::PlayGame_LevelIntroduction:
      drawLevelStart(font4x6, level);
      nextState = GameState::PlayGame_FlashingCar;
      break;

    case GameState::PlayGame_InitLevel:
      thrusterFrame = 0;
      initLevel(level, player, customer, levelNumber);
      launchCustomer(level, customer, RANDOM_START_POSITION, RANDOM_END_POSITION);
      state = nextState;
      // break;  -- Fall through intentional.
        
    case GameState::PlayGame_FlashingCar:
    case GameState::PlayGame:
      #ifdef DEBUG
      Serial.print("PX: ");
      Serial.print(player.getX().getInteger());
      Serial.print(", PY: ");
      Serial.print(player.getY().getInteger());
      Serial.print(" - LX: ");
      Serial.print(level.xOffset.getInteger());
      Serial.print(", LY: ");
      Serial.print(level.yOffset.getInteger());
      Serial.println(", Fuel: ");
      #endif

      inGame(font4x6, level, player, customer);
      break;

    case GameState::MaxLevelCompleted:
      drawMaxLevelCompleted(font4x6);
      break;

    case GameState::GameOver_Init:
      sound.tones(GameOver_Theme);
      state = GameState::GameOver;
      // break;  -- Fall through intentional.

    case GameState::GameOver:
      gameoverScreen();
      levelNumber = 0;
      break;

    case GameState::SaveScore:
      highScore.reset();
      highScore.setSlotNumber(EEPROM_Utils::saveScore(player.getScore()));
      state = GameState::HighScore;
      fadeInEffect.reset(0, HEIGHT, 1);
      // break; Fall-through intentional.

    case GameState::HighScore:
      renderHighScore(highScore);
      break;

  }

  arduboy.display(true);

  switch (state) {

    case GameState::SplashScreen:
    case GameState::PlayGame:
      moveCab(level, player, customer);
      break;

    default: break;

  }

}


#include "src/Utils/Arduboy2Ext.h"

//__attribute__((noinline)) 
void gameoverScreen() {

  scrollingBackground(true);
  arduboy.fillRect(0, 16, 128, 31, BLACK);
  arduboy.drawLine(0, 13, 128, 13, BLACK);
  arduboy.drawLine(0, 15, 128, 15, BLACK);
  arduboy.drawLine(0, 17, 128, 17, WHITE);
  arduboy.drawLine(0, 45, 128, 45, WHITE);
  arduboy.drawLine(0, 47, 128, 47, BLACK);
  arduboy.drawLine(0, 49, 128, 49, BLACK);
  Sprites::drawOverwrite(25, 20, gameover, 0);

  font4x6.setCursor(36, 35);
  font4x6.print(F("SCORE:"));

  uint8_t digits[5];
  extractDigits(digits, player.getScore());
  for (uint8_t i = 5; i > 0; --i) 
  font4x6.print(digits[i - 1]);
  
  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    state = GameState::SaveScore;
  }
  
}

#include "src/Utils/Arduboy2Ext.h"

uint8_t clearScores = 0;


// --------------------------------------------------------------------------------------
//  Render High Score screen ..
//

void renderHighScore(HighScore &highScore) {

  uint8_t xOffset = 35;
  uint8_t yOffset = 21;

  scrollingBackground(false);

  arduboy.fillRect(23, 0, 82, 64, BLACK);
  arduboy.drawFastVLine(24, 0, 64, WHITE);
  arduboy.drawFastVLine(103, 0, 64, WHITE);

  Sprites::drawOverwrite(28, 6, highscoreText, 0);
  
  for (uint8_t x =0; x < MAX_NUMBER_OF_SCORES; x++) {

    Slot slot = EEPROM_Utils::getSlot(x);

    Sprites::drawOverwrite(xOffset, yOffset, font4x6_Full, slot.getChar0());
    Sprites::drawOverwrite(xOffset + 6, yOffset, font4x6_Full, slot.getChar1());
    Sprites::drawOverwrite(xOffset + 12, yOffset, font4x6_Full, slot.getChar2());


    // Score ..
    {
      uint8_t digits[6] = {};
      extractDigits(digits, slot.getScore());
    
      for (uint8_t i = 0, x2 = xOffset + 54; i < 6; ++i, x2 -= 5) {
        Sprites::drawOverwrite(x2, yOffset, font4x6_Full, (digits[i] + 52));
      }
      
    }

    yOffset = yOffset + 8;

  }


  if (highScore.getSlotNumber() != DO_NOT_EDIT_SLOT) {

    yOffset = 21;
    alternate++;
  
    if (alternate < 15) {

      Sprites::drawOverwrite(xOffset - 5, yOffset + (highScore.getSlotNumber() * 8) - 1, arrow, 0);
      Sprites::drawOverwrite(xOffset + 62, yOffset + (highScore.getSlotNumber() * 8) - 1, arrow, 0);
    
    }
    else if (alternate > 30) {

      alternate = 0;

    }

    if (highScore.getCharIndex() == 0) {
      arduboy.fillRect(xOffset - 1, yOffset + (highScore.getSlotNumber() * 8) - 1, 6, 8, WHITE);
      Sprites::drawErase(xOffset, yOffset + (highScore.getSlotNumber() * 8), font4x6_Full, highScore.getChar(0));
    }
    else {
      Sprites::drawOverwrite(xOffset, yOffset + (highScore.getSlotNumber() * 8), font4x6_Full, highScore.getChar(0));
    }

    if (highScore.getCharIndex() == 1) {
      arduboy.fillRect(xOffset + 5, yOffset + (highScore.getSlotNumber() * 8) - 1, 6, 8, WHITE);
      Sprites::drawErase(xOffset + 6, yOffset + (highScore.getSlotNumber() * 8), font4x6_Full, highScore.getChar(1));
    }
    else {
      Sprites::drawOverwrite(xOffset + 6, yOffset + (highScore.getSlotNumber() * 8), font4x6_Full, highScore.getChar(1));
    }

    if (highScore.getCharIndex() == 2) {
      arduboy.fillRect(xOffset + 11, yOffset + (highScore.getSlotNumber() * 8) - 1, 6, 8, WHITE);
      Sprites::drawErase(xOffset + 12, yOffset + (highScore.getSlotNumber() * 8), font4x6_Full, highScore.getChar(2));
    }
    else {
      Sprites::drawOverwrite(xOffset + 12, yOffset + (highScore.getSlotNumber() * 8), font4x6_Full, highScore.getChar(2));
    }


    // Handle buttons ..

    uint8_t charIndex = highScore.getCharIndex();

    if (arduboy.justPressed(UP_BUTTON))       { highScore.incChar(charIndex); }
    if (arduboy.justPressed(DOWN_BUTTON))     { highScore.decChar(charIndex); }
    if (arduboy.justPressed(LEFT_BUTTON))     { highScore.decCharIndex(); } 
    if (arduboy.justPressed(RIGHT_BUTTON))    { highScore.incCharIndex(); } 

    if (arduboy.justPressed(A_BUTTON)) { 
      
      EEPROM_Utils::writeChars(highScore.getSlotNumber(), highScore);
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
      state = GameState::SplashScreen_Init; 
      
    }
    
  }

}

#include "src/Utils/Arduboy2Ext.h"

// void vsBoot() {
  
//   Sprites::drawOverwrite(30, 17, bootlogo, 0);

//   switch (bootCounter) {

//     case 0:
      
//       if (!fadeOutEffect.isComplete()) {

//         fadeOutEffect.draw(arduboy);
//         fadeOutEffect.update();

//       }

//       if (fadeOutEffect.isComplete())   state = GameState::SplashScreen_Init;

//       break;

//     case 1:

//       fadeOutEffect.reset(0, HEIGHT, 1);
//       --bootCounter;
//       break;

//     default:
  
//       --bootCounter;
//       break;
      
//   }

// }

void titleScreen(Level &level, Player &player, Customer &customer) {
  
  int16_t customerXVal = customer.getX() + level.xOffset.getInteger();
  int16_t customerYVal = customer.getY() + level.yOffset.getInteger();

  if (gotoCounter != 0)   gotoCounter--;

  if (ouchCounter != 0) { 

    ouchCounter--;
    if (ouchCounter == 0) {
      gateToRender = (gateToRender == 0 ? 1 : 0);
      launchCustomer(level, customer, gateToRender, GO_TO_GATE);
    }

  }

  Sprites::drawOverwrite(0, 7, SpaceCabSplash_1, 0);
  Sprites::drawOverwrite(29, 31, SpaceCabSplash_2, 0);
  drawLevel_Intro(level, gateToRender);

  playerDisplay(player);
  customerDisplay(level, player, customer, customerXVal, customerYVal);
  handleInput(player);


  checkCollisionWithCustomer(level, player, customer);

  drawGoto(level, player, customer, customerXVal, customerYVal);
  drawOuch(customer, customerXVal, customerYVal);

  if (player.getY() >= 41 && player.getYDelta()> 0) {
    player.setLandingGearDown(true);
  } 
  
}

#include "src/Utils/Arduboy2Ext.h"


// ----------------------------------------------------------------------------------------------------------
//  Can the player move left ?
// ----------------------------------------------------------------------------------------------------------

bool canMoveLeft(Level &level, uint16_t playerXPosition, uint16_t playerYPosition) {
 

  // We are not yet at the left hand edge of tile so movement is possible ..

  if (playerXPosition % TILE_SIZE != 0) return true;
	

  // Retrieve the tile from the level defintion for x, y1 ..

  uint8_t tileX = (playerXPosition / 8) - 1;
  uint8_t tileY1 = (playerYPosition / 8);
  uint8_t tile1 = level.getLevelData(tileX, tileY1);

  if (isTileSolid(tile1)) return false;


  // Are we are actually straddling two tiles vertically?  If so test the second tile ..

  if (playerYPosition % TILE_SIZE != 0) {

    uint8_t tileY2 = (playerYPosition / 8) + 1;
    uint8_t tile2 = level.getLevelData(tileX, tileY2);

    if (isTileSolid(tile2)) return false;

  }

  return true;

}


// ----------------------------------------------------------------------------------------------------------
//  Can the player move right ?
// ----------------------------------------------------------------------------------------------------------

bool canMoveRight(Level &level, uint16_t playerXPosition, uint16_t playerYPosition) {


  // We are not yet at the right hand edge of tile so movement is possible..

  if ((playerXPosition + PLAYER_WIDTH) % TILE_SIZE != TILE_SIZE - 1) return true;


  // Retrieve the tile from the level defintion for x, y1 ..
  
  uint8_t tileX = ((playerXPosition + PLAYER_WIDTH) / 8) + 1;
  uint8_t tileY1 = (playerYPosition / 8);
  uint8_t tile1 = level.getLevelData(tileX, tileY1);

  if (isTileSolid(tile1)) return false;


  // Are we are actually straddling two tiles vertically?  If so test the second tile ..

  if (playerYPosition % TILE_SIZE != 0) {

    uint8_t tileY2 = (playerYPosition / 8) + 1;
    uint8_t tile2 = level.getLevelData(tileX, tileY2);

    if (isTileSolid(tile2)) return false;

  }

  return true;
  
}


// ----------------------------------------------------------------------------------------------------------
//  Can the player move up ?
// ----------------------------------------------------------------------------------------------------------

bool canMoveUp(Level &level, uint16_t playerXPosition, uint16_t playerYPosition) {


  // We are not yet at the top edge of tile so movement is possible ..

  if (playerYPosition % TILE_SIZE != 0) return true;


  // If we are at the top of the screen then we can go no higher ..

  if (playerYPosition == 0) return false;


  // The player is 17 pixels wide so always straddles 3 tiles .. 

  uint8_t tileY = (playerYPosition / 8) - 1;
  uint8_t tileX1 = (playerXPosition / 8);
  uint8_t tileX2 = (playerXPosition / 8) + 1;
  uint8_t tileX3 = (playerXPosition / 8) + 2;


  // Retrieve the the three tiles and test them in order ..

  uint8_t tile1 = level.getLevelData(tileX1, tileY);
  if (isTileSolid(tile1)) return false;
	
  uint8_t tile2 = level.getLevelData(tileX2, tileY);
  if (isTileSolid(tile2)) return false;
	
  uint8_t tile3 = level.getLevelData(tileX3, tileY);
  if (isTileSolid(tile3)) return false;

  return true;

}



// ----------------------------------------------------------------------------------------------------------
//  Can the player move down ?
// ----------------------------------------------------------------------------------------------------------

bool canMoveDown(Level &level, uint16_t playerXPosition, uint16_t playerYPosition, uint8_t size) {


  // We are not yet at the bottom edge of tile so movement is possible ..

  if ((playerYPosition + size) % TILE_SIZE != 0) return true;


  // We are at the bottom of the level so no further movement is possible ..

  if ((playerYPosition + size) == level.getHeight()) return false;


  // The player is 17 pixels wide so always straddles 3 tiles .. 

  uint8_t tileY = ((playerYPosition + size) / 8);
  uint8_t tileX1 = (playerXPosition / 8);
  uint8_t tileX2 = (playerXPosition / 8) + 1;
  uint8_t tileX3 = (playerXPosition / 8) + 2;


  // Retrieve the three tiles and test them in order ..

  uint8_t tile1 = level.getLevelData(tileX1, tileY);
  if (isTileSolid(tile1)) return false;
	
  uint8_t tile2 = level.getLevelData(tileX2, tileY);
  if (isTileSolid(tile2)) return false;
	
  uint8_t tile3 = level.getLevelData(tileX3, tileY);
  if (isTileSolid(tile3)) return false;

  return true;

}


// ----------------------------------------------------------------------------------------------------------
//  Move the cab ..
// ----------------------------------------------------------------------------------------------------------

void moveCab(Level &level, Player &player, Customer &customer) {

  SQ15x16 playerYDeltaVal = player.getYDeltaVal();
  SQ15x16 playerXDeltaVal = player.getXDeltaVal();

  uint16_t playerXPosition = player.getXDisplay() - level.getXOffsetDisplay();
  uint16_t playerYPosition = player.getYDisplay() - level.getYOffsetDisplay();


  // --  Moving up -----------------------------------------------------------------------------------------

  if (player.getYDelta() < 0) { 

    if (canMoveUp(level, playerXPosition, playerYPosition)) {

      player.decRetractLandingGear();
      player.setJustLanded(false);
      fuelSound[0] = 0;

      if (level.yOffset < 0) {

        if (player.getY() < PLAYER_Y_CENTRE) {
          if (level.yOffset < playerYDeltaVal) {
            level.yOffset = level.yOffset - playerYDeltaVal;
          }
          else {
            level.yOffset = 0;
          }
        }
        else if (player.getY() == PLAYER_Y_CENTRE) {
          if (level.yOffset < PLAYER_Y_CENTRE && (level.getWidthInTiles() != 16 || level.getHeightInTiles() != 8)) {
            level.yOffset = level.yOffset - playerYDeltaVal;
          }
          else {
            player.setY(player.getY() + playerYDeltaVal - level.yOffset);
            level.yOffset = 0;
          }
        }
        else if (player.getY() > PLAYER_Y_CENTRE) {
          if (player.getY() + playerYDeltaVal > PLAYER_Y_CENTRE) {
            player.setY(player.getY() + playerYDeltaVal);
          }
          else {
            level.yOffset = level.yOffset + PLAYER_Y_CENTRE - player.getY() - playerYDeltaVal;
            player.setY(PLAYER_Y_CENTRE);
          }
        }

      }
      else {

        if (player.getY() > -playerYDeltaVal) {
          player.setY(player.getY() + playerYDeltaVal);
        }
        else {
          player.setY(0);
        }

      }

    }
    else {

      // Are we at the top of the screen ?

      if (player.getY().getInteger() == 0) {

        player.setScore(player.getScore() + customer.getFare());
        sound.tones(coinsSound);

        if (levelNumber == MAX_NUMBER_OF_LEVELS) {
          //TODO End of GameState
          state = GameState::MaxLevelCompleted;
        }
        else {
          if (state == GameState::SplashScreen) {
            state = GameState::PlayGame_InitGame;
          }
          else {
            state = GameState::PlayGame_LevelIntroduction_Init;
            levelNumber++;
          }
          player.setFaresCompleted(0);
        }

      }

    }
    
  }


  // --  Moving down --------------------------------------------------------------------------------------------

  if (player.getYDelta() > 0) { 
    
    if (canMoveDown(level, playerXPosition, playerYPosition, player.getHeight())) {
      if (player.getY() < PLAYER_Y_CENTRE) {                         
        if (player.getY() + playerYDeltaVal < PLAYER_Y_CENTRE) {
          player.setY(player.getY() + playerYDeltaVal);
        }
        else {
          if (level.getWidthInTiles() != 16 || level.getHeightInTiles() != 8) {
            level.yOffset = PLAYER_Y_CENTRE - player.getY() - playerYDeltaVal;
          }
          player.setY(PLAYER_Y_CENTRE);
        }
      }
      else if (player.getY() == PLAYER_Y_CENTRE) {    
        
        if (level.yOffset - playerYDeltaVal > -level.getHeight() + HEIGHT && (level.getWidthInTiles() != 16 || level.getHeightInTiles() != 8)) {
          level.yOffset = level.yOffset - playerYDeltaVal;
        }
        else {
          // player.setY(player.getY() + ((level.getHeight() - HEIGHT) + level.yOffset) + playerYDeltaVal);
          player.setY(player.getY() + playerYDeltaVal);
          if (level.getWidthInTiles() != 16 || level.getHeightInTiles() != 8) {
            level.yOffset = -level.getHeight() + HEIGHT;
          }
        }
      }
      else if (player.getY() > PLAYER_Y_CENTRE) {
        if (player.getY() + playerYDeltaVal + player.getHeight() < HEIGHT) {
          player.setY(player.getY() + playerYDeltaVal);
        }
        else {
          player.setY(HEIGHT - player.getHeight());
        }

      }

    }
    else {

      player.setXDelta(0);
      player.setJustLanded(true);


      // If we have landed within 8 pixels of a customer then pick up the fare ..

      if (!player.getPickingUpCustomer() && !player.isCarryingCustomer() && customer.getStatus() != CustomerStatus::Dead) {

        uint16_t customerXPosition = customer.getX();
        uint16_t customerYPosition = customer.getY() + CUSTOMER_HEIGHT;

        int16_t left = customerXPosition - CUSTOMER_PICKUP_RANGE - PLAYER_WIDTH;
        int16_t right = customerXPosition + CUSTOMER_WIDTH + CUSTOMER_PICKUP_RANGE;

        if ((left <= static_cast<int16_t>(playerXPosition) && static_cast<int16_t>(playerXPosition) <= right) &&
            (customerYPosition - 2 <= playerYPosition + player.getHeight() && playerYPosition <= customerYPosition + player.getHeight() + 2)) {

          customer.setStatus(CustomerStatus::BoardingCab);
          customer.setXWalkingOffset(0);
          player.setPickingUpCustomer(true);
          gotoCounter = GOTO_COUNTER_MAX;

          if (customerXPosition + CUSTOMER_WIDTH_HALF < player.getXDisplay() - level.getXOffsetDisplay() + PLAYER_WIDTH_HALF) {
            customer.setWalkingDirection(Direction::Right);
          }
          else {
            customer.setWalkingDirection(Direction::Left);
          }

        }

      }


      // If we have landed within 8 pixels of a flag then drop off the fare ..

      else if (player.isCarryingCustomer() && customer.getStatus() != CustomerStatus::Dead) {

        uint8_t x = ((player.getXDisplay() - level.getXOffsetDisplay()) / 8) + 1; // Move over one to account for 17 pixels
        uint8_t y = ((player.getYDisplay() - level.getYOffsetDisplay()) / 8) + 1;

        if (player.isCarryingCustomer() && diffT(customer.getXDestinationTile(), x) < 3 && customer.getYDestinationTile() == y) {

          player.setCarryingCustomer(false);
          player.setScore(player.getScore() + customer.getFare());
          player.incFaresCompleted();
          sound.tones(coinsSound);
          dollarsCount = DOLLARS_COUNT_MAX;

          if (player.getFaresCompleted() >= level.getFaresRequired()) {
            launchCustomer(level, customer, RANDOM_START_POSITION, GO_TO_GATE);
          }
          else {
            launchCustomer(level, customer, RANDOM_START_POSITION, RANDOM_END_POSITION);
          }

        }

      }

    }

    player.setRetractLandingGear(PLAYER_RETRACT_LANDING_GEAR);

  }


  // --  Moving left --------------------------------------------------------------------------------------------

  if (player.getXDelta() < 0) { 

    if (canMoveLeft(level, playerXPosition, playerYPosition)) {

      if (level.xOffset < 0) {

        if (player.getX() < PLAYER_X_CENTRE) {
          if (level.xOffset < playerXDeltaVal) {
            level.xOffset = level.xOffset - playerXDeltaVal;
          }
          else {
            level.xOffset = 0;
          }
        }
        else if (player.getX() == PLAYER_X_CENTRE) {
          if (level.xOffset < playerXDeltaVal && (level.getWidthInTiles() != 16 || level.getHeightInTiles() != 8)) {
            level.xOffset = level.xOffset - playerXDeltaVal;
          }
          else {
            player.setX(player.getX() + playerXDeltaVal - level.xOffset);
            level.xOffset = 0;
          }
        }
        else if (player.getX() > PLAYER_X_CENTRE) {
          if (player.getX() + playerXDeltaVal > PLAYER_X_CENTRE) {
            player.setX(player.getX() + playerXDeltaVal);
          }
          else {
            level.xOffset = level.xOffset + PLAYER_X_CENTRE - player.getX() - playerXDeltaVal;
            player.setX(PLAYER_X_CENTRE);
          }
        }

      }
      else {

        if (player.getX() > -playerXDeltaVal) {
          player.setX(player.getX() + playerXDeltaVal);
        }
        else {
          player.setX(0);
        }

      }

    }
    
  }


  // --  Moving Right -------------------------------------------------------------------------------------------

  if (player.getXDelta() > 0) {

    if (canMoveRight(level, playerXPosition, playerYPosition)) {

      if (player.getX() < PLAYER_X_CENTRE) {
        if (player.getX() + playerXDeltaVal < PLAYER_X_CENTRE) {
          player.setX(player.getX() + playerXDeltaVal);
        }
        else {
          if (level.getWidthInTiles() != 16 || level.getHeightInTiles() != 8) {
            level.xOffset = PLAYER_X_CENTRE - player.getX() - playerXDeltaVal;
          }
          player.setX(PLAYER_X_CENTRE);
        }
      }
      else if (player.getX() == PLAYER_X_CENTRE) {                            
        if (level.xOffset - playerXDeltaVal > -level.getWidth() + WIDTH && (level.getWidthInTiles() != 16 || level.getHeightInTiles() != 8)) {
          level.xOffset = level.xOffset - playerXDeltaVal;
        }
        else {
          player.setX(player.getX() + ((level.getWidth() - WIDTH) + level.xOffset) + playerXDeltaVal);
          if (level.getWidthInTiles() != 16 || level.getHeightInTiles() != 8) {
            level.xOffset = -level.getWidth() + WIDTH;
          }
        }
      }
      else if (player.getX() > PLAYER_X_CENTRE) {

        if (player.getX() + playerXDeltaVal + PLAYER_WIDTH < WIDTH) {
          player.setX(player.getX() + playerXDeltaVal);
        }
        else {
          player.setX(WIDTH - PLAYER_WIDTH);
        }

      }

    }

  }

}

#include "src/Utils/Arduboy2Ext.h"


void handleInput(Player &player) {


  // You cannot move while loading a customer ..

  if (player.getPickingUpCustomer()) return;

  if (state != GameState::PlayGame_FlashingCar && arduboy.justPressed(B_BUTTON) && player.getFuel() != 0) {

      player.setLandingGearDown(!player.isLandingGearDown());

  }
  else {
      
    if (arduboy.everyXFrames(4)) {

      if (!player.hasJustLanded()) {

        if (arduboy.pressed(LEFT_BUTTON)) {
          player.decXDelta();
          player.setFrame(1);
        }

        else if (arduboy.pressed(RIGHT_BUTTON)) {
          player.incXDelta();
          player.setFrame(0);
        }

      }

      if (arduboy.pressed(A_BUTTON) && player.getFuel() > 0) {

        switch (player.getFrame()) {

          case 0: 
            Sprites::drawExternalMask(player.getXDisplay(), player.getYDisplay() + 8, thrusterRight, thrusterRightMask, thrusterFrame, thrusterFrame);
            break;

          case 1:
            Sprites::drawExternalMask(player.getXDisplay(), player.getYDisplay() + 8, thrusterLeft, thrusterLeftMask, thrusterFrame, thrusterFrame);
            break;

        }


        // Alternate thruster frame every 5 frames ..

        if (arduboy.everyXFrames(5)) {
          ++thrusterFrame;
          thrusterFrame %=2;
        }

        player.decYDelta();
        sound.tone(NOTE_C1, 50, NOTE_C2, 50, NOTE_C1, 50);

      }

      if (arduboy.everyXFrames(8)) {


        // If the A Button is not being pressed, then we should start falling ..

        if (arduboy.notPressed(A_BUTTON) || player.getFuel() == 0) {
          player.incYDelta(); 
        }

      }


      // If the player is not pressing the left or right button then start slowing down ..

      if (arduboy.everyXFrames(16)) {

        if (arduboy.notPressed(LEFT_BUTTON) && arduboy.notPressed(RIGHT_BUTTON)) {

          if (player.getXDelta() > 0)  player.decXDelta();
          if (player.getXDelta() < 0)  player.incXDelta();

        }

      }

    }

  }

}


//------------------------------------------------------------------------------
//  Launch a new customer.
//
//  Valid launching positions are derived from the level design.
//------------------------------------------------------------------------------

void launchCustomer(Level &level, Customer &customer, uint8_t defaultStartPosition, uint8_t defaultEndingPosition) {

  const uint8_t numberOfPositions = level.getNumberOfCustomerPositions();
  uint8_t customerStartingPos = 0;
  uint8_t customerDestination = 0;

  if (numberOfPositions > 1) {

    customerStartingPos = (defaultStartPosition == RANDOM_START_POSITION ? random(numberOfPositions) : defaultStartPosition);
    customerDestination = (defaultEndingPosition == RANDOM_END_POSITION ? random(numberOfPositions) : defaultEndingPosition);


    // Ensure new customer is not placed in the location the last customer was dropped at ..

    while (defaultStartPosition == RANDOM_START_POSITION && (customerStartingPos == customer.getStartingPosition() || customerStartingPos == customer.getDestinationPosition())) {
        customerStartingPos = random(numberOfPositions);
    }


    // Make sure the starting point and destination are different!

    while (defaultStartPosition == RANDOM_START_POSITION && customerStartingPos == customerDestination) {
      customerDestination = random(numberOfPositions);
    }

  }
  else {

    customerStartingPos = 0;
    customerDestination = GO_TO_GATE;

  }

  const uint8_t *levelStartingPosition = levelStartingPositions[level.getLevelNumber()];
  customer.setXTile(pgm_read_byte(&levelStartingPosition[customerStartingPos * 2]));
  customer.setYTile(pgm_read_byte(&levelStartingPosition[(customerStartingPos * 2) + 1]));
  customer.setStartingPosition(customerStartingPos);
  customer.setDestinationPosition(customerDestination);

  if (customerDestination != GO_TO_GATE) {

    const uint8_t *levelEndingPosition = levelEndingPositions[level.getLevelNumber()];
    customer.setXDestinationTile(pgm_read_byte(&levelEndingPosition[customerDestination * 2]));
    customer.setYDestinationTile(pgm_read_byte(&levelEndingPosition[(customerDestination * 2) + 1]));

  }

  customer.setFrame(0);
  customer.setStatus(CustomerStatus::Alive);
  customer.setFare(random(FARE_MIN, FARE_MAX));

}


//------------------------------------------------------------------------------
//  Has the player collided with the customer ?
//------------------------------------------------------------------------------

void checkCollisionWithCustomer(Level &level, Player &player, Customer &customer) {

  if (player.isCarryingCustomer() || customer.getStatus() == CustomerStatus::Dead) return;

  Rect playerRect = { static_cast<int16_t>(player.getXDisplay()), static_cast<int16_t>(player.getYDisplay()), PLAYER_WIDTH, player.getHeight() };


  // Check customer collision only if they are on-screen ..

  int16_t customerXVal = customer.getX() + level.xOffset.getInteger();
  int16_t customerYVal = customer.getY() + level.yOffset.getInteger();

  if (customerXVal >= -CUSTOMER_WIDTH && customerXVal < WIDTH && customerYVal >= -CUSTOMER_HEIGHT && customerYVal < HEIGHT) {
  
    Rect customerRect = { customerXVal, customerYVal, CUSTOMER_WIDTH, CUSTOMER_HEIGHT };

    if (arduboy.collide(playerRect, customerRect)) {

      ouchCounter = OUCH_COUNTER_MAX;
      customer.setStatus(CustomerStatus::Dead);
      player.setPickingUpCustomer(false);

    }

  }

}



//------------------------------------------------------------------------------
//  Handle collisions with level elements ..
//------------------------------------------------------------------------------

void checkCollisionWithLevelElements(Level &level, Player &player) {

  player.setFuelling(false);

  uint8_t tileAlreadyTested[TILE_COUNT];

  for (uint8_t i = 0; i < TILE_COUNT; i++)  tileAlreadyTested[i] = 0;

  uint16_t playerXPosition = player.getXDisplay() - level.getXOffsetDisplay();
  uint16_t playerYPosition = player.getYDisplay() - level.getYOffsetDisplay();

  uint8_t tileX1 = (playerXPosition / 8);
  uint8_t tileX2 = (playerXPosition / 8) + 1;
  uint8_t tileX3 = (playerXPosition / 8) + 2;
  uint8_t tileY1 = (playerYPosition / 8);
  uint8_t tileY2 = (playerYPosition / 8) + 1;

  uint8_t tile = level.getLevelData(tileX1, tileY1);
  if (tileAlreadyTested[tile] == 0) {
    checkCollisionWithLevelElements_TestElement(level, player, tileX1, tileY1, tile);
    tileAlreadyTested[tile] = 1;
  }

  tile = level.getLevelData(tileX2, tileY1);
  if (tileAlreadyTested[tile] == 0) {
    checkCollisionWithLevelElements_TestElement(level, player, tileX2, tileY1, tile);
    tileAlreadyTested[tile] = 1;
  }

  tile = level.getLevelData(tileX3, tileY1);
  if (tileAlreadyTested[tile] == 0) {
    checkCollisionWithLevelElements_TestElement(level, player, tileX3, tileY1, tile); 
    tileAlreadyTested[tile] = 1;
  }

  if (playerYPosition / 8 != 0) {

    tile = level.getLevelData(tileX1, tileY2);
    if (tileAlreadyTested[tile] == 0) {
      checkCollisionWithLevelElements_TestElement(level, player, tileX1, tileY2, tile);
      tileAlreadyTested[tile] = 1;
    }

    tile = level.getLevelData(tileX2, tileY2);
    if (tileAlreadyTested[tile] == 0) {
      checkCollisionWithLevelElements_TestElement(level, player, tileX2, tileY2, tile);
      tileAlreadyTested[tile] = 1;
    }

    tile = level.getLevelData(tileX3, tileY2);
    if (tileAlreadyTested[tile] == 0) {
      checkCollisionWithLevelElements_TestElement(level, player, tileX3, tileY2, tile);
      tileAlreadyTested[tile] = 1;
    }

  } 

}

void checkCollisionWithLevelElements_TestElement(Level &level, Player &player, uint8_t x, uint8_t y, uint8_t tile) {

  switch (tile) {

    case FUEL1:
      player.setFuelling(true);
      if (fuelSound[0] == 0) { fuelSound[0] = 200; }

      if (arduboy.getFrameCount(4) == 0) {
        Fuel *fuel = level.getFuel(x, y);
        if (fuel->getFuelLeft() > 0 && player.getFuel() < level.getFuelMax()) {
          fuelSound[0] = fuelSound[0] + 10;
          sound.tonesInRAM(fuelSound);
          player.incFuel();
          fuel->decFuel();
        }
        else {
          fuelSound[0] = 0;
        }
      }
      break;
    
    case LEVE1:
      level.changeInternalGate(GateMode::Open);
      sound.tone(NOTE_C1, 80, NOTE_D3, 50, NOTE_F6, 80);
      break;

    case SPIKU:
    case SPIKD:
      player.setStatus(PlayerStatus::OutOfFuel_Max);
      break;
   
  }

  // Going down?

  if (!player.isLandingGearDown() && player.getYDelta() > 0) {

    if (isTileSolid(tile)) {

      player.setStatus(PlayerStatus::OutOfFuel_Max);

    }

  }

}


void updateStatus(Player &player, Customer &customer) {


  // Burn fuel ..

  if (!player.isFuelling() && player.getStatus() == PlayerStatus::Active) {


    // If we are thrusting (yDelta < 0) then burn fuel proportionally to the speed.  Otherwsae burn slowly ..

    if ((player.getYDelta() < 0 && arduboy.everyXFrames(FUEL_BURN_RATE_DIV / -player.getYDelta())) || arduboy.everyXFrames(FUEL_BURN_RATE_MIN)) {

      player.decFuel();

      if (player.getFuel() == 0) {

        player.setLandingGearDown(false);

      }

    }

  }


  // Update player status ..

  switch (player.getStatus()) {

    case PlayerStatus::OutOfFuel_Max:
      sound.tones(crashSound);
      player.decStatus();
      break;

    case PlayerStatus::OutOfFuel_Min ... PlayerStatus::OutOfFuel_MaxMinusOne:
      player.decStatus();
      break;

    case PlayerStatus::OutOfFuel_End:
      player.setStatus(PlayerStatus::Inactive);
      player.decNumberOfLives();

      if (player.getNumberOfLives() > 0) {
        level.reset(level.getLevelNumber());
        player.setStatus(PlayerStatus::Active);
        state = GameState::PlayGame_InitLevel;
        nextState = GameState::PlayGame_FlashingCar;
      }
      else {
        state = GameState::GameOver_Init;
      }
      
      break;

    default:  break;

  }


  // Decrease fare ..

  if (arduboy.everyXFrames(FARE_X_FRAMES)) {

    fareCount++;

    if (fareCount > FARE_COUNT) {
      fareCount = 0;
      customer.decFare();
    }

  }

}


//------------------------------------------------------------------------------
//  Play the game! 
//------------------------------------------------------------------------------

void inGame(Font4x6 &font4x6, Level &level, Player &player, Customer &customer) {

  flashingCounter++;
  flashingCounter = flashingCounter % FLASH_MAX;

  int16_t customerXVal = customer.getX() + level.xOffset.getInteger();
  int16_t customerYVal = customer.getY() + level.yOffset.getInteger();

  // Handle counters ..

  if (gotoCounter != 0)     gotoCounter--;
  
  if (ouchCounter != 0) { 
    ouchCounter--;
    if (ouchCounter == 0)   launchCustomer(level, customer, RANDOM_START_POSITION, RANDOM_END_POSITION);
  }
  
  level.decInternalGateCounter();
  


  // Render level,player and customers ..

  drawLevel(level);

  if (player.getStatus() == PlayerStatus::Active) {
  
    handleInput(player);
    checkCollisionWithCustomer(level, player, customer);

    if (state == GameState::PlayGame) {
      checkCollisionWithLevelElements(level, player);
    }

  }

  if (state != GameState::PlayGame_FlashingCar || flashingCounter < (FLASH_MAX / 2)) {
    playerDisplay(player);
  }

  drawDollars(player);
  customerDisplay(level, player, customer, customerXVal, customerYVal);
  drawHUD(font4x6, player, customer);
  drawGoto(level, player, customer, customerXVal, customerYVal);
  drawOuch(customer, customerXVal, customerYVal);

  if (state == GameState::PlayGame_FlashingCar) {

    if (arduboy.justPressed(A_BUTTON)) {
      state = GameState::PlayGame;
    }

  }
  else {
 
    updateStatus(player, customer);

  }
  
}

#include "src/Utils/Arduboy2Ext.h"

void drawLevel_Intro(Level &level, uint8_t gateToRender) {

  for (uint8_t y = 0; y < level.getHeightInTiles(); y++) {

    for (uint8_t x = 0; x < level.getWidthInTiles(); x++) {

      const int16_t bitmapX = (x * TILE_SIZE);
      const int16_t bitmapY = (y * TILE_SIZE);

      uint8_t tile = level.getLevelData(x, y);

      switch (tile) {

        case EMPTY: break;

        case SIGN1:
          {
            bool found = false;
            const uint8_t numberOfPositions = level.getNumberOfCustomerPositions();
            const uint8_t *levelEndingPosition = levelEndingPositions[level.getLevelNumber()];

            for (uint8_t i = 0; i < numberOfPositions; ++i) {

              const uint8_t startPosX = pgm_read_byte(&levelEndingPosition[i * 2]);
              const uint8_t startPosY = pgm_read_byte(&levelEndingPosition[(i * 2) + 1]);

              if (x == startPosX && y == startPosY && gateToRender == i) {

                Sprites::drawOverwrite(bitmapX, bitmapY, tiles, tile);
                arduboy.fillRect(bitmapX + 2, bitmapY - 2, 5, 7, WHITE);
                Sprites::drawErase(bitmapX + 3, bitmapY - 1, font3x5_Numbers, (i + 1));
                found = true;

              }

            }

            if (!found) Sprites::drawOverwrite(bitmapX, bitmapY, tiles, GRASS);
            
          }

          break;

        default:
          Sprites::drawOverwrite(bitmapX, bitmapY, tiles, tile);
          break;
          
      }

    }

  }

}


void drawLevel(Level &level) {

  for (uint8_t y = 0; y < level.getHeightInTiles(); y++) {

    for (uint8_t x = 0; x < level.getWidthInTiles(); x++) {

      const int16_t tileX = (x * TILE_SIZE);
      const int16_t tileY = (y * TILE_SIZE);
      const int16_t bitmapX = tileX + level.getXOffsetDisplay();
      const int16_t bitmapY = tileY + level.getYOffsetDisplay();
      

      // Do we really need to render the tile?

      if (bitmapX < -8 || bitmapX >= WIDTH || bitmapY < -8 || bitmapY >= HEIGHT)
        continue;

      uint8_t tile = level.getLevelData(x, y);

      switch (tile) {

        case EMPTY: 
          {
            uint16_t tileFrame = ((y * level.getHeightInTiles()) + x + 3) % 27;
#ifdef SPARKLE
            if (tileFrame % 8 == 0 && arduboy.getFrameCount() % 4 != (y * level.getHeightInTiles() + x) % 4) {
#else
            if (tileFrame % 8 == 0) {
#endif              
              Sprites::drawOverwrite(bitmapX, bitmapY, Empty_With_Stars, tileFrame >> 3);
            }
          }
          break;

        case SIGN1:

          Sprites::drawOverwrite(bitmapX, bitmapY, tiles, tile);
          arduboy.fillRect(bitmapX + 2, bitmapY - 2, 5, 7, WHITE);

          if (flashingCounter < (FLASH_MAX / 2)) {

            const uint8_t numberOfPositions = level.getNumberOfCustomerPositions();
            const uint8_t *levelEndingPosition = levelEndingPositions[level.getLevelNumber()];

            for (uint8_t i = 0; i < numberOfPositions; ++i) {

              const uint8_t startPosX = pgm_read_byte(&levelEndingPosition[i * 2]);
              const uint8_t startPosY = pgm_read_byte(&levelEndingPosition[(i * 2) + 1]);

              if (x == startPosX && y == startPosY) {

                Sprites::drawErase(bitmapX + 3, bitmapY - 1, font3x5_Numbers, (i + 1));

              }

            }

          }

          break;


        case FUEL1:
          {
            Fuel *fuel = level.getFuel(x, y);
            Sprites::drawOverwrite(bitmapX, bitmapY, FuelImgs, fuel->getFuelLeftPerCent());
          }
          break;

        default: 
          Sprites::drawOverwrite(bitmapX, bitmapY, tiles, tile);
          break;
          
      }

    }

  }
             
}

#define HUD_OFFSET 10
#define HUD_WIDTH 66

//__attribute__((noinline)) 
void drawHUD(Font4x6 &font4x6, Player &player, Customer &customer) {

  Sprites::drawOverwrite(0, 56, SpaceCabHUD, 0);
  font4x6.setCursor(13, 57);

  {
    uint8_t digits[5];
    extractDigits(digits, player.getScore());
    for (uint8_t i = 5; i > 0; --i) 
    font4x6.print(digits[i - 1]);
  }

  if (player.getFuel() > 0 && player.getFuel() < PLAYER_FUEL_MIN_BLINK && flashingCounter < (FLASH_MAX / 2)) {
  
    arduboy.fillRect(44, 57, 30, 8 , BLACK);
  
  }
  else {

    uint8_t digitsFuel[3];
    font4x6.setCursor(57, 57);
    extractDigits(digitsFuel, player.getFuel());
    for(uint8_t i = 3; i > 0; --i) 
    font4x6.print(digitsFuel[i - 1]);

  }

  {
    uint8_t digitsLives[1];
    font4x6.setCursor(94, 57);
    extractDigits(digitsLives, player.getNumberOfLives());
    for(uint8_t i = 1; i > 0; --i) 
    font4x6.print(digitsLives[i - 1]);
  }

  {
    uint8_t digitsFare[2];
    font4x6.setCursor(115, 57);
    extractDigits(digitsFare, customer.getFare());
    for(uint8_t i = 2; i > 0; --i)
    font4x6.print(digitsFare[i - 1]);
  }

}

void scrollingBackground(bool scrollLeft) {

  uint8_t xCount = 0;

  arduboy.drawFastHLine(0, 0, WIDTH, WHITE);

  for (int16_t x = backdropx; x < 128; x = x + 8) {

    for (uint8_t y = 0; y < 7; ++y) {

      if ((xCount % 2 == 0 && y % 2 == 0) || (xCount % 2 == 1 && y % 2 == 1)) {
        Sprites::drawOverwrite(x, 4 + (y * 8), checkeredBG_White, 0);
      }

    }

    ++xCount;

  }

  arduboy.drawFastHLine(0, HEIGHT - 1, WIDTH, WHITE);

  if (arduboy.everyXFrames(2)) { // when running at 60fps

    if (scrollLeft) {
      --backdropx;
      if (backdropx <= -WIDTH) {
        backdropx = 0;
      }
    }
    else {
      ++backdropx;
      if (backdropx >= 0) {
        backdropx = -WIDTH;
      }
    }
  
  }

}

void playerDisplay(Player &player) {

  uint8_t const *imageName = nullptr;
  uint8_t const *maskName = nullptr;
  int8_t xOffset = 0;
  int8_t yOffset = 0;
  uint8_t frameNumber = player.getFrame();

  switch (player.getStatus()) {

    case PlayerStatus::OutOfFuel_Img1_Start ... PlayerStatus::OutOfFuel_Img1_End:
      imageName = SpaceTaxi_OutOfFuel_1;
      maskName = SpaceTaxi_OutOfFuel_1_Mask;
      xOffset = -5;
      yOffset = -5;
      frameNumber = 0;
      break;

    case PlayerStatus::OutOfFuel_Img2_Start ... PlayerStatus::OutOfFuel_Img2_End:
      imageName = SpaceTaxi_OutOfFuel_2;
      maskName = SpaceTaxi_OutOfFuel_2_Mask;
      xOffset = -5;
      yOffset = -5;
      frameNumber = 0;
      break;

    case PlayerStatus::OutOfFuel_Img3_Start ... PlayerStatus::OutOfFuel_Img3_End:
      imageName = SpaceTaxi_OutOfFuel_3;
      maskName = SpaceTaxi_OutOfFuel_3_Mask;
      xOffset = -5;
      yOffset = -5;
      break;

    case PlayerStatus::OutOfFuel_Img4_Start ... PlayerStatus::OutOfFuel_Img4_End:
      imageName = SpaceTaxi_OutOfFuel_4;
      maskName = SpaceTaxi_OutOfFuel_4_Mask;
      xOffset = -5;
      yOffset = -5;
      break;

    case PlayerStatus::Active:

      if (player.isLandingGearDown()) {
        imageName = SpaceTaxi_LandingGear;
        maskName = SpaceTaxi_LandingGear_Mask;
      }
      else {
        imageName = SpaceTaxi;
        maskName = SpaceTaxi_Mask;
      }
      break;

    default: break;

  }

  if (imageName != nullptr) {
    Sprites::drawExternalMask(player.getXDisplay() + xOffset, player.getYDisplay() + yOffset, imageName, maskName, frameNumber, frameNumber);
  }

}


void customerDisplay(Level &level, Player &player, Customer &customer, int16_t customerXVal, int16_t customerYVal) {

  uint8_t frame = customer.getFrame();

  if (!player.isCarryingCustomer() && customerXVal >= -CUSTOMER_WIDTH && customerXVal < WIDTH && customerYVal >= -CUSTOMER_HEIGHT && customerYVal < HEIGHT) {

    switch (customer.getStatus()) {

      case CustomerStatus::Dead:

        Sprites::drawExternalMask(customerXVal, customerYVal, Customer_Flat_Img, Customer_Flat_Mask, 0, 0);
        break;

      case CustomerStatus::Alive:

        Sprites::drawExternalMask(customerXVal, customerYVal, Customer_Img, Customer_Img_Mask, frame, frame);
        if (arduboy.everyXFrames(15)) {
          customer.incFrame();
        }

        break;

      case CustomerStatus::BoardingCab:
        {
          int8_t walkingOffset = customer.getXWalkingOffset();

          if (customer.getWalkingDirection() == Direction::Left) {

            Sprites::drawExternalMask(customerXVal + walkingOffset, customerYVal, Customer_WalkingLeft, Customer_WalkingLeft_Mask, frame, frame);

          }
          else {

            Sprites::drawExternalMask(customerXVal + walkingOffset, customerYVal, Customer_WalkingRight, Customer_WalkingRight_Mask, frame, frame);

          }

          if (arduboy.everyXFrames(8)) {

            uint16_t customerXPosition = customer.getX();

            if (customerXPosition + CUSTOMER_WIDTH_HALF + walkingOffset != player.getXDisplay() - level.getXOffsetDisplay() + PLAYER_WIDTH_HALF) {

              customer.incFrame();

              if (customer.getWalkingDirection() == Direction::Left) {
                customer.decXWalkingOffset();
              }
              else {
                customer.incXWalkingOffset();
              }

            }
            else {

              player.setCarryingCustomer(true);
              player.setPickingUpCustomer(false);
              if (player.getFaresCompleted() >= level.getFaresRequired())  level.openLevelGates(); 

              sound.tone(NOTE_E6, 50, NOTE_E3, 50, NOTE_E2, 50);

            }

          }

        }

        break;

    }

  }
  else if (state == GameState::PlayGame)  { 

    if (player.isCarryingCustomer() && customer.getDestinationPosition() == GO_TO_GATE) { return; }

    uint16_t customerX = 0;
    uint16_t customerY = 0;
    uint8_t arrowX = 0;
    uint8_t arrowY = 0;
    uint8_t playerX = player.getXDisplay();
    uint8_t playerY = player.getYDisplay();

    Direction direction = Direction::Up;


    // Render arrows.

    if (flashingCounter < (FLASH_MAX / 2)) { 

      if (!player.isCarryingCustomer()) {

        customerX = customer.getX();
        customerY = customer.getY();

      }
      else {

        customerX = customer.getXDestinationTile() * TILE_SIZE;
        customerY = customer.getYDestinationTile() * TILE_SIZE;

      }


      // If the customer location is on screen then do not render ..
      {
        int16_t customerXDisplay = customerX + level.getXOffsetDisplay();
        int16_t customerYDisplay = customerY + level.getYOffsetDisplay();

        if (customerXDisplay >= -CUSTOMER_WIDTH && customerXDisplay < WIDTH && customerYDisplay >= -CUSTOMER_HEIGHT && customerYDisplay < HEIGHT) {
          return;
        } 

      }

      int16_t dX = customerX - (player.getX() - level.xOffset).getInteger();
      int16_t dY = customerY - (player.getY() - level.yOffset).getInteger();

      if (dX < -playerX) {

        if (dY < -playerY)                            { direction = Direction::UpLeft; arrowX = 0; arrowY = 0; }
        if (dY >= -playerY && dY < -playerY + 56)     { direction = Direction::Left; arrowX = 0; arrowY = playerY + dY - 4; }
        if (dY >= -playerY + 56)                      { direction = Direction::DownLeft; arrowX = 0; arrowY = 48; }

      }

      if (dX >= -playerX && dX < -playerX + WIDTH) {

        if (dY < 0)                                   { direction = Direction::Up; arrowX = playerX + dX; arrowY = 0; }
        if (dY > 0)                                   { direction = Direction::Down; arrowX = playerX + dX; arrowY = 48; }

      }

      if (dX >= -playerX + WIDTH) {

        if (dY < -playerY)                            { direction = Direction::UpRight; arrowX = 120; arrowY = 0; }
        if (dY >= -playerY && dY < -playerY + 56)     { direction = Direction::Right; arrowX = 120; arrowY = playerY + dY - 4; }
        if (dY >= -playerY + 56)                      { direction = Direction::DownRight; arrowX = 120; arrowY = 48; }

      }

      Sprites::drawExternalMask(arrowX, arrowY, ArrowImgs, ArrowMasks, static_cast<uint8_t>(direction), static_cast<uint8_t>(direction));

    }

  }

}


void drawDollars(Player &player) {

  if (dollarsCount > 0) {

    uint8_t idx = (DOLLARS_COUNT_MAX - dollarsCount) / DOLLARS_COUNT_MULT;

    Sprites::drawExternalMask(player.getXDisplay() - 5, player.getYDisplay() - 10, dollars, dollars_Mask, idx, idx);
    dollarsCount--;
  
  }

}


void drawGoto(Level &level, Player &player, Customer &customer, int16_t customerXVal, int16_t customerYVal) {

  if (gotoCounter > 0) {

    int8_t walkingOffset = customer.getXWalkingOffset() + 1;
    
    if (player.getFaresCompleted() >= level.getFaresRequired()) {
      Sprites::drawExternalMask(customerXVal + walkingOffset, customerYVal - 13, Hail_GoGate, Hail_Mask, 0, 0);
    }
    else {
      Sprites::drawExternalMask(customerXVal + walkingOffset, customerYVal - 13, Hail, Hail_Mask, 0, 0);
      Sprites::drawErase(customerXVal + walkingOffset + 25, customerYVal - 9, font4x6_Full, customer.getDestinationPosition() + 53);
    }

  }

}

void drawOuch(Customer &customer, int16_t customerXVal, int16_t customerYVal) {

  if (ouchCounter > 0) {

    Sprites::drawExternalMask(customerXVal + customer.getXWalkingOffset() + 1, customerYVal - 13, Ouch, Hail_Mask, 0, 0);

  }

}

#include "src/Utils/Arduboy2Ext.h"

static constexpr uint8_t ppotFrames = (1.6 *60); // 1.6s animation loop


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void splashScreen_Init() {

    state = GameState::PPOT;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void splashScreen() { 

    auto justPressed = arduboy.justPressedButtons();

    if (justPressed > 0) {
        
        state = GameState::SplashScreen_Init; 

    }


    Sprites::drawOverwrite(32, 16, Images::PPOT, 0);

    uint8_t p = 0x11; // Default pixel position 0 is hidden in the image

    switch (arduboy.getFrameCount(ppotFrames)) {

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
#include "src/Utils/Arduboy2Ext.h"
#include "src/Utils/Vector2.h"

Vector2 stars[16];

constexpr Vector2 centreScreen = Vector2(WIDTH / 2, HEIGHT / 2);

const Vector2 offsets[16] PROGMEM = {
	Vector2(1.0000, 0.0000),
	Vector2(0.9239, 0.3827),
	Vector2(0.7071, 0.7071),
	Vector2(0.3827, 0.9239),
	Vector2(0.0000, 1.0000),
	Vector2(-0.3827, 0.9239),
	Vector2(-0.7071, 0.7071),
	Vector2(-0.9239, 0.3827),
	Vector2(-1.0000, 0.0000),
	Vector2(-0.9239, -0.3827),
	Vector2(-0.7071, -0.7071),
	Vector2(-0.3827, -0.9239),
	Vector2(0.0000, -1.0000),
	Vector2(0.3827, -0.9239),
	Vector2(0.7071, -0.7071),
	Vector2(0.9239, -0.3827),
};

void initialiseStars(void) {

	for(uint8_t i = 0; i < 16; ++i) stars[i] = centreScreen;
	starsInitialised = true;

}

void resetStars(void) {
	starsInitialised = false;
}

void updateStarfield(void) {

	for(uint8_t i = 0; i < 16; ++i) {

		stars[i].x += SQ7x8::fromInternal(pgm_read_word(&offsets[i].x));
		stars[i].y += SQ7x8::fromInternal(pgm_read_word(&offsets[i].y));
		
		if (stars[i].x < 0 || stars[i].y < 0) stars[i] = centreScreen;

	}

}

void drawStarfield() {

	for(uint8_t i = 0; i < 16; ++i) {
		arduboy.drawPixel(static_cast<int8_t>(stars[i].x), static_cast<int8_t>(stars[i].y));
  }
  
}

void drawLevelStart(Font4x6 &font4x6, Level &level) {

  if (!starsInitialised) initialiseStars();

  updateStarfield();
  drawStarfield();

  arduboy.fillRect(18, 23, 94, 17, BLACK);
  arduboy.drawFastHLine(19, 25, 92, WHITE);
  arduboy.drawFastHLine(19, 38, 92, WHITE);

  font4x6.setCursor(22 + level.getLevelNameOffset(), 28);
  font4x6.print(getLevelName(level));

  if (arduboy.justPressed(A_BUTTON)) { 
    
    state = GameState::PlayGame_InitLevel; 
    
  }

}

void drawMaxLevelCompleted(Font4x6 &font4x6) {

  if (!starsInitialised) initialiseStars();

  updateStarfield();
  drawStarfield();

  arduboy.fillRect(15, 18, 98, 30, BLACK);
  arduboy.drawFastHLine(16, 19, 96, WHITE);
  arduboy.drawFastHLine(16, 46, 96, WHITE);

  font4x6.setCursor(43, 24);
  font4x6.print(F("You completed"));
  font4x6.setCursor(49, 34);
  font4x6.print(F("all levels!"));
	Sprites::drawOverwrite(18, 21, EndOfGameBadge, 0);

  if (arduboy.justPressed(A_BUTTON)) { 
    
    state = GameState::GameOver_Init; 
    
  }

}
