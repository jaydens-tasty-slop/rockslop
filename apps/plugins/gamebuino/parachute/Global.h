#ifndef PARACHUTE_GLOBAL
#define PARACHUTE_GLOBAL

// -------------------------------------------------------------------------
// Global constants
// -------------------------------------------------------------------------

constexpr uint8_t  screenWidth  = 160;
constexpr uint8_t  screenHeight = 128;
constexpr uint8_t  sliceHeight  = 8;                    // Redefined the size of the buffer to take less memory
constexpr uint8_t  slices = screenHeight / sliceHeight; // Number of horizontal slices to be cut is calculated
constexpr uint16_t transColor   = 0xdfff;


// -------------------------------------------------------------------------
// Define value for spritesheet to use
// -------------------------------------------------------------------------

constexpr uint8_t  idSpritesheetA = 0;
constexpr uint8_t  idSpritesheetB = 1;


// -------------------------------------------------------------------------
// Define value of the first sprite of each colonne of paachutiste
// -------------------------------------------------------------------------

constexpr uint8_t  firstSpriteColonne0 = 0;
constexpr uint8_t  firstSpriteColonne1 = 7;
constexpr uint8_t  firstSpriteColonne2 = 13;
constexpr uint8_t  lastSpriteColonne2 = 17;


// -------------------------------------------------------------------------
// Define global variables
// -------------------------------------------------------------------------

uint16_t score;
uint16_t minHighscore;
int16_t  misses;
int16_t  moveTick;
int16_t  spawnDelay;
int8_t   spawnCount;
int8_t   speedMax;
uint8_t  nbParachutesLaunched;
int8_t   parachutes[10];
int8_t   helicopterAnimation;
int8_t   sharkAnimation;
int8_t   floodedAnimation;
uint8_t  speedBlades;

// -------------------------------------------------------------------------
// All possible game state
// -------------------------------------------------------------------------

enum class GameState : uint8_t {
  stateHome,
  stateRun,
  stateGameOver,
  statePause
};

GameState gameState {GameState::stateHome};

#endif
