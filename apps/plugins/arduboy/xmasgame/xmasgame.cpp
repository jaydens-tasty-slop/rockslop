#include <Tinyfont.h>
#include <Arduboy2.h>
#include <ArduboyPlaytune.h>
#include <ArdVoice.h>
//#include <PGMWrap.h>

#include "EEPROMAnything.h"
#include "structs.h"
#include "levels.h"
#include "music.h"
#include "images.h"
#include "voices.h"

//#define DEBUG
//#define MIRRORING

//Initialize the arduboy object
Arduboy2 arduboy;
ArdVoice ardvoice;
ArduboyPlaytune tunes(arduboy.audio.enabled);
Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, Arduboy2::width(), Arduboy2::height());

const int EEPROM_START = EEPROM_STORAGE_SPACE_START + 100;
GameState gameState = GameState::Splash;
unsigned int currentLevel = 0;
bool usedPieces[16], levelBoard[15][7], levelPiece[15][8];
byte currentPieceSelection = 0, currentPiecePositionX = 0, currentPiecePositionY = 0, oldLoadedPiece = 0,
     currentLevelWidth, currentLevelHeight, currentLevelTotalPieces, currentPieceWidth, currentPieceHeight,
     currentPieceNumber,  boardInitialXPosition, boardInitialYPosition;
const byte *currentLevelPieceStartAddress;
unsigned long timerExpireOn;
const byte pieceWidthInPixels = 8, pieceHeightInPixels = 8;

void initializeAudio();
unsigned int getSave();
void setSave(const unsigned int & level);

