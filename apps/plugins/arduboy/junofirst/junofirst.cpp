#include <Arduino.h>
#include "src/utils/Arduboy2Ext.h"
#include "src/entity/Player.h"
#include "src/entity/Enemy.h"
#include "src/entity/Bullet.h"
#include "src/entity/Slot.h"
#include "src/entity/HighScore.h"
#include "src/images/Images.h"
#include "src/images/Fonts.h"
#include "src/utils/Enums.h"
#include "src/utils/Utils.h"
#include "src/utils/EEPROM_Utils.h"
#include "src/levels/Level.h"
#include "src/levels/Formations.h"
#include "src/utils/FadeEffects.h"
#include "src/sounds/Sounds.h"
#include <ArduboyTones.h>

Arduboy2Ext arduboy;
ArduboyTones sound(arduboy.audio.enabled);
GameState gameState = GameState::SplashScreen_Init;

uint8_t introDelay = 0;
uint8_t alternate = 0;
uint8_t alternate2 = 0;
uint8_t horizonIncrement = 0;
uint8_t fuelBonusDisplay = 0;
uint8_t fuelBonus = 0;
uint8_t restart = 0;

Player player;
Bullet playerBullet;
Enemy enemies[MAX_NUMBER_OF_ENEMIES];
Bullet bullets[MAX_NUMBER_OF_BULLETS];
Level level;

HighScore highScore;
FadeInEffect fadeInEffect;



// --------------------------------------------------------------------------------------
//  Setup ..
//
/* Arduino IDE auto-prototypes */
void Play();
void Credits();
void GameOver();
void HighScore();
void RenderScreen(Player *player, Enemy *enemies);
void ScoreTable();
void Intro();
void splashScreen_Init();
void splashScreen();

void setup() {

  arduboy.boot();
  arduboy.flashlight();
  arduboy.systemButtons();
  arduboy.setFrameRate(DEFAULT_FRAME_RATE);
  arduboy.initRandomSeed();
  arduboy.audio.begin();

  EEPROM_Utils::initEEPROM(false);

}


// --------------------------------------------------------------------------------------
//  Main Loop ..
//
void loop() {

  if (!(arduboy.nextFrame())) return;
  arduboy.pollButtons();

  switch (gameState) {

    case GameState::SplashScreen_Init:
      splashScreen_Init();
      break;

    case GameState::SplashScreen:
      splashScreen();
      break;

    case GameState::Intro_Init:
      arduboy.setFrameRate(DEFAULT_FRAME_RATE);
      fadeInEffect.reset();
      gameState = GameState::Intro;
      sound.tones(intro_theme);
      // break; Fall-through intentional.

    case GameState::Intro:
      Intro();
      break;

    case GameState::ScoreTable:
      sound.noTone();
      ScoreTable();
      player.resetGame();
      level.resetGame(enemies, bullets, &playerBullet);
      break;

    case GameState::Wave_Init:
      player.resetWave();
      level.resetWave(enemies, bullets, &playerBullet);
      introDelay = 12;
      alternate = 0;
      gameState = GameState::Wave;
      arduboy.setFrameRate(level.getFrameRate());
      arduboy.setRGBled(0, 0, 0);
      fuelBonusDisplay = 0;
      // break; Fall-through intentional.

    case GameState::Wave:
 
      if (arduboy.everyXFrames(20)) { introDelay--; }
    
      if (introDelay == 0) {
        level.launchFormation(enemies);
        gameState = GameState::GamePlay;
      }

      Play();
      break;

    case GameState::WaveCompleted_Init:

      for (uint8_t x = 0; x < MAX_NUMBER_OF_BULLETS; x++) {

        Bullet *bullet = &bullets[x];
        bullet->setY(0);

      }

      playerBullet.setY(0);

      introDelay = 12;
      alternate = 0;
      gameState = GameState::WaveCompleted;
      fuelBonusDisplay = 0;
      // break; Fall-through intentional.

    case GameState::WaveCompleted:
 
      if (arduboy.everyXFrames(20)) { introDelay--; }
    
      if (introDelay == 0) {
        gameState = GameState::Wave_Init;
      }

      Play();
      break;

    case GameState::Paused:
    case GameState::GamePlay:
      Play();
      break;

    case GameState::GameOver_Init:
      arduboy.setFrameRate(DEFAULT_FRAME_RATE);
      gameState = GameState::GameOver;
      fadeInEffect.reset();
      sound.tones(end_of_game);
      arduboy.setRGBled(0, 0, 0);
      // break; Fall-through intentional.

    case GameState::GameOver:
      GameOver();
      break;

    case GameState::SaveScore:
      highScore.reset();
      highScore.setSlotNumber(EEPROM_Utils::saveScore(level.getScore(), level.getWave()));
      gameState = GameState::HighScore;
      fadeInEffect.reset();
      // break; Fall-through intentional.

    case GameState::HighScore:
      HighScore();
      break;

    case GameState::Credits:
      Credits();
      break;

    default: break;

  }
  
  arduboy.display(CLEAR_BUFFER);

}


