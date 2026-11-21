#include <Arduino.h>
/* ----------------------------------------------------------------------------
 *   Pipes  by  Simon Holmes, 2017 
 * ---------------------------------------------------------------------------- 
 */
#include <Arduboy2.h> 
#include "Images.h"
#include "Puzzles.h"
#include <ArduboyTones.h>

#define NO_SCREENSHOTS

#define NOTHING                      0
#define PIPE_HORIZONTAL_LR           1
#define PIPE_HORIZONTAL_RL           2
#define PIPE_VERTICAL_TB             3
#define PIPE_VERTICAL_BT             4
#define PIPE_CORNER_TL               5
#define PIPE_CORNER_LT               6
#define PIPE_CORNER_TR               7
#define PIPE_CORNER_RT               8
#define PIPE_CORNER_BL               9
#define PIPE_CORNER_LB               10
#define PIPE_CORNER_BR               11
#define PIPE_CORNER_RB               12
#define NODE                         15

#define GRID_WIDTH                   11
#define GRID_HEIGHT                  11

#define UP                           0
#define DOWN                         1
#define LEFT                         2
#define RIGHT                        3

#define STATE_INTRO                  0
#define STATE_LEVEL_SELECT           1
#define STATE_PUZZLE_SELECT          2
#define STATE_INIT_GAME              3
#define STATE_NO_SELECTION           4
#define STATE_NODE_SELECTED          5
#define STATE_GAME_OVER              6
#define STATE_SPLASH_INIT            7
#define STATE_SPLASH                 8

#define PUZZLE_5X5                   5
#define PUZZLE_6X6                   6
#define PUZZLE_7X7                   7
#define PUZZLE_8X8                   8
#define PUZZLE_9X9                   9

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
Sprites sprites;


/* ----------------------------------------------------------------------------
 *  Basic structure to hold a node value and coordinates ..
 */
struct Node {
  byte x;
  byte y;
  byte value;
};


/* ----------------------------------------------------------------------------
 *  Player structure to record the highlighted and selected node ..
 */
struct Player {
  Node highlightedNode;
  Node selectedNode;
}
player = 
  { 
    {0, 0, 0}, 
    {0, 0, 0} 
  };

/* ----------------------------------------------------------------------------
 *  Scroll bar structure used to record the scroll bar location and rendering 
 *  properties.  These change depending on the number of rows in the puzzle. 
 */
struct Slider {
  byte unit;                              // Number of pixels / row for the slider.
  byte overall;                           // Height of the slider portion in pixels.
};


/* ----------------------------------------------------------------------------
 *  Puzzle structure.
 */
struct Puzzle {
  byte level;                             // Level being played, eg. PUZZLE_5X5, PUZZLE_6X6 ..
  byte index;                             // Puzzle within the current level being played.
  Node maximum;                           // Used to store the dimensions of the puzzle based
                                          // on the puzzle level, eg. PUZZLE_5X5 has a maximum
                                          // x and y value of 5.
  Node offset;                            // Stores the offest x and y values to ensure the 
                                          // is rendered centrally on the screen.
  Slider slider;                          // Scrollbar slider details.
  byte board[9][9];                       // Actual board details - supports maximum of 9 x 9.
}
puzzle = 
  {  0, 0,
    {0, 0, 0}, 
    {0, 0, 0}, 
    {0, 0}, 
    {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    },
    
  };

const byte* nodes[] = {0, node_1, node_2, node_3, node_4, node_5, node_6, node_7, node_8, node_9, node_10, node_11, node_12, node_13, node_14, node_15, node_16, node_17, node_18 };
const byte* pipes[] = {pipe_nothing, pipe_horizontal, pipe_horizontal, pipe_vertical, pipe_vertical, pipe_corner_TL, pipe_corner_TL, 
                       pipe_corner_TR, pipe_corner_TR, pipe_corner_BL, pipe_corner_BL, pipe_corner_BR, pipe_corner_BR};
const byte levels[] = {PUZZLE_5X5, PUZZLE_6X6, PUZZLE_7X7, PUZZLE_8X8, PUZZLE_9X9};

byte frame = 0;


/* ----------------------------------------------------------------------------
 *   Initialise the Arduboy and get ready ..
 */
/* Arduino IDE auto-prototypes */
bool initEEPROM();
void updateEEPROM(byte puzzleLevel, byte index);
byte readEEPROM(byte puzzleLevel);
void gameOver();
void gameOverDelay(int delayLength);
void splashScreen_Init();
void splashScreen();
void levelSelect();
void renderLevelDetail(byte x, byte y, byte level, byte highlight);
void puzzleSelect();
void renderPuzzleOption(byte x, byte y, String message, byte highlight);
void play_InitGame();
void play_NoSelection();
byte calculateTopRow();
void play_NodeSelected();
void updatePipeWhenReversing(byte x, byte y);
bool validMove(byte direction, Node selectedNode, int x, int y);
void renderBoard(int xOffset, int yOffset, byte topRow);
void playClearSelectionTune();
void playSelectNodeTune();
void playMatchNodeTune();
void playSplashTune();
void playPuzzleSolved();
void drawSplash();
bool splashAnimation(byte x, byte y, const uint8_t *bitmap, uint8_t frame, bool skip);
bool splashDelay(int delayLength, bool skip);
void splashWaitForever();
byte leftValue(byte val);
byte rightValue(byte val);
void initBoard(byte puzzleType, byte puzzleNumber);
bool nodeAlreadyPlayed(byte value);
void clearBoard(byte nodeValue);
bool isNode(byte x, byte y);
byte getNodeValue(byte x, byte y);
bool isPipe(byte x, byte y);
byte getPipeValue(byte x, byte y);
void setCellValue(byte x, byte y, byte pipeValue, byte nodeValue);
void clearSelection();
void clearHighlightAndSelection();
bool isPuzzleComplete();
byte getNumberOfPuzzles(byte puzzleLevel);
void toggleSoundSettings();
void drawHorizontalDottedLine(int x1, int x2, int y);
void screenShot();

void setup() {

  puzzle.level = PUZZLE_5X5;
  puzzle.index = 0;

  initEEPROM(); 

  arduboy.boot();
  arduboy.systemButtons();
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
  arduboy.audio.begin();
  
}


byte gameState = STATE_SPLASH_INIT;
byte prevState = STATE_SPLASH_INIT;


/* ----------------------------------------------------------------------------
 *  Play pipes !
 */
void loop() {

  if (!(arduboy.nextFrame())) return;
  arduboy.pollButtons();
  
  switch (gameState) {

    case STATE_SPLASH_INIT:
      splashScreen_Init();
      /*-fallthrough*/

    case STATE_SPLASH:
      splashScreen();
      break;

    case STATE_INTRO:
      drawSplash();
      break;

    case STATE_LEVEL_SELECT:
      levelSelect();
      break;

    case STATE_PUZZLE_SELECT:
      puzzleSelect();
      break;

    case STATE_INIT_GAME:
      play_InitGame();
      break;

    case STATE_NO_SELECTION:
      play_NoSelection();
      break;

    case STATE_NODE_SELECTED:
      play_NodeSelected();
      break;

    case STATE_GAME_OVER:
      gameOver();
      break;

  }
  
  arduboy.display();

}




#define EEPROM_START            EEPROM_STORAGE_SPACE_START 
#define EEPROM_START_C1         EEPROM_START
#define EEPROM_START_C2         EEPROM_START + 1
#define EEPROM_5X5              EEPROM_START + 2
#define EEPROM_6X6              EEPROM_START + 3
#define EEPROM_7X7              EEPROM_START + 4
#define EEPROM_8X8              EEPROM_START + 5
#define EEPROM_9X9              EEPROM_START + 6
#define EEPROM_PUZZLE_OFFSET    EEPROM_START - 3


/* ----------------------------------------------------------------------------
 *   Is the EEPROM initialised? 
 *   
 *   Looks for the characters 'L' and 'P' in the first two bytes of the EEPROM
 *   memory range starting from byte EEPROM_STORAGE_SPACE_START.  If not found,
 *   it resets the settings ..
 */
