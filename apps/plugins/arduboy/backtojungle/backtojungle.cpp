#include <Arduino.h>
#include <ab_math.h>
#include <PGMWrap.h>
#include <Arduboy2.h>
#include <ArduboyPlaytune.h>

#include "EEPROMAnything.h"
#include "music.h"
#include "sprites.h"
#include "phrases.h"
#include "images.h"
#include "structs.h"

//#define DEBUG
//#define MIRRORING

//Initialize the arduboy object
Arduboy2 arduboy;
ArduboyPlaytune tunes(arduboy.audio.enabled);

const int INVALID = -32768, EEPROM_START = EEPROM_STORAGE_SPACE_START + 100;
const byte BYTE_MAX = 0xFF;

// ------ Game parameters -------
const char STARTLIVES = 3, STARTINGLEVEL = 0;
int FORWARD_BOOST = 12; // Use higher value to make the player jump further.
// -----------------------------

char lives = STARTLIVES;

unsigned long currentFrame = 0, currentLevelTime = 0;
byte currentLevel = STARTINGLEVEL, specialCar;
GameState gameState = GameState::Splash;
unsigned long wait = 0, debounce = 0;
unsigned int score = 0, highscore = 0;
bool specialCarBonus = true, fasterDialogs = false;
double playerx, playery;

/* Arduino IDE auto-prototypes */
unsigned int getHiScore();
void initializeAudio();
bool waitForButton(bool isModal = true, bool useAlternativeButton = false, byte offset = 7);
void animateDialog(const unsigned char animal[], const unsigned char str[], bool showLives = false);
void softReset();
void doWinSoundEffect();
void doWinDialog();
void doPause();
void doCrash();
void doIntro();
void doRandomTip();
void doSplash();
void doIntroTheme();
GameState doGame(bool interactive);
void doMainMenu();
void setHiScore(const unsigned int & score);

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.clear();
  arduboy.initRandomSeed();

  initializeAudio();

#if defined(DEBUG) || defined(MIRRORING)
  Serial.begin(9600);
#endif

#ifdef DEBUG
  delay(1000);
  Serial.println("Debug enabled");
#endif

  // Load highscore
  highscore = getHiScore();
}

void initializeAudio()
{
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
}


// Holds the game until B is pressed (or returns B state if is not modal)
bool waitForButton(bool isModal = true, bool useAlternativeButton = false, byte offset = 7)
{
  arduboy.drawBitmap(arduboy.width() - 8, arduboy.height() - offset, useAlternativeButton ? nextAlt : next, 8, 7);

#ifdef MIRRORING
  Serial.write(arduboy.getBuffer(), arduboy.width() * arduboy.height() / 8);
#endif
  arduboy.display();

  if (isModal)
  {
    while (arduboy.pressed(useAlternativeButton ? A_BUTTON : B_BUTTON)) {};
    while (arduboy.notPressed(useAlternativeButton ? A_BUTTON : B_BUTTON)) {};
    while (arduboy.pressed(useAlternativeButton ? A_BUTTON : B_BUTTON)) {};
    return true;
  }
  else
    return arduboy.pressed(useAlternativeButton ? A_BUTTON : B_BUTTON);
}

