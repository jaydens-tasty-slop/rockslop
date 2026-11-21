// HANGMAN!
// --------
// by serisman <arduboy@serisman.com>
//
// version: 1.2.1
//  - Minor formatting/structure changes
// version: 1.2.0
//  - Added Sound Effects (with ability to turn them off)
//  - Added a menu system (can always get back to the main menu with the B button)
//  - Moved the wins/losses score statistics to a separate screen with a reset option
// version: 1.1.0
//  - Added cumulative wins/losses tracking (save/restore to/from EEPROM)
//  - Show correct answer on loss
// version: 1.0.0
//  - Initial Release

#include <ArduboyTones.h>
#include <ArduboyTonesPitches.h>

#include <Arduboy2.h>
Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

//#define DEBUG 1

#include "EEPROM_Utils.h"
void nextFrame();
void showTitle(uint8_t cur);
void moveCursor(uint8_t num);
void nextTitleFrame();
void drawLogo();
void drawTitleMenu();
void drawMenuCursor(uint8_t num);
void toggleSound();
void showStats();
void nextStatsFrame();
void loadStats();
void saveStats();
void resetStats();
void drawStats();
void drawStatsMenu();
void startPlaying();
void nextPlayFrame();
void nextCorrectFrame();
void nextDeadFrame();
void drawScore();
void drawHangman();
void drawWord();
void drawKeyboard();
void drawCorrect();
void drawDead();
void scoreResponse(char letter);
void pickAWord();

#include "words.h"
#define sprintf_P sprintf
#include "sounds.h"

#define FPS                 30u

#define MODE_TITLE    0
#define MODE_STATS    1
#define MODE_PLAY     2
#define MODE_CORRECT  4
#define MODE_DEAD     5

uint8_t mode = MODE_TITLE;
bool paused = false;

uint8_t wins = 99;
uint8_t losses = 99;
uint8_t hangman = 0;
uint16_t previousWordIndex = 0xFFFF;
char currentWord[9];
uint8_t cursor = 0;
uint8_t cursorBlink = 0;
uint8_t usedLetters[26];

char buf[30];

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(FPS);
  sound.tones(soundWin);

  loadStats();
}

void loop() {
  if (arduboy.nextFrame()) {
    nextFrame();
  } else {
    arduboy.idle();
  }
}

void nextFrame() {
  arduboy.pollButtons();
  arduboy.clear();
  switch (mode) {
    case MODE_TITLE:    nextTitleFrame();   break;
    case MODE_STATS:    nextStatsFrame();   break;
    case MODE_PLAY:     nextPlayFrame();    break;
    case MODE_CORRECT:  nextCorrectFrame(); break;
    case MODE_DEAD:     nextDeadFrame();    break;
  }
  arduboy.display();
}

void showTitle(uint8_t cur) {
  mode = MODE_TITLE;
  cursor = cur;
}

void moveCursor(uint8_t num) {
  if (arduboy.justPressed(UP_BUTTON)) {
    if (cursor > 0) cursor --;
  } else if (arduboy.justPressed(DOWN_BUTTON)) {
    if (cursor < (num-1)) cursor ++;
  }
}

void nextTitleFrame() {
  moveCursor(3);

  if (arduboy.justPressed(A_BUTTON)) {
    if (cursor == 0)
      startPlaying();
    else if (cursor == 1)
      toggleSound();
    else if (cursor == 2)
      showStats();
    return;
  }

  if (!paused && arduboy.everyXFrames(FPS/3u)) {
    if (hangman < 6) hangman++;
    else hangman = 0;
  }

  drawLogo();
  drawTitleMenu();
  drawHangman();
}

void drawLogo() {
  arduboy.setCursor(0,6);
  arduboy.print(F("HANGMAN!"));
  arduboy.drawFastHLine(0,15,45);
  arduboy.setCursor(0,18);
  arduboy.print(F(" by serisman"));
}

void drawTitleMenu() {
  arduboy.setCursor(10,64-(8*3));
  if (paused)
    arduboy.print(F("Resume"));
  else
    arduboy.print(F("Start"));

  arduboy.setCursor(10,64-(8*2));
  arduboy.print(F("Sound:"));
  if (arduboy.audio.enabled()) {
    arduboy.print(F("ON"));
  } else {
    arduboy.print(F("OFF"));
  }

  arduboy.setCursor(10,64-(8*1));
  arduboy.print(F("Stats"));
  
  drawMenuCursor(3);
}

