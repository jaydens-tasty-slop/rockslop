typedef unsigned char u8;
typedef unsigned short u16;
void titleScreen();
void winScreen();
void lossScreen();
void helpScreen();
void pickScreen();
void waitScreen();
void resetGame();
void drawChessBoard();
void drawUnits();
void drawStats();
void moveUnit(u8 gridId);
bool playerPend();
bool aiAct();
void gameLoop();

#include <Arduboy2.h>
#include "src/fonts/Font4x6.h"

#define STATE_TITLE 0
#define STATE_PICK 1
#define STATE_WAIT 2
#define STATE_WIN 3
#define STATE_LOSE 4
#define STATE_HELP 5

#define UNIT_NONE 0
#define UNIT_PLAYER 1
#define UNIT_AI 2

#define GRID_NW 0
#define GRID_NE 1
#define GRID_SW 2
#define GRID_SE 3
#define GRID_CT 4

#define GRID_NUM 5

#define UNIT_RADIUS 4
#define CROSS_RADIUS 1
#define HOVER_RADIUS 7

typedef uint8_t u8;

typedef struct {
    u8 x;
    u8 y;
} coord_t;

typedef struct {
    u8 node[GRID_NUM];
    u8 size;
} grid_set_t;
void findMovable(grid_set_t* grids, u8 unitId);

