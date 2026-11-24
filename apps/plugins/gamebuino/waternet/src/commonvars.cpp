#include <Gamebuino-Meta.h>
#include <stdint.h>

#include "commonvars.h"
#include "../res/blocktiles.h"
#include "../res/congratsscreenmap.h"
#include "../res/congratsscreentiles.h"
#include "../res/selectortiles.h"
#include "../res/titlescreenmap.h"
#include "../res/fulltitlescreenmap.h"

uint8_t startPos, menuPos, 
        maxLevel, selectedLevel, boardX, boardY, difficulty, 
        gameState, boardWidth, boardHeight, boardSize,
        levelDone, titleStep, gameMode, posAdd, debugMode, 
        mainMenu, option, needRedraw;
 
int16_t selectionX, selectionY;
uint16_t moves;
unsigned char level[maxBoardSize];
uint32_t randomSeedGame;

Image blockTiles(BLOCKTILES_DATA);
Image titlescreenMap(TITLESCREENMAP_DATA);
Image congratsMap(CONGRATSSCREENMAP_DATA);
Image selectorTiles(SELECTORTILES_DATA);
Image congratsScreenTiles(CONGRATSTILES_DATA);
Image fullTitlescreenMap(FULLTITLESCREENMAP_DATA);