void drawMenuCursor(uint8_t num) {
  arduboy.setCursor(0,64-(8*num)+(cursor*8));
  arduboy.print(F(">"));
}

void toggleSound() {
  if (arduboy.audio.enabled()) {
    sound.tones(soundIncorrect);
    delay(250);
  }
  arduboy.audio.toggle();
  if (arduboy.audio.enabled()) {
    sound.tones(soundCorrect);
    delay(250);
  }
  arduboy.audio.saveOnOff();
}

void showStats() {
  mode = MODE_STATS;
  cursor = 1;
}

void nextStatsFrame() {
  moveCursor(2);

  if (arduboy.justPressed(A_BUTTON)) {
    if (cursor == 0)
      resetStats();
    else if (cursor == 1)
      showTitle(2);
    return;
  }

  if (arduboy.justPressed(B_BUTTON)) {
    showTitle(2);
    return;
  }

  if (!paused && arduboy.everyXFrames(FPS/3u)) {
    if (hangman < 6) hangman++;
    else hangman = 0;
  }

  drawStats();
  drawStatsMenu();
  drawHangman();
}

void loadStats() {
  EEPROM_init();
  wins = EEPROM_getWins();
  losses = EEPROM_getLosses();
}

void saveStats() {
	EEPROM_saveScore(wins, losses);
}

void resetStats() {
  wins = 0;
  losses = 0;
  saveStats();
}

void drawStats() {
  sprintf_P(buf, PSTR(" %u Wins\n %u Losses"), wins, losses);
  arduboy.setCursor(0,10);
  arduboy.print(buf);
}

void drawStatsMenu() {
  arduboy.setCursor(10,64-(8*2));
  arduboy.print(F("Reset"));

  arduboy.setCursor(10,64-(8*1));
  arduboy.print(F("Back"));

  drawMenuCursor(2);
}

void startPlaying() {
  if (!paused) {
    pickAWord();
    hangman = 0;
    memset(usedLetters, 0, sizeof(usedLetters));
  }
  paused = false;
  cursor = 0;
  mode = MODE_PLAY;
}

void nextPlayFrame() {
  if (arduboy.justPressed(RIGHT_BUTTON)) {
    if (cursor == 25) cursor = 0;
    else cursor++;
  }
  if (arduboy.justPressed(LEFT_BUTTON)) {
    if (cursor == 0) cursor = 25;
    else cursor--;
  }
  if (arduboy.justPressed(UP_BUTTON)) {
    if (cursor > 8) cursor -= 9;
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    if (cursor < (26-9)) cursor += 9;
  }

  if (arduboy.justPressed(A_BUTTON)) {
    if (usedLetters[cursor] == 0) {
      usedLetters[cursor] = 1;
      scoreResponse(cursor+65);
    }
  }

#ifdef DEBUG
  if (arduboy.justPressed(B_BUTTON)) { // For easier testing...
    startPlaying();
    return;
  }
#endif

  if (arduboy.justPressed(B_BUTTON)) {
    paused = true;
    showTitle(0);
    return;
  }

  if (arduboy.everyXFrames(FPS/3u)) {
    cursorBlink = 1-cursorBlink;
  }

  drawScore();
  drawHangman();
  drawWord();
  drawKeyboard();
}

void nextCorrectFrame() {
  if (arduboy.justPressed(A_BUTTON)) {
    startPlaying();
    return;
  }
  if (arduboy.justPressed(B_BUTTON)) {
    showTitle(0);
    return;
  }

  drawScore();
  drawHangman();
  drawWord();
  drawCorrect();
}

void nextDeadFrame() {
  if (arduboy.justPressed(A_BUTTON)) {
    startPlaying();
    return;
  }
  if (arduboy.justPressed(B_BUTTON)) {
    showTitle(0);
    return;
  }

  drawScore();
  drawHangman();
  drawWord();
  drawDead();
}

void drawScore() {
  sprintf_P(buf, PSTR("%uW-%uL"), wins, losses);
  arduboy.setCursor((128-50)-(strlen(buf)*6),0);
  arduboy.print(buf);
}