// Displays an animal image with an animated dialog and optionally the remaining lives
void animateDialog(const unsigned char animal[], const unsigned char str[], bool showLives = false)
{
  const int xposimage = 0;
  const int xpostext = 52;
  int ypos = 8;

  arduboy.clear();

  if (showLives)
  {
    ypos = 0;

    // Show lives
    for (byte l = 0; l < lives; l++)
      arduboy.drawBitmap(2 + (l * 16), arduboy.height() - 11, life, 15, 11);
  }

  // Draw marquee
  arduboy.drawRect(xposimage, ypos + 1, 50, 50);

  // Animal
  arduboy.drawBitmap(xposimage + 1, ypos + 2, animal, 48, 48);

  bool doDelays = true;
  int pos = ypos;
  arduboy.setCursor(xpostext, pos);

  for (byte i = 0; i < BYTE_MAX; i++)
  {
    char c = pgm_read_byte(str++);

    switch (c)
    {
      case '\0':
        i = 254; // Exit
        break;

      case '_':
        pos += 9;
        arduboy.setCursor(xpostext, pos );
        break;

      case '%':
        arduboy.display();
        arduboy.delayShort(doDelays ? 200 : 1);
        break;

      case ' ':
        arduboy.print(c);
        break;

      case '$':
        arduboy.print(highscore);
        arduboy.display();
        doWinSoundEffect();
        break;

      case '&':
        arduboy.print(score);

        if (score > 0)
        {
          arduboy.display();
          doWinSoundEffect();
        }
        break;

      case '|':
        // Nothing
        break;

      case '^':
        waitForButton();
        break;


      default:
        arduboy.print(c);
        tunes.tone(900, doDelays ? 20 : ((fasterDialogs && gameState != GameState::MainMenu) ? 1 : 10));
        arduboy.display();
        arduboy.delayShort(doDelays ? 100 : ((fasterDialogs && gameState != GameState::MainMenu) ? 1 : 12));
        break;
    }

    if (arduboy.pressed(B_BUTTON))
      doDelays = false;
  }

#ifdef MIRRORING
  Serial.write(arduboy.getBuffer(), arduboy.width() * arduboy.height() / 8);
#endif

  arduboy.display();
  arduboy.delayShort(doDelays ? 1500 : ((fasterDialogs && gameState != GameState::MainMenu) ? 10 : 300));

  while (arduboy.pressed(B_BUTTON));
}

// Restarts the game from the splash screen (the AVR build jumped to address 0)
void softReset()
{
  gameState = GameState::Splash;
}

// Plays and animates the win chime
void doWinSoundEffect()
{
  tunes.stopScore();
  arduboy.invert(true);
  tunes.stopScore();
  tunes.playScore(win);
  arduboy.delayShort(1700);
  arduboy.invert(false);
}

// Handles the end of a level, display a small talk in between levels
void doWinDialog()
{
#ifdef DEBUG
  Serial.println("LEVEL WIN");
  Serial.print("currentLevel=");
  Serial.println(currentLevel);
#endif

  doWinSoundEffect();

  // Update current score
  long elapsedTime = millis() - currentLevelTime;
  score += (currentLevel + 1) * 100 + lives * 25 + (60 - min(elapsedTime / 1000, 60)); // Current level, lives and elapsed time (up to a minute) defines the score

  // Save HI-SCORE
  if (score > highscore)
  {
    highscore = score;
    setHiScore(score);
  }

  switch (currentLevel++)
  {
    case 0:
      animateDialog(snakebig, playing1);
      animateDialog(monkeybig, playing2);
      break;

    case 1:
      animateDialog(snakebig, playing1);
      animateDialog(monkeybig, playing3);
      animateDialog(monkeybig, playing4);
      break;

    case 2:
      animateDialog(monkeybig, playing41);
      animateDialog(snakebig, intro7);
      break;

    case 3:
      animateDialog(monkeybig, playing5);
      break;

    case 4:
      animateDialog(giraffebig, playing6);
      break;

    case 5:
      animateDialog(giraffebig, playing7);
      animateDialog(monkeybig, playing8);
      break;

    case 6:
      animateDialog(monkeybig, playing9);
      animateDialog(monkeybig, playing91);
      animateDialog(giraffebig, playing92);
      bool blinking = 0;
      wait = millis() + 2000;

      arduboy.clear();
      arduboy.drawCompressed(0, 0, ending);

      do
      {
        // Animate glowing
        arduboy.drawBitmap(55, 34, ending_effect, 23, 15, blinking);
        blinking = !blinking;
        arduboy.delayShort(30);
        arduboy.display();

      } while (millis() < wait || !waitForButton(false));

      while (!arduboy.notPressed(A_BUTTON + B_BUTTON)); // Wait for button release

      animateDialog(monkeybig, playing93);
      animateDialog(giraffebig, playing94);
      animateDialog(monkeybig, playing95);
      animateDialog(giraffebig, playing96);
      animateDialog(snakebig, playing1);
      animateDialog(giraffebig, playing961);

      // Ending animation
      waitForButton();
      tunes.playScore(travelling);
      arduboy.clear();
      arduboy.display();

      for (byte i = 0; i < 8; i++)
      {
        arduboy.delayShort(150);
        arduboy.invert(true);
        arduboy.delayShort(150);
        arduboy.invert(false);
      }

      animateDialog(snakebig, playing97);
      softReset();
      break;
  }
  waitForButton();
}