const u8 PROGMEM bmpPavilion[] = {
    50, 50,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x40, 0x80, 0x80, 0xc0, 0xc0, 0xe0, 0xf0, 0xf8, 
    0xfc, 0xfe, 0xcf, 0xff, 0xff, 0xff, 0xfe, 0xfc, 
    0xf8, 0xf0, 0xe0, 0xc0, 0xc0, 0x80, 0x80, 0xc0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x03, 0x0e, 0x1e, 0x3e, 0x1e, 
    0x3e, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0x3e, 0x1e, 0x3e, 0x1e, 0x0c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x0d, 0x05, 
    0x0d, 0xcf, 0xff, 0xff, 0x01, 0x05, 0x01, 0x05, 
    0x05, 0x01, 0x01, 0x07, 0xff, 0xff, 0xff, 0x05, 
    0x07, 0x07, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xff, 
    0xe0, 0xe0, 0xe0, 0xf0, 0xff, 0xff, 0x50, 0x50, 
    0x50, 0xf0, 0x50, 0x50, 0x50, 0x50, 0xff, 0xff, 
    0xff, 0x10, 0xc0, 0x80, 0xff, 0x60, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x1e, 
    0x16, 0x17, 0x27, 0x27, 0x27, 0x07, 0x07, 0x0f, 
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 
    0x0f, 0x07, 0x07, 0x27, 0x27, 0x27, 0x03, 0x10, 
    0x10, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const u8 PROGMEM bmpHand[] = {
    6, 4,
    0x02, 0x06, 0x0f, 0x0b, 0x0b, 0x0e, 
};

const u8 PROGMEM bmpNotepad[] = {
    20, 20,
    0x00, 0x00, 0xf8, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x88, 0xc8, 0xe0, 0xf0, 0x78, 0xbc, 
    0x1e, 0x0c, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 
    0x00, 0x00, 0x00, 0x0c, 0x0e, 0x0f, 0x07, 0x03, 
    0x01, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x03, 
    0x00, 0x00, 0x00, 0x00,
};

const coord_t gridPos[GRID_NUM] = {
    {34, 16}, // GRID_NW
    {66, 16}, // GRID_NE
    {34, 48}, // GRID_SW
    {66, 48}, // GRID_SE
    {50, 32}, // GRID_CT
};

/***
Chessboard topology:

  NW -------- NE
    | \    /
    |  \  /
    |   CT
    |  /  \
    | /    \
  SW -------- SE

***/
const grid_set_t gridNext[GRID_NUM] = {
    // GRID_NW
    {{GRID_NE, GRID_CT, GRID_SW}, 3},
    // GRID_NE
    {{GRID_NW, GRID_CT}, 2},
    // GRID_SW
    {{GRID_NW, GRID_CT, GRID_SE}, 3},
    // GRID_SE
    {{GRID_SW, GRID_CT}, 2},
    // GRID_CT
    {{GRID_NW, GRID_SW, GRID_NE, GRID_SE}, 4},
};

u8 gameState = STATE_TITLE;
u8 unitMap[GRID_NUM] = {0};
u8 gridNone = GRID_CT;

grid_set_t playerMovable = {0};
u8 hoverIndex = 0;

unsigned long aiDelayTick = 0;

unsigned int moveCount = 0;

bool playerFirst = true;

Arduboy2 arduboy;
Font4x6 font4x6 = Font4x6();

void setup() { 
    arduboy.boot();
    arduboy.setFrameRate(45);
    arduboy.display();
    arduboy.initRandomSeed();
    arduboy.clear();
}

void loop() {
    if (!arduboy.nextFrame()) {
        return;
    }
    arduboy.pollButtons();
    arduboy.clear();
    gameLoop();
    arduboy.display();
}

void titleScreen() {
    arduboy.setCursor(48, 12);
    arduboy.print(F("PONG HAU K'I"));
    font4x6.setCursor(59, 36);
    font4x6.print(F("Move first"));
    font4x6.setCursor(98, 46);
    font4x6.print(F("No"));

    arduboy.drawBitmap(2, 8, bmpPavilion + 2, bmpPavilion[0], bmpPavilion[1], WHITE);

    u8 handY = playerFirst ? 38 : 48;
    arduboy.drawBitmap(112, handY, bmpHand + 2, bmpHand[0], bmpHand[1], WHITE);

    if (arduboy.justPressed(UP_BUTTON)
        || arduboy.justPressed(DOWN_BUTTON)
        || arduboy.justPressed(LEFT_BUTTON)
        || arduboy.justPressed(RIGHT_BUTTON)) {
        playerFirst = !playerFirst;
    }
    if (arduboy.justPressed(A_BUTTON)) {
        resetGame();
        if (!playerFirst) {
            aiDelayTick = millis();
            gameState = STATE_WAIT;
        } else {
            playerPend();
            gameState = STATE_PICK;
        } 
    }
    if (arduboy.justPressed(B_BUTTON)) {
        gameState = STATE_HELP;
    }
}

void winScreen() {
    drawChessBoard();
    drawUnits();
    drawStats();

    arduboy.setCursor(6, 30);
    arduboy.print(F("WIN"));
    if (arduboy.justPressed(A_BUTTON)) {
        gameState = STATE_TITLE;
    }
}

void lossScreen() {
    drawChessBoard();
    drawUnits();
    drawStats();

    arduboy.setCursor(6, 30);
    arduboy.print(F("LOSE"));
    if (arduboy.justPressed(A_BUTTON)) {
        gameState = STATE_TITLE;
    }
}

void helpScreen() {
    font4x6.setCursor(9, 4);
    font4x6.print(F("A simple chess known\nin China and a few\nother Asian countries."));
    font4x6.setCursor(9, 34);
    font4x6.print(F("Move 1 of 2 pieces by\nturn. Who gets blocked\nloses game."));
    if (arduboy.justPressed(A_BUTTON)) {
        gameState = STATE_TITLE;
    }
}

void pickScreen() {
    drawChessBoard();
    drawUnits();
    drawStats();

    u8 gridPicked = playerMovable.node[hoverIndex];
    arduboy.drawCircle(gridPos[gridPicked].x, gridPos[gridPicked].y, HOVER_RADIUS, WHITE);

    if (arduboy.justPressed(UP_BUTTON)
        || arduboy.justPressed(DOWN_BUTTON)
        || arduboy.justPressed(LEFT_BUTTON)
        || arduboy.justPressed(RIGHT_BUTTON)) {
        hoverIndex = (hoverIndex + 1) % playerMovable.size;
    }
    if (arduboy.justPressed(A_BUTTON)) {
        moveUnit(gridPicked);
        aiDelayTick = millis();
        gameState = STATE_WAIT;
    }
}

void waitScreen() {
    drawChessBoard();
    drawUnits();
    drawStats();

    if (aiDelayTick > 0) {
        unsigned long tick = millis();
        if (tick - aiDelayTick < 1000) {
            return;
        }
        aiDelayTick = 0;
    }

    if (!aiAct()) {
        gameState = STATE_WIN;
        return;
    }

    if (!playerPend()) {
        gameState = STATE_LOSE;
        return;
    }

    gameState = STATE_PICK;
}

void resetGame() {
    unitMap[GRID_NW] = UNIT_AI;
    unitMap[GRID_SW] = UNIT_AI;
    unitMap[GRID_NE] = UNIT_PLAYER;
    unitMap[GRID_SE] = UNIT_PLAYER;
    unitMap[GRID_CT] = UNIT_NONE;

    if (!playerFirst) {
        unitMap[GRID_NW] = UNIT_PLAYER;
        unitMap[GRID_SW] = UNIT_PLAYER;
        unitMap[GRID_NE] = UNIT_AI;
        unitMap[GRID_SE] = UNIT_AI;
    }

    gridNone = GRID_CT;
    moveCount = 0;

    randomSeed(millis());
}

void drawChessBoard() {
    u8 w = gridPos[GRID_NE].x - gridPos[GRID_NW].x;
    u8 h = gridPos[GRID_SW].y - gridPos[GRID_NW].y;
    arduboy.drawFastHLine(gridPos[GRID_NW].x, gridPos[GRID_NW].y, w, WHITE);
    arduboy.drawFastVLine(gridPos[GRID_NW].x, gridPos[GRID_NW].y, h, WHITE);
    arduboy.drawFastHLine(gridPos[GRID_SW].x, gridPos[GRID_SW].y, w, WHITE);
    arduboy.drawLine(gridPos[GRID_NW].x, gridPos[GRID_NW].y, gridPos[GRID_SE].x, gridPos[GRID_SE].y, WHITE);
    arduboy.drawLine(gridPos[GRID_SW].x, gridPos[GRID_SW].y, gridPos[GRID_NE].x, gridPos[GRID_NE].y, WHITE);

    arduboy.drawBitmap(80, 10, bmpNotepad + 2, bmpNotepad[0], bmpNotepad[1], WHITE);
}

void drawUnits() {
    for (int i = 0; i < GRID_NUM; i++) {
        if (unitMap[i] == UNIT_PLAYER) {
            arduboy.fillCircle(gridPos[i].x, gridPos[i].y, UNIT_RADIUS, WHITE);
        } else if (unitMap[i] == UNIT_AI) {
            arduboy.fillCircle(gridPos[i].x, gridPos[i].y, UNIT_RADIUS, WHITE);
            arduboy.fillCircle(gridPos[i].x, gridPos[i].y, UNIT_RADIUS - 1, BLACK);
        } else {
            u8 w = 2 * CROSS_RADIUS + 1;
            arduboy.fillRect(gridPos[i].x - CROSS_RADIUS, gridPos[i].y - CROSS_RADIUS, w, w, WHITE);
        }
    }
}

void drawStats() {
    font4x6.setCursor(80, 36);
    font4x6.print(F("TURN"));
    font4x6.setCursor(86, 46);
    font4x6.print(moveCount / 2);
}

void findMovable(grid_set_t* grids, u8 unitId) {
    grid_set_t next = gridNext[gridNone];
    grids->size = 0;
    for (int i = 0; i < next.size; i++) {
        u8 gridId = next.node[i];
        if (unitMap[gridId] == unitId) {
            grids->node[grids->size] = gridId;
            grids->size++;
        }
    }
}

void moveUnit(u8 gridId) {
    u8 unitId = unitMap[gridId];
    unitMap[gridId] = UNIT_NONE;
    unitMap[gridNone] = unitId;
    gridNone = gridId;
    moveCount++;
}

bool playerPend() {
    findMovable(&playerMovable, UNIT_PLAYER);
    if (playerMovable.size == 0) {
        return false;
    }
    hoverIndex = 0;
    return true;
}

bool aiAct() {
    grid_set_t aiMovable = {0};
    findMovable(&aiMovable, UNIT_AI);
    if (aiMovable.size == 0) {
        return false;
    }

    u8 aiMoveIndex = random(aiMovable.size);
    u8 aiMove = aiMovable.node[aiMoveIndex];
    moveUnit(aiMove);
    return true;
}

void gameLoop() {
    switch (gameState) {
        case STATE_TITLE:
            titleScreen();
            break;
        case STATE_PICK:
            pickScreen();
            break;
        case STATE_WAIT:
            waitScreen();
            break;
        case STATE_WIN:
            winScreen();
            break;
        case STATE_LOSE:
            lossScreen();
            break;
        case STATE_HELP:
            helpScreen();
            break;
        default:
            break;
    }
}