bool initEEPROM() {

  byte c1 = EEPROM.read(EEPROM_START_C1);
  byte c2 = EEPROM.read(EEPROM_START_C2);

  if (c1 != 76 || c2 != 80) { // LP 76 80
  
    EEPROM.update(EEPROM_START_C1, 76);
    EEPROM.update(EEPROM_START_C2, 80);
    EEPROM.update(EEPROM_5X5, 0);
    EEPROM.update(EEPROM_6X6, 0);
    EEPROM.update(EEPROM_7X7, 0);
    EEPROM.update(EEPROM_8X8, 0);
    EEPROM.update(EEPROM_9X9, 0);
      
  }

  return true; /* upstream fell off the end of a bool function */

}


/* ----------------------------------------------------------------------------
 *   Update the saved puzzle index for the nominated level.
 */
void updateEEPROM(byte puzzleLevel, byte index) {

  EEPROM.update(EEPROM_PUZZLE_OFFSET + puzzleLevel, index);

}


/* ----------------------------------------------------------------------------
 *   Read the saved puzzle index for the nominated level.
 */
byte readEEPROM(byte puzzleLevel) {

  return EEPROM.read(EEPROM_PUZZLE_OFFSET + puzzleLevel);

}

/* ----------------------------------------------------------------------------
 *   Render the Game Over screen.
 *   
 *   If this is the last puzzle in the level, the message 'Level Finished' is
 *   displayed otherwise a simple 'Congratulations' is shown.
 */