// Pause game
void doPause()
{
  tunes.stopScore();
  tunes.playScore(pause);
  arduboy.invert(true);
  arduboy.setCursor(47, 28);
  arduboy.print("PAUSED");
  arduboy.display();
  arduboy.delayShort(300);
  waitForButton();
  arduboy.invert(false);
}

// Animal crashed with a car
void doCrash()
{
  tunes.stopScore();
  tunes.tone(1300, 400);

  arduboy.invert(true);
  arduboy.delayShort(500);
  tunes.tone(1290, 600);

  arduboy.invert(false);
  arduboy.delayShort(100);
  arduboy.invert(true);
  arduboy.delayShort(900);
  arduboy.invert(false);
}

// Intro dialog and animation
void doIntro()
{
  animateDialog(monkeybig, intro1);
  animateDialog(giraffebig, intro2);
  animateDialog(monkeybig, intro3);
  animateDialog(snakebig, intro4);
  animateDialog(monkeybig, intro5);
  animateDialog(monkeybig, intro51, true);
  animateDialog(giraffebig, intro6);
  animateDialog(snakebig, intro7);
  waitForButton();
}

// Plays a random monkey game tip
void doRandomTip()
{
  byte r = random(0, 30);
  unsigned char const * tip = tip3; // 5/30 chance
  if (r >= 21) // 9/30 chance
    tip = tip1;
  else if (r >= 15) // 5/30 chance
    tip = tip2;
  else if (r >= 10) //  5/30 chance
    tip = tip4;
  else
    switch (r)
    {
      // About 1/30 chance each
      case 2: tip = tip7; break;
      case 4: tip = tip9; break;
      case 5: tip = tip6; break;
      case 6: tip = tip5; break;
      case 7: tip = tip8; break;
    }
  animateDialog(monkey2, tip);
}

// Splash animation
void doSplash()
{
  arduboy.clear();
  arduboy.display();
  arduboy.delayShort(500);

  for (int i = -8; i < 26; i = i + 1)
  {
    arduboy.clear();
    arduboy.setCursor(13, i);
    arduboy.print("Innovation Garage");
    arduboy.display();
    arduboy.delayShort(20);
  }

  arduboy.delayShort(600);
  arduboy.setCursor(41, 35);
  arduboy.print("presents");
  arduboy.display();
  arduboy.delayShort(2000);
  gameState = GameState::MainMenu;
}

// Starts the menu music
void doIntroTheme()
{
  if (!tunes.playing())
    tunes.playScore(introtheme);
}