void setup() {
  arduboy.begin();
  //arduboy.boot();
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
  currentLevel = getSave();
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

// Draw the current pieces in the board, returns true if the board is full
bool drawBoardPieces()
{
  bool isComplete = true;
  // Draw board pieces
  for (int x = 0; x < currentLevelWidth; x++)
    for (int y = 0; y < currentLevelHeight; y++)
      if (levelBoard[x][y])
        arduboy.fillRect(boardInitialXPosition + x * pieceWidthInPixels, boardInitialYPosition + y * pieceHeightInPixels,
                         pieceWidthInPixels, pieceHeightInPixels);
      else
        isComplete = false;

  return isComplete;
}

// Load a piece into memory
void loadPiece(byte pieceNumber)
{
  byte pieceNum = pgm_read_byte_near(currentLevelPieceStartAddress + pieceNumber);
  int piecePos = pgm_read_word_near(pieces + pieceNum);
  int pieceEnd = pgm_read_word_near(pieces + pieceNum + 1);

  byte w = pgm_read_byte_near(piecesBytes + piecePos);

  // Retrieve piece
  byte totalBytes = pieceEnd - piecePos - 1;
  int totalBits = totalBytes * 8;
  bool bits[totalBits];

  for (int i = 0; i < totalBytes; i++)
  {
    byte pieceByte = pgm_read_byte_near(piecesBytes + piecePos + 1 + i);
    for (int b = 0; b < 8; b++)
      bits[(i * 8) + b] = bitRead(pieceByte, b);
  }

  byte lastBit = 0;
  for (int i = 0; i < totalBits; i++)
  {
    levelPiece[i % w][i / w] = bits[i];

    if (bits[i])
      lastBit = i;
  }

  currentPieceWidth = w;
  currentPieceHeight = 1 + (lastBit / w);
  currentPieceNumber = pieceNumber;
}

// Load a level into memory
void loadLevel(unsigned int levelNumber)
{
  int currentLevelPos = pgm_read_word_near(levels + levelNumber);
  int currentLevelEnd = pgm_read_word_near(levels + levelNumber + 1);

  currentLevelWidth = pgm_read_byte_near(levelsBytes + currentLevelPos);
  currentLevelHeight = pgm_read_byte_near(levelsBytes + currentLevelPos + 1);

  for (int i = 0; i < currentLevelWidth; i++)
    for (int j = 0; j < currentLevelHeight; j++)
      levelBoard[i][j] = false;

  currentLevelPieceStartAddress = levelsBytes + currentLevelPos + 2;
  currentLevelTotalPieces = currentLevelEnd - currentLevelPos - 2;
  currentPieceNumber = 0;
  currentLevel = levelNumber;
}

// Draws the current piece, returns true if the space is empty to drop the piece in the board
bool drawCurrentPiece(byte xoffset, byte yoffset, bool placePiece = false)
{
  bool boardCanReceivePiece = true, putBow = true;
  for (int x = 0; x < currentPieceWidth; x++)
    for (int y = 0; y < currentPieceHeight; y++)
    {
      if (levelPiece[x][y])
        if (levelBoard[x + xoffset][y + yoffset])
        {
          boardCanReceivePiece = false;
          arduboy.drawRoundRect(boardInitialXPosition + (xoffset + x) * pieceWidthInPixels,
                                boardInitialYPosition + (y + yoffset) * pieceHeightInPixels, pieceWidthInPixels, pieceHeightInPixels, 1, BLACK);
        }
        else
        {
          if (placePiece)
            levelBoard[x + xoffset][y + yoffset] = true;

          arduboy.fillRoundRect(boardInitialXPosition + (xoffset + x)  * pieceWidthInPixels,
                                boardInitialYPosition + (y + yoffset) * pieceHeightInPixels, pieceWidthInPixels, pieceHeightInPixels, 1);

        }

      if (putBow && x == currentPieceWidth - 1)
      {
        if (!levelBoard[x + xoffset][y + yoffset] )
          arduboy.drawBitmap(boardInitialXPosition + (xoffset + x)  * pieceWidthInPixels,
                             boardInitialYPosition + (y + yoffset) * pieceHeightInPixels, bow, 8, 8, BLACK);
        putBow = false;
      }

    }

  return boardCanReceivePiece;
}

void drawHUD()
{
  // Hud
  tinyfont.setCursor(0, 0);
  tinyfont.print("LEVEL ");

  if (currentLevel < 99)
    tinyfont.print("0");
  if (currentLevel < 9)
    tinyfont.print("0");
  tinyfont.print(currentLevel + 1);
  tinyfont.setCursor(WIDTH / 2 + 5, 0);
  tinyfont.print("REMAINING ");

  byte left = 0;
  for (int i = 0; i < currentLevelTotalPieces; i++)
    if (!usedPieces[i])
      left++;
  if (left < 9)
    tinyfont.print("0");
  tinyfont.print(left);
}

GameState doPlacingPiece()
{
  GameState newState = GameState::PlayingPlacingPiece;
  arduboy.clear();

  drawHUD();

  if (arduboy.justPressed(LEFT_BUTTON)) {
    tunes.tone(1100, 30);
    currentPiecePositionX -= currentPiecePositionX > 0;
  }
  if (arduboy.justPressed(RIGHT_BUTTON)) {
    tunes.tone(1100, 30);
    currentPiecePositionX += currentPiecePositionX < currentLevelWidth - currentPieceWidth;
  }
  if (arduboy.justPressed(UP_BUTTON)) {
    tunes.tone(1100, 30);
    currentPiecePositionY -= currentPiecePositionY > 0;
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    tunes.tone(1100, 30);
    currentPiecePositionY += currentPiecePositionY < currentLevelHeight - currentPieceHeight;
  }

  // Draw board boundary
  arduboy.drawRect(boardInitialXPosition - 1, boardInitialYPosition - 1, pieceWidthInPixels * currentLevelWidth + 2,
                   pieceHeightInPixels * currentLevelHeight + 2);
  drawBoardPieces();

  bool canPieceBeDroppedHere = drawCurrentPiece(currentPiecePositionX, currentPiecePositionY);

  if (arduboy.justPressed(B_BUTTON)) // Change the piece
    newState = GameState::PlayingChoosingPiece;

  if (arduboy.justPressed(A_BUTTON)) // Try to place the piece
  {
    if (canPieceBeDroppedHere && !usedPieces[currentPieceNumber])
    {
      drawCurrentPiece(currentPiecePositionX, currentPiecePositionY, true);
      usedPieces[currentPieceNumber] = true;

      // Search next unnused piece
      for (int i = 0; i < currentLevelTotalPieces; i++)
        if (!usedPieces[i])
        {
          currentPieceSelection = i;
          break;
        }

      newState = drawBoardPieces() ? GameState::AdvanceToNextLevel : GameState::PlayingChoosingPiece;
      tunes.tone(1300, 50);
      delay(50);
      tunes.tone(1500, 100);
    }
    else
    {
      // TODO: Handle error or warning
      tunes.tone(400, 200);
    }
  }
  arduboy.display();

  return newState;
}

bool blink = false;
bool doSplash()
{
  arduboy.clear();
  arduboy.drawCompressed(0, 0, splash_packer);

  if (arduboy.everyXFrames(blink ? 30 : 10))
    blink = !blink;

  if (blink)
  {
    arduboy.setCursor(45, HEIGHT - 8);
    arduboy.print("Press B");
  }

  arduboy.display();
  return arduboy.justPressed(B_BUTTON) || arduboy.justPressed(A_BUTTON);
}

void loadPieceToPlay(byte piece)
{
  currentPiecePositionX = 0;
  currentPiecePositionY = 0;
  loadPiece(piece);
}

// Draw the menu for choosing the next piece
bool doChoosingPiece()
{
  arduboy.clear();

  arduboy.setCursor(0, 0);
  arduboy.print("CHOOSE GIFT PATTERN");
  arduboy.setCursor(0, HEIGHT - 8);
  arduboy.print("Press A+B to reset");

  int xini = 0;
  const byte yini = 25, sideSize = 4;

  // Measure
  for (int i = 0; i < currentPieceSelection; i++)
  {
    loadPiece(i);
    xini += currentPieceWidth * sideSize + 3;
  }

  if (xini > WIDTH / 2)
    xini = -xini / 2;
  else
    xini = 0;

  for (int i = 0; i < currentLevelTotalPieces; i++)
  {
    loadPiece(i);

    for (int x = 0; x < currentPieceWidth; x++)
      for (int y = 0; y < currentPieceHeight; y++)
        if (levelPiece[x][y])
          arduboy.fillRoundRect(xini + 1 + i * sideSize + x * sideSize, yini + y * sideSize, sideSize, sideSize, 1);

    if (currentPieceSelection == i)
    {
      arduboy.drawBitmap(xini + i * sideSize + 2, yini + 10, handmask, 13, 13, BLACK);
      arduboy.drawBitmap(xini + i * sideSize + 3, yini + 11, hand, 12, 12);
    }

    if (usedPieces[i])
      arduboy.drawBitmap(xini + i * sideSize, yini - 14, check, 12, 12);
    else
      arduboy.drawRect(xini + i * sideSize + 2, yini - 12, 9, 9);

    xini += currentPieceWidth * sideSize + 3;
  }

  if (arduboy.justPressed(LEFT_BUTTON)) {
    tunes.tone(1100, 30);
    currentPieceSelection -= currentPieceSelection > 0;
  }

  if (arduboy.justPressed(RIGHT_BUTTON)) {
    tunes.tone(1100, 30);
    currentPieceSelection += currentPieceSelection < currentLevelTotalPieces - 1;
  }

  arduboy.display();

  bool exit = false;

  if (arduboy.justPressed(A_BUTTON))
    if (usedPieces[currentPieceSelection])
    {
      // TODO: Handle error or warning
      tunes.tone(400, 200);
    }
    else
    {
      tunes.tone(1500, 100);
      delay(50);
      tunes.tone(1300, 50);

      loadPieceToPlay(currentPieceSelection);
      exit = true;
    }

  if (arduboy.justPressed(B_BUTTON) && !usedPieces[oldLoadedPiece])
  {
    loadPieceToPlay(oldLoadedPiece);
    exit = true;
  }

  return exit;
}
bool resetSound = true;

void loop()
{
  if (!arduboy.nextFrame())
    return;

  arduboy.pollButtons();

  // Reset routine
  if (gameState == GameState::PlayingPlacingPiece || gameState == GameState::PlayingChoosingPiece)
  {
    if (arduboy.pressed(A_BUTTON + B_BUTTON))
    {
      resetSound = true;
      for (int i = 0; i < 255; i++)
      {
        tunes.tone(2000 - i * 40, 6);
        delay(4);
      }

      arduboy.setRGBled(0, 0, 0);
      gameState = GameState::LevelStart;
    }
  }

  switch (gameState)
  {
    case GameState::AdvanceToNextLevel:
      // TODO: Handle winning, ending, etc
      drawHUD();
      arduboy.display();

      currentLevel++;
      setSave(currentLevel);
      gameState = GameState::LevelStart;
      break;

    case GameState::Splash:
      if (!tunes.playing())
        tunes.playScore(splash);

      if (doSplash())
      {
        tunes.stopScore();
        gameState = GameState::LevelStart;
      }
      break;

    case GameState::LevelStart:
      loadLevel(currentLevel);

      boardInitialXPosition = (WIDTH - currentLevelWidth * pieceWidthInPixels) / 2;
      boardInitialYPosition = 3 + (HEIGHT - currentLevelHeight * pieceHeightInPixels) / 2;

      currentPieceSelection = 0;
      loadPieceToPlay(currentPieceSelection);

      for (int i = 0; i < currentLevelTotalPieces; i++)
        usedPieces[i] = false;
      currentPieceSelection = 0;

      timerExpireOn = millis() + 1000;
      gameState = GameState::LevelStartPreSplash;

      tunes.stopScore();

      if (currentLevel == 0 || resetSound)
      {
        ardvoice.playVoice(hohoho);
        resetSound = false;
      }
      else
      {
        tunes.playScore(win);
      }

      break;

    case GameState::LevelStartPreSplash:
      if (millis() > timerExpireOn)
      {
        gameState = GameState::LevelStartSplash;
        timerExpireOn = millis() + 1000;
      }
      break;

    case GameState::LevelStartSplash:
      arduboy.clear();
      arduboy.setCursor(24, (HEIGHT - 8) / 2);
      arduboy.print("Level ");
      arduboy.print(currentLevel + 1);
      arduboy.print(" Start!");
      arduboy.display();

      if (millis() > timerExpireOn)
      {
        gameState = GameState::PlayingPlacingPiece;
      }
      break;

    case GameState::PlayingPlacingPiece:
      if (!tunes.playing())
        tunes.playScore(level);

      gameState = doPlacingPiece();

      //if (gameState != GameState::PlayingPlacingPiece)
      //  tunes.stopScore();
      //byte oldLoadedPiece = currentPieceNumber;
      break;

    case GameState::PlayingChoosingPiece:
      /*if (!tunes.playing())
        tunes.playScore(choose);*/

      if (doChoosingPiece())
      {
        //tunes.stopScore();
        gameState = GameState::PlayingPlacingPiece;
      }
      break;

    /*
       case GameState::PlayingPlacingPiece:
       {
       if (arduboy.justPressed(LEFT_BUTTON))
       {
       currentLevel--;
       }

       if (arduboy.justPressed(RIGHT_BUTTON))
       {
       currentLevel++;
       }

       if (arduboy.justPressed(UP_BUTTON))
       {
       currentPieceNumber++;
       loadPiece(currentPieceNumber);
       }

       if (arduboy.justPressed(DOWN_BUTTON))
       {
       currentPieceNumber--;
       loadPiece(currentPieceNumber);
       }

       if (arduboy.justPressed(A_BUTTON))
       {
       loadLevel(currentLevel);
       Serial.println("Level loaded.");
       }

       arduboy.clear();
       tinyfont.setCursor(0, 0);
       tinyfont.print("LEVEL: ");
       tinyfont.print(currentLevel);
       tinyfont.print("\nTOTAL PIECES: ");
       tinyfont.print(currenLevelTotalPieces);
       tinyfont.print("\nLEVEL SIZE W,H: ");
       tinyfont.print(currentLevelWidth);
       tinyfont.print(",");
       tinyfont.print(currentLevelHeight);
       tinyfont.print("\nPIECE: ");
       tinyfont.print(currentPieceNumber);
       tinyfont.print("\nPIECE SIZE W,H: ");
       tinyfont.print(currentPieceWidth);
       tinyfont.print(",");
       tinyfont.print(currentPieceHeight);
       tinyfont.print("\n\nLOAD BOARD WITH (A)\nBOARD:       PIECE: ");

       // Draw level and piece
       int xini = 3;
       int yini = HEIGHT - 12;

       arduboy.drawRect(xini - 1, yini - 1, currentLevelWidth + 2, currentLevelHeight + 2);

       for (int x = 0; x < currentLevelWidth; x++)
       for (int y = 0; y < currentLevelHeight; y++)
       if (levelBoard[x][y])
        arduboy.drawPixel(xini + x, yini + y);

       xini = 30;
       //arduboy.drawRect(xini - 1, yini - 1, currentPieceWidth + 2, currentPieceHeight + 2);


       for (int x = 0; x < currentPieceWidth; x++)
       for (int y = 0; y < currentPieceHeight; y++)
       if (levelPiece[x][y])
       {
        arduboy.drawPixel(xini + x, yini + y);
       }

       arduboy.display();
       }
       break // */

    default: // Level
      /*if (!tunes.playing() && gameState == GameState::Playing)
        tunes.playScore(level);*/
      //gameState = doGame();
      //gameState = doGame(gameState == GameState::Playing);

      break;
  }
}


const byte CRC = 0xAE, OFFSET = 12;
const unsigned int CRC_C = 48000;

// Get the latest solved puzzle
unsigned int getSave()
{
  unsigned int level, crc;

  bool isValid = false;
  if (EEPROM.read(EEPROM_START) == CRC)
  {
    EEPROM_readAnything(EEPROM_START + OFFSET, level);
    EEPROM_readAnything(EEPROM_START + OFFSET + sizeof(level), crc);

    if (level + crc == CRC_C)
      isValid = true;
  }

  return isValid ? level : 0;
}

// Save the last solved puzzle
void setSave(const unsigned int & level)
{
  EEPROM.write(EEPROM_START, CRC);
  EEPROM_writeAnything(EEPROM_START + OFFSET, level);
  EEPROM_writeAnything(EEPROM_START + OFFSET + sizeof(level), CRC_C - level);
}