void drawHangman() {
  const uint8_t LEFT = (128-45);
  // width: 50
  // height: 64

  arduboy.fillRect(LEFT+5, 61, 40, 3); // ground
  arduboy.fillRect(LEFT+30, 0, 3, 64); // post
  arduboy.fillRect(LEFT+10, 0, 20, 3); // bar
  arduboy.fillRect(LEFT+10, 0, 3, 10); // noose

  if (hangman > 0)  // head
    arduboy.drawCircle(LEFT+11, 15, 5);
  if (hangman > 1)  // body
    arduboy.drawFastVLine(LEFT+11, 20, 20);
  if (hangman > 2)  // left arm
    arduboy.drawLine(LEFT+0, 22, LEFT+11, 25);
  if (hangman > 3)  // right arm
    arduboy.drawLine(LEFT+22, 22, LEFT+11, 25);
  if (hangman > 4)  // left leg
    arduboy.drawLine(LEFT+0, 52, LEFT+11, 40);
  if (hangman > 5)  // right leg
    arduboy.drawLine(LEFT+22, 52, LEFT+11, 40);

  if (paused) {
    arduboy.setCursor(LEFT+7,64/2-4);
    arduboy.print(F("Paused"));
  }
}

void drawWord() {

#ifdef DEBUG
  arduboy.setCursor(0,0);
  arduboy.print(currentWord); // for easier testing...
#endif

  uint8_t x = 0;
  const uint8_t y = 12;
  for (uint8_t chr=0; chr<strlen(currentWord); chr++) {
    uint8_t letter = currentWord[chr];
    if (usedLetters[letter-65] == 1) {
      arduboy.setCursor(x, y);
      arduboy.write(letter);
    } else {
      if (mode == MODE_DEAD) {
        arduboy.setCursor(x, y);
        arduboy.write(letter+32); // be nice and show the correct answer
      }
      arduboy.drawFastHLine(x-1, y+9, 7);
    }
    x += 9;
  }
}

void drawKeyboard() {
  uint8_t x = 0;
  uint8_t y = 19;
  for (uint8_t chr=0; chr<26; chr++) {
    if (chr % 9 == 0) {
      x = 0;
      y += 12;
    } else {
      x += 9;
    }

    if (cursor == chr && cursorBlink) {
      arduboy.drawFastHLine(x-1, y+8, 7);
    }

    arduboy.setCursor(x, y);
    if (usedLetters[chr] == 0) {
      arduboy.write(chr+65);
    } else {
      arduboy.write(chr+97); // use lower case for already used letters
    }
  }
}

void drawCorrect() {
  arduboy.setCursor(0,35);
  arduboy.print(F("YOU GOT IT!"));
}

void drawDead() {
  arduboy.setCursor(0,35);
  arduboy.print(F("YOU'RE DEAD!"));
}

void scoreResponse(char letter) {
  bool allDone = true;
  bool letterOk = false;
  for (uint8_t chr=0; chr<strlen(currentWord); chr++) {
    char wordLetter = currentWord[chr];
    if (usedLetters[wordLetter-65] == 0)
      allDone = false;
    if (wordLetter == letter)
      letterOk = true;
  }

  if (allDone) {
    sound.tones(soundWin);
    mode = MODE_CORRECT;
    wins++;
    saveStats();
  } else if (letterOk) {
    sound.tones(soundCorrect);
  } else {
    hangman++;
    if (hangman == 6) {
      sound.tones(soundDead);
      mode = MODE_DEAD;
      losses++;
      saveStats();
    } else {
      sound.tones(soundIncorrect);
    }
  }
}

void pickAWord() {
  uint16_t newWordIndex;
  uint8_t wordCount;
  uint16_t offset=0;

  //arduboy.initRandomSeed();
  randomSeed(millis());
  do {
    newWordIndex = random(WORD_COUNT);
  } while (newWordIndex == previousWordIndex);
  previousWordIndex = newWordIndex;

  for (uint8_t wordLen=2; wordLen<=8; wordLen++) {
    wordCount = pgm_read_byte(WORD_COUNTS + wordLen);
    if (newWordIndex < wordCount) {
      memset(currentWord, 0, sizeof(currentWord));
      offset += (newWordIndex * (uint16_t)wordLen);
      for (uint8_t i=0; i<wordLen; i++, offset++) {
        currentWord[i] = pgm_read_byte(WORDS + offset);
      }
      return;
    }
    newWordIndex -= wordCount;
    offset += ((uint16_t)wordCount * (uint16_t)wordLen);
  }
}