// Main game loop, use interactive false for the level intro
GameState doGame(bool interactive)
{
  GameState exitCode = !interactive ? GameState::StartLevelIntro : ( playerx > arduboy.width() - 3 ? GameState::Win : GameState::Playing);
  arduboy.clear();

  // Streets
  const int laneoffset = 61;
  arduboy.drawLine(20, 0, 20, arduboy.height()); // Lane 1
  arduboy.drawLine(20 + laneoffset, 0, 20 + laneoffset, arduboy.height()); // Lane 2

  const byte streetLine = 16;
  for (byte i = 0; i < arduboy.height(); i += streetLine)
  {
    arduboy.drawLine(39, i, 39, i + 7);  // Lane 1
    arduboy.drawLine(39 + laneoffset, i, 39 + laneoffset, i + 7); // Lane 2
  }

  arduboy.drawLine(58, 0, 58, arduboy.height()); // Lane 1
  arduboy.drawLine(58 + laneoffset, 0, 58 + laneoffset, arduboy.height()); // Lane 2

  // Player
  playerSpecs player = playerTypes[currentLevel];

  if (playerx == INVALID)
    playerx = -player.width;

  if (playery == INVALID)
    playery = (arduboy.height() - player.height) / 2;

  arduboy.drawBitmap(floor(playerx), floor(playery), player.mask, player.width, player.height, BLACK);
  arduboy.drawBitmap(floor(playerx), floor(playery), player.bitmap, player.width, player.height);

  // TODO: Make this dynamic
  const byte blockx = 34, blocky = 30, blockw = 11, blockh = 5;

  if (currentLevel < 2)
  {
    // Road blocks
    arduboy.drawBitmap(blockx, blocky, block, blockw, blockh); // Lane 1
  }

  if (interactive)
  {
    if (arduboy.pressed(B_BUTTON))
    {
      // Pause game
      doPause();
      return GameState::Playing;
    }

    int multiplier = 1;
    if ( millis() > debounce)
    {
      if (arduboy.pressed(A_BUTTON))
      {
        debounce = millis() + 500;
        multiplier = FORWARD_BOOST;
      }
    }
    else if (!arduboy.pressed(A_BUTTON))
      debounce -= 50; // Decrease the wait
    else
      debounce += 50; // Increase the wait

    double newx = playerx, newy = playery;
    if (arduboy.pressed(LEFT_BUTTON))    newx -= multiplier * player.vx;
    if (arduboy.pressed(RIGHT_BUTTON))   newx += multiplier * player.vx;
    if (arduboy.pressed(UP_BUTTON))      newy -= multiplier * player.vy;
    if (arduboy.pressed(DOWN_BUTTON))    newy += multiplier * player.vy;

    // Check if the new position is valid
    bool valid = false;

    if (newx >= 0 && newy >= -2 && (newy + player.height) <= arduboy.height() + 2)
    {
      if (currentLevel < 2)
        valid = !arduboy.collide({(byte)newx, (byte)newy, player.width, player.height},
      {blockx + 2, blocky + 2, blockw - 4, blockh - 4});
      else
        valid = true;
    }

    if (valid && (playerx != newx || playery != newy)) // Update
    {
      if (multiplier > 1)
        tunes.tone(1100, 30);
      else if (wait < millis())
      {
        tunes.tone(600, 6);
        wait = millis() + 200;
      }
      playerx = newx;
      playery = newy;
    }
  }

  // Border
  arduboy.drawBitmap(0, 0, fence, 4, 32);
  arduboy.drawBitmap(0, 32, fence, 4, 32);
  arduboy.drawBitmap(arduboy.width() - 5, 0, right_foliage, 6, 32);
  arduboy.drawBitmap(arduboy.width() - 5, 32, right_foliage, 6, 32);

  // Enemies
  byte enemy = 0;
  for (byte e = enemiesOffset[currentLevel]; e < enemiesOffset[currentLevel + 1]; e++)
  {
    if (currentFrame > enemies[e].wait)
    {
      enemySpecs specs = enemyTypes[enemies[e].type];
      enemiesPos[enemy] += enemies[e].vy;

      if (playerx + player.width >= enemies[e].x && playerx <= enemies[e].x + specs.width)
        arduboy.drawBitmap(enemies[e].x, floor(enemiesPos[enemy]), specialCar == e ? car2mask : specs.mask , specs.width, specs.height, BLACK);
      arduboy.drawBitmap(enemies[e].x, floor(enemiesPos[enemy]), specialCar == e ? car2 : specs.bitmap, specs.width, specs.height);

      // Loop
      if (enemiesPos[enemy] < -32)
      {
        enemiesPos[enemy] = arduboy.height();

        if (specialCar == e)
          specialCar = BYTE_MAX;
        else if (specialCar == BYTE_MAX && enemies[e].type == CAR)
          if (random(0, 50) == 7)
          {
            if (specialCarBonus)
            {
              score += 500;
              specialCarBonus = false;
            }
            tunes.stopScore();
            tunes.playScore(car2music);
            specialCar = e;
          }
      }

      // Check collision
      if (arduboy.collide({(byte)playerx + 2, (byte)playery + 2, (byte)(player.width - 4), (byte)(player.height - 4)},
      {enemies[e].x + 2, (byte)(enemiesPos[enemy] + 2), (byte)(specs.width - 4), (byte)(specs.height - 4)}))
      {
        doCrash();
        exitCode = GameState::Lose;
      }
    }
    enemy++;
  }

  // Foliage
  arduboy.drawBitmap(63, -10, tree, 18, 18);
  arduboy.drawBitmap(57, 14, tree, 18, 18);
  arduboy.drawBitmap(65, 37, tree, 18, 18);
  arduboy.drawBitmap(56, 58, tree, 18, 18);

  currentFrame++;

#ifdef MIRRORING
  Serial.write(arduboy.getBuffer(), arduboy.width() * arduboy.height() / 8);
#endif

  arduboy.display();
  return exitCode;
}