void gameOver() { 
  
  arduboy.delayShort(1000);

  arduboy.clear();
      
  sprites.drawOverwrite(112,  0, logo_straight_TB_noflange, frame);
  sprites.drawOverwrite(112, 16, logo_elbow_TL, frame);
  sprites.drawOverwrite(96,  16, logo_elbow_TR, frame);
  sprites.drawOverwrite(96,   0, logo_elbow_LB, frame);
  sprites.drawOverwrite(80,   0, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(64,   0, logo_straight_LR, frame);
  sprites.drawOverwrite(48,   0, logo_elbow_RB, frame);
  sprites.drawOverwrite(48,  16, logo_straight_TB_noflange, frame);
  sprites.drawOverwrite(48,  32, logo_elbow_TR, frame);
  sprites.drawOverwrite(64,  32, logo_elbow_LB, frame);
  sprites.drawOverwrite(64,  48, logo_straight_TB_noflange, frame);

  sprites.drawOverwrite(0,    0, logo_straight_TB_noflange, frame);
  sprites.drawOverwrite(0,   16, logo_straight_TB, frame);
  sprites.drawOverwrite(0,   32, logo_elbow_TR, frame);
  sprites.drawOverwrite(16,  32, logo_straight_LR, frame);
  sprites.drawOverwrite(32,  32, logo_elbow_TL, frame);
  sprites.drawOverwrite(32,  16, logo_elbow_RB, frame);
  sprites.drawOverwrite(48,  16, logo_cross_underlap_hflange, frame);
  sprites.drawOverwrite(64,  16, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(80,  16, logo_elbow_LB, frame);
  sprites.drawOverwrite(80,  32, logo_elbow_TR, frame);
  sprites.drawOverwrite(96,  32, logo_straight_LR, frame);
  sprites.drawOverwrite(112, 32, logo_straight_LR_noflange, frame);

  sprites.drawOverwrite(0,    0, logo_cross_underlap, frame);
  sprites.drawOverwrite(16,   0, logo_elbow_LB, frame);
  sprites.drawOverwrite(16,  16, logo_straight_TB, frame);
  sprites.drawOverwrite(16,  32, logo_cross_overlap_hflange, frame);
  sprites.drawOverwrite(16,  48, logo_elbow_TR, frame);
  sprites.drawOverwrite(32,  48, logo_straight_LR, frame);
  sprites.drawOverwrite(48,  48, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(64,  48, logo_cross_underlap_hflange, frame);
  sprites.drawOverwrite(80,  48, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(96,  48, logo_elbow_LB, frame);
  arduboy.display();

  
  // Play 'puzzle solved' tune before the message is shown ..

  playPuzzleSolved();
  arduboy.delayShort(1000);

  arduboy.setTextColor(BLACK);
  arduboy.setTextBackground(WHITE);

  arduboy.fillRect(12, 23, 104, 18, BLACK);
  arduboy.fillRect(13, 24, 102, 16, WHITE);
  
  if (puzzle.index == getNumberOfPuzzles(puzzle.level) - 1) {
  
    arduboy.setCursor(21, 28);
    arduboy.print("Level Finished!");
    gameState = STATE_LEVEL_SELECT;
  
  }
  else {
   
    arduboy.setCursor(18, 28);
    arduboy.print("Congratulations!");
    gameState = STATE_INIT_GAME;

  }

  puzzle.index++;
  updateEEPROM(puzzle.level, puzzle.index);
  
  arduboy.display();
  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);

  gameOverDelay(1000);
  
}


/* ----------------------------------------------------------------------------
 *   Delay the screen for a period of time.  
 *   
 *   The delay can be cancelled by clicking either the A or B button.
 */
void gameOverDelay(int delayLength) {

  arduboy.pollButtons();

  while (delayLength >= 0) {

    if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) { break; /* return true;*/ }
    
    arduboy.delayShort(5);  
    delayLength--;

  }
  
}



static constexpr uint8_t ppotFrames = (1.6 *30); // 1.6s animation loop


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void splashScreen_Init() {

    gameState = STATE_SPLASH;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void splashScreen() { 

    if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
        
        gameState = STATE_INTRO;

    }


    Sprites::drawOverwrite(32, 16, PPOT, 0);

    uint8_t p = 0x11; // Default pixel position 0 is hidden in the image

    switch (arduboy.frameCount % ppotFrames) {

        // 2nd quarter of frames
        case (ppotFrames*1/4) ... (ppotFrames*2/4)-1:
            p = 0x31; // Move pixel down to position 1
            /*-fallthrough*/

        // 1st quarter of frames
        case 0 ... (ppotFrames*1/4)-1:
            // Flash 'Play' arrow by clearing the image
            // Overwrite directly to the screen buffer 
            arduboy.sBuffer[91 +(3*WIDTH)] = 0x00;
            arduboy.sBuffer[92 +(3*WIDTH)] = 0x00;
            arduboy.sBuffer[93 +(3*WIDTH)] = 0x00;
            arduboy.sBuffer[94 +(3*WIDTH)] = 0x00;
            arduboy.sBuffer[95 +(3*WIDTH)] = 0x00;
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

#define MENU_ITEM_3            3
#define MENU_ITEM_3_X          19
#define MENU_ITEM_3_1_Y        19
#define MENU_ITEM_3_2_Y        29
#define MENU_ITEM_3_3_Y        39
#define MENU_ITEM_3_WIDTH      92   
#define MENU_ITEM_3_HEIGHT     9

#define MENU_ITEM_2            2
#define MENU_ITEM_2_X          17
#define MENU_ITEM_2_1_Y        23
#define MENU_ITEM_2_2_Y        35
#define MENU_ITEM_2_WIDTH      98   
#define MENU_ITEM_2_HEIGHT     11


byte levelSelect_selectedItem = 0;
byte levelSelect_topItem = 0;
byte puzzleSelect_selectedItem = 0;


/* ----------------------------------------------------------------------------
 *   Handle the scolling and selection of the levels.  
 */
void levelSelect() {

  clearHighlightAndSelection();

  arduboy.clear();
  
  sprites.drawOverwrite(-2, -2, logo_elbow_RB, frame);
  sprites.drawOverwrite(14, -2, logo_straight_LR, frame);
  sprites.drawOverwrite(30, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(46, -2, logo_straight_LR, frame);
  sprites.drawOverwrite(62, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(78, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(80, -2, logo_straight_LR, frame);
  sprites.drawOverwrite(84, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(100, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(114, -2, logo_elbow_LB, frame);

  sprites.drawOverwrite(-2, 14, logo_straight_TB, frame);
  sprites.drawOverwrite(-2, 24, logo_straight_TB_noflange, frame);
  sprites.drawOverwrite(-2, 17, logo_straight_TB, frame);
  sprites.drawOverwrite(-2, 36, logo_straight_TB, frame);

  sprites.drawOverwrite(114, 14, logo_straight_TB, frame);
  sprites.drawOverwrite(114, 24, logo_straight_TB_noflange, frame);
  sprites.drawOverwrite(114, 17, logo_straight_TB, frame);
  sprites.drawOverwrite(114, 36, logo_straight_TB, frame);

  sprites.drawOverwrite(-2, 52, logo_elbow_TR, frame);
  sprites.drawOverwrite(14, 52, logo_straight_LR, frame);
  sprites.drawOverwrite(30, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(46, 52, logo_straight_LR, frame);
  sprites.drawOverwrite(62, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(78, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(80, 52, logo_straight_LR, frame);
  sprites.drawOverwrite(84, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(100, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(114, 52, logo_elbow_TL, frame);


  if (arduboy.justPressed(UP_BUTTON) && levelSelect_selectedItem > 0) 					            { levelSelect_selectedItem--; }
  if (arduboy.justPressed(DOWN_BUTTON) && levelSelect_selectedItem < sizeof(levels) - 1) 	  { levelSelect_selectedItem++; }
  if (arduboy.justPressed(B_BUTTON)) 												                                { gameState = STATE_INTRO; }
  
  if (arduboy.justPressed(A_BUTTON)) { 
	  
	  puzzle.level = levels[levelSelect_selectedItem];
	  puzzle.index = 0;


    // If the player has completed some puzzles, give them the option to restart  
    // the level or puzzle.  Otherwise simply start the puzzle ..
    
    if (readEEPROM(puzzle.level) > 0) {
      
  	  gameState = STATE_PUZZLE_SELECT;
      prevState = STATE_LEVEL_SELECT;
      puzzleSelect_selectedItem = 0;
     
    }
    else {
      
      gameState = STATE_INIT_GAME;
      
    }
	  
  }
  
  
  // Adjust the top menu item based on selected item ..
  
  if (levelSelect_selectedItem < MENU_ITEM_3 - 1) {

	  levelSelect_topItem = 0;

  }
  else {

    if (levelSelect_selectedItem > sizeof(levels) - MENU_ITEM_3 + 1) {

      levelSelect_topItem = sizeof(levels) - MENU_ITEM_3;

  	}
  	else {
   
      levelSelect_topItem = levelSelect_selectedItem - 1;
          
  	}

  }
	
  
  // Render menu ..
  
  renderLevelDetail(MENU_ITEM_3_X, MENU_ITEM_3_1_Y, levels[levelSelect_topItem], (levelSelect_topItem == levelSelect_selectedItem));
  renderLevelDetail(MENU_ITEM_3_X, MENU_ITEM_3_2_Y, levels[levelSelect_topItem + 1], (levelSelect_topItem + 1 == levelSelect_selectedItem));
  renderLevelDetail(MENU_ITEM_3_X, MENU_ITEM_3_3_Y, levels[levelSelect_topItem + 2], (levelSelect_topItem + 2 == levelSelect_selectedItem));

}


/* ----------------------------------------------------------------------------
 *   Render a single level menu item.  
 */
void renderLevelDetail(byte x, byte y, byte level, byte highlight) {

  if (highlight) {
	  
  	arduboy.setTextColor(BLACK);
  	arduboy.setTextBackground(WHITE);
  	arduboy.fillRect(x - 1, y - 1, MENU_ITEM_3_WIDTH, MENU_ITEM_3_HEIGHT, WHITE);
	  
  }
	
  arduboy.setCursor(x, y);
  
  switch (level) {
  
  	case PUZZLE_5X5:
  		arduboy.print("Practice  ");
      arduboy.print(readEEPROM(PUZZLE_5X5) < 10 ? " " : "");
      arduboy.print(readEEPROM(PUZZLE_5X5));
  		arduboy.print("/");
  		arduboy.print(puzzles_5x5_count);
  		break;
  	  
  	case PUZZLE_6X6:
  		arduboy.print("Easy      ");
      arduboy.print(readEEPROM(PUZZLE_6X6) < 10 ? " " : "");
  		arduboy.print(readEEPROM(PUZZLE_6X6));
      arduboy.print("/");
  		arduboy.print(puzzles_6x6_count);
  		break;
  	  
  	case PUZZLE_7X7:
  		arduboy.print("Medium    ");
      arduboy.print(readEEPROM(PUZZLE_7X7) < 10 ? " " : "");
  		arduboy.print(readEEPROM(PUZZLE_7X7));
      arduboy.print("/");
  		arduboy.print(puzzles_7x7_count);
  		break;
  	  
  	case PUZZLE_8X8:
  		arduboy.print("Hard      ");
      arduboy.print(readEEPROM(PUZZLE_8X8) < 10 ? " " : "");
  		arduboy.print(readEEPROM(PUZZLE_8X8));
      arduboy.print("/");
  		arduboy.print(puzzles_8x8_count);
  		break;
  	  
  	case PUZZLE_9X9:
  		arduboy.print("Extreme   ");
      arduboy.print(readEEPROM(PUZZLE_9X9) < 10 ? " " : "");
  		arduboy.print(readEEPROM(PUZZLE_9X9));
  		arduboy.print("/");
  		arduboy.print(puzzles_9x9_count);
  		break;  		
  }
  
  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);
  
}


/* ----------------------------------------------------------------------------
 *   Handle the puzzle select menu.  
 *   
 *   Users can simply restart the level or continue playing.  If this screen
 *   was accessed via the 'level select' screen the "Continue Playing" message  
 *   is shown otherwise the "Restart Puzzle" message is displayed.
 */
void puzzleSelect() {

  clearHighlightAndSelection();

  arduboy.clear();
 
  sprites.drawOverwrite(-2, -2, logo_elbow_RB, frame);
  sprites.drawOverwrite(14, -2, logo_straight_LR, frame);
  sprites.drawOverwrite(30, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(46, -2, logo_straight_LR, frame);
  sprites.drawOverwrite(62, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(78, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(80, -2, logo_straight_LR, frame);
  sprites.drawOverwrite(84, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(100, -2, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(114, -2, logo_elbow_LB, frame);

  sprites.drawOverwrite(-2, 14, logo_straight_TB, frame);
  sprites.drawOverwrite(-2, 24, logo_straight_TB_noflange, frame);
  sprites.drawOverwrite(-2, 17, logo_straight_TB, frame);
  sprites.drawOverwrite(-2, 36, logo_straight_TB, frame);

  sprites.drawOverwrite(114, 14, logo_straight_TB, frame);
  sprites.drawOverwrite(114, 24, logo_straight_TB_noflange, frame);
  sprites.drawOverwrite(114, 17, logo_straight_TB, frame);
  sprites.drawOverwrite(114, 36, logo_straight_TB, frame);

  sprites.drawOverwrite(-2, 52, logo_elbow_TR, frame);
  sprites.drawOverwrite(14, 52, logo_straight_LR, frame);
  sprites.drawOverwrite(30, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(46, 52, logo_straight_LR, frame);
  sprites.drawOverwrite(62, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(78, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(80, 52, logo_straight_LR, frame);
  sprites.drawOverwrite(84, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(100, 52, logo_straight_LR_noflange, frame);
  sprites.drawOverwrite(114, 52, logo_elbow_TL, frame);
   
  if (arduboy.justPressed(UP_BUTTON) && puzzleSelect_selectedItem > 0)         { puzzleSelect_selectedItem--; }
  if (arduboy.justPressed(DOWN_BUTTON) && puzzleSelect_selectedItem < 1)       { puzzleSelect_selectedItem++; }
  if (arduboy.justPressed(B_BUTTON))                                           { gameState = STATE_LEVEL_SELECT; }
  
  if (arduboy.justPressed(A_BUTTON)) { 
	  
	  if (puzzleSelect_selectedItem == 1) {
		  
		  puzzle.index = 0;
      updateEEPROM(puzzle.level, puzzle.index);
           
	  }
	  
	  if (puzzleSelect_selectedItem == 0) {


      // If all puzzles in the current level are completed, simply re-show the last puzzle ..
      
		  puzzle.index = (readEEPROM(puzzle.level) == getNumberOfPuzzles(puzzle.level) ? readEEPROM(puzzle.level) -1 : readEEPROM(puzzle.level));
		  
	  }
	  
	  gameState = STATE_INIT_GAME;
	  
  }
  
	
  
  // Render menu ..
  
  renderPuzzleOption(MENU_ITEM_2_X, MENU_ITEM_2_1_Y, (prevState == STATE_LEVEL_SELECT ? "Continue Playing" : " Restart Puzzle "), (puzzleSelect_selectedItem == 0));
  renderPuzzleOption(MENU_ITEM_2_X, MENU_ITEM_2_2_Y, "  Reset Level", (puzzleSelect_selectedItem == 1));
    
}


/* ----------------------------------------------------------------------------
 *   Render a single puzzle menu item.  
 */
void renderPuzzleOption(byte x, byte y, String message, byte highlight) {

  if (highlight) {
	  
	  arduboy.setTextColor(BLACK);
	  arduboy.setTextBackground(WHITE);
	  arduboy.fillRect(x - 1, y - 2, MENU_ITEM_2_WIDTH, MENU_ITEM_2_HEIGHT, WHITE);
	  
  }
	
  arduboy.setCursor(x, y);
  arduboy.print(message);
  
  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);
  
}

/* ----------------------------------------------------------------------------
 *   Initialise the game based on the selected level and puzzle numbers.
 */
void play_InitGame() {

  arduboy.clear();
  initBoard(puzzle.level, puzzle.index);
  gameState = STATE_NO_SELECTION;
  
}


/* ----------------------------------------------------------------------------
 *   Handle the game play before the user selects a node.
 *   
 *   The user can scroll to any node on the board using the directional arrows.
 *   Clicking the A button on a node selects it.  Clicking the B button returns
 *   the user to the level select (if currently on the first puzzle) or the 
 *   puzzle select screen (if they are currently working on a puzzle).
 */
void play_NoSelection() {

  if (arduboy.justPressed(LEFT_BUTTON) && player.highlightedNode.x > 0)                         { player.highlightedNode.x--; }
  if (arduboy.justPressed(RIGHT_BUTTON) && player.highlightedNode.x < puzzle.maximum.x - 1)     { player.highlightedNode.x++; }
  if (arduboy.justPressed(UP_BUTTON) && player.highlightedNode.y > 0)                           { player.highlightedNode.y--; }
  if (arduboy.justPressed(DOWN_BUTTON) && player.highlightedNode.y < puzzle.maximum.y - 1)      { player.highlightedNode.y++; }

  if (arduboy.justPressed(A_BUTTON) && isNode(player.highlightedNode.x, player.highlightedNode.y)) {

    if (nodeAlreadyPlayed(getNodeValue(player.highlightedNode.x, player.highlightedNode.y))) {

      clearBoard(getNodeValue(player.highlightedNode.x, player.highlightedNode.y));
      player.selectedNode.value = getNodeValue(player.highlightedNode.x, player.highlightedNode.y);
      player.selectedNode.x = player.highlightedNode.x;
      player.selectedNode.y = player.highlightedNode.y;
      gameState = STATE_NODE_SELECTED;

    }
    else {

      player.selectedNode.value = getNodeValue(player.highlightedNode.x, player.highlightedNode.y);
      player.selectedNode.x = player.highlightedNode.x;
      player.selectedNode.y = player.highlightedNode.y;
      gameState = STATE_NODE_SELECTED;
      playSelectNodeTune();

    }

  }

  if (arduboy.justPressed(B_BUTTON)) {

    if (puzzle.index == 0) {
        
      gameState = STATE_LEVEL_SELECT;
      prevState = STATE_NO_SELECTION;

    }
    else {
        
      gameState = STATE_PUZZLE_SELECT;
      prevState = STATE_NO_SELECTION;
        
    }
      
  }
  
  renderBoard(puzzle.offset.x, puzzle.offset.y - calculateTopRow() * GRID_HEIGHT, calculateTopRow());
  
}


/* ----------------------------------------------------------------------------
 *   Calculate the top row of the puzzle to render.
 *   
 *   Games greater than 5 x 5 cannot be rendered on the screen completely.  The
 *   top row to display is calculated based on the player's highlighted cell.
 */
byte calculateTopRow() {

  byte topRow = 0;

  if (player.highlightedNode.y <= 2) {

    topRow = 0;

  }
  else {

    if (player.highlightedNode.y>=3 && player.highlightedNode.y <= puzzle.maximum.y - 4) { 

      topRow = player.highlightedNode.y - 2; 

    }
    else {
    
      topRow = (puzzle.maximum.y - 5 >= 0 ? puzzle.maximum.y - 5 : 0); 

    }

  }

  return topRow;
  
}


/* ----------------------------------------------------------------------------
 *   Handle the game play after the user selects a node.
 *   
 *   The user can only navigate through empty cells or onto the node that matches
 *   the one previously selected.  They can also back-track along the pipe they
 *   have already layed.
 *   
 *   Pressing the B button will clear the pipe that is currently being layed.
 */
void play_NodeSelected() {

  if (arduboy.justPressed(LEFT_BUTTON)) {
    
    if (validMove(LEFT, player.selectedNode, player.highlightedNode.x - 1, player.highlightedNode.y)) {

      switch (getPipeValue(player.highlightedNode.x, player.highlightedNode.y)) {

        case PIPE_HORIZONTAL_LR:
        case PIPE_CORNER_TR:
        case PIPE_CORNER_BR:
          updatePipeWhenReversing(player.highlightedNode.x - 1, player.highlightedNode.y);
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, NOTHING, NOTHING);
          break;

        case PIPE_HORIZONTAL_RL:
        case PIPE_CORNER_TL:
        case PIPE_CORNER_BL:
          if (!isNode(player.highlightedNode.x - 1, player.highlightedNode.y)) { setCellValue(player.highlightedNode.x - 1, player.highlightedNode.y, PIPE_HORIZONTAL_RL, player.selectedNode.value); }
          break;

        case PIPE_CORNER_LT:
        case PIPE_CORNER_RT:
        case PIPE_VERTICAL_BT:
          if (!isNode(player.highlightedNode.x - 1, player.highlightedNode.y)) { setCellValue(player.highlightedNode.x - 1, player.highlightedNode.y, PIPE_HORIZONTAL_RL, player.selectedNode.value); }
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, PIPE_CORNER_BL, player.selectedNode.value);
          break;

        case PIPE_CORNER_LB:
        case PIPE_CORNER_RB:
        case PIPE_VERTICAL_TB:
          if (!isNode(player.highlightedNode.x - 1, player.highlightedNode.y)) { setCellValue(player.highlightedNode.x - 1, player.highlightedNode.y, PIPE_HORIZONTAL_RL, player.selectedNode.value); }
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, PIPE_CORNER_TL, player.selectedNode.value);
          break;

        case NODE:
          setCellValue(player.highlightedNode.x - 1, player.highlightedNode.y, PIPE_HORIZONTAL_RL, player.selectedNode.value);
          break;

      }
  
  
      // Have we selected the original node?  If so clear the board of this pipe ..
      
      if (player.highlightedNode.x - 1 == player.selectedNode.x && player.highlightedNode.y == player.selectedNode.y) {
    
        clearBoard(player.selectedNode.value);
        clearSelection();
        gameState = STATE_NO_SELECTION;
        playClearSelectionTune();
          
      }
  
  
      // Have we selected a pair node?  
      
      if (isNode(player.highlightedNode.x - 1, player.highlightedNode.y)  && getNodeValue(player.highlightedNode.x - 1, player.highlightedNode.y) == player.selectedNode.value) {
    
        clearSelection();
        gameState = STATE_NO_SELECTION;
        playMatchNodeTune();
 

        // Is the level finished ?

        if (isPuzzleComplete()) {

          gameState = STATE_GAME_OVER;
    
        }
                 
      }  
      
      player.highlightedNode.x--;
      
    }
    
  }

  if (arduboy.justPressed(RIGHT_BUTTON)) {
    
    if (validMove(RIGHT, player.selectedNode, player.highlightedNode.x + 1, player.highlightedNode.y)) {

      switch (getPipeValue(player.highlightedNode.x, player.highlightedNode.y)) {

        case PIPE_HORIZONTAL_RL:
        case PIPE_CORNER_TL:
        case PIPE_CORNER_BL:
          updatePipeWhenReversing(player.highlightedNode.x + 1, player.highlightedNode.y);
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, NOTHING, NOTHING);
          break;

        case PIPE_HORIZONTAL_LR:
        case PIPE_CORNER_TR:
        case PIPE_CORNER_BR:
          if (!isNode(player.highlightedNode.x + 1, player.highlightedNode.y)) { setCellValue(player.highlightedNode.x + 1, player.highlightedNode.y, PIPE_HORIZONTAL_LR, player.selectedNode.value); }
          break;

        case PIPE_CORNER_RT:
        case PIPE_CORNER_LT:
        case PIPE_VERTICAL_BT:
          if (!isNode(player.highlightedNode.x + 1, player.highlightedNode.y)) { setCellValue(player.highlightedNode.x + 1, player.highlightedNode.y, PIPE_HORIZONTAL_LR, player.selectedNode.value); }
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, PIPE_CORNER_BR, player.selectedNode.value);
          break;

        case PIPE_CORNER_RB:
        case PIPE_CORNER_LB:
        case PIPE_VERTICAL_TB:
          if (!isNode(player.highlightedNode.x + 1, player.highlightedNode.y)) { setCellValue(player.highlightedNode.x + 1, player.highlightedNode.y, PIPE_HORIZONTAL_LR, player.selectedNode.value); }
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, PIPE_CORNER_TR, player.selectedNode.value);
          break;

        case NODE:
          setCellValue(player.highlightedNode.x + 1, player.highlightedNode.y, PIPE_HORIZONTAL_LR, player.selectedNode.value);
          break;

      }
  
  
      // Have we selected the original node?  If so clear the board of this pipe ..
      
      if (player.highlightedNode.x + 1 == player.selectedNode.x && player.highlightedNode.y == player.selectedNode.y) {
    
        clearBoard(player.selectedNode.value);
        clearSelection();
        gameState = STATE_NO_SELECTION;
        playClearSelectionTune();
          
      }
  
  
      // Have we selected a pair node?  
      
      if (isNode(player.highlightedNode.x + 1, player.highlightedNode.y)  && getNodeValue(player.highlightedNode.x + 1, player.highlightedNode.y) == player.selectedNode.value) {
    
        clearSelection();
        gameState = STATE_NO_SELECTION;
        playMatchNodeTune();
  

        // Is the level finished ?

        if (isPuzzleComplete()) {

          gameState = STATE_GAME_OVER;
    
        }
                
      }
    
      player.highlightedNode.x++;
  
    }

  }

  if (arduboy.justPressed(UP_BUTTON)) {
    
    if (validMove(UP, player.selectedNode, player.highlightedNode.x, player.highlightedNode.y - 1)) {

      switch (getPipeValue(player.highlightedNode.x, player.highlightedNode.y)) {

        case PIPE_VERTICAL_TB:
        case PIPE_CORNER_RB:
        case PIPE_CORNER_LB:
          updatePipeWhenReversing(player.highlightedNode.x, player.highlightedNode.y - 1);
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, NOTHING, NOTHING);
          break;

        case PIPE_VERTICAL_BT:
        case PIPE_CORNER_LT:
        case PIPE_CORNER_RT:
          if (!isNode(player.highlightedNode.x, player.highlightedNode.y - 1)) { setCellValue(player.highlightedNode.x, player.highlightedNode.y - 1, PIPE_VERTICAL_BT, player.selectedNode.value); }
          break;

        case PIPE_CORNER_TR:
        case PIPE_CORNER_BR:
        case PIPE_HORIZONTAL_LR:
          if (!isNode(player.highlightedNode.x, player.highlightedNode.y - 1)) { setCellValue(player.highlightedNode.x, player.highlightedNode.y - 1, PIPE_VERTICAL_BT, player.selectedNode.value); }
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, PIPE_CORNER_LT, player.selectedNode.value); 
          break;

        case PIPE_CORNER_TL:
        case PIPE_CORNER_BL:
        case PIPE_HORIZONTAL_RL:
          if (!isNode(player.highlightedNode.x, player.highlightedNode.y - 1)) { setCellValue(player.highlightedNode.x, player.highlightedNode.y - 1, PIPE_VERTICAL_BT, player.selectedNode.value); }
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, PIPE_CORNER_RT, player.selectedNode.value);
          break;

        case NODE:
          setCellValue(player.highlightedNode.x, player.highlightedNode.y - 1, PIPE_VERTICAL_BT, player.selectedNode.value);
          break;

      }
  
  
      // Have we selected the original node?  If so clear the board of this pipe ..
      
      if (player.highlightedNode.x == player.selectedNode.x && player.highlightedNode.y - 1 == player.selectedNode.y) {
    
        clearBoard(player.selectedNode.value);
        clearSelection();
        gameState = STATE_NO_SELECTION;
        playClearSelectionTune();
         
      }
  
  
      // Have we selected a pair node?  
      
      if (isNode(player.highlightedNode.x, player.highlightedNode.y - 1)  && getNodeValue(player.highlightedNode.x, player.highlightedNode.y - 1) == player.selectedNode.value) {
    
        clearSelection();
        gameState = STATE_NO_SELECTION;
        playMatchNodeTune();
 

        // Is the level finished ?

        if (isPuzzleComplete()) {

          gameState = STATE_GAME_OVER;
    
        }
                 
      }  
    
      player.highlightedNode.y--;
    }

  }

  if (arduboy.justPressed(DOWN_BUTTON)) {
    
    if (validMove(DOWN, player.selectedNode, player.highlightedNode.x, player.highlightedNode.y + 1)) {

      switch (getPipeValue(player.highlightedNode.x, player.highlightedNode.y)) {

        case PIPE_VERTICAL_BT:
        case PIPE_CORNER_RT:
        case PIPE_CORNER_LT:
          updatePipeWhenReversing(player.highlightedNode.x, player.highlightedNode.y + 1);
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, NOTHING, NOTHING);
          break;

        case PIPE_VERTICAL_TB:
        case PIPE_CORNER_LB:
        case PIPE_CORNER_RB:
          if (!isNode(player.highlightedNode.x, player.highlightedNode.y + 1)) { setCellValue(player.highlightedNode.x, player.highlightedNode.y + 1, PIPE_VERTICAL_TB, player.selectedNode.value); }
          break;

        case PIPE_CORNER_TR:
        case PIPE_CORNER_BR:
        case PIPE_HORIZONTAL_LR:
          if (!isNode(player.highlightedNode.x, player.highlightedNode.y + 1)) { setCellValue(player.highlightedNode.x, player.highlightedNode.y + 1, PIPE_VERTICAL_TB, player.selectedNode.value); }
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, PIPE_CORNER_LB, player.selectedNode.value);
          break;

        case PIPE_CORNER_TL:
        case PIPE_CORNER_BL:
        case PIPE_HORIZONTAL_RL:
          if (!isNode(player.highlightedNode.x, player.highlightedNode.y + 1)) { setCellValue(player.highlightedNode.x, player.highlightedNode.y + 1, PIPE_VERTICAL_TB, player.selectedNode.value); }
          setCellValue(player.highlightedNode.x, player.highlightedNode.y, PIPE_CORNER_RB, player.selectedNode.value);
          break;

        case NODE:
          setCellValue(player.highlightedNode.x, player.highlightedNode.y + 1, PIPE_VERTICAL_TB, player.selectedNode.value);
          break;
        
      }

  
      // Have we selected the original node?  If so clear the board of this pipe ..
      
      if (player.highlightedNode.x == player.selectedNode.x && player.highlightedNode.y + 1 == player.selectedNode.y) {
    
        clearBoard(player.selectedNode.value);
        clearSelection();
        gameState = STATE_NO_SELECTION;
        playClearSelectionTune();
          
      }
   
  
      // Have we selected a pair node?  
      
      if (isNode(player.highlightedNode.x, player.highlightedNode.y + 1)  && getNodeValue(player.highlightedNode.x, player.highlightedNode.y + 1) == player.selectedNode.value) {
    
        clearSelection();
        gameState = STATE_NO_SELECTION;
        playMatchNodeTune();


        // Is the level finished ?

        if (isPuzzleComplete()) {

          gameState = STATE_GAME_OVER;
    
        }
  
      }  
   
      player.highlightedNode.y++;

    }

  }

  if (arduboy.justPressed(B_BUTTON)) {

    clearBoard(player.selectedNode.value);
    clearSelection();
    gameState = STATE_NO_SELECTION;
      
  }


  renderBoard(puzzle.offset.x, puzzle.offset.y - calculateTopRow() * GRID_HEIGHT, calculateTopRow());
  
}


/* ----------------------------------------------------------------------------
 *   Fix the pipe graphics when reversing back on yourself.
 */
void updatePipeWhenReversing(byte x, byte y) {

  switch (getPipeValue(x, y)) {

    case PIPE_CORNER_TR:           
    case PIPE_CORNER_TL:                   
    case PIPE_VERTICAL_TB:                 
      setCellValue(x, y, PIPE_VERTICAL_TB, player.selectedNode.value);
      break;

    case PIPE_CORNER_BR:          
    case PIPE_CORNER_BL:          
    case PIPE_VERTICAL_BT:        
      setCellValue(x, y, PIPE_VERTICAL_BT, player.selectedNode.value);
      break;

    case PIPE_CORNER_LT:
    case PIPE_CORNER_LB:
    case PIPE_HORIZONTAL_LR:
      setCellValue(x, y, PIPE_HORIZONTAL_LR, player.selectedNode.value);
      break;
           
    case PIPE_CORNER_RT:                   
    case PIPE_CORNER_RB: 
    case PIPE_HORIZONTAL_RL:        
      setCellValue(x, y, PIPE_HORIZONTAL_RL, player.selectedNode.value);
      break;
 
  }
 
}


/* ----------------------------------------------------------------------------
 *   Is the nominated move valid?
 *   
 *   The user can only navigate through empty cells or onto the node that matches
 *   the one previously selected.  They can also back-track along the pipe they
 *   have already layed.
 *   
 *   direction:     The button the user clicked, eg. UP, DOWN, LEFT and Right.
 *   selectedNode:  The node previously selected.
 *   x and y:       Coordinates to test.  Passed as char to allow negatives.
 */
bool validMove(byte direction, Node selectedNode, int x, int y) {

  
  // Off the grid!

  if (x < 0 || x >= puzzle.maximum.x || y < 0 || y >= puzzle.maximum.y) return false;
  
  
  // Is it a clear cell or the matching node?
  
  if (
      (!isNode(x,y) && getPipeValue(x,y) == NOTHING) ||
      (isNode(x,y) && getNodeValue(x,y) == selectedNode.value && (x != selectedNode.x || y != selectedNode.y))
     ) return true;
  

  // Is the pipe turning back on itself?

  switch (direction) {

    case (UP):
    
      switch (getPipeValue(player.highlightedNode.x, player.highlightedNode.y)) {

        case PIPE_VERTICAL_TB:
        case PIPE_CORNER_RB:
        case PIPE_CORNER_LB:
          return true;

      }

      break;
      
    case (DOWN):
    
      switch (getPipeValue(player.highlightedNode.x, player.highlightedNode.y)) {
          
        case PIPE_VERTICAL_BT:
        case PIPE_CORNER_LT:
        case PIPE_CORNER_RT:
          return true;

      }

      break;

    case (LEFT):
    
      switch (getPipeValue(player.highlightedNode.x, player.highlightedNode.y)) {
   
        case PIPE_CORNER_TR:
        case PIPE_CORNER_BR:
        case PIPE_HORIZONTAL_LR:
          return true;

      }

      break;

    case (RIGHT):
    
      switch (getPipeValue(player.highlightedNode.x, player.highlightedNode.y)) {

        case PIPE_CORNER_TL:
        case PIPE_CORNER_BL:
        case PIPE_HORIZONTAL_RL:
          return true;

      }

      break;

  }
  
  return false;
  
}

#define SCROLLBAR_X          119
#define SCROLLBAR_Y          0
#define SCROLLBAR_WIDTH      9
#define SCROLLBAR_HEIGHT     64

/* ----------------------------------------------------------------------------
 *   Render the board.
 */
void renderBoard(int xOffset, int yOffset, byte topRow) { 

  arduboy.clear();


  // Draw stripey background ..
  
  for (int y = 0; y < 256; y+=3) {
    arduboy.drawLine(y, 0, 0, y, WHITE);
  }
  

  // Clear background of actual board ..
  
  arduboy.fillRect(xOffset - 3, yOffset - 3 , (puzzle.maximum.x * GRID_WIDTH) + 7, (puzzle.maximum.y * GRID_HEIGHT) + 7, BLACK);
  arduboy.drawRect(xOffset - 2, yOffset - 2 , (puzzle.maximum.x * GRID_WIDTH) + 5, (puzzle.maximum.y * GRID_HEIGHT) + 5, WHITE);


  // Draw placed pipes ..
  
  for (int y = 0; y < puzzle.maximum.y; y++) {
      
    for (int x = 0; x < puzzle.maximum.x; x++) {
      
      if (isPipe(x,y)) {
        
        sprites.drawExternalMask((x * GRID_WIDTH) + xOffset, (y * GRID_HEIGHT) + yOffset, pipes[getPipeValue(x, y)], pipe_mask, frame, frame);

      }
        
    }
      
  }



  // Draw placed pipes ..
  
  for (int y = 0; y < puzzle.maximum.y; y++) {
      
    for (int x = 0; x < puzzle.maximum.x; x++) {
      
      if (isNode(x, y)) {
        
        sprites.drawOverwrite((x * GRID_WIDTH + 2) + xOffset, (y * GRID_HEIGHT + 2) + yOffset, nodes[getNodeValue(x,y)], frame);
        
      }
        
    }
      
  }

  
  // Draw grid marks ..

  for (int y = 0; y <= puzzle.maximum.y; y++) {
    for (int x = 0; x <= puzzle.maximum.x; x++) {
      arduboy.drawPixel((x * GRID_WIDTH) + xOffset, (y * GRID_HEIGHT) + yOffset, WHITE);
    }
  }

  
  // Draw selected cell ..
  
  arduboy.drawRect((player.highlightedNode.x * GRID_WIDTH) + xOffset, (player.highlightedNode.y * GRID_HEIGHT) + yOffset, GRID_WIDTH + 1, GRID_HEIGHT + 1, WHITE);


  // Draw scrollbar if one is needed ..

  if (puzzle.slider.unit > 0) {
    
    arduboy.fillRect(SCROLLBAR_X - 1, SCROLLBAR_Y, SCROLLBAR_WIDTH + 1, SCROLLBAR_HEIGHT, BLACK);
    arduboy.drawRect(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, WHITE);
    arduboy.fillRect(SCROLLBAR_X + 2, SCROLLBAR_Y + 6 + (topRow * puzzle.slider.unit), SCROLLBAR_WIDTH - 4, puzzle.slider.overall, WHITE);


    // Top arrow ..

    arduboy.fillTriangle(SCROLLBAR_X + 4, SCROLLBAR_Y + 2, SCROLLBAR_X + 2, SCROLLBAR_Y + 4, SCROLLBAR_X + 6, SCROLLBAR_Y + 4, WHITE);


    // Bottom arrow ..
    
    arduboy.fillTriangle(SCROLLBAR_X + 2, SCROLLBAR_Y + SCROLLBAR_HEIGHT - 5, SCROLLBAR_X + 6, SCROLLBAR_Y + SCROLLBAR_HEIGHT - 5, SCROLLBAR_X + 4, SCROLLBAR_Y + SCROLLBAR_HEIGHT - 3, WHITE);

  }
    
}

const uint16_t puzzleSolved[] PROGMEM = {
  NOTE_C4,50, NOTE_D4,50, NOTE_E4,50, 
  NOTE_C4,50, NOTE_D4,50, NOTE_E4,50, 
  NOTE_C4,50, NOTE_D4,50, NOTE_E4,50, 
  TONES_END
};

/* ----------------------------------------------------------------------------
 *   Play the 'Clear Selection' tune.
 */
void playClearSelectionTune() {
  
  sound.tone(NOTE_C2,50, NOTE_D2,50, NOTE_E2,50);
  
}


/* ----------------------------------------------------------------------------
 *   Play the 'Select Node' tune.
 */
void playSelectNodeTune() {
  
  sound.tone(NOTE_C4, 50);

}


/* ----------------------------------------------------------------------------
 *   Play the 'Match Node' tune.
 */
void playMatchNodeTune() {

  sound.tone(NOTE_C4,50, NOTE_D4,50, NOTE_E4,50);

}


/* ----------------------------------------------------------------------------
 *   Play the 'Splash' tune.
 */
void playSplashTune() {
  
  sound.tone(NOTE_C1,50, NOTE_D1,50, NOTE_C1,50);
 
}

/* ----------------------------------------------------------------------------
 *   Play the 'Puzzle Solved' tune.
 */
void playPuzzleSolved() {

  sound.tones(puzzleSolved);
  
}


#define ANIMATION_DELAY_SHORT     20


/* ----------------------------------------------------------------------------
 *   Render the Splash screen.
 *   
 *   The pipes are rendered one-by-one to simulate the laying of pipe.  The
 *   rendering function allows the user to press the A button to skip past the
 *   animations or the B button to toggle sounds.
 */
void drawSplash() { 

  bool skipSplash = false;
  
  arduboy.clear(); 
  sprites.drawOverwrite(120, 56, sound_icon, (arduboy.audio.enabled() ? 0 : 1));
  arduboy.display();
    
  skipSplash = splashAnimation(112,  0, logo_straight_TB_noflange, frame, skipSplash);
  skipSplash = splashAnimation(112, 16, logo_elbow_TL, frame, skipSplash);
  skipSplash = splashAnimation(96,  16, logo_elbow_TR, frame, skipSplash);
  skipSplash = splashAnimation(96,   0, logo_elbow_LB, frame, skipSplash);
  skipSplash = splashAnimation(80,   0, logo_straight_LR_noflange, frame, skipSplash);
  skipSplash = splashAnimation(64,   0, logo_straight_LR, frame, skipSplash);
  skipSplash = splashAnimation(48,   0, logo_elbow_RB, frame, skipSplash);
  skipSplash = splashAnimation(48,  16, logo_straight_TB_noflange, frame, skipSplash);
  skipSplash = splashAnimation(48,  32, logo_elbow_TR, frame, skipSplash);
  skipSplash = splashAnimation(64,  32, logo_elbow_LB, frame, skipSplash);
  skipSplash = splashAnimation(64,  48, logo_straight_TB_noflange, frame, skipSplash);

  skipSplash = splashDelay(200, skipSplash);

  skipSplash = splashAnimation(0,    0, logo_straight_TB_noflange, frame, skipSplash);
  skipSplash = splashAnimation(0,   16, logo_straight_TB, frame, skipSplash);
  skipSplash = splashAnimation(0,   32, logo_elbow_TR, frame, skipSplash);
  skipSplash = splashAnimation(16,  32, logo_straight_LR, frame, skipSplash);
  skipSplash = splashAnimation(32,  32, logo_elbow_TL, frame, skipSplash);
  skipSplash = splashAnimation(32,  16, logo_elbow_RB, frame, skipSplash);
  skipSplash = splashAnimation(48,  16, logo_cross_underlap_hflange, frame, skipSplash);
  skipSplash = splashAnimation(64,  16, logo_straight_LR_noflange, frame, skipSplash);
  skipSplash = splashAnimation(80,  16, logo_elbow_LB, frame, skipSplash);
  skipSplash = splashAnimation(80,  32, logo_elbow_TR, frame, skipSplash);
  skipSplash = splashAnimation(96,  32, logo_straight_LR, frame, skipSplash);
  skipSplash = splashAnimation(112, 32, logo_straight_LR_noflange, frame, skipSplash);

  skipSplash = splashDelay(200, skipSplash);

  skipSplash = splashAnimation(0,    0, logo_cross_underlap, frame, skipSplash);
  skipSplash = splashAnimation(16,   0, logo_elbow_LB, frame, skipSplash);
  skipSplash = splashAnimation(16,  16, logo_straight_TB, frame, skipSplash);
  skipSplash = splashAnimation(16,  32, logo_cross_overlap_hflange, frame, skipSplash);
  skipSplash = splashAnimation(16,  48, logo_elbow_TR, frame, skipSplash);
  skipSplash = splashAnimation(32,  48, logo_straight_LR, frame, skipSplash);
  skipSplash = splashAnimation(48,  48, logo_straight_LR_noflange, frame, skipSplash);
  skipSplash = splashAnimation(64,  48, logo_cross_underlap_hflange, frame, skipSplash);
  skipSplash = splashAnimation(80,  48, logo_straight_LR_noflange, frame, skipSplash);
  skipSplash = splashAnimation(96,  48, logo_elbow_LB, frame, skipSplash);

  skipSplash = splashDelay(200, skipSplash);

  if (!skipSplash) {

    arduboy.fillRect(19, 18, 92, 30, BLACK);
    drawHorizontalDottedLine(20, 110, 19);
    drawHorizontalDottedLine(20, 110, 46);
    sprites.drawOverwrite(23, 21, logo, frame);
    arduboy.display();

    splashWaitForever();
    
  }

  gameState = STATE_LEVEL_SELECT;
  
}


/* ----------------------------------------------------------------------------
 *   Render the a single pipe animation.
 *   
 *   The pipes are rendered one-by-one to simulate the laying of pipe.  The
 *   rendering function allows the user to press the A button to skip past the
 *   animations or the B button to toggle sounds.
 */
bool splashAnimation(byte x, byte y, const uint8_t *bitmap, uint8_t frame, bool skip) {

  if (skip) return true;
  
  int i = ANIMATION_DELAY_SHORT;
  
  sprites.drawOverwrite(x, y, bitmap, frame);
  arduboy.display();
  
  playSplashTune();

  while (i >= 0) {

    arduboy.pollButtons();

    if (arduboy.justPressed(A_BUTTON)) { return true; }

    if (arduboy.justPressed(B_BUTTON)) { 

    	toggleSoundSettings();
      sprites.drawOverwrite(120, 56, sound_icon, (arduboy.audio.enabled() ? 0 : 1));
      arduboy.display();
    	
    }
    
    arduboy.delayShort(15);  
    i--;

  }

  return false;
  
}


/* ----------------------------------------------------------------------------
 *   Delay the screen for a period of time.  
 *   
 *   The delay can be cancelled by clicking either the A button.  Clicking the
 *   B button will toggle the sound on or off.
 */
bool splashDelay(int delayLength, bool skip) {

  if (skip) return true;


  while (delayLength >= 0) {

    arduboy.pollButtons();

    if (arduboy.justPressed(A_BUTTON)) { return true; }

    if (arduboy.justPressed(B_BUTTON)) { 

    	toggleSoundSettings();
      sprites.drawOverwrite(120, 56, sound_icon, (arduboy.audio.enabled() ? 0 : 1));
      arduboy.display();
        
    }
    
    arduboy.delayShort(5);  
    delayLength--;

  }

  return false;
  
}


/* ----------------------------------------------------------------------------
 *   Delay the screen forever.  
 *   
 *   The delay can be cancelled by clicking either the A button.  Clicking the
 *   B button will toggle the sound on or off.
 */
void splashWaitForever() {

  while (true) {

    arduboy.pollButtons();

    if (arduboy.justPressed(A_BUTTON)) { break; }

    if (arduboy.justPressed(B_BUTTON)) { 

      toggleSoundSettings();
      sprites.drawOverwrite(120, 56, sound_icon, (arduboy.audio.enabled() ? 0 : 1));
      arduboy.display();
       
    }
    
    arduboy.delayShort(5);  

  }
  
}

/* ----------------------------------------------------------------------------
 *  Return the upper 4 bits of a byte.
 */
byte leftValue(byte val) {

  return val >> 4; 
      
}


/* ----------------------------------------------------------------------------
 *  Return the lower 4 bits of a byte.
 */
byte rightValue(byte val) {

  return val & 0x0F; 
      
}


/* ----------------------------------------------------------------------------
 *  Initialise the board.
 */
void initBoard(byte puzzleType, byte puzzleNumber) {

  byte x = 0;
  byte y = 0;
  byte z = 0;
  
  byte bytesToRead = (puzzleType % 2 == 0 ? (puzzleType / 2) * puzzleType : ((puzzleType + 1) / 2) * puzzleType);

  puzzle.maximum.x = puzzle.maximum.y = puzzleType;
  puzzle.offset.x = pgm_read_byte(&puzzles_details[(puzzleType - 5) * 4]);
  puzzle.offset.y = pgm_read_byte(&puzzles_details[((puzzleType - 5) * 4) + 1]);
  puzzle.slider.unit = pgm_read_byte(&puzzles_details[((puzzleType - 5) * 4) + 2]);
  puzzle.slider.overall = pgm_read_byte(&puzzles_details[((puzzleType - 5) * 4) + 3]);

  for (int i = (puzzleNumber * bytesToRead); i < ((puzzleNumber + 1) * bytesToRead); i++) {

  	switch (puzzleType) {
      
  	  case PUZZLE_5X5:
  		  z = pgm_read_byte(&puzzles_5x5[i]);
  		  break;
        
      case PUZZLE_6X6:
        z = pgm_read_byte(&puzzles_6x6[i]);
        break;
        
      case PUZZLE_7X7:
        z = pgm_read_byte(&puzzles_7x7[i]);
        break;
        
      case PUZZLE_8X8:
        z = pgm_read_byte(&puzzles_8x8[i]);
        break;
  		  
  	  case PUZZLE_9X9:
  		  z = pgm_read_byte(&puzzles_9x9[i]);
  		  break;
  
  	}

    puzzle.board[y][x] = 0;
	  
	  if ((x <= puzzle.maximum.x) && leftValue(z) > 0) {
		  
		  puzzle.board[y][x] = 0xF0 | leftValue(z);
		  
	  }
	  
	  x++;

    puzzle.board[y][x] = 0;
	  
	  if ((x <= puzzle.maximum.x) && rightValue(z) > 0) {
		  
		  puzzle.board[y][x] = 0xF0 | rightValue(z);
		  
	  }
	  
	  x++;
	  
	  if (x >= puzzle.maximum.x) { y++; x = 0; }
		  
  }

}


/* ----------------------------------------------------------------------------
 *  Has the node already been played?
 */
bool nodeAlreadyPlayed(byte value) { 

  for (byte y = 0; y < puzzle.maximum.y; y++) {
    
    for (byte x = 0; x < puzzle.maximum.x; x++) {
      
      if (getNodeValue(x, y) == value && !isNode(x, y)) {
        
        return true;
        
      }
      
    }
      
  }
    
  return false;
    
}


/* ----------------------------------------------------------------------------
 *  Clear the board of pipes for a certain node value.
 */
void clearBoard(byte nodeValue) {

  for (byte y = 0; y < puzzle.maximum.y; y++) {
    
    for (byte x = 0; x < puzzle.maximum.x; x++) {
      
      if (getNodeValue(x, y) == nodeValue && !isNode(x, y)) {
        
        puzzle.board[y][x] = NOTHING;
        
      }
      
    }
    
  }
  
}


/* ----------------------------------------------------------------------------
 *  Is the position nominated a node?
 */
bool isNode(byte x, byte y) {

  return (puzzle.board[y][x] & 0xF0) == 0xF0;
  
}


/* ----------------------------------------------------------------------------
 *  Get the node value for the position.
 */
byte getNodeValue(byte x, byte y) {
  
  return (puzzle.board[y][x] & 0x0F);

}


/* ----------------------------------------------------------------------------
 *  Is the position nominated a pipe?
 */
bool isPipe(byte x, byte y) {

  return (puzzle.board[y][x] & 0xF0) > 0x00 && (puzzle.board[y][x] & 0xF0) != 0xF0;
  
}


/* ----------------------------------------------------------------------------
 *  Get the pipe value for the position.
 */
byte getPipeValue(byte x, byte y) {
  
  return (puzzle.board[y][x] & 0xF0) >> 4;

}


/* ----------------------------------------------------------------------------
 *  Set the pipe value for the position.
 */
void setCellValue(byte x, byte y, byte pipeValue, byte nodeValue) {
  
  puzzle.board[y][x] = (pipeValue << 4) | nodeValue;
  
}


/* ----------------------------------------------------------------------------
 *  Clear the player's selection.
 */
void clearSelection() {

  player.selectedNode.value = 0;
  player.selectedNode.x = 0;
  player.selectedNode.y = 0;

}


/* ----------------------------------------------------------------------------
 *  Clear the player's highlight and selection.
 */
void clearHighlightAndSelection() {

  player.selectedNode.value = 0;
  player.selectedNode.x = 0;
  player.selectedNode.y = 0;
  player.highlightedNode.x = 0;
  player.highlightedNode.y = 0;

}


/* ----------------------------------------------------------------------------
 *  Is the puzzle complete?  All cells should have a 'node' value.
 */
bool isPuzzleComplete() {

  for (byte y = 0; y < puzzle.maximum.y; y++) {
    
    for (byte x = 0; x < puzzle.maximum.x; x++) {
      
      if (getNodeValue(x, y) == 0) {
        
        return false;
        
      }
      
    }
    
  }

  return true;
  
}


/* ----------------------------------------------------------------------------
 *  Return the number of puzzles for a nominated level.
 */
byte getNumberOfPuzzles(byte puzzleLevel) {

  switch (puzzleLevel) {
    
    case PUZZLE_5X5:
      return puzzles_5x5_count;
    
    case PUZZLE_6X6:
      return puzzles_6x6_count;
    
    case PUZZLE_7X7:
      return puzzles_7x7_count;
    
    case PUZZLE_8X8:
      return puzzles_8x8_count;
    
    case PUZZLE_9X9:
      return puzzles_9x9_count;

  }

  return 0;
  
}


/* ----------------------------------------------------------------------------
 *  Toggle the sound setting and commit to the EEPROM.
 */
void toggleSoundSettings() {

  if (arduboy.audio.enabled()) {
  
    arduboy.audio.off(); 
    arduboy.audio.saveOnOff();
  
  }
  else {
  
    arduboy.audio.on(); 
    arduboy.audio.saveOnOff();
  
  }
    
}


/* ----------------------------------------------------------------------------
 *  Draw a horizontal dotted line. 
 *  
 *  So much nicer than a solid line!
 */
void drawHorizontalDottedLine(int x1, int x2, int y) {

  for (int x3 = x1; x3 <= x2; x3+=2) {
    arduboy.drawPixel(x3, y, WHITE);
  }
  
}


/* ----------------------------------------------------------------------------
 *  Sends a screen shot to the programmer.
 *  
 */
#ifdef SCREENSHOTS

void screenShot() {
  Serial.write(arduboy.getBuffer(), 128 * 64 / 8);
  arduboy.delayShort(1000);
}

#endif

#ifdef SCREENSHOTS
//  screenShot();
#endif