// --------------------------------------------------------------------------------------
//  Let's play!
//
void Play() {

  #ifdef USE_INVERT
  arduboy.invert(false);
  #endif


  // Update the ground's position ..

#if GROUND_SPEED == 1
  const uint8_t speedLookup[] = {0, 16, 8, 0, 4};
#endif
#if GROUND_SPEED == 2
  const uint8_t speedLookup[] = {0, 12, 6, 0, 3};
#endif
#if GROUND_SPEED == 3
  const uint8_t speedLookup[] = {0, 8, 4, 0, 2};
#endif

  if (gameState != GameState::Paused) {

    uint8_t speed = speedLookup[absT(player.getYDelta())];

    if (horizonIncrement == speed) {

      level.incHorizon( player.getYDelta() > 0 ? 1 : -1);
      horizonIncrement = 0;
  

      // Update the enemies and bullets relative to the player position ..

      for (uint8_t x = 0; x < MAX_NUMBER_OF_ENEMIES; x++) {

        Enemy *enemy = &enemies[x];

        if (enemy->getStatus() == EnemyStatus::Active) {

          enemy->moveRelativeToPlayer(&player);

        }

      }

      for (uint8_t x = 0; x < MAX_NUMBER_OF_BULLETS; x++) {

        Bullet *bullet = &bullets[x];

        if (bullet->getY() > 0) {

          bullet->moveRelativeToPlayer(&player);

        }

      }
      
    }

    horizonIncrement++;

  }
  

  if (gameState != GameState::Wave && gameState != GameState::WaveCompleted && gameState != GameState::Paused) {

    #ifdef INC_HEALTH
    player.incHealth();
    #endif

    level.decDoubleUpPoints();

    if (arduboy.everyXFrames(FRAME_RATE_DEC_FUEL)) {
      player.decFuel();
    }


    if (arduboy.everyXFrames(FRAME_RATE_2)) {

      for (uint8_t x = 0; x < MAX_NUMBER_OF_ENEMIES; x++) {

        Enemy *enemy = &enemies[x];

        if (enemy->getStatus() == EnemyStatus::Active) {
          if (enemy->move(&player)) level.decInPlay();
        }    

      }

    }

  }

  RenderScreen(&player, enemies);
 

  // Handle players actions ..

  if (player.getStatus() == PlayerStatus::Active && gameState != GameState::Paused) {

    if (arduboy.pressed(DOWN_BUTTON))     { if (player.decYDelta()) horizonIncrement = 0; }
    if (arduboy.pressed(UP_BUTTON))       { if (player.incYDelta()) horizonIncrement = 0; }
    if (arduboy.pressed(LEFT_BUTTON))     { player.decX(); }
    if (arduboy.pressed(RIGHT_BUTTON))    { player.incX(); }

    if (gameState != GameState::Wave && gameState != GameState::WaveCompleted && (arduboy.justPressed(A_BUTTON) || (level.inDoubleUpPhase() && arduboy.pressed(A_BUTTON)))) {

      if (playerBullet.getY() == 0) {

        playerBullet.setY(45);
        playerBullet.setX(player.getX() + 7);
        sound.tones(player_fires_bullet);

      }

    }

    if (!arduboy.pressed(DOWN_BUTTON) && !arduboy.pressed(UP_BUTTON) && arduboy.everyXFrames(FRAME_RATE_16)) { player.decelerate(); }

  }

  if (gameState != GameState::Wave && gameState != GameState::WaveCompleted && arduboy.justPressed(B_BUTTON)) gameState = (gameState == GameState::GamePlay ? GameState::Paused : GameState::GamePlay);
  if (gameState != GameState::Wave && gameState != GameState::WaveCompleted && gameState != GameState::Paused) {


    // Update player bullet position ..

    uint8_t bulletY = playerBullet.getY();
    playerBullet.setY(bulletY > 10 ? bulletY - 5 : 0);

    for (uint8_t x = 0; x < MAX_NUMBER_OF_BULLETS; x++) {

      Bullet *bullet = &bullets[x];

      if (bullet->getY() > 0) {

        bullet->move(&player);

      }

    }


    // Did our bullet hit an enemy or enemy bullet?

    if (playerBullet.getY() > 0) {

      Rect bulletRect = { playerBullet.getX() - 1, playerBullet.getY() - 5, 3, 5 };

      for (uint8_t x = 0; x < MAX_NUMBER_OF_ENEMIES; x++) {

        Enemy *enemy = &enemies[x];

        if (enemy->getStatus() == EnemyStatus::Active && enemy->getProtection() == 0) {

          Rect enemyRect = { enemy->getXDisplay(), enemy->getYDisplay(), enemy->getWidth(), enemy->getHeight() };

          if (arduboy.collide(bulletRect, enemyRect)) {

            enemy->setStatus(EnemyStatus::Explosion4);
            sound.tones(enemy_explosion);

            uint16_t scoreInc = 0;
            bool doubleUpPoints = level.inDoubleUpPhase();

            switch (enemy->getType()) {

              case EnemyType::EnemyType1:
                scoreInc = (!doubleUpPoints ? 150 : 300);
                break;

              case EnemyType::EnemyType2:
                scoreInc = (!doubleUpPoints ? 300 : 600);
                break;

              case EnemyType::EnemyType3:
                scoreInc = (!doubleUpPoints ? 450 : 900);
                break;

              case EnemyType::EnemyType4:
                scoreInc = (!doubleUpPoints ? 600 : 1200);;
                break;

              case EnemyType::AstronautCaptured:
                {
                  bool enemyLaunched = false;
                  scoreInc = 450;


                  // If possible, launch a new enemy (astronaut) in the same place as the original ..

                  for (uint8_t y = 0; y < MAX_NUMBER_OF_ENEMIES; y++) {

                    Enemy *newEnemy = &enemies[y];

                    if (newEnemy->getStatus() == EnemyStatus::Dead) {
                      newEnemy->setType(EnemyType::AstronautReleased);
                      newEnemy->setMovementSequence(MovementSequence::Sequence_4);
                      newEnemy->setStatus(EnemyStatus::Active);
                      newEnemy->setX(enemy->getX());
                      newEnemy->setY(enemy->getY());
                      newEnemy->setYDelta(-1);
                      newEnemy->setProtection(10);
                      enemyLaunched = true;
                      level.incEnemiesLaunched();
                      level.incInPlay();
                      break;
                      
                    }

                  }

                  // If no enemy launched, the re-use the current one ..

                  if (!enemyLaunched) {

                      enemy->setType(EnemyType::AstronautReleased);
                      enemy->setMovementSequence(MovementSequence::Sequence_4);
                      enemy->setStatus(EnemyStatus::Active);
                      enemy->setProtection(5);
                      enemy->setYDelta(-1);

                  }

                }
                break;

              case EnemyType::AstronautReleased:
                break;

            }

            level.setScore(level.getScore() + scoreInc);

          }

        }    

      }

      for (uint8_t x = 0; x < MAX_NUMBER_OF_BULLETS; x++) {

        Bullet *bullet = &bullets[x];

        if (bullet->getY() > 0) {

          Rect eneymBulletRect = { bullet->getX(), bullet->getY(), 2, 2 };

          if (arduboy.collide(bulletRect, eneymBulletRect)) {

            bullet->setY(0);
            Sprites::drawSelfMasked(bullet->getX() - 2, bullet->getY() - 2, alien_far, 0);
            sound.tones(bullets_collide);

          }

        }

      }

    }


    // Did any enemy hit us ..

    if (player.getStatus() == PlayerStatus::Active) {

      Rect playerRect = { player.getX() + 1, player.getY() + 4, 13, 8 };

      for (uint8_t x = 0; x < MAX_NUMBER_OF_ENEMIES; x++) {

        Enemy *enemy = &enemies[x];

        if (enemy->isVisible() && enemy->getStatus() == EnemyStatus::Active && enemy->getYDisplay() > 20) {

          Rect enemyRect = { enemy->getXDisplay() + 1, enemy->getYDisplay() + 1, static_cast<uint8_t>(enemy->getWidth() - 2), static_cast<uint8_t>(enemy->getHeight() - 2)};

          if (arduboy.collide(enemyRect, playerRect)) {

            switch (enemy->getType()) {

              case EnemyType::EnemyType1 ... EnemyType::AstronautCaptured:

                if (!enemy->getPlayerOverlap()) {

                  enemy->setPlayerOverlap(true);
                  enemy->setStatus(EnemyStatus::Explosion4);

                  #ifdef SINGLE_FLASH
                  #ifdef USE_INVERT
                  arduboy.invert(true);
                  #endif
                  arduboy.setRGBled(RED_LED, 16);
                  #else 
                  player.setHealthCountdown(HEALTH_COUNTDOWN);
                  #endif

                  uint8_t health = player.getHealth();

                  if (health > 0) {

                    player.setHealth(--health);
                    sound.tones(player_hit_by_alien);

                  }
                  else {

                    player.setStatus(PlayerStatus::Explosion4);
                    sound.tones(player_explosion);

                  }

                }
                
                break;

              case EnemyType::AstronautReleased:
                enemy->setStatus(EnemyStatus::Dead);
                sound.tones(double_up);
                level.setDoubleUpPoints(DOUBLE_UP_POINTS_DELAY);
                level.decInPlay();
                break;

            }

          }
          else {

            enemy->setPlayerOverlap(false);

          }

        }

      }


      // Did any enemy bullet hit us ..

      for (uint8_t x = 0; x < MAX_NUMBER_OF_BULLETS; x++) {

        Bullet *bullet = &bullets[x];

        if (bullet->getY() > 0) {

          Rect bulletRect = { bullet->getX(), bullet->getY(), 2, 2 };

          if (arduboy.collide(bulletRect, playerRect)) {

            #ifdef SINGLE_FLASH
            #ifdef USE_INVERT
            arduboy.invert(true);
            #endif
            arduboy.setRGBled(RED_LED, 16);
            #else 
            player.setHealthCountdown(HEALTH_COUNTDOWN);
            #endif

            uint8_t health = player.getHealth();
            bullet->setY(0);

            if (health > 0) {

              player.setHealth(--health);
              sound.tones(player_hit_by_bullet);

            }
            else {

              player.setStatus(PlayerStatus::Explosion4);
              sound.tones(player_explosion);

            }

          }

        }

      }

    }


    // Update dying enemy images ..

    if (arduboy.everyXFrames(FRAME_RATE_4)) {

      for (uint8_t x = 0; x < MAX_NUMBER_OF_ENEMIES; x++) {

        Enemy *enemy = &enemies[x];
        EnemyStatus enemyStatus = enemy->getStatus();

        if (enemyStatus > EnemyStatus::Dead && enemyStatus < EnemyStatus::Active) {

          enemy->setStatus(--enemyStatus);

          if (enemyStatus == EnemyStatus::Dead) {
            level.decInPlay();
          }

        }

      }


      // Update dying player image ..

      PlayerStatus status = player.getStatus();

      if (status > PlayerStatus::Dead && status < PlayerStatus::Active) {

        player.setStatus(--status);

        if (status == PlayerStatus::Dead) {

          player.decLives();

          if (player.getLives() > 0) {
            gameState = GameState::Wave_Init;
          }
          else {
            gameState = GameState::GameOver_Init;
            
          }

        }

      }

    }



    // Update spawning enemy images ..

    if (arduboy.everyXFrames(FRAME_RATE_3)) {

      for (uint8_t x = 0; x < MAX_NUMBER_OF_ENEMIES; x++) {

        Enemy *enemy = &enemies[x];
        EnemyStatus enemyStatus = enemy->getStatus();

        if (enemyStatus > EnemyStatus::Active) {

          enemy->setStatus(--enemyStatus);

        }

      }

    }


    // Enemy takes a shot? 

    if (random(0, level.getBulletFrequency()) == 0) {

      bool bulletLaunched = false;

      for (uint8_t x = 0; x < MAX_NUMBER_OF_BULLETS; x++) {

        Bullet *bullet = &bullets[x];

        if (bullet->getY() == 0) { 
          
          uint8_t y = random(0, MAX_NUMBER_OF_ENEMIES);

          for (uint8_t x = 0; x < MAX_NUMBER_OF_ENEMIES; x++) {
            
            uint8_t x2 = (x + y) % MAX_NUMBER_OF_ENEMIES;

            Enemy *enemy = &enemies[x2];
            EnemyStatus enemyStatus = enemy->getStatus();

            if (enemy->getType() >= EnemyType::EnemyType1 && enemy->getType() <= EnemyType::EnemyType4 && enemy->getYDelta() >= 0 && enemyStatus == EnemyStatus::Active && enemy->inShootingRange()) {

              uint8_t enemyMiddle = enemy->getXDisplay() + (enemy->getWidth() / 2);

              bullet->setX(enemyMiddle);
              bullet->setY(absT(enemy->getYDisplay() + enemy->getHeight()));
              bullet->setYDelta(enemy->getYDelta() + 1);

              switch ((player.getX() + 7 - enemyMiddle) / (player.getY() - enemy->getYDisplay())) {

                case -999 ... -1:
                  bullet->setXDelta(-1);
                  break;

                case 0:
                  bullet->setXDelta(0);
                  break;

                case 1 ... 999:
                  bullet->setXDelta(1);
                  break;

              }

              sound.tones(enemy_fires_bullet);
              bulletLaunched = true;
              break;

            }

          }

        }

        if (bulletLaunched) break;

      }

    }


    // Are we out of fuel?

    if (player.getFuel() == 0 && player.getStatus() == PlayerStatus::Active) {

      player.setStatus(PlayerStatus::Explosion4);
      sound.tones(player_explosion);

    }


    // Launch a new formation ?

    level.decCountDown();

    if (level.getCountDown() == 0 && level.getInPlay() <= MAX_NUMBER_OF_ENEMIES - MAX_NUMBER_OF_ENEMIES_PER_FORMATION && level.getEnemiesLaunchedThisWave() < level.getEnemiesInWave()) {

      uint8_t numberOfEnemies = level.launchFormation(enemies);
      sound.tones(formation_launch[numberOfEnemies - 1]);

    }


    // Have we cleared the wave ?
    
    if (gameState == GameState::GamePlay && level.getInPlay() == 0 && level.getEnemiesLaunchedThisWave() >= level.getEnemiesInWave()) {

      fuelBonus = clamp(player.getFuel(), static_cast<uint8_t>(0), static_cast<uint8_t>(9));
      fuelBonusDisplay = 0;
      level.incWave();
      gameState = GameState::WaveCompleted_Init;

    }

  }


  // Flash the red LED? 

  #ifndef SINGLE_FLASH
  uint8_t health = player.getHealthCountdown();

  arduboy.setRGBled(RED_LED, 0);

  if (health > 0) {

    player.decHealthCountdown();

    if (health % 2 == 0) {

      #ifdef USE_INVERT
      arduboy.invert(true);
      #endif
      arduboy.setRGBled(RED_LED, 16);

    }

  }
  #endif

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

  Sprites::drawOverwrite(37, 6, credits, 0);
  arduboy.drawFastHLine(37, 25, 54, WHITE);
  arduboy.drawFastHLine(37, 27, 54, WHITE);


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

    gameState = GameState::Intro_Init; 
    
  }

}
#include "src/utils/Arduboy2Ext.h"