// Shows and handles the main menu
void doMainMenu()
{
  bool exit = false;
  char option = 0;

  // Track menu movement pattern
  byte pos = 0;
  const byte max = 17;
  byte seq[] = {2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2};
  byte mov[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  do {
    doIntroTheme();
    arduboy.clear();
    arduboy.drawCompressed(14, 2, intro);
    arduboy.setCursor(24, 55);
    arduboy.print((char)0x11);

    switch (option)
    {
      case 0: arduboy.setCursor(33, 55); arduboy.print("Start game"); break;
      case 1: arduboy.setCursor(33, 55); arduboy.print("Sound: "); arduboy.print(arduboy.audio.enabled() ? " ON" : "OFF"); break;
      case 2: arduboy.setCursor(39, 55); arduboy.print("Hi-score"); break;
      case 3: arduboy.setCursor(43, 55); arduboy.print("Credits"); break;
      case 4: arduboy.setCursor(52, 55); arduboy.print("Help"); break;
    }
    arduboy.setCursor(arduboy.width() - 24 - 8, 55);
    arduboy.print((char)0x10);

#ifdef MIRRORING
    Serial.write(arduboy.getBuffer(), arduboy.width() * arduboy.height() / 8);
#endif

    arduboy.display();
    arduboy.pollButtons();

    if (arduboy.justPressed(B_BUTTON))
    {
      while (arduboy.pressed(B_BUTTON));

      switch (option)
      {
        case 0: {
            exit = true;
            bool ee = true;
            for (byte i = 0; i < max; i++)
              if (mov[pos++] != seq[i])
              {
                ee = false;
                break;
              }
              else
                pos %= max;

            if (ee)
            {
              FORWARD_BOOST += 500;
              doWinSoundEffect();
            }
          } break;
        case 1: tunes.stopScore(); arduboy.audio.toggle(); arduboy.audio.saveOnOff(); break;
        case 2: animateDialog(monkey2, (score == 0 && highscore == 0) ? hiscore0 : hiscore1); waitForButton(); break;
        case 3: animateDialog(qr, credits0); waitForButton(); animateDialog(qr, credits01); waitForButton(); animateDialog(qr, credits1); animateDialog(qr, credits2); waitForButton(); break;
        case 4: animateDialog(monkey2, tip0); doRandomTip(); waitForButton(); break;
      }
    }
    else
    {
      if (arduboy.justPressed(LEFT_BUTTON))
      {
        mov[pos++] = 1;
        option--;
        tunes.tone(1000, 30);
      }

      if (arduboy.justPressed(RIGHT_BUTTON))
      {
        mov[pos++] = 2;
        option++;
        tunes.tone(1000, 30);
      }

      pos %= max; // Bounded
      option = option < 0 ? 4 : (option > 4 ? 0 : option);
    }
  } while (!exit);

  tunes.stopScore();
  tunes.tone(987, 120);
  arduboy.delayShort(60);
  tunes.tone(1318, 400);
  gameState = GameState::Intro;
}

void loop()
{
  if (!arduboy.nextFrame())
    return;

  switch (gameState)
  {
    case GameState::Splash:
      doIntroTheme();
      doSplash();

    case GameState::MainMenu:
      doIntroTheme();
      doMainMenu();
      break;

    case GameState::Intro:
      doIntro();

    case GameState::SkipIntro:
      specialCarBonus = false;
      score = 0;
      currentLevel = 0;
      lives = STARTLIVES;
      gameState = GameState::StartLevel;
      break;

    case GameState::End:
      if (!tunes.playing())
        tunes.playScore(gameovertheme);

      arduboy.clear();
      if (wait < millis())
      {
        // Show gameover text
        arduboy.setCursor(12, 25);
        arduboy.setTextSize(2);
        arduboy.print("GAME OVER");
        arduboy.setTextSize(1);
      }

      if (millis() > debounce)
      {
        // Show score
        arduboy.setCursor(0, arduboy.height() - 7);
        arduboy.print("Score: ");
        arduboy.print(score);

        arduboy.setCursor(0, 0);
        arduboy.print("Hi: ");
        arduboy.print(highscore);

        while (!arduboy.notPressed(A_BUTTON + B_BUTTON)); // Wait for button release

        arduboy.setCursor(arduboy.width() - 40, 0);
        arduboy.print("Retry");
        arduboy.setCursor(arduboy.width() - 34, arduboy.height() - 7);
        arduboy.print("Menu");
        arduboy.display();

        do
        {
          if ( waitForButton(false))
            gameState = GameState::MainMenu;
          else if ( waitForButton(false, true, arduboy.height()))
            gameState = GameState::SkipIntro;
        } while (gameState == GameState::End);

        while (!arduboy.notPressed(A_BUTTON + B_BUTTON)); // Wait for button release
        tunes.stopScore();
      }
      arduboy.display();

      break;

    case GameState::Win:
      doWinDialog();
      gameState = GameState::StartLevel;
      break;

    case GameState::GameOver:
    case GameState::Lose:
      if (--lives <= 0)
      {
        animateDialog(monkeybig, gameover2);
        arduboy.delayShort(1000);
        gameState = GameState::End;
        wait =  millis() + (arduboy.audio.enabled() ? 2000 : 0);
        debounce = wait + 2000;
      }
      else
      {
        animateDialog(monkeybig, gameover1, true);
        waitForButton();
        gameState = GameState::StartLevel;
      }
      break;

    case GameState::StartLevel:
      for (byte i = 0; i < enemiesMax; i++)
        enemiesPos[i] = arduboy.height();

      tunes.playScore(levelintro);
      playerx = INVALID;
      playery = INVALID;
      currentFrame = 0;
      specialCar = BYTE_MAX;

    case GameState::StartLevelIntro:
      if (playerx != INVALID)
        playerx += 0.15;

      if (playerx > 3)
      {
        gameState = GameState::Playing;
        currentLevelTime = millis();
      }

    default: // Level
      if (!tunes.playing() && gameState == GameState::Playing)
        tunes.playScore(level);

      gameState = doGame(gameState == GameState::Playing);

#ifdef DEBUG
      Serial.print("PLAYING, NEXT STATE: ");
      Serial.println(gameState);
#endif
      break;
  }
}



const byte CRC = 0xAE, OFFSET = 2;
const unsigned int CRC_C = 48000;

// Get the high score
unsigned int getHiScore()
{
  unsigned int score, crc;

  bool isValid = false;
  if (EEPROM.read(EEPROM_START) == CRC)
  {
    EEPROM_readAnything(EEPROM_START + OFFSET, score);
    EEPROM_readAnything(EEPROM_START + OFFSET + sizeof(score), crc);

    if (score + crc == CRC_C)
      isValid = true;
  }

  if (score > 500) // At least the user passed the second screen
    fasterDialogs = true;

  return isValid ? score : 0;
}

// Save the high score
void setHiScore(const unsigned int & score)
{
  EEPROM.write(EEPROM_START, CRC);
  EEPROM_writeAnything(EEPROM_START + OFFSET, score);
  EEPROM_writeAnything(EEPROM_START + OFFSET + sizeof(score), CRC_C - score);
}

