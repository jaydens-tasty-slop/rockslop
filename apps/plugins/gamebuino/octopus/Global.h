#pragma once
#ifndef OCTOPUS_GLOBAL
#define OCTOPUS_GLOBAL

// -------------------------------------------------------------------------
// Global constants
// -------------------------------------------------------------------------

constexpr uint8_t  screenWidth  = 160;
constexpr uint8_t  screenHeight = 128;
constexpr uint8_t  sliceHeight  = 8;                    // Redefined the size of the buffer to take less memory
constexpr uint8_t  slices = screenHeight / sliceHeight; // Number of horizontal slices to be cut is calculated
constexpr uint16_t transColor   = 0xdfff;
constexpr uint16_t sRed   = 0x00f8;
constexpr uint16_t sGreen = 0xe007;
constexpr uint16_t sPink  = 0x1ff8;
constexpr uint16_t sBlack = 0x0000;

// -------------------------------------------------------------------------
// Define global variables
// -------------------------------------------------------------------------

int16_t  moveTick;
int16_t  maxTick = 100;
uint16_t minHighscore;
uint8_t animFrame = 0;
bool catchGold = false;
bool hited = false;
bool inGold = false;
uint16_t score = 0;
uint16_t getPoint = 0;
uint8_t life = 3;
bool subGold = false;
uint8_t subMove = 0;
uint8_t barcaState = 0;
bool moveSubButton = true;
uint8_t dieFrame = 0;
bool endGame = false;
uint16_t highScore = 0;
uint8_t diverToShow = 0;
uint8_t diverArmToShow = 0;
uint8_t diverBagToShow = 0;
bool debug = false;
// just for debug
uint16_t oldVal;
uint8_t speedAnim = 4;

// -------------------------------------------------------------------------
// Sprite visibility
// -------------------------------------------------------------------------

bool showSpriteDiver1 = false;
bool showSpriteDiver1Arm1 = false;
bool showSpriteDiver1Arm2 = false;
bool showSpriteDiver2 = false;
bool showSpriteDiver3 = false;

// legs animation markers
uint8_t octopusLeg[] = {1,1,1,1,1,};
uint8_t octopusMoveLeg[] = {1,1,1,1,1,};

// legs lengths
const uint8_t octopusLegLength[] = {0,4,5,4,3,};

bool showOctopusLeg1[4];
constexpr bool showOctopusLeg1Settings[5][4]
{
  { false, false, false, false,},
  { true,  false, false, false,},
  { true,   true, false, false,},
  { true,   true,  true, false,},
  { true,   true,  true,  true,},
};

bool showOctopusLeg2[5];
constexpr bool showOctopusLeg2Settings[6][5]
{
  { false, false, false, false, false,},
  { true,  false, false, false, false,},
  { true,   true, false, false, false,},
  { true,   true,  true, false, false,},
  { true,   true,  true,  true, false,},
  { true,   true,  true,  true,  true,},
};

bool showOctopusLeg3[4];
constexpr bool showOctopusLeg3Settings[5][4]
{
  { false, false, false, false, },
  { true,  false, false, false, },
  { true,   true, false, false, },
  { true,   true,  true, false, },
  { true,   true,  true,  true, },
};

bool showOctopusLeg4[3];
constexpr bool showOctopusLeg4Settings[4][3]
{
  { false, false, false, },
  { true,  false, false, },
  { true,   true, false, },
  { true,   true,  true, },
};


// -------------------------------------------------------------------------
// All possible game state
// -------------------------------------------------------------------------

enum class GameState : uint8_t {
  home,
  run,
  gameOver,
  pauseScreen
};

GameState gameState {GameState::home};

#endif