// --------------------------------------------------------------------------------------
//  Game Over !
//
void GameOver() {

  Sprites::drawOverwrite(1, 16, gameOver, 0);
  
  if (!fadeInEffect.isComplete()) {

    fadeInEffect.draw(arduboy);
    fadeInEffect.update();

  }

  // If 'A' bUtton is pressed move to game play ..

  if (arduboy.justPressed(A_BUTTON)) { 

//    player.reset();  
    gameState = GameState::SaveScore; 
    introDelay = 0;
    
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

  Sprites::drawOverwrite(17, 0, highScoreImg, 0);

  for (uint8_t x =0; x < MAX_NUMBER_OF_SCORES; x++) {

    Slot slot;
    EEPROM_Utils::getSlot(x, &slot);

    Sprites::drawOverwrite(xOffset, yOffset, font, slot.getChar0());
    Sprites::drawOverwrite(xOffset + 5, yOffset, font, slot.getChar1());
    Sprites::drawOverwrite(xOffset + 10, yOffset, font, slot.getChar2());


    // Score ..
    {
      uint8_t digits[6] = {};
      extractDigits(digits, slot.getScore());
      
      for (uint8_t i = 0, x2 = xOffset + 49; i < 6; ++i, x2 -= 5) {
        Sprites::drawOverwrite(x2, yOffset, numbers, digits[i]);
      }
      
    }


    // Wave ..
    
    Sprites::drawOverwrite(xOffset + 65, yOffset, numbers, slot.getWave() / 10);
    Sprites::drawOverwrite(xOffset + 70, yOffset, numbers, slot.getWave() % 10);

    yOffset = yOffset + 8;

  }


  if (highScore.getSlotNumber() != DO_NOT_EDIT_SLOT) {

    yOffset = 23;
    alternate++;
  
    if (alternate < 40) {

      Sprites::drawOverwrite(xOffset - 6, yOffset + (highScore.getSlotNumber() * 8), arrowLeft, 0);
      Sprites::drawOverwrite(xOffset + 77, yOffset + (highScore.getSlotNumber() * 8), arrowRight, 0);
    
    }
    else if (alternate > 80) {

      alternate = 0;

    }

    if (highScore.getCharIndex() == 0) {
      arduboy.fillRect(xOffset - 1, yOffset + (highScore.getSlotNumber() * 8) - 1, 5, 7, WHITE);
      Sprites::drawErase(xOffset, yOffset + (highScore.getSlotNumber() * 8), font, highScore.getChar(0));
    }
    else {
      Sprites::drawOverwrite(xOffset, yOffset + (highScore.getSlotNumber() * 8), font, highScore.getChar(0));
    }

    if (highScore.getCharIndex() == 1) {
      arduboy.fillRect(xOffset + 4, yOffset + (highScore.getSlotNumber() * 8) - 1, 5, 7, WHITE);
      Sprites::drawErase(xOffset + 5, yOffset + (highScore.getSlotNumber() * 8), font, highScore.getChar(1));
    }
    else {
      Sprites::drawOverwrite(xOffset + 5, yOffset + (highScore.getSlotNumber() * 8), font, highScore.getChar(1));
    }

    if (highScore.getCharIndex() == 2) {
      arduboy.fillRect(xOffset + 9, yOffset + (highScore.getSlotNumber() * 8) - 1, 5, 7, WHITE);
      Sprites::drawErase(xOffset + 10, yOffset + (highScore.getSlotNumber() * 8), font, highScore.getChar(2));
    }
    else {
      Sprites::drawOverwrite(xOffset + 10, yOffset + (highScore.getSlotNumber() * 8), font, highScore.getChar(2));
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
      gameState = GameState::Intro_Init; 
      
    }
    
  }

}
#include "src/utils/Arduboy2Ext.h"

// --------------------------------------------------------------------------------------
//  Render the screen
//
void RenderScreen(Player *player, Enemy *enemies) {


  // Render the horizon ;

  const uint8_t horizon[5][7] = {
                                  { 0, 2,  6, 12, 20, 30, 42 },
                                  { 0, 2,  7, 13, 22, 32, 45 },
                                  { 0, 3,  8, 15, 24, 35, 48 },
                                  { 0, 3,  9, 16, 26, 37, 51 },
                                  { 0, 4, 10, 18, 28, 40, 54 }
                                };                        

  uint8_t row = level.getHorizon();

  for (uint8_t col = 0; col < HORIZON_COL_COUNT; col++) {

    uint8_t y = horizon[row][col] + 4;
    arduboy.drawHorizontalDottedLine((col + 2) / 2, WIDTH, y, col + 2);

  }


  // Render the enemies ..

  for (uint8_t x = 0; x < MAX_NUMBER_OF_ENEMIES; x++) {

    Enemy *enemy = &enemies[x];

    uint8_t const *imageName = nullptr;
    uint8_t const *maskName = nullptr;
    uint8_t const *spawnImageName = nullptr;
    uint8_t const *spawnMaskName = nullptr;    
    uint8_t frame = 0;

    if (enemy->isVisible()) {

      switch (enemy->getStatus()) {

        case EnemyStatus::Active:
        
          switch (enemy->getSize()) {

            case ImageSize::Horizon:

              imageName = alien_horizon;
              maskName = alien_horizon_mask;
              break;

            case ImageSize::Far:

              imageName = alien_far;
              maskName = alien_far_mask;
              break;

            case ImageSize::Medium:
       
              frame = static_cast<uint8_t>(enemy->getType());

              if (arduboy.getFrameCount(ENEMY_FRAME_COUNT) < ENEMY_FRAME_COUNT_HALF) {
                imageName = alien_medium_1;
                maskName = alien_medium_1_mask;
              }
              else {
                imageName = alien_medium_2;
                maskName = alien_medium_2_mask;
              }

              break;

            case ImageSize::Close:
     
              frame = static_cast<uint8_t>(enemy->getType());

              if (arduboy.getFrameCount(ENEMY_FRAME_COUNT) < ENEMY_FRAME_COUNT_HALF) {
                imageName = alien_close_1;
                maskName = alien_close_1_mask;
              }
              else {
                imageName = alien_close_2;
                maskName = alien_close_2_mask;
              }

              break;

          }


          // Render image ..

          Sprites::drawExternalMask(enemy->getXDisplay(), enemy->getYDisplay(), imageName, maskName, frame, frame);
          break;


        case EnemyStatus::Dead ... EnemyStatus::Explosion4:

          switch (enemy->getStatus()) {

            case EnemyStatus::Explosion1:
              imageName = alien_close_explosion_4;
              break;

            case EnemyStatus::Explosion2:
              imageName = alien_close_explosion_3;
              break;

            case EnemyStatus::Explosion3:
              imageName = alien_close_explosion_2;
              break;

            case EnemyStatus::Explosion4:
              imageName = alien_close_explosion_1;
              break;

            default: break;
              
          }

          Sprites::drawSelfMasked(enemy->getXDisplay(), enemy->getYDisplay(), imageName, 0);
          break;


        case EnemyStatus::Spawn1 ... EnemyStatus::Spawn8:
        
          uint8_t spawnIdx = static_cast<uint8_t>(enemy->getStatus()) - static_cast<uint8_t>(EnemyStatus::Spawn1);
          frame = static_cast<uint8_t>(enemy->getType());

          switch (enemy->getSize()) {

            case ImageSize::Medium:

              if (arduboy.getFrameCount(ENEMY_FRAME_COUNT) < ENEMY_FRAME_COUNT_HALF) {
                imageName = alien_medium_1;
                maskName = alien_medium_1_mask;
              }
              else {
                imageName = alien_medium_2;
                maskName = alien_medium_2_mask;
              }

              spawnImageName = spawning_medium;
              spawnMaskName = spawning_medium_mask;

              break;

            case ImageSize::Close:

              if (arduboy.getFrameCount(ENEMY_FRAME_COUNT) < ENEMY_FRAME_COUNT_HALF) {
                imageName = alien_close_1;
                maskName = alien_close_1_mask;
              }
              else {
                imageName = alien_close_2;
                maskName = alien_close_2_mask;
              }

              spawnImageName = spawning_close;
              spawnMaskName = spawning_close_mask;


              break;

          }

          Sprites::drawExternalMask(enemy->getXDisplay(), enemy->getYDisplay(), imageName, maskName, frame, frame);
          Sprites::drawExternalMask(enemy->getXDisplay() - 1, enemy->getYDisplay(), spawnImageName, spawnMaskName, spawnIdx, spawnIdx);
          break;

      }
 
    }
    else { // Enemy is not visible ..

      if (enemy->getStatus() == EnemyStatus::Active) {

        if (enemy->getYDisplay() >= 64 && enemy->getYDisplay() <= VISIBLE_SCREEN_WIDTH) {

          if (enemy->getXDisplay() < static_cast<int16_t>(0)) {

            arduboy.drawPixel(0, 59, BLACK);
            arduboy.drawFastVLine(0, 60, 3);
            arduboy.drawFastHLine(0, 63, 3);
            arduboy.drawPixel(4, 63, BLACK);

          }

          if (enemy->getXDisplay() > static_cast<int16_t>(VISIBLE_SCREEN_WIDTH)) {

            arduboy.drawPixel(113, 59, BLACK);
            arduboy.drawFastVLine(113, 60, 3);
            arduboy.drawFastHLine(110, 63, 3);
            arduboy.drawPixel(109, 63, BLACK);

          }
          else {

            arduboy.drawPixel(enemy->getXDisplay() + 2, 63, BLACK);
            arduboy.drawFastHLine(enemy->getXDisplay() + 3, 63, 6);
            arduboy.drawPixel(enemy->getXDisplay() + 9, 63, BLACK);

          }

        }
        else {

          if (enemy->getXDisplay() < static_cast<int16_t>(0)) {

            arduboy.drawPixel(0, enemy->getYDisplay() + 2, BLACK);
            arduboy.drawFastVLine(0, enemy->getYDisplay() + 3, 6, WHITE);
            arduboy.drawPixel(0, enemy->getYDisplay() + 9, BLACK);

          }

          if (enemy->getXDisplay() > static_cast<int16_t>(VISIBLE_SCREEN_WIDTH)) {

            arduboy.drawPixel(113, enemy->getYDisplay() + 2, BLACK);
            arduboy.drawFastVLine(113, enemy->getYDisplay() + 3, 6, WHITE);
            arduboy.drawPixel(113, enemy->getYDisplay() + 9, BLACK);

          }

        }

      }

    }

  }


  // Render the space ship ..  

  const uint8_t frames[] = {0, 4, 3, 3, 2};
  uint8_t frameRate = frames[absT(player->getYDelta())];

  if (player->getStatus() == PlayerStatus::Active) {
      
    switch (player->getYDelta()) {

      case -4 ... -1:

        if (arduboy.getFrameCount(frameRate) % frameRate < frameRate / 2) { 
          Sprites::drawExternalMask(player->getX(), player->getY(), spaceship_backwards_1, spaceship_backwards_1_mask, 0, 0);
        }
        else {
          Sprites::drawExternalMask(player->getX(), player->getY(), spaceship_backwards_2, spaceship_backwards_2_mask, 0, 0);
        }
        break;

      case 0:

        Sprites::drawExternalMask(player->getX(), player->getY(), spaceship_neutral, spaceship_neutral_mask, 0, 0);
        break;

      case 1 ... 4:

        if (arduboy.getFrameCount(frameRate) % frameRate < frameRate / 2) { 
          Sprites::drawExternalMask(player->getX(), player->getY(), spaceship_advance_1, spaceship_advance_1_mask, 0, 0);
        }
        else {
          Sprites::drawExternalMask(player->getX(), player->getY(), spaceship_advance_2, spaceship_advance_2_mask, 0, 0);
        }
        break;

    }

  }
  else {

    uint8_t const *imageName = nullptr;

    switch (player->getStatus()) {

      case PlayerStatus::Explosion1:
        imageName = alien_close_explosion_4;
        break;

      case PlayerStatus::Explosion2:
        imageName = alien_close_explosion_3;
        break;

      case PlayerStatus::Explosion3:
        imageName = alien_close_explosion_2;
        break;

      case PlayerStatus::Explosion4:
        imageName = alien_close_explosion_1;
        break;

      default: 
        imageName = alien_close_explosion_5;
        break;
        
    }

    Sprites::drawSelfMasked(player->getX(), player->getY(), imageName, 0);
    
  }


  // Render player bullet ..

  if (playerBullet.getY() > 0) {

    arduboy.drawFastVLine(playerBullet.getX(), playerBullet.getY(), 5, WHITE); 

  }


  // Render enemy bullets ..

  for (uint8_t x = 0; x < MAX_NUMBER_OF_BULLETS; x++) {

    Bullet *bullet = &bullets[x];

    if (bullet->getY() > 0) { 

      arduboy.drawRect(bullet->getX(), bullet->getY(), 2, 2, WHITE);

    }

  }


  if (gameState == GameState::GamePlay && arduboy.everyXFrames(2)) {
    
    alternate++;

  }


  // Render scoreboard ..

  Sprites::drawOverwrite(114, 0, blankScoreboard, 0);


  // Score ..

  uint8_t digits[6] = {};
  extractDigits(digits, level.getScore());
  
  for (int8_t i = 5, y = 1; i >= 0; --i, y += 5) {
    Sprites::drawOverwrite(123, y, numbers_vert, digits[i]);
  }


  if (gameState == GameState::Wave || gameState == GameState::WaveCompleted) {
  
    if (introDelay % 2 == 1) {

      for (int8_t i = player->getLives(), y = 64 - (player->getLives() * 6); i > 0; --i, y += 6) {
        Sprites::drawOverwrite(123, y, life_vert, 0);
      }

    }
    else {
      
      for (int8_t i = player->getLives() - 1, y = 64 - ((player->getLives() - 1) * 6); i > 0; --i, y += 6) {
        Sprites::drawOverwrite(123, y, life_vert, 0);
      }

    }

  }
  else {
      
    for (int8_t i = player->getLives() - 1, y = 64 - ((player->getLives() - 1) * 6); i > 0; --i, y += 6) {
      Sprites::drawOverwrite(123, y, life_vert, 0);
    }
    
  }

  Sprites::drawOverwrite(116, 0, fuelGauge, 0);
  arduboy.drawLine(119, 28, 119, (player->getFuel() + 6), BLACK);

  uint8_t health = player->getHealth() * 2;
  Sprites::drawOverwrite(116, 39, shieldGauge, 0);
  arduboy.drawLine(119, 61, 119, 46 + health, BLACK);


  // Wave completed ?

  if (gameState == GameState::WaveCompleted) {

    if (arduboy.everyXFrames(12) && fuelBonusDisplay < fuelBonus) {

      fuelBonusDisplay++;
      level.setScore(level.getScore() + FUEL_BONUS_INC);
      
    }

    arduboy.fillRect(19, 18, 78, 22, BLACK);
    Sprites::drawSelfMasked(20, 19, waveCompleted, 0);

    Sprites::drawSelfMasked(74, 30, numbers, (fuelBonusDisplay * FUEL_BONUS_INC_DIV) / 10);
    Sprites::drawSelfMasked(79, 30, numbers, (fuelBonusDisplay * FUEL_BONUS_INC_DIV) % 10);
    Sprites::drawSelfMasked(84, 30, numbers, 0);
    Sprites::drawSelfMasked(89, 30, numbers, 0);

  }



  // Start of game or wave?

  if (gameState == GameState::Wave) {

    if (introDelay != 2 && introDelay != 4 && introDelay != 6 && introDelay != 8) {

      if (level.getWave() == 1 && player->getLives() == MAX_NUMBER_OF_LIVES) {

        arduboy.fillRect(30, 21, 53, 13, BLACK);
        Sprites::drawSelfMasked(31, 22, getReady, 0);

      }
      else {

        arduboy.fillRect(35, 21, 46, 13, BLACK);
        Sprites::drawSelfMasked(36, 22, startOfWave, 0);
        Sprites::drawSelfMasked(67, 25, numbers, level.getWave() / 10);
        Sprites::drawSelfMasked(72, 25, numbers, level.getWave() % 10);

      }

    }
    else {

      if (introDelay == 2) {
        if (!sound.playing()) sound.tones(blib2);
      }
      else {
        if (!sound.playing()) sound.tones(blib);
      }

    }

  }

  if (gameState == GameState::Paused) {
    
    arduboy.fillRect(44, 21, 33, 13, BLACK);
    Sprites::drawSelfMasked(41, 22, pause, 0);

  }


  // Double up score ..

  alternate2++;
  if (alternate2 > 96) alternate2 = 0;

  if (level.inDoubleUpPhase()) {

    arduboy.setRGBled(0, (alternate2 < 48 ? (alternate2 + 2) / 2 : 50 - (alternate2 / 2)), 0);
   
    if (alternate2 < 48) {

      Sprites::drawSelfMasked(117, 31, x2_flashing, 0);
 
    }

  }
  else {

    arduboy.setRGBled(0, 0, 0);

  }

}

#include "src/utils/Arduboy2Ext.h"


// --------------------------------------------------------------------------------------
//  Render a score table ..
//
void ScoreTable() {

  alternate++;
  if (alternate > 108) alternate = 0;
  bool first = alternate % 36 < 18;


  // Render score table ..

  Sprites::drawOverwrite(20, 1, scoreTable, 0);

  Sprites::drawOverwrite(10, 21, (first ? alien_close_1 : alien_close_2), 0);
  Sprites::drawOverwrite(29, 20, ellipsis, 0);
  Sprites::drawOverwrite(44, 23, numbers, 1);  
  Sprites::drawOverwrite(49, 23, numbers, 5);  
  Sprites::drawOverwrite(54, 23, numbers, 0);  

  Sprites::drawOverwrite(10, 31, (first ? alien_close_2 : alien_close_1), 1);
  Sprites::drawOverwrite(29, 30, ellipsis, 0);
  Sprites::drawOverwrite(44, 33, numbers, 3);  
  Sprites::drawOverwrite(49, 33, numbers, 0);  
  Sprites::drawOverwrite(54, 33, numbers, 0);  
 
  Sprites::drawOverwrite(10, 41, alien_close_1, 04);
  Sprites::drawOverwrite(29, 40, ellipsis, 0);
  Sprites::drawOverwrite(44, 43, numbers, 4);  
  Sprites::drawOverwrite(49, 43, numbers, 5);  
  Sprites::drawOverwrite(54, 43, numbers, 0); 

  Sprites::drawOverwrite(70, 21, (first ? alien_close_1 : alien_close_2), 2);
  Sprites::drawOverwrite(89, 20, ellipsis, 0);
  Sprites::drawOverwrite(104, 23, numbers, 6);  
  Sprites::drawOverwrite(109, 23, numbers, 0);  
  Sprites::drawOverwrite(114, 23, numbers, 0); 

  Sprites::drawOverwrite(70, 31, (first ? alien_close_2 : alien_close_1), 3);
  Sprites::drawOverwrite(89, 30, ellipsis, 0);
  Sprites::drawOverwrite(104, 33, numbers, 2);  
  Sprites::drawOverwrite(109, 33, numbers, 5);  
  Sprites::drawOverwrite(114, 33, numbers, 0);  
 
  Sprites::drawOverwrite(70, 41, alien_close_1, 5);
  Sprites::drawOverwrite(89, 40, ellipsis, 0);
  Sprites::drawOverwrite(107, 43, x2_score_table, 0);  


  // Display 'Press A' button after a short period of time ..

  if (introDelay < INTRO_DELAY) {
    introDelay++;
  }
  else {    
    Sprites::drawOverwrite(51, 55, pressA, 0);
  }


  // If 'A' bUtton is pressed move to game play ..

  if (arduboy.justPressed(A_BUTTON)) { 

//    player.reset(); 
//    level.reset(enemies, bullets, &playerBullet);
    gameState = GameState::Wave_Init; 
    
  }

}
#include "src/utils/Arduboy2Ext.h"
#include "src/utils/FadeEffects.h"


IntroductionState selectedIntroItem = IntroductionState::Start;
constexpr const static uint8_t UPLOAD_DELAY = 16;


// --------------------------------------------------------------------------------------
//  Show introduction screen ..
//
void Intro() {


  // Draw logo ..

  Sprites::drawOverwrite(18, 1, logo, 0);


  if (!fadeInEffect.isComplete()) {

    fadeInEffect.draw(arduboy);
    fadeInEffect.update();

  }


  // Display 'Press A' button after a short period of time ..

  if (introDelay < INTRO_DELAY) {
    introDelay++;
  }
  else {    

    Sprites::drawOverwrite(12, 55, start_normal, 0);
    Sprites::drawOverwrite(45, 55, highscore_normal, 0);
    Sprites::drawOverwrite(88, 55, credits_normal, 0);

    if (selectedIntroItem == IntroductionState::Start)      { Sprites::drawOverwrite(12, 62, start_highlight, 0); }
    if (selectedIntroItem == IntroductionState::HighScore)  { Sprites::drawOverwrite(45, 62, highscore_highlight, 0); }
    if (selectedIntroItem == IntroductionState::Credits)    { Sprites::drawOverwrite(88, 62, credits_highlight, 0); }

  }


  // Handle keypress ..

  if (arduboy.justPressed(LEFT_BUTTON) && selectedIntroItem > IntroductionState::Start) { selectedIntroItem--; }
  if (arduboy.justPressed(RIGHT_BUTTON) && selectedIntroItem < IntroductionState::Credits) { selectedIntroItem++; }

  if (arduboy.justPressed(A_BUTTON)) { 

    switch (selectedIntroItem) {

      case IntroductionState::Start:
        gameState = GameState::ScoreTable; 
        introDelay = 0;
        break;

      case IntroductionState::HighScore:
        gameState = GameState::HighScore; 
        break;

      case IntroductionState::Credits:
        gameState = GameState::Credits; 
        break;

    }
    
  }

  // Restart ?

	if (arduboy.pressed(DOWN_BUTTON)) {

		if (restart < UPLOAD_DELAY) {
			restart++;
		}
		else {
      arduboy.exitToBootloader();
		}

	}
	else {
		restart = 0;
	}

}
#include "src/utils/Arduboy2Ext.h"

static constexpr uint8_t ppotFrames = (1.6 *DEFAULT_FRAME_RATE); // 1.6s animation loop 


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

    auto justPressed = arduboy.justPressedButtons();

    if (justPressed > 0) {
        
        gameState = GameState::Intro_Init; 

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
