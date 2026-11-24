#include <Arduino.h>
#include "src/utils/Arduboy2Ext.h"
#include "src/entities/Entities.h"
#include "src/images/Images.h"
#include "src/fonts/Font3x5.h"
#include "src/utils/Constants.h"
#include "src/utils/Structs.h"
#include "src/utils/EEPROM_Utils.h"

#ifdef ARDUBOY_TONES_SOUNDS
#include <ArduboyTones.h>
#include "src/sounds/Sounds.h"
#endif

using FlashStringHelper = const __FlashStringHelper *;

#ifndef DEBUG
ARDUBOY_NO_USB
#endif 
/* Arduino IDE auto-prototypes */
void byteBeatStep();
void credits_Init();
void credits();
void hackGame_Init();
void hackGame();
void hackGame_HackOver();
void hackGame_HackScore();
void renderInverseCell(const uint8_t x, const uint8_t y);
void renderInverseCell_Objectives(const uint8_t x, const uint8_t y, const bool inverse);
void renderBoard();
void renderSelection();
void printZero();
void printTime(uint8_t minutes, uint8_t seconds);
void renderObjectives();
uint8_t renderScoreObjectives();
void showMap_Init();
void showMap();
void renderDeckName(uint8_t deckIdx);
void renderPopup_Resizable(const uint8_t x, const uint8_t y, uint8_t height, uint8_t width);
void renderPopup_Large(const uint8_t x, const uint8_t y, bool large);
void renderDistricts();
void renderCommercialMenuOption(bool available, uint8_t wordIdx, uint16_t value);
void print(Print & printer, uint8_t wordIndex);
void println(Print & printer, uint8_t wordIndex);
void showMap_Message_Init();
void showMap_Message();
void handleTransition();
void splashScreen_Init();
void splashScreen();
void stealthGame_Init();
void stealthGame();
void clearGuardAlerts();
void createReticleTarget(uint8_t idx, int16_t x);
void gotoMap();
void gotoMapNextState(GameState next);
void renderPlayer();
void renderGuard(Guard &guard);
void renderForeground(const int16_t xOffset);
void renderBackground(const int16_t xOffset);
void renderTallBuilding(const int16_t x, const int8_t door);
void renderMedBuilding(const int16_t x, const int8_t door);
void renderLowBuilding(const int16_t x, const int8_t door);
void renderGauges();
void renderTerminals();
void tv_Init();
void tv();
void title_Init();
void title();
void clearLEDs();
void printHex(uint8_t data);
void initByteBeat();
void disableByteBeat();
void enableByteBeat();
uint8_t isSubArray(uint8_t fullArray[], uint8_t subArray[], uint8_t fullArrayLength, uint8_t subArrayLength);
void flickering(uint8_t y);

Arduboy2Ext arduboy;

#ifdef ARDUBOY_TONES_SOUNDS
ArduboyTones sound(arduboy.audio.enabled);
#endif

#ifdef BYTE_BEAT_SOUNDS

/* The AVR drove a timer-4 PWM speaker from timer-1 interrupts at 7812.5 Hz;
 * here the mixer thread calls byteBeatStep() at that rate (ab_tunes_fill below)
 * and the "PWM compare register" is a plain variable. */
static volatile uint8_t byteBeatOut;
static volatile bool byteBeatOn;
#define OCR4A byteBeatOut

extern "C" int ab_tunes_fill(int16_t *buf, int frames, int rate)
{
    static uint32_t acc;
    if (!byteBeatOn) return 0;
    for (int i = 0; i < frames; i++) {
        acc += 15625;                /* 2 x 7812.5 Hz, in 1/2 Hz units */
        while (acc >= 2 * (uint32_t)rate) {
            acc -= 2 * (uint32_t)rate;
            byteBeatStep();
        }
        buf[i] = ab_audio_enabled() ? ((int)byteBeatOut - 128) * 20 : 0;
    }
    return 1;
}

long t;
#ifdef BYTE_BEAT_SOUNDS_1
uint8_t hpISR = 46;
#endif
#ifdef BYTE_BEAT_SOUNDS_3
uint8_t byteBeatIdx = ByteBeatIndex::Map;
#endif

void byteBeatStep()
{
    t++;
    #ifdef BYTE_BEAT_SOUNDS_1
    OCR4A = ((t*(t>>8|t>>18)&hpISR&t>>3))^(t&t>>8|t>>20);
    #endif
    #ifdef BYTE_BEAT_SOUNDS_3
    switch (byteBeatIdx) {

        case ByteBeatIndex::Hack:
            OCR4A = ((t*((t>>8)|(t>>18))&33&t>>3))^((t&t>>8)|(t>>20));
            break;

        case ByteBeatIndex::Map:
            OCR4A = (((t>>7)|((t&t)>1))|(t&t>>8)|(t>>7))|(t+(t>>8))|((t>>7));
            break;

        case ByteBeatIndex::Street:
            OCR4A = ((t>>6)^(t&0x25))|((t+(t^(t>>11))-t* ((t%24?2:6)&t>>11))^(t<<1&(t&0x256?t>>4:t>>10)));
            break;
            
    }
    #endif

}
#endif

Font3x5 font3x5 = Font3x5(6);
GameState gameState = GameState::Splash_Init;
MapGameVars mapGameVars;
StealthGameVars stealthGameVars;
HackGameVars hackGameVars;
TVSequenceVars tvSequenceVars;
Player player;

#ifdef TV_TRANSITION
    GameState nextState = GameState::Splash_Init;
#endif

void setup() {
  
	arduboy.boot();
	arduboy.systemButtons();
    arduboy.setFrameRate(50);
    arduboy.initRandomSeed();
    arduboy.audio.begin();

    #ifdef ARDUBOY_TONES_SOUNDS
    arduboy.audio.begin();
    #endif

    EEPROM_Utils::initEEPROM(false);
    initByteBeat();

}   


void loop() {

    if ( !arduboy.nextFrame() ) return;    
	arduboy.pollButtons();

    switch (gameState) {

        case GameState::Splash_Init:
            splashScreen_Init();
            /*-fallthrough*/

        case GameState::Splash:
            splashScreen();
            break;

        case GameState::Title_Init:
            title_Init();
            /*-fallthrough*/

        case GameState::Title:
            title();
            break;

        case GameState::HackGame_Init:
            hackGame_Init();
            /*-fallthrough*/

        case GameState::HackGame:
            hackGame();
            break;

        case GameState::HackOver_Disconnected:
        case GameState::HackOver_BufferFull:
        case GameState::HackOver_CannotMove:
        case GameState::HackOver_AllHacksPlayed:
            hackGame_HackOver();
            break;

        case GameState::HackScore:
            hackGame_HackScore();
            break;

        case GameState::ShowMap_Init:
            showMap_Init();
            /*-fallthrough*/

        case GameState::ShowMap:
            showMap();
            break;

        case GameState::Messages_Start_Init ... GameState::Messages_End_Init:
            showMap_Message_Init();
            break;

        case GameState::Messages_Start ... GameState::Messages_End:
            showMap_Message();
            break;

        case GameState::StealthGame_Init:
            stealthGame_Init();
            /*-fallthrough*/

        case GameState::StealthGame:
            stealthGame();
            break;

#ifdef TV_TRANSITION

            case GameState::TV_Init:
                tv_Init();
                /*-fallthrough*/

            case GameState::TV:
                tv();
                break;
            
#endif

#ifdef CREDITS

            case GameState::Credits_Init:
                credits_Init();
                /*-fallthrough*/

            case GameState::Credits:
                credits();
                break;
            
#endif

    }

    arduboy.display(true);

}

#include <Arduboy2.h>

#ifdef CREDITS

void credits_Init() {

    gameState = GameState::Credits;
    tvSequenceVars.counter = 0;

}   

void credits() {

    Sprites::drawOverwrite(5, 12, Images::Title_02, 0);
    flickering(12);
    
    if (arduboy.isFrameCount(4)) tvSequenceVars.counter++;
    if (tvSequenceVars.counter >= 200) tvSequenceVars.counter = 0;

    Sprites::drawOverwrite(13, 43, Images::LeftFade2, 0);
    Sprites::drawOverwrite(107, 43, Images::RightFade, 0);

    arduboy.drawFastHLine(18, 43, 89);
    arduboy.drawFastHLine(18, 51, 89);
    
    switch (tvSequenceVars.counter % 40) {

        case 0 ... 32:
            font3x5.setCursor(20, 44);
            print(font3x5, 137 + tvSequenceVars.counter / 40);
            break;

        default:
            break;
    }

    if (arduboy.justPressed(A_BUTTON)) { 

        gameState = GameState::Title_Init;

    }

}
#endif
#include "src/utils/Arduboy2Ext.h"

void hackGame_Init() {

    #ifndef PUZZLE_LOAD_TEST
    const uint8_t *hack_in_play = pgm_read_ptr(&Hacks::hacks[player.getHackIndex()]);  
    #endif
    
    uint8_t counter = 0;
    uint8_t contiguous = 0;

    clearLEDs();


    #ifdef PUZZLE_LOAD_TEST

    for (uint8_t a = 0; a < 20; a++) {
        for (uint16_t b = 0; b < 1000; b++) {
            
            const uint8_t *hack_in_play = pgm_read_ptr(&Hacks::hacks[a]);  

            counter = 0;
            contiguous = 0;

            Serial.print(a);
            Serial.print(" ");
            Serial.println(b);

    #endif


    hackGameVars.grid.clear();
    hackGameVars.grid.setSize(pgm_read_byte(&hack_in_play[counter++]));
    hackGameVars.grid.setBufferSize(pgm_read_byte(&hack_in_play[counter++]));
    hackGameVars.timer = pgm_read_byte(&hack_in_play[counter++]);

    hackGameVars.objectives.clear();

    uint8_t clues = pgm_read_byte(&hack_in_play[counter++]);
    uint8_t maxNumber = pgm_read_byte(&hack_in_play[counter++]);  
    hackGameVars.grid.setTarget(pgm_read_byte(&hack_in_play[counter++]));  

    for (uint8_t i = 0; i < clues; i++) {

        uint8_t length = pgm_read_byte(&hack_in_play[counter++]);    
        uint8_t maxValue = pgm_read_byte(&hack_in_play[counter++]);    
        uint8_t overlap = pgm_read_byte(&hack_in_play[counter++]);    
        uint8_t overlapIdx = pgm_read_byte(&hack_in_play[counter++]);    

        if (i == 0 || overlap != NO_HACK_ID) {

            contiguous++;

        }

        hackGameVars.objectives.generate(i, length, maxValue, overlap, overlapIdx);

    }

    #ifndef PUZZLE_LOAD_TEST
    hackGameVars.objectives.print();
    #endif

    hackGameVars.grid.placeObjectives(hackGameVars.objectives, contiguous, maxNumber);

    #ifndef PUZZLE_LOAD_TEST
    hackGameVars.grid.print();
    #endif

    hackGameVars.objectives.shuffle();
    hackGameVars.renumber(arduboy);

    #ifndef PUZZLE_LOAD_TEST
    hackGameVars.objectives.print();
    #endif

    #ifdef PUZZLE_LOAD_TEST
    }}
    #endif

    #ifdef BYTE_BEAT_SOUNDS_3
    byteBeatIdx = ByteBeatIndex::Hack;
    #endif

    hackGameVars.timerStart = false;
    gameState = GameState::HackGame;

}   


void hackGame() {

    auto justPressed = arduboy.justPressedButtons();


    // House keeping ..

    if (arduboy.isFrameCount(30, 0) && hackGameVars.timerStart && hackGameVars.timer > 0) hackGameVars.timer--;

    #ifdef USE_LED_ANALOGUE
    
        if (hackGameVars.ledDelay > 0) {
            
            hackGameVars.ledDelay--;

        }
        else {
                
            clearLEDs();

        }
    
    #endif


    // Handle movements

    switch (hackGameVars.selectionMode) {

        case SelectionMode::Grid:

            switch (hackGameVars.grid.getSelectionType()) {

                case SelectionType::Cell:

                    if (justPressed & LEFT_BUTTON)       hackGameVars.grid.decX();
                    if (justPressed & RIGHT_BUTTON)      hackGameVars.grid.incX();

                    break;

                case SelectionType::Column:

                    if ((justPressed & UP_BUTTON) && hackGameVars.grid.getY() > 0) {

                        for (uint8_t y = hackGameVars.grid.getY(); y > 0; y--) {
                            if (hackGameVars.grid.getCell_Selection(hackGameVars.grid.getX(), y - 1) == Constants::Grid_Blank) {
                                hackGameVars.grid.setY(y - 1);
                                break;
                            }
                        }

                    }

                    else if ((justPressed & DOWN_BUTTON) && hackGameVars.grid.getY() < hackGameVars.grid.getSize() - 1) {

                        for (uint8_t y = hackGameVars.grid.getY() + 1; y < hackGameVars.grid.getSize(); y++) {
                            if (hackGameVars.grid.getCell_Selection(hackGameVars.grid.getX(), y) == Constants::Grid_Blank) {
                                hackGameVars.grid.setY(y);
                                break;
                            }
                        }

                    }

                    break;

                case SelectionType::Row:

                    if ((justPressed & LEFT_BUTTON) && hackGameVars.grid.getX() > 0) {

                        for (uint8_t x = hackGameVars.grid.getX(); x > 0; x--) {

                            if (hackGameVars.grid.getCell_Selection(x - 1, hackGameVars.grid.getY()) == Constants::Grid_Blank) {
                                hackGameVars.grid.setX(x - 1);
                                break;
                            }
                        }

                    }

                    else if ((justPressed & RIGHT_BUTTON) && hackGameVars.grid.getX() < hackGameVars.grid.getSize() - 1) {

                        for (uint8_t x = hackGameVars.grid.getX() + 1; x < hackGameVars.grid.getSize(); x++) {

                            if (hackGameVars.grid.getCell_Selection(x, hackGameVars.grid.getY()) == Constants::Grid_Blank) {
                                hackGameVars.grid.setX(x);
                                break;
                            }

                        }

                    }

                    break;

            }

            if ((justPressed & A_BUTTON) && hackGameVars.grid.getCell_Selection(hackGameVars.grid.getX(), hackGameVars.grid.getY()) == Constants::Grid_Blank) {

                uint8_t installedHacks = hackGameVars.installedHacks;
                uint8_t failedHacks = hackGameVars.failedHacks;

                hackGameVars.timerStart = true;
                hackGameVars.grid.select();
                renderObjectives(); // so we can see if a hack was made.

                if (installedHacks != hackGameVars.installedHacks) {

                    hackGameVars.ledDelay = 12;

                    #ifdef USE_LED_ANALOGUE
                        arduboy.setRGBled(GREEN_LED, 32);
                    #endif

                    #ifdef ARDUBOY_TONES_SOUNDS
                        sound.tones(Sounds::Success);
                    #endif
                    
                }
                else if (failedHacks != hackGameVars.failedHacks) {

                    hackGameVars.ledDelay = 12;

                    #ifdef USE_LED_ANALOGUE
                        arduboy.setRGBled(RED_LED, 32);
                    #endif

                    #ifdef ARDUBOY_TONES_SOUNDS
                        sound.tones(Sounds::Failure);
                    #endif

                }

            }

            else if (justPressed & B_BUTTON) {

                hackGameVars.selectionMode = SelectionMode::Objectives;

            }

            break;

        case SelectionMode::Objectives:

            if ((justPressed & LEFT_BUTTON) && hackGameVars.grid.getObjX() > 0) {

                hackGameVars.grid.setObjX(hackGameVars.grid.getObjX() - 1);
                
            }

            else if ((justPressed & RIGHT_BUTTON) && hackGameVars.grid.getObjX() < hackGameVars.objectives.getObjective(hackGameVars.grid.getObjY()).getLength() - 1) {
                hackGameVars.grid.setObjX(hackGameVars.grid.getObjX() + 1);

            }

            else if (justPressed & UP_BUTTON) {

                if (hackGameVars.grid.getObjY() > 0) {

                    hackGameVars.grid.setObjY(hackGameVars.grid.getObjY() - 1);
                 
                }

            }

            else if (justPressed & DOWN_BUTTON) {

                if (hackGameVars.grid.getObjY() < hackGameVars.objectives.getCount() - 1) {

                    hackGameVars.grid.setObjY(hackGameVars.grid.getObjY() + 1);
 
                }

            }

            else if (justPressed & B_BUTTON) {

                hackGameVars.selectionMode = SelectionMode::Grid;

            }

    }


    // Render screen ..

    renderBoard();
    renderObjectives();
    renderSelection();


    // Is the hack over?

    if (hackGameVars.timer == 0) {
        gameState = GameState::HackOver_Disconnected;
    }

    else if (hackGameVars.availableHacks == 0) {
        gameState = GameState::HackOver_AllHacksPlayed;
    }

    else if (hackGameVars.grid.getSelectionCount() == hackGameVars.grid.getBufferSize()) {
        gameState = GameState::HackOver_BufferFull;
    }

    /* Valid Move Testing
    bool validMove = false;

    switch (hackGameVars.grid.getSelectionType()) {

        case SelectionType::Cell:
            validMove = true;
            break;

        case SelectionType::Column:

            for (uint8_t y = 0; y < hackGameVars.grid.getSize(); y++) {

                if (hackGameVars.grid.getCell_Selection(hackGameVars.grid.getX(), y) == Constants::Grid_Blank) {

                    validMove = true;
                    break;

                }

            }

            break;

        case SelectionType::Row:

            for (uint8_t x = 0; x < hackGameVars.grid.getSize(); x++) {

                if (hackGameVars.grid.getCell_Selection(x, hackGameVars.grid.getY()) == Constants::Grid_Blank) {

                    validMove = true;
                    break;

                }

            }

            break;

    }

    if (!validMove) {

        gameState = GameState::HackOver_CannotMove;

    }
    */
 
}

void hackGame_HackOver() {

    clearLEDs();
    renderBoard();
    renderObjectives();
    renderSelection();

    Sprites::drawPlusMask(36, 23, Images::Dialogue, 0);
    font3x5.setCursor(40, 26);

    switch (gameState) {

        case GameState::HackOver_Disconnected:
            font3x5.print(F("DISCONNECTED"));
            break;

        case GameState::HackOver_BufferFull:
            font3x5.print(F("BUFFER FULL"));
            break;

        case GameState::HackOver_CannotMove:
            font3x5.print(F("CANNOT MOVE"));
            break;

        case GameState::HackOver_AllHacksPlayed:
            Sprites::drawPlusMask(25, 23, Images::Dialogue_Large, 0);
            break;

        default: break;

    }

     if (arduboy.justPressed(A_BUTTON)) {

         gameState = GameState::HackScore;

     }

}

void hackGame_HackScore() {

    uint8_t thisScore = renderScoreObjectives();

    auto justPressed = arduboy.justPressedButtons();

    if (justPressed & A_BUTTON) {

        player.setDollars(player.getDollars() + thisScore);

        if (thisScore < hackGameVars.grid.getTarget()) {

            player.incThreatLevel(10);
            mapGameVars.setMessage(Message::Hack_Fail);

            #ifdef TV_TRANSITION
                gameState = GameState::TV_Init;
                nextState = GameState::Message_Hack_Target_Not_Achieved_Init;
            #else
                gameState = GameState::Message_Hack_Target_Not_Achieved_Init;
            #endif

        }
        else {

            uint8_t idx = player.getRandomPuzzle(true);
            player.incHackSuccess();

            #ifdef TV_TRANSITION
                gameState = GameState::TV_Init;
            #endif

            Message message = Message::Hack_Success;

            switch (idx) {

                case Constants::Hack_Game_Over:
                    message = Message::Game_Over_Success;
                    #ifdef TV_TRANSITION
                        nextState = GameState::Message_Game_Over_Success_Init;
                    #else
                        gameState = GameState::Message_Game_Over_Success_Init;
                    #endif
                    break;

                case Constants::Hack_None_Available:
                {
                    const bool playerCanPurchaseDeck = player.canPurchaseADeck();
                    message = playerCanPurchaseDeck ? Message::Hack_Success : Message::Hacks_Game_Over;
                    #ifdef TV_TRANSITION
                        nextState = playerCanPurchaseDeck ? GameState::Message_Hacks_Achieved_Init : GameState::Message_Hacks_Game_Over;
                    #else
                        gameState = playerCanPurchaseDeck ? GameState::Message_Hacks_Achieved_Init : GameState::Message_Hacks_Game_Over;
                    #endif
                    break;
                }

                default:
                    #ifdef TV_TRANSITION
                    nextState = GameState::Message_Hacks_Achieved_Init;
                    #else
                    gameState = GameState::Message_Hacks_Achieved_Init;
                    #endif
                    break;

            }

            mapGameVars.setMessage(message);

        }

    }

}
#include "src/utils/Arduboy2Ext.h"

#define DISP_GRID_COL_WIDTH 12
#define DISP_GRID_ROW_HEIGHT 8
#define DISP_TITLE_OBJECTIVE_X 76
#define DISP_OBJECTIVES_LEFT DISP_TITLE_OBJECTIVE_X + 3
#define DISP_OBJECTIVES_TOP 11
#define DISP_OBJECTIVES_SPACING_X 10
#define DISP_OBJECTIVES_SPACING_Y 7
#define DISP_OBJECTIVES_COUNT 6

#define DISP_SCORES_SPACING_Y 8
#define DISP_SCORES_HACK_LEFT 57
#define DISP_SCORES_STATUS_LEFT 1
#define DISP_SCORES_TOP 10

const uint8_t hackGrid_X_Offsets[] = { 0, 0, 0, 0, DISP_GRID_COL_WIDTH, DISP_GRID_COL_WIDTH / 2, 0 };
const uint8_t hackGrid_Y_Offsets[] = { 0, 0, 0, 0, DISP_GRID_ROW_HEIGHT, DISP_GRID_ROW_HEIGHT / 2, 0 };

void renderInverseCell(const uint8_t x, const uint8_t y) {
    
    uint8_t xOffset = hackGrid_X_Offsets[hackGameVars.grid.getSize()];
    uint8_t yOffset = hackGrid_Y_Offsets[hackGameVars.grid.getSize()];

    uint16_t xPos = x * DISP_GRID_COL_WIDTH + xOffset;
    uint16_t yPos = y * DISP_GRID_ROW_HEIGHT + yOffset;

    arduboy.fillRect(xPos, yPos, DISP_GRID_COL_WIDTH + 1, DISP_GRID_ROW_HEIGHT + 1, WHITE);
    font3x5.setTextColor(BLACK);
    font3x5.setCursor(3 + xPos, 1 + yPos);
    printHex(hackGameVars.grid.getCell(x, y));
    font3x5.setTextColor(WHITE);

}

void renderInverseCell_Objectives(const uint8_t x, const uint8_t y, const bool inverse) {

    uint16_t xPos = DISP_OBJECTIVES_LEFT + (x * DISP_OBJECTIVES_SPACING_X);
    uint16_t yPos = DISP_OBJECTIVES_TOP + (y * DISP_OBJECTIVES_SPACING_Y);

    arduboy.fillRect(xPos - 1, yPos, DISP_OBJECTIVES_SPACING_X, DISP_OBJECTIVES_SPACING_Y, inverse ? WHITE : BLACK);
    font3x5.setTextColor(inverse ? BLACK : WHITE);
    font3x5.setCursor(xPos, yPos);
    printHex(hackGameVars.objectives.getObjective(y).getCell(x));
    font3x5.setTextColor(WHITE);

}

void renderBoard() {
    
    uint8_t gridSize = hackGameVars.grid.getSize();
    uint8_t xOffset = hackGrid_X_Offsets[gridSize];
    uint8_t yOffset = hackGrid_Y_Offsets[gridSize];


    // Render board ..
    
    for (uint8_t i = 0; i < gridSize + 1; i++) {

        arduboy.drawHorizontalDottedLine(xOffset, DISP_GRID_COL_WIDTH * gridSize + xOffset, (DISP_GRID_ROW_HEIGHT * i) + yOffset);
        arduboy.drawVerticalDottedLine(yOffset, DISP_GRID_ROW_HEIGHT * gridSize + yOffset, (DISP_GRID_COL_WIDTH * i) + xOffset);

    }

    for (uint8_t y = 0; y < gridSize; y++) {

        for (uint8_t x = 0; x < gridSize; x++) {

            font3x5.setCursor(3 + (x * DISP_GRID_COL_WIDTH) + xOffset, 1 + (y * DISP_GRID_ROW_HEIGHT) + yOffset);
            printHex(hackGameVars.grid.getCell(x, y));

            if (hackGameVars.grid.getCell_Selection(x, y) != Constants::Grid_Blank) {

                Sprites::drawSelfMasked(x * DISP_GRID_COL_WIDTH + xOffset, y * DISP_GRID_ROW_HEIGHT + yOffset, Images::GridCheck, 0);

            }
            
        }

    }


    // Rendered selected column / row ..

    switch (hackGameVars.selectionMode) {

        case SelectionMode::Grid:
            {
                uint8_t yVal = hackGameVars.grid.getY() * DISP_GRID_ROW_HEIGHT + yOffset;

                switch (hackGameVars.grid.getSelectionType()) {

                    case SelectionType::Cell:
                        arduboy.drawRect(xOffset, yVal, (DISP_GRID_COL_WIDTH * gridSize) + 1, DISP_GRID_ROW_HEIGHT + 1);
                        renderInverseCell(hackGameVars.grid.getX(), hackGameVars.grid.getY());
                        break;

                    case SelectionType::Column:
                        arduboy.drawRect(hackGameVars.grid.getX() * DISP_GRID_COL_WIDTH + xOffset, yOffset, DISP_GRID_COL_WIDTH + 1, (DISP_GRID_ROW_HEIGHT * gridSize) + 1);
                        renderInverseCell(hackGameVars.grid.getX(), hackGameVars.grid.getY());
                        break;

                    case SelectionType::Row:
                        arduboy.drawRect(xOffset, yVal, (DISP_GRID_COL_WIDTH * gridSize) + 1, DISP_GRID_ROW_HEIGHT + 1);
                        renderInverseCell(hackGameVars.grid.getX(), hackGameVars.grid.getY());
                        break;

                }

            }

            break;

        case SelectionMode::Objectives:

            for (uint8_t y = 0; y < gridSize; y++) {

                for (uint8_t x = 0; x < gridSize; x++) {

                    if (hackGameVars.grid.getCell(x, y) == hackGameVars.objectives.getObjective(hackGameVars.grid.getObjY()).getCell(hackGameVars.grid.getObjX())) {
                        
                        renderInverseCell(x, y);

                    }
                    
                }

            }

            break;

    }

}

void renderSelection() {

    // Render selection ..

    font3x5.setCursor(1, 50);
    font3x5.print(F("BUFFER"));

    Sprites::drawOverwrite(26, 51, Images::LeftFade, 0);    
    Sprites::drawOverwrite(65, 51, Images::RightLower_Fade, 0);  
    arduboy.drawFastHLine(34, 51, 31);  

    for (uint8_t i = 0; i < hackGameVars.grid.getBufferSize(); i++) {

        if (i < hackGameVars.grid.getSelectionCount()) {
            font3x5.setCursor(1 + (i * DISP_OBJECTIVES_SPACING_X), 56);
            printHex(hackGameVars.grid.getSelection(i));
        }

        arduboy.drawFastHLine(1 + (i * DISP_OBJECTIVES_SPACING_X), 63, 7);

    }

}

void printZero()
{
    font3x5.print("0");
}

void printTime(uint8_t minutes, uint8_t seconds)
{
    if (minutes < 10) printZero();
    font3x5.print(minutes);

    font3x5.print(".");

    if (seconds < 10) printZero();
    font3x5.print(seconds);
}

void renderObjectives() { 
    
    uint8_t availableHacks = hackGameVars.objectives.getCount();

    hackGameVars.installedHacks = 0;
    hackGameVars.failedHacks = 0;


    // Render hackGameVars.objectives ..

    arduboy.drawFastHLine(DISP_TITLE_OBJECTIVE_X + 4, 0, 55);
    arduboy.drawFastHLine(DISP_TITLE_OBJECTIVE_X, 8, 55);
    arduboy.drawFastHLine(DISP_TITLE_OBJECTIVE_X + 4, 55, 55);
    arduboy.drawFastHLine(DISP_TITLE_OBJECTIVE_X, 63, 55);
    arduboy.drawFastVLine(DISP_TITLE_OBJECTIVE_X, 9, 35);

    Sprites::drawOverwrite(WIDTH - 6, 0, Images::RightFade, 0);
    Sprites::drawOverwrite(WIDTH - 6, 55, Images::RightFade, 0);
    Sprites::drawOverwrite(DISP_TITLE_OBJECTIVE_X, 41, Images::DownFade, 0);
    Sprites::drawSelfMasked(DISP_TITLE_OBJECTIVE_X, 0, Images::LeftCorner, 0);
    Sprites::drawSelfMasked(DISP_TITLE_OBJECTIVE_X, 55, Images::LeftCorner, 0);

    font3x5.setCursor(DISP_TITLE_OBJECTIVE_X + 6, 1);
    font3x5.print(F("HACKS"));

    font3x5.setCursor(DISP_TITLE_OBJECTIVE_X + 6, 56);
    font3x5.print(F("TIME:"));

    if (hackGameVars.timer > 0 && (hackGameVars.timer > 5 || arduboy.getFrameCountHalf(15))) {

        printTime(hackGameVars.timer / 60, hackGameVars.timer % 60);

        #ifdef USE_LED_ANALOGUE
        arduboy.setRGBled(BLUE_LED, 0);
        #endif

    }
    else if (hackGameVars.timer <= 0) {

        font3x5.print(F("00.00"));

        #ifdef USE_LED_ANALOGUE
        arduboy.setRGBled(BLUE_LED, 0);
        #endif

    }
    else {

        #ifdef USE_LED_ANALOGUE
        arduboy.setRGBled(BLUE_LED, 32);
        #endif

    }


    // Render objective list ..

    uint8_t targetAchieved = 0;

    for (uint8_t j = 0; j < hackGameVars.objectives.getCount(); j++) {

        Objective objective = hackGameVars.objectives.getObjective(j);
        uint8_t highlightLength = isSubArray(hackGameVars.grid.getSelections(), objective.getCells(), hackGameVars.grid.getSelectionCount(), objective.getLength());

        if (highlightLength == objective.getLength() && hackGameVars.selectionMode == SelectionMode::Grid) {

            Sprites::drawSelfMasked(DISP_OBJECTIVES_LEFT - 1, DISP_OBJECTIVES_TOP + (j * DISP_OBJECTIVES_SPACING_Y), Images::Installed, 0);

            availableHacks--;
            hackGameVars.installedHacks++;
            targetAchieved = targetAchieved + objective.getScore();

        }
        else {

            if (objective.getLength() - highlightLength > hackGameVars.grid.getBufferSize() - hackGameVars.grid.getSelectionCount() && hackGameVars.selectionMode == SelectionMode::Grid) {

                Sprites::drawSelfMasked(DISP_OBJECTIVES_LEFT - 1, DISP_OBJECTIVES_TOP + (j * DISP_OBJECTIVES_SPACING_Y), Images::Failed, 0);
                availableHacks--;
                hackGameVars.failedHacks++;

            }
            else {

                for (uint8_t i = 0; i < Constants::Objective_Max_Length; i++) {

                    if (objective.getCell(i) != Constants::Grid_Blank) {

                        if (highlightLength > 0) {

                            arduboy.fillRect(DISP_OBJECTIVES_LEFT + (i * DISP_OBJECTIVES_SPACING_X) - 1, DISP_OBJECTIVES_TOP + (j * DISP_OBJECTIVES_SPACING_Y), 10, 7, WHITE);
                            font3x5.setTextColor(BLACK);

                        }
                        else {

                            font3x5.setTextColor(WHITE);

                        }

                        font3x5.setCursor(DISP_OBJECTIVES_LEFT + (i * DISP_OBJECTIVES_SPACING_X), DISP_OBJECTIVES_TOP + (j * DISP_OBJECTIVES_SPACING_Y));
                        printHex(objective.getCell(i));
                        if (highlightLength > 0) highlightLength--;

                    }

                }

            }
            
        }

    }


    // Render selected element ..

    if (hackGameVars.selectionMode == SelectionMode::Objectives) {

        renderInverseCell_Objectives(hackGameVars.grid.getObjX(), hackGameVars.grid.getObjY(), arduboy.getFrameCountHalf(32));

    }


    // Return the availableHacks ..

    hackGameVars.availableHacks = availableHacks;



    #ifdef SHOW_PUZZLE_NUMBER

        font3x5.setCursor(108, 1);
        font3x5.print("ID:");
        if (player.getHackIndex() < 10) printZero();
        font3x5.print(player.getHackIndex());
    
    #else

        Sprites::drawOverwrite(107, 0, Images::Target, 0);
        font3x5.setCursor(118, 1);

        uint8_t target = hackGameVars.grid.getTarget();
        uint8_t togo = target > targetAchieved ? target - targetAchieved : 0;
        if (togo < 10) printZero();
        font3x5.print(togo);

    #endif

}


uint8_t renderScoreObjectives() { // Returns number of active hacks available.

    uint8_t score = 0;


    // Render objectives ..

    #ifdef NEW_TOTALS

        Sprites::drawOverwrite(0, 0, Images::LeftFade2, 0);
        Sprites::drawOverwrite(46, 0, Images::RightFade, 0);
        Sprites::drawOverwrite(55, 0, Images::LeftFade2, 0);
        Sprites::drawOverwrite(96, 0, Images::RightFade, 0);
        Sprites::drawOverwrite(105, 0, Images::LeftFade2, 0);
        Sprites::drawOverwrite(121, 0, Images::RightFade, 0);

        Sprites::drawOverwrite(0, 57, Images::LeftFade, 0);
        Sprites::drawOverwrite(121, 57, Images::RightFade, 0);

        arduboy.drawFastHLine(5, 0, 41);
        arduboy.drawFastHLine(60, 0, 36);
        arduboy.drawFastHLine(112, 0, 9);
        
        arduboy.drawFastHLine(6, 8, 40);
        arduboy.drawFastHLine(61, 8, 35);
        arduboy.drawFastHLine(113, 8, 8);

        arduboy.drawFastHLine(6, 57, 115);

        font3x5.setCursor(13, 1);
        font3x5.print(F("STATUS        HACK"));
        font3x5.setCursor(106, 1);
        font3x5.print(F("MONEY"));

    #endif

    #ifdef OLD_TOTALS

        arduboy.drawHorizontalDottedLine(0, WIDTH, 0);
        arduboy.drawHorizontalDottedLine(0, WIDTH, 8);
        arduboy.drawHorizontalDottedLine(0, WIDTH, 57);

        font3x5.setCursor(1, 1);
        font3x5.print(F("STATUS        HACK"));
        font3x5.setCursor(107, 1);
        font3x5.print(F("SCORE"));

    #endif



    // Render objective list ..

    for (uint8_t j = 0; j < hackGameVars.objectives.getCount(); j++) {

        Objective objective = hackGameVars.objectives.getObjective(j);
        uint8_t highlightLength = isSubArray(hackGameVars.grid.getSelections(), objective.getCells(), hackGameVars.grid.getSelectionCount(), objective.getLength());

        for (uint8_t i = 0; i < Constants::Objective_Max_Length; i++) {

            if (objective.getCell(i) != Constants::Grid_Blank) {

                font3x5.setCursor(DISP_SCORES_HACK_LEFT + (i * DISP_OBJECTIVES_SPACING_X), DISP_SCORES_TOP + (j * DISP_SCORES_SPACING_Y));
                printHex(objective.getCell(i));

            }

        }

        if (highlightLength == objective.getLength()) {

            Sprites::drawSelfMasked(DISP_SCORES_STATUS_LEFT - 1, DISP_SCORES_TOP + (j * DISP_SCORES_SPACING_Y), Images::Installed, 0);

            font3x5.setCursor(119 + (objective.getScore() < 10 ? 4 : 0), DISP_SCORES_TOP + (j * DISP_SCORES_SPACING_Y));
            font3x5.print(objective.getScore());

            score = score + objective.getScore();

        }
        else {

            Sprites::drawSelfMasked(DISP_SCORES_STATUS_LEFT - 1, DISP_SCORES_TOP + (j * DISP_SCORES_SPACING_Y), Images::Failed, 0);

            font3x5.setCursor(123, DISP_SCORES_TOP + (j * DISP_SCORES_SPACING_Y));
            printZero();
            
        }

    }

    font3x5.setCursor(47 + (score < 10 ? 4 : 0) + (hackGameVars.grid.getTarget() < 10 ? 4 : 0), 58);
    font3x5.print(F("TARGET "));
    font3x5.print(hackGameVars.grid.getTarget());
    font3x5.print(" / TOTAL ");
    font3x5.print(score);

    return score;

}
#include "src/utils/Arduboy2Ext.h"

void showMap_Init() {

    gameState = GameState::ShowMap;

}   


void showMap() {

    auto justPressed = arduboy.justPressedButtons();

    clearLEDs();

    if (mapGameVars.menu_Index == Constants::Map_Closed) {

        if (justPressed & LEFT_BUTTON) {
            mapGameVars.menu--;
        }

        else if (justPressed & RIGHT_BUTTON) {
            mapGameVars.menu++;
        }

        else if ((justPressed & DOWN_BUTTON) && mapGameVars.menu != MapMenu::SlumsDistrict) {
            mapGameVars.menu = MapMenu::SlumsDistrict;
        }

        else if ((justPressed & UP_BUTTON) && mapGameVars.menu == MapMenu::SlumsDistrict) {
            mapGameVars.menu = MapMenu::CommercialDistrict;
        }

        else if (justPressed & A_BUTTON) {
            mapGameVars.menu_Index = 0;
        }

    }
    else {

        if (justPressed & B_BUTTON) {

            switch (mapGameVars.menu) {

                case MapMenu::CyberDeck:
                    mapGameVars.menu = MapMenu::SafeHouse;
                    break;

                default:
                    mapGameVars.menu_Index = Constants::Map_Closed;
                    break;

            }

        }

        switch (mapGameVars.menu) {

            case MapMenu::CommercialDistrict:

                if (mapGameVars.currentLocation == MapMenu::CommercialDistrict) {

                    if ((justPressed & UP_BUTTON) && mapGameVars.menu_Index > 0) {
                        mapGameVars.menu_Index--;
                    }

                    else if ((justPressed & DOWN_BUTTON) && mapGameVars.menu_Index < 2) {
                        mapGameVars.menu_Index++;
                    }

                    else if (justPressed & A_BUTTON) {

                        if (player.getDeck(mapGameVars.menu_Index + 1)) {

                            #ifdef ARDUBOY_TONES_SOUNDS
                            sound.tones(Sounds::InvalidChoice);
                            #endif
   
                        }
                        else if (player.getDeckPrice(mapGameVars.menu_Index + 1) > player.getDollars()) {

                            #ifdef ARDUBOY_TONES_SOUNDS
                            sound.tones(Sounds::InvalidChoice);
                            #endif

                        }
                        else {

                            player.setDeck(mapGameVars.menu_Index + 1, true);
                            player.decDollars(player.getDeckPrice(mapGameVars.menu_Index + 1));

                            uint8_t i = player.getFirstAvailDeck(true);
                            if (i != Constants::No_Deck_Avail) mapGameVars.menu_Index = i - 1;

                        }

                    }

                }
                else {

                    if (justPressed & A_BUTTON) {
                    
                        gameState = GameState::Message_Move_To_CommercialDistrict_Init;
                        mapGameVars.nextLocation = MapMenu::CommercialDistrict;

                        switch (mapGameVars.commercialDistrictMessage) {

                            case Message::CommercialDistrict_1:
                                mapGameVars.commercialDistrictMessage = Message::CommercialDistrict_2;
                                break;

                            case Message::CommercialDistrict_2:
                                mapGameVars.commercialDistrictMessage = Message::CommercialDistrict_3;
                                break;

                            default:
                                mapGameVars.commercialDistrictMessage = Message::CommercialDistrict_1;
                                break;

                        }

                        mapGameVars.setMessage(mapGameVars.commercialDistrictMessage);

                    }

                }
                
                break;

            case MapMenu::SlumsDistrict:

                if (mapGameVars.currentLocation == MapMenu::SlumsDistrict) {

                    if ((justPressed & UP_BUTTON) && mapGameVars.menu_Index > 0) {
                        mapGameVars.menu_Index--;
                    }

                    else if ((justPressed & DOWN_BUTTON) && mapGameVars.menu_Index < 1) {
                        mapGameVars.menu_Index++;
                    }

                    else if (justPressed & A_BUTTON) {

                        switch (mapGameVars.menu_Index) {

                            case 0:
                                if (player.getDollars() < mapGameVars.threatPrice) {

                                    #ifdef ARDUBOY_TONES_SOUNDS
                                    sound.tones(Sounds::InvalidChoice);
                                    #endif

                                }
                                else {
                                    player.decDollars(mapGameVars.threatPrice);
                                    mapGameVars.threatPrice = 40 + (random(0, 5) * 5);
                                    player.decThreatLevel(10);
                                }
                                break;

                            case 1:
                                mapGameVars.menu_Index = Constants::Map_Closed;
                                break;

                        }

                    }

                }
                else {

                    if (justPressed & A_BUTTON) {
                    
                        gameState = GameState::Message_Move_To_SlumsDistrict_Init;
                        mapGameVars.nextLocation = MapMenu::SlumsDistrict;

                        switch (mapGameVars.slumsDistrictMessage) {

                            case Message::SlumsDistrict_1:
                                mapGameVars.slumsDistrictMessage = Message::SlumsDistrict_2;
                                break;

                            case Message::SlumsDistrict_2:
                                mapGameVars.slumsDistrictMessage = Message::SlumsDistrict_3;
                                break;

                            default:
                                mapGameVars.slumsDistrictMessage = Message::SlumsDistrict_1;
                                break;

                        }

                        mapGameVars.setMessage(mapGameVars.slumsDistrictMessage);

                    }

                }
                
                break;
                
            case MapMenu::SafeHouse:

                if (mapGameVars.currentLocation == MapMenu::SafeHouse) {
                    
                    bool gameSaved = EEPROM_Utils::isGameSaved();

                    if ((justPressed & UP_BUTTON) && mapGameVars.menu_Index > 0) {
                        mapGameVars.menu_Index--;
                    }

                    else if ((justPressed & DOWN_BUTTON) && mapGameVars.menu_Index < (gameSaved ? 3 : 2)) {
                        mapGameVars.menu_Index++;
                    }

                    else if (justPressed & A_BUTTON) {

                        switch (mapGameVars.menu_Index) {

                            case 0:
                                {
                                    uint8_t idx = player.getRandomPuzzle(false);

                                    switch (idx) {

                                        case Constants::Hack_None_Available:
                                            mapGameVars.hackingMessage = Message::Hack_None_Available;
                                            break;

                                        default:

                                            switch (mapGameVars.hackingMessage) {

                                                case Message::Hacking_1:
                                                    mapGameVars.hackingMessage = Message::Hacking_2;
                                                    break;

                                                case Message::Hacking_2:
                                                    mapGameVars.hackingMessage = Message::Hacking_3;
                                                    break;

                                                default:
                                                    mapGameVars.hackingMessage = Message::Hacking_1;
                                                    break;

                                            }

                                            break;

                                    }

                                    gameState = GameState::Message_Move_To_Hacking;
                                    mapGameVars.setMessage(mapGameVars.hackingMessage);

                                }
                                break;

                            case 1:
                                mapGameVars.menu = MapMenu::CyberDeck;
                                break;

                            case 2:
                                EEPROM_Utils::saveGame(player);
                                #ifdef ARDUBOY_TONES_SOUNDS
                                sound.tones(Sounds::LoadSaveGame);
                                #endif
                                break;

                            case 3:
                                EEPROM_Utils::loadGame(player);
                                #ifdef ARDUBOY_TONES_SOUNDS
                                sound.tones(Sounds::LoadSaveGame);
                                #endif
                                break;

                        }

                    }

                }
                else {

                    if (justPressed & A_BUTTON) {
                    
                        gameState = GameState::Message_Move_To_SafeHouse;
                        mapGameVars.nextLocation = MapMenu::SafeHouse;

                        switch (mapGameVars.safeHouseMessage) {

                            case Message::SafeHouse_1:
                                mapGameVars.safeHouseMessage = Message::SafeHouse_2;
                                break;

                            case Message::SafeHouse_2:
                                mapGameVars.safeHouseMessage = Message::SafeHouse_3;
                                break;

                            default:
                                mapGameVars.safeHouseMessage = Message::SafeHouse_1;
                                break;

                        }

                        mapGameVars.setMessage(mapGameVars.safeHouseMessage);

                    }

                }  

                break;


            case MapMenu::CyberDeck:

                if (justPressed & UP_BUTTON) {
                    player.decDeckIndex();
                }

                else if (justPressed & DOWN_BUTTON) {
                    player.incDeckIndex();
                }

                else if (justPressed & A_BUTTON) {
                    mapGameVars.menu = MapMenu::SafeHouse;
                }

                break;

        }

    }

    renderDistricts();

    switch (mapGameVars.menu) {

        case MapMenu::CommercialDistrict:

            if (arduboy.getFrameCountHalf(48)) Sprites::drawOverwrite(61, 13, Images::MapMenu_Arrow, 0);

            if (mapGameVars.menu_Index != Constants::Map_Closed) {

                if (mapGameVars.currentLocation != MapMenu::CommercialDistrict) {

                    renderPopup_Resizable(45, 20, 25, 53);
                    Sprites::drawSelfMasked(49, 25 + (mapGameVars.menu_Index * 6), Images::Pointer, 0);
                    print(font3x5, Words::Move_Here);

                }
                else {

                    renderPopup_Large(39, 20, false);
                    font3x5.setCursor(39 + 9, 20 + 3);

                    for (uint8_t i = 1; i < Constants::Max_Deck_Count; i++){
                        renderCommercialMenuOption(!player.getDeck(i), Words::No_Deck + i, player.getDeckPrice(i));
                    }
                    
                    font3x5.setCursor(39 + 8, 20 + 25);
                    font3x5.print(F("CYBERDECKS"));

                    if (player.getAvailDeckCount() > 0) Sprites::drawSelfMasked(43, 25 + (mapGameVars.menu_Index * 6), Images::Pointer, 0);

                }

            }
            break;

        case MapMenu::SlumsDistrict:

            if (arduboy.getFrameCountHalf(48)) Sprites::drawOverwrite(87, 54, Images::MapMenu_Arrow, 0);

            if (mapGameVars.menu_Index != Constants::Map_Closed) {

                if (mapGameVars.currentLocation != MapMenu::SlumsDistrict) {

                    renderPopup_Resizable(17, 3, 25, 53);
                    Sprites::drawSelfMasked(21, 8 + (mapGameVars.menu_Index * 6), Images::Pointer, 0);
                    print(font3x5, Words::Move_Here);
                    mapGameVars.menu_Index = 0;

                }
                else {

                    renderPopup_Large(3, 3, true);
                    font3x5.setCursor(7, 7);
                    font3x5.print(F("LOWER~THREAT~10%\n~ YES ("));
                    font3x5.print(mapGameVars.threatPrice);
                    font3x5.print(F(")\n~ NO"));
                    font3x5.setCursor(24, 28);
                    font3x5.print(F("TERRY B."));

                    Sprites::drawSelfMasked(7, 16 + (mapGameVars.menu_Index * 6), Images::Pointer, 0);

                }

            }
            break;
            
        case MapMenu::SafeHouse:
        case MapMenu::CyberDeck:

            if (arduboy.getFrameCountHalf(48)) Sprites::drawOverwrite(91, 15, Images::MapMenu_Arrow, 0);

            if (mapGameVars.menu_Index != Constants::Map_Closed) {

                if (mapGameVars.currentLocation != MapMenu::SafeHouse) {

                    renderPopup_Resizable(36, 3, 26, 53);
                    print(font3x5, Words::Move_Here);

                }
                else {

                    bool gameSaved = EEPROM_Utils::isGameSaved();

                    renderPopup_Resizable(36, 3, 26 + (gameSaved ? 6 : 0), 53);
                    println(font3x5, Words::Safe_House_Opts);

                    if (gameSaved) {

                        font3x5.print(F("LOAD"));

                    }

                }

                if (mapGameVars.menu == MapMenu::SafeHouse) {

                    Sprites::drawSelfMasked(41, 8 + (mapGameVars.menu_Index * 6), Images::Pointer, 0);

                }
                else {

                    renderPopup_Resizable(45, 18, player.getAvailDeckCount() > 0 ? 25 : 31, 62);
                    for (uint8_t i = 0; i < Constants::Max_Deck_Count; i++) {
                        renderDeckName(i);
                    }

                    Sprites::drawSelfMasked(49, 23 + (player.getDeckIndex_ForDisplay() * 6), Images::Pointer, 0);

                }

            }
            break;
            
    }

    Sprites::drawOverwrite(1, 50, Images::MapMenu_Legend, 0);
    font3x5.setCursor(10, 50);
    font3x5.print(player.getDollars());
    font3x5.setCursor(10, 57);
    font3x5.print(player.getThreatLevel());
    font3x5.print("%");

}

void renderDeckName(uint8_t deckIdx) {

    if (player.getDeck(deckIdx)) {

        print(font3x5, Words::No_Deck + deckIdx);
        if (player.getHackSuccess(deckIdx) < 10)     font3x5.print(" ");
        if (player.getHackSuccess_Max(deckIdx) < 10) font3x5.print(" ");
        font3x5.print(player.getHackSuccess(deckIdx));
        font3x5.print("/");
        font3x5.println(player.getHackSuccess_Max(deckIdx));

    }

}

void renderPopup_Resizable(const uint8_t x, const uint8_t y, uint8_t height, uint8_t width) {

    arduboy.fillRect(x, y, width, height, BLACK);
    arduboy.drawFastHLine(x + 2, y + 1, width - 1);
    arduboy.drawFastHLine(x + 2, y + height - 2, width);
    Sprites::drawOverwrite(x, y + (height - 25), Images::MapMenu_L, 0);
    Sprites::drawOverwrite(x + width, y + (height - 25), Images::MapMenu_R, 0);
    arduboy.drawFastVLine(x + 1, y + 1, (height - 25));
    arduboy.drawFastVLine(x + width, y + 1, (height - 25));

    font3x5.setCursor(x+ 10, y + 3);

}

void renderPopup_Large(const uint8_t x, const uint8_t y, bool large) {

    arduboy.fillRect(x, y, 90 + (large ? 13 : 0), 37, BLACK);
    arduboy.drawFastHLine(x + 2, y + 1, 87 + (large ? 13 : 0));
    arduboy.drawFastHLine(x + 2, y + 23, 53 + (large ? 13 : 0));
    arduboy.drawFastHLine(x + 2, y + 33, 53 + (large ? 13 : 0));
    arduboy.drawFastHLine(x + 2, y + 35, 87 + (large ? 13 : 0));
    Sprites::drawOverwrite(x + 1, y + 1, Images::MapMenu_Lrg_L, 0);
    Sprites::drawOverwrite(x + 54 + (large ? 13 : 0), y + 1, Images::MapMenu_Lrg_M, 0);
    Sprites::drawOverwrite(x + 88 + (large ? 13 : 0), y + 1, Images::MapMenu_Lrg_R, 0);
    Sprites::drawOverwrite(x + 61 + (large ? 13 : 0), y + 4, Images::MapMenu_Face_00, 0);

}

void renderDistricts() {

    Sprites::drawOverwrite(3, 0, Images::CommercialDist, 0);
    Sprites::drawOverwrite(34, 23, Images::SlumsDist, 0);
    Sprites::drawOverwrite(94, 16, Images::SafeHouse, 0);

    font3x5.setCursor(28, 0); //6
    font3x5.print(F("COMMERCIAL\n  DISTRICT"));
    font3x5.setCursor(60, 41);
    font3x5.print(F("THE~SLUMS\nDISTRICT"));
    font3x5.setCursor(92, 2);
    font3x5.print(F(" SAFE\nHOUSE"));

}

void renderCommercialMenuOption(bool available, uint8_t wordIdx, uint16_t value) {

    if (available) {
        print(font3x5, wordIdx);
        font3x5.print(value);
        font3x5.print(F("\n"));
    }
    else {
        print(font3x5, Words::Sold_Out);
    }

}
#include "src/utils/Arduboy2Ext.h"

constexpr char word_000[] PROGMEM = "\n";
constexpr char word_001[] PROGMEM = "I";
constexpr char word_002[] PROGMEM = "NEED";
constexpr char word_003[] PROGMEM = "TO";
constexpr char word_004[] PROGMEM = "TAKE";
constexpr char word_005[] PROGMEM = "THAT";
constexpr char word_006[] PROGMEM = "CORP";
constexpr char word_007[] PROGMEM = "DOWN";
constexpr char word_008[] PROGMEM = "AT";
constexpr char word_009[] PROGMEM = "ALL";
constexpr char word_010[] PROGMEM = "COSTS!";
#ifdef EXPLICIT_TEXT
    constexpr char word_011[] PROGMEM = "DAMN,"; // E+
#else
    constexpr char word_011[] PROGMEM = "DARN,";
#endif
constexpr char word_012[] PROGMEM = "WASN'T";
constexpr char word_013[] PROGMEM = "PREPARED";
constexpr char word_014[] PROGMEM = "BREACH";
constexpr char word_015[] PROGMEM = "THROUGH";
constexpr char word_016[] PROGMEM = "THIS.";
constexpr char word_017[] PROGMEM = "UP";
constexpr char word_018[] PROGMEM = "GOES";
constexpr char word_019[] PROGMEM = "MY";
constexpr char word_020[] PROGMEM = "THREAT";
constexpr char word_021[] PROGMEM = "LEVEL!";
constexpr char word_022[] PROGMEM = "THANK";
constexpr char word_023[] PROGMEM = "YOU";
constexpr char word_024[] PROGMEM = "FOR";
constexpr char word_025[] PROGMEM = "THE";
constexpr char word_026[] PROGMEM = "INTEL!";
constexpr char word_027[] PROGMEM = "WILL";
constexpr char word_028[] PROGMEM = "WITH";
constexpr char word_029[] PROGMEM = "YOUR";
constexpr char word_030[] PROGMEM = "OWN";
#ifdef EXPLICIT_TEXT
    constexpr char word_031[] PROGMEM = "SHIT!"; // E+
#else
    constexpr char word_031[] PROGMEM = "DATA!";
#endif
constexpr char word_032[] PROGMEM = "NOW";
constexpr char word_033[] PROGMEM = "TERRY,";
constexpr char word_034[] PROGMEM = "MAKE";
constexpr char word_035[] PROGMEM = "IT";
constexpr char word_036[] PROGMEM = "HOME.";
constexpr char word_037[] PROGMEM = "JUST";
constexpr char word_038[] PROGMEM = "INTEL";
constexpr char word_039[] PROGMEM = "AVOID";
constexpr char word_040[] PROGMEM = "NCPD'S";
constexpr char word_041[] PROGMEM = "LAMEST.";
constexpr char word_042[] PROGMEM = "HACK";
constexpr char word_043[] PROGMEM = "THEIR";
constexpr char word_044[] PROGMEM = "SYSTEMS";
constexpr char word_045[] PROGMEM = "MORE";
constexpr char word_046[] PROGMEM = "WHAT";
#ifdef EXPLICIT_TEXT
    constexpr char word_047[] PROGMEM = "HELL,"; // E+
#else
    constexpr char word_047[] PROGMEM = "HECK,";
#endif
constexpr char word_048[] PROGMEM = "CAN'T";
constexpr char word_049[] PROGMEM = "EVEN";
constexpr char word_050[] PROGMEM = "GET";
constexpr char word_051[] PROGMEM = "HOME";
constexpr char word_052[] PROGMEM = "WITHOUT";
constexpr char word_053[] PROGMEM = "A";
constexpr char word_054[] PROGMEM = "FIGHT!";
constexpr char word_055[] PROGMEM = "LET'S";
constexpr char word_056[] PROGMEM = "HIT";
constexpr char word_057[] PROGMEM = "WHERE";
constexpr char word_058[] PROGMEM = "HURTS.";
constexpr char word_059[] PROGMEM = "HOW";
constexpr char word_060[] PROGMEM = "MUCH";
constexpr char word_061[] PROGMEM = "UNCOVER";
constexpr char word_062[] PROGMEM = "TODAY.";
constexpr char word_063[] PROGMEM = "SELL";
constexpr char word_064[] PROGMEM = "THEM";
constexpr char word_065[] PROGMEM = "DOWN.";
constexpr char word_066[] PROGMEM = "HE";
constexpr char word_067[] PROGMEM = "CAN";
constexpr char word_068[] PROGMEM = "CLEAR";
constexpr char word_069[] PROGMEM = "NAME...";
constexpr char word_070[] PROGMEM = "PRICE.";
constexpr char word_071[] PROGMEM = "WHEN";
constexpr char word_072[] PROGMEM = "PAYING";
constexpr char word_073[] PROGMEM = "RIGHT";
constexpr char word_074[] PROGMEM = "PEOPLE";
constexpr char word_075[] PROGMEM = "LOWER";
constexpr char word_076[] PROGMEM = "LEVEL";
constexpr char word_077[] PROGMEM = "HERE.";
constexpr char word_078[] PROGMEM = "HOPE";
constexpr char word_079[] PROGMEM = "FIND";
constexpr char word_080[] PROGMEM = "CONTACT";
constexpr char word_081[] PROGMEM = "HELP";
constexpr char word_082[] PROGMEM = "ME";
constexpr char word_083[] PROGMEM = "OUT.";
constexpr char word_084[] PROGMEM = ""; // Unused
constexpr char word_085[] PROGMEM = "SHOPS.";
constexpr char word_086[] PROGMEM = "SOME";
constexpr char word_087[] PROGMEM = "UPGRADES.";
constexpr char word_088[] PROGMEM = "THERE";
constexpr char word_089[] PROGMEM = "GETTING";
constexpr char word_090[] PROGMEM = "CAUGHT.";
constexpr char word_091[] PROGMEM = "IF";
constexpr char word_092[] PROGMEM = "AM";
constexpr char word_093[] PROGMEM = "STUCK,";
constexpr char word_094[] PROGMEM = "REMOTE";
constexpr char word_095[] PROGMEM = "HACKING";
constexpr char word_096[] PROGMEM = "MIGHT";
constexpr char word_097[] PROGMEM = "THOSE";
constexpr char word_098[] PROGMEM = "CORPO";
#ifdef EXPLICIT_TEXT
    constexpr char word_099[] PROGMEM = "BASTARDS"; // E+
#else
    constexpr char word_099[] PROGMEM = "CROOKS";
#endif
constexpr char word_100[] PROGMEM = "PAY";
constexpr char word_101[] PROGMEM = "THEY";
constexpr char word_102[] PROGMEM = "HAVE";
constexpr char word_103[] PROGMEM = "DONE.";
constexpr char word_104[] PROGMEM = "BY";
constexpr char word_105[] PROGMEM = "SERVERS";
constexpr char word_106[] PROGMEM = "AND";
constexpr char word_107[] PROGMEM = "GOOD.";
constexpr char word_108[] PROGMEM = "ALLOW";
constexpr char word_109[] PROGMEM = "FURTHER.";
constexpr char word_110[] PROGMEM = "ONCE";
constexpr char word_111[] PROGMEM = "HACKS";
constexpr char word_112[] PROGMEM = "BEEN";
constexpr char word_113[] PROGMEM = "COMPLETED.";
constexpr char word_114[] PROGMEM = "PURCHASE";
constexpr char word_115[] PROGMEM = "OR";
constexpr char word_116[] PROGMEM = "SWAP";
constexpr char word_117[] PROGMEM = "CONTINUE";
constexpr char word_118[] PROGMEM = "CANNOT";
constexpr char word_119[] PROGMEM = "AFFORD";
constexpr char word_120[] PROGMEM = "ANY";
constexpr char word_121[] PROGMEM = "UNFORTUNATELY,";
constexpr char word_122[] PROGMEM = "TIME";
constexpr char word_123[] PROGMEM = "HERE";
constexpr char word_124[] PROGMEM = "IS";
constexpr char word_125[] PROGMEM = "OVER!";
constexpr char word_126[] PROGMEM = "CONGRATULATIONS!";
constexpr char word_127[] PROGMEM = "TOOK";
constexpr char word_128[] PROGMEM = "BALANCE";
constexpr char word_129[] PROGMEM = "OF";

constexpr char word_130[] PROGMEM = "NO~DECK ";
constexpr char word_131[] PROGMEM = "DECK~V1 ";
constexpr char word_132[] PROGMEM = "DECK~V2 ";
constexpr char word_133[] PROGMEM = "DECK~V3 ";
constexpr char word_134[] PROGMEM = "HACKING\nCYBERDECKS\nSAVE";
constexpr char word_135[] PROGMEM = "MOVE HERE";
constexpr char word_136[] PROGMEM = "-~SOLD~OUT~-\n";

#ifdef CREDITS

constexpr char word_137[] PROGMEM = " CYBERHACK: PPOT~2021";
constexpr char word_138[] PROGMEM = "  DEVELOPER:~FILMOTE";
constexpr char word_139[] PROGMEM = " ~GRAPHICS:~VAMPIRICS ";
constexpr char word_140[] PROGMEM = "TESTING:~TUXINATOR2009";
constexpr char word_141[] PROGMEM = "MORAL~SUPPORT:~JESSICA";

#endif

constexpr const char * words[] PROGMEM {
	word_000, word_001, word_002, word_003, word_004, word_005, word_006, word_007, 
	word_008, word_009, word_010, word_011,	word_012, word_013, word_014, word_015,
	word_016, word_017, word_018, word_019,	word_020, word_021, word_022, word_023,
	word_024, word_025, word_026, word_027,	word_028, word_029, word_030, word_031,
	word_032, word_033, word_034, word_035,	word_036, word_037, word_038, word_039,
	word_040, word_041, word_042, word_043,	word_044, word_045, word_046, word_047,
	word_048, word_049, word_050, word_051,	word_052, word_053, word_054, word_055,
	word_056, word_057, word_058, word_059,	word_060, word_061, word_062, word_063,
	word_064, word_065, word_066, word_067,	word_068, word_069, word_070, word_071,
    word_072, word_073, word_074, word_075, word_076, word_077, word_078, word_079, 
    word_080, word_081, word_082, word_083, word_084, word_085, word_086, word_087, 
    word_088, word_089, word_090, word_091, word_092, word_093, word_094, word_095, 
    word_096, word_097, word_098, word_099, word_100, word_101, word_102, word_103, 
    word_104, word_105, word_106, word_107, word_108, word_109, word_110, word_111, 
    word_112, word_113, word_114, word_115, word_116, word_117, word_118, word_119, 
    word_120, word_121, word_122, word_123, word_124, word_125, word_126, word_127,
    word_128, word_129, word_130, word_131, word_132, word_133, word_134, word_135,
    word_136,

    #ifdef CREDITS
    word_137, word_138, word_139, word_140, word_141, 
    #endif

};

constexpr uint8_t sentence_00_Length = 11;
constexpr uint8_t sentence_01_Length = 13;
constexpr uint8_t sentence_02_Length = 13;
constexpr uint8_t sentence_03_Length = 12;
constexpr uint8_t sentence_04_Length = 8;
constexpr uint8_t sentence_05_Length = 7;
constexpr uint8_t sentence_06_Length = 11;
constexpr uint8_t sentence_07_Length = 16;
constexpr uint8_t sentence_08_Length = 14;
constexpr uint8_t sentence_09_Length = 11;
constexpr uint8_t sentence_10_Length = 10;
constexpr uint8_t sentence_11_Length = 12;
constexpr uint8_t sentence_12_Length = 12;
constexpr uint8_t sentence_13_Length = 13;
constexpr uint8_t sentence_14_Length = 21;
constexpr uint8_t sentence_15_Length = 15;
constexpr uint8_t sentence_16_Length = 15;
constexpr uint8_t sentence_17_Length = 16;
constexpr uint8_t sentence_18_Length = 14;
constexpr uint8_t sentence_19_Length = 12;
constexpr uint8_t sentence_20_Length = 7;
constexpr uint8_t sentence_21_Length = 16;

constexpr uint8_t sentences_Lengths[] PROGMEM {
	sentence_00_Length, sentence_01_Length, sentence_02_Length, sentence_03_Length, sentence_04_Length, sentence_05_Length, 
    sentence_06_Length, sentence_07_Length, sentence_08_Length, sentence_09_Length, sentence_10_Length, sentence_11_Length,
    sentence_12_Length, sentence_13_Length, sentence_14_Length, sentence_15_Length, sentence_16_Length, sentence_17_Length,
    sentence_18_Length, sentence_19_Length, sentence_20_Length, sentence_21_Length,  
};

constexpr uint8_t sentence_data[] PROGMEM { 
1, 2, 3, 4, 5, 0, 6, 7, 8, 9, 10,                                                       // 00 I need to take that Corp down at all costs!
32, 3, 34, 35, 36, 0, 37, 2, 3, 39, 0, 40, 41,                                          // 01 Now to make it home. Just need to avoid NCPDs lamest.
2, 3, 34, 35, 36, 0, 42, 43, 44, 24, 0, 45, 26,                                         // 02 Need to make it home, hack their system for more intel!
46, 25, 47, 48, 0, 49, 50, 51, 52, 53, 0, 54,                                           // 03 What the [hell]/[heck], can’t even get home without a fight!
55, 56, 25, 06, 57, 0, 35, 58,                                                          // 04 Let's hit the Corp where it hurts.
59, 60, 27, 1, 61, 0, 62 ,                                                              // 05 How much will I uncover today.
1, 2, 45, 38, 3, 63, 0, 3, 4, 64, 65,                                                   // 06 I need more intel to sell to take them down.
1, 2, 3, 50, 3, 33, 0, 66, 67, 68, 19, 69, 0, 24, 53, 70,                               // 07 I need to get to Terry, he can clear my name… for a price.
71, 72, 25, 73, 0, 74, 1, 67, 75, 19, 0, 20, 76, 77,                                    // 08 When paying the right people I can lower my threat level here.
78, 1, 27, 79, 5, 0, 80, 3, 81, 82, 83,                                                 // 09 Hope I will find that contact to help me out.
55, 50, 3, 25, 85, 0, 1, 2, 86, 87,                                                     // 10 Let's get to the shops. I need some upgrades.
2, 87, 37, 2, 0, 3, 50, 88, 52, 0, 89, 90 ,                                             // 11 Need upgrades. Just need to get there without getting caught.
91, 1, 92, 93, 94, 0, 95, 96, 81, 82, 0, 83,                                            // 12 If I am stuck, remote hacking might help me out.
97, 98, 99, 0, 2, 3, 100, 24, 46, 0, 101, 102, 103,                                     // 13 Those CORPO [bastards]/[crooks] need to pay for what they have done. 
104, 95, 43, 105, 0, 1, 67, 79, 38, 106, 0, 63, 35, 3, 108, 82, 3, 0, 42, 64, 109,      // 14 By hacking their servers I can find intel and sell it to allow me to hack them further.
110, 1, 102, 46, 1, 2, 1, 0, 67, 4, 64, 7, 24, 0, 107,                                  // 15 Once I have what I need I can take them down for good.
11, 1, 12, 13, 0, 3, 14, 15, 16, 0, 17, 18, 19, 20, 21,                                 // 16 [Damn]/[Darn], I wasn’t prepared to breach through this. Up goes my threat level…
22, 23, 24, 25, 26, 0, 1, 27, 4, 23, 7, 0, 28, 29, 30, 31,                              // 17 Thank you for the intel! I will take you down with your own [shit]/[data]!
9, 111, 102, 112, 0, 113, 3, 117, 0, 114, 115, 116, 0, 87,                              // 18 All hacks have been completed. To continue purchase or swap upgrades.
9, 111, 113, 0, 121, 23, 0, 118, 119, 120, 0, 87,                                       // 19 All hacks completed. Unfortunately, You cannot afford any upgrades.
121, 29, 0, 122, 123, 124, 125,                                                         // 20 Unfortunately, your time here is over. 
126, 0, 9, 111, 113, 0, 23, 127, 25, 6, 7, 0, 28, 53, 128, 129,                         // 21 Congratulations! All Hacks completed. You took the Corp down with a balance of
};


inline FlashStringHelper readFlashStringPointer(const char * const * flashString) noexcept {
	return static_cast<FlashStringHelper>(pgm_read_ptr(flashString));
}

void print(Print & printer, uint8_t wordIndex) {
	printer.print(readFlashStringPointer(&words[wordIndex]));
}

void println(Print & printer, uint8_t wordIndex) {
    print(printer, wordIndex);
    printer.print("\n");
}

void showMap_Message_Init() {

    clearLEDs();
    mapGameVars.counter = 1;
    gameState = static_cast<GameState>(static_cast<uint8_t>(gameState) + static_cast<uint8_t>(GameState::Messages_End) - static_cast<uint8_t>(GameState::Messages_Start) + 1);

    #ifdef BYTE_BEAT_SOUNDS_3
    byteBeatIdx = ByteBeatIndex::Map;
    #endif

}

void showMap_Message() {

    uint8_t y = 33;
    uint16_t startingPos = 0;
    
    renderDistricts();
    
    arduboy.fillRect(0, 27, WIDTH, HEIGHT, BLACK);
    arduboy.drawFastHLine(0, 28, WIDTH);
    arduboy.drawFastHLine(35, 30, 80);
    arduboy.drawFastVLine(35, 30, 26);
    
    Sprites::drawOverwrite(35, 54, Images::DownFade, 0);
    Sprites::drawOverwrite(116, 30, Images::RightFade2, 0);

    font3x5.setCursor(39, y);

    for (uint8_t i = 0; i < static_cast<uint8_t>(mapGameVars.getMessage()); i++) {
        startingPos = startingPos + pgm_read_byte(&sentences_Lengths[i]);
    }

    for (uint16_t i = startingPos; i < startingPos + mapGameVars.counter; i++) {

        if (pgm_read_byte(&sentence_data[i]) == 0) {

            y = y + 7;
            font3x5.setCursor(39, y);

        }
        else {

            print(font3x5, pgm_read_byte(&sentence_data[i]));
            font3x5.print(F("~"));

        }

    }

    uint8_t sentenceLength = pgm_read_byte(&sentences_Lengths[static_cast<uint8_t>(mapGameVars.getMessage())]);

    switch (mapGameVars.getMessage()) {

        case Message::Face_Player_Start ... Message::Face_Player_End:
            Sprites::drawOverwrite(0, 30, Images::Portrait_00, 0);
            if (mapGameVars.counter < sentenceLength && arduboy.getFrameCountHalf(8)) {

                Sprites::drawOverwrite(11, 47, Images::Portrait_01, 0);

            }
            break;

        default:
            Sprites::drawOverwrite(6, 32, Images::MapMenu_Face_00, 0);
            arduboy.drawFastHLine(0, 63, 36);
            if (mapGameVars.counter < sentenceLength && arduboy.getFrameCountHalf(8)) {

                Sprites::drawOverwrite(17, 49, Images::MapMenu_Face_01, 0);

            }
            break;

    }

    if (arduboy.isFrameCount(6)) {
        if (sentenceLength > mapGameVars.counter) {
            mapGameVars.counter++;
        }
    }

    if (mapGameVars.getMessage() == Message::Game_Over_Success && sentenceLength == mapGameVars.counter) {

        font3x5.setCursor(103, 54);
        font3x5.print(player.getDollars());
        font3x5.print("!");

    }

    if (arduboy.justPressed(B_BUTTON)) {

        if (mapGameVars.counter < sentenceLength) {
    
            mapGameVars.counter = sentenceLength;

        }
        else {

            handleTransition();

        }

    }

    if (arduboy.justPressed(A_BUTTON)) {

        handleTransition();

    }

}

void handleTransition() {

    switch (gameState) {

        case GameState::Message_Intro:

            switch (mapGameVars.getMessage()) {

                case Message::Intro_1:
                    mapGameVars.setMessage(Message::Intro_2);
                    break;

                case Message::Intro_2:
                    mapGameVars.setMessage(Message::Intro_3);
                    break;

                case Message::Intro_3:
                    gotoMap();
                    break;

                default: break;
                    
            }

            break;

        case GameState::Message_Hack_Target_Not_Achieved:

            if (player.getThreatLevel() == 100) {
                gameState = GameState::Message_Game_Over_Init;
                mapGameVars.setMessage(Message::Game_Over);
            }
            else {
                gotoMap();
            }
            break;

        case GameState::Message_Hacks_Achieved:
            gotoMap();
            break;

        case GameState::Message_Move_To_Hacking:

            switch (mapGameVars.getMessage()) {

                case Message::Hack_None_Available:

                    gotoMap();
                    break;

                default:
                    gameState = GameState::HackGame_Init;
                    break;
            
            }
            break;

        case GameState::Message_Game_Over:
            #ifdef BYTE_BEAT_SOUNDS    
                disableByteBeat();
            #endif
            break;

        case GameState::Message_Game_Over_Success:
            gameState = GameState::Credits_Init;
            #ifdef BYTE_BEAT_SOUNDS    
                disableByteBeat();
            #endif
            break;

        case GameState::Message_Hacks_Game_Over:
            gameState = GameState::Message_Game_Over_Init;
            mapGameVars.setMessage(Message::Game_Over);
            break;

        default:
            gameState = GameState::StealthGame_Init;
            break;

    }

}
#include "src/utils/Arduboy2Ext.h"

static constexpr uint8_t ppotFrames = (1.6 *50); // 1.6s animation loop


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void splashScreen_Init() {

    gameState = GameState::Splash;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void splashScreen() { 

    auto justPressed = arduboy.justPressedButtons();

    if (justPressed > 0) {
        
        #ifdef TV_TRANSITION
            gameState = GameState::TV_Init; 
            nextState = GameState::Title_Init; 
        #else
            gameState = GameState::Title_Init; 
        #endif

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

#include <Arduboy2.h>

const uint8_t PROGMEM offsets[] = { 0, 8, 8, 6 };

const uint8_t PROGMEM coords[] = { 
    0, 29, 17, 
    17, 12, 18,    
    17, 12, 14,
    31, 12, 14,
    31, 21, 14, 
    45, 21, 6,
    45, 27, 10,
    55, 5, 23,
    55, 5, 23,
    78, 5, 26,
    78, 26, 18,
    96, 14, 13,
    96, 14, 19,
    115, 14, 20,
};


void stealthGame_Init() {
                
    gameState = GameState::StealthGame;
    stealthGameVars.reset();
    player.resetStealth();

    uint8_t max = BUILDING_COUNT / 2;
    uint16_t x = 10;

    for (uint8_t i = 0; i < max; i++) {

        BuildingType buildingType = static_cast<BuildingType>(random(static_cast<uint8_t>(BuildingType::Min), static_cast<uint8_t>(BuildingType::Max) + 1));
        Building &building = stealthGameVars.buildings[i];
        building.type = buildingType;
        building.x = x;

        x = x + random(BUILDING_LOW_WIDTH + 10, BUILDING_LOW_WIDTH + 80);

    }

    x = 30;

    for (uint8_t i = max; i < BUILDING_COUNT; i++) {

        Building &building = stealthGameVars.buildings[i];
        building.type = BuildingType::Terminal;
        building.x = x;

        x = x + random(100, 300);

    }

    uint16_t span = -(Constants::Town_RightPos) / Constants::Max_Guards;
    uint16_t walkSpan = span + 60;

    for (uint8_t i = 0; i < Constants::Max_Guards; i++) {

        Guard &guard = stealthGameVars.guards[i];
        guard.setSpan(walkSpan);
        guard.setMovement(i % 2 == 0 ? Movement::WalkingRight : Movement::WalkingLeft);
        guard.setX(70 + (span / 2) + (span * i));
        guard.setSteps(random((walkSpan / 4), (walkSpan / 2)));
        guard.setSteps(random((walkSpan / 4), (walkSpan / 2)));
        guard.setAlerted(false);

    }

    #ifdef BYTE_BEAT_SOUNDS_3
    byteBeatIdx = ByteBeatIndex::Street;
    #endif

}

void stealthGame() {

	auto justPressed = arduboy.justPressedButtons();
	auto pressed = arduboy.pressedButtons();


    //SJH
    // if (arduboy.justPressed(B_BUTTON)) {

    //     gameState = GameState::ShowMap;
    //     mapGameVars.currentLocation = mapGameVars.nextLocation;
    //     // arduboy.invert(false); 

    //     if (mapGameVars.currentLocation == MapMenu::CommercialDistrict) {
    //         mapGameVars.menu_Index = player.getFirstAvailDeck(false) - 1;
    //     }

    // }


    uint16_t distToGuard = 512;

    arduboy.invert(false); 
    
    if (player.getHealth() > 0 && (!player.getShowReticle() || arduboy.getFrameCount(4) == 0)) { 

        if (arduboy.isFrameCount(128)) {

            if (player.incMemory()) {
                #ifdef ARDUBOY_TONES_SOUNDS
                sound.tones(Sounds::MemoryFull);
                #endif
            }

        }

        if (arduboy.isFrameCount(64)) {

            player.incHealth();

        }


        if (arduboy.isFrameCount(2)) {


            // Close doors ..

            for (uint8_t i = 0; i < BUILDING_COUNT; i++) {

                bool closing = stealthGameVars.buildings[i].incDoor();

                if (closing) {
                    player.setMovement(Movement::IdleRight);
                }

            }


            // Move background if necessary ..

            if (stealthGameVars.moveOffset < 0) {

                stealthGameVars.world.inc(0);
                stealthGameVars.moveOffset++;

            }
            else if (stealthGameVars.moveOffset > 0) {

                stealthGameVars.world.dec(0);
                stealthGameVars.moveOffset--;

            }

        }


        // Handle guard movements ..

        for (uint8_t i = 0; i < Constants::Max_Guards; i++) {
            
            Guard &guard = stealthGameVars.guards[i];

            int16_t guardXPos = guard.getX() + stealthGameVars.world.buildingXPos;
            int16_t distToGuard_Test = guardXPos - player.getX();

            if (abs(distToGuard_Test) < distToGuard && !guard.isDazedAndConfused()) {

                distToGuard = abs(distToGuard_Test);

            }

            if (distToGuard_Test <= Constants::Dist_To_Torch) {

                if (!player.isHiddenOrCrouching()) {

                    if (!guard.isDazedAndConfused()) guard.setAlerted(true);

                }

            }

            guard.move(-distToGuard_Test);


        }

    }

    if (distToGuard <= Constants::Dist_To_Torch && !player.isHiddenOrCrouching()) {

        player.decHealth();

    }


    // Handle player movements (if not mission over!)..

    if (player.getHealth() > 0) {
            
        if (justPressed & A_BUTTON || player.getShowReticle()) {

            if (player.getMemory() >= 3) {

                switch (player.getMovement()) {

                    case Movement::WalkingLeft:
                        player.setMovement(Movement::IdleLeft);
                        break;

                    case Movement::WalkingRight:
                        player.setMovement(Movement::IdleRight);
                        break;

                    default: break;
                        
                }

                if (!player.getShowReticle()) {
                    player.setShowReticle(true);
                    arduboy.invert(true);
                    justPressed = 0;
                    #ifdef ARDUBOY_TONES_SOUNDS
                    sound.tones(Sounds::ReticleShown);
                    #endif
                }

                if (justPressed & LEFT_BUTTON) {
                    if (stealthGameVars.reticleTargetIdx > 0) {
                        stealthGameVars.reticleTargetIdx--;
                    }
                    else {
                        stealthGameVars.reticleTargetIdx = stealthGameVars.reticleTargetCount - 1;
                    }
                }

                else if (justPressed & RIGHT_BUTTON) {
                    if (stealthGameVars.reticleTargetIdx < stealthGameVars.reticleTargetCount - 1) {
                        stealthGameVars.reticleTargetIdx++;
                    }
                    else {
                        stealthGameVars.reticleTargetIdx = 0;
                    }
                }

                if (justPressed & A_BUTTON) {
                    
                    arduboy.invert(true);
                    player.decMemory(3);
                    player.setShowReticle(false);
                    
                    ReticleTarget &reticleTarget = stealthGameVars.getReticleTarget(stealthGameVars.reticleTargetIdx);

                    if (reticleTarget.type == TargetType::Door) {

                        stealthGameVars.buildings[reticleTarget.buildingIdx].door = 1;

                    }

                    if (reticleTarget.type == TargetType::LightPole) {

                        stealthGameVars.questionCount = 255;
                        stealthGameVars.lampPoleIdx = reticleTarget.buildingIdx;

                        for (uint8_t i = 0; i < Constants::Max_Guards; i++) {

                            Guard &guard = stealthGameVars.guards[i];
                            int16_t guardXPos = guard.getX() + stealthGameVars.world.buildingXPos;

                            if (guardXPos >= -30 && guardXPos < WIDTH + 30) {

                                if (guardXPos < reticleTarget.x) {

                                    guard.setMovement(Movement::LookingUpRight);

                                }
                                else {

                                    guard.setMovement(Movement::LookingUpLeft);

                                }

                                guard.setSteps(255);

                            }

                        }

                    }

                    #ifdef ARDUBOY_TONES_SOUNDS
                        sound.tones(Sounds::ReticleSelect);
                    #endif


                }

                if (justPressed & B_BUTTON) {
                    
                    arduboy.invert(true);
                    player.setShowReticle(false);

                }

            }
            else {

                #ifdef ARDUBOY_TONES_SOUNDS
                    sound.tones(Sounds::InvalidChoice);
                #endif


            }

        }
        else {

            player.setShowReticle(false);
                
            if (justPressed & LEFT_BUTTON) {
                player.setMovement(Movement::IdleLeft);
            }

            else if (pressed & LEFT_BUTTON) {

                player.setMovement(Movement::WalkingLeft);

                if (player.getX() > Constants::Player_CentrePos) {

                    player.decX();

                }
                else if (stealthGameVars.world.buildingXPos < 130) {

                    stealthGameVars.world.inc(arduboy.getFrameCount(2));

                }
                else if (stealthGameVars.world.buildingXPos == 130) {

                    if (player.getX() > 2) {
                        player.decX();
                    }

                }

            }

            else if (player.getMovement() == Movement::WalkingLeft) {

                player.setMovement(Movement::IdleLeft);

            }
            


            // Right ..

            if (justPressed & RIGHT_BUTTON) {

                player.setMovement(Movement::IdleRight);

            }
            else if (pressed & RIGHT_BUTTON) {

                player.setMovement(Movement::WalkingRight);

                if (player.getX() < Constants::Player_CentrePos) {

                    player.incX();

                }
                else {

                    if (stealthGameVars.world.buildingXPos > Constants::Town_RightPos) {
                    
                        stealthGameVars.world.dec(arduboy.getFrameCount(2));

                    }
                    else {

                        if (player.getX() < 128) {
                            player.incX();
                        }
                        else {

                            #ifdef TV_TRANSITION
                                gotoMapNextState(GameState::TV_Init);
                            #else
                                gotoMap();
                            #endif
                            mapGameVars.currentLocation = mapGameVars.nextLocation;


                            // Prime menu index if moving to commerical district ..

                            if (mapGameVars.currentLocation == MapMenu::CommercialDistrict) {
                                mapGameVars.menu_Index = player.getFirstAvailDeck(false) - 1;
                            }


                        }

                    }

                }

            }

            else if (player.getMovement() == Movement::WalkingRight) {

                player.setMovement(Movement::IdleRight);

            }


            // Up button ..

            if (justPressed & UP_BUTTON) {

                int16_t playerXPos = player.getX() - stealthGameVars.world.buildingXPos;
                for (uint8_t i = 0; i < BUILDING_COUNT / 2; i++) {

                    Building &building = stealthGameVars.buildings[i];

                    switch (building.type) {

                        case BuildingType::LowBuilding:
                        case BuildingType::MedBuilding:
                        case BuildingType::HighBuilding:

                            if (building.x - 2 < playerXPos && building.x + 12 > playerXPos && building.getDoorOpen()) {

                                player.setMovement(Movement::Hidden);
                                clearGuardAlerts();
                                stealthGameVars.moveOffset = building.x - playerXPos + pgm_read_byte(&offsets[static_cast<uint8_t>(building.type)]);

                            }
                            break;

                        default: break;

                    }

                }

            }

            // else if (!(pressed & UP_BUTTON)) {

            //     player.setMovement(Movement::IdleRight);

            // }


            // Down button ..

            if (justPressed & DOWN_BUTTON) {

                int16_t playerXPos = player.getX() - stealthGameVars.world.foregroundXPos;

                for (uint8_t i = BUILDING_COUNT / 2; i < BUILDING_COUNT; i++) {
                    
                    Building &building = stealthGameVars.buildings[i];

                    if (building.x - 20 < playerXPos && building.x - 10 > playerXPos) {

                        player.setMovement(Movement::CrouchRight);
                        stealthGameVars.moveOffset = (building.x - playerXPos - 12) / 2;
                        clearGuardAlerts();

                    }

                    else if ((building.x + BUILDING_TERMINAL_WIDTH - 5 < playerXPos) && (building.x + BUILDING_TERMINAL_WIDTH + 5 > playerXPos)) {

                        player.setMovement(Movement::CrouchLeft);
                        stealthGameVars.moveOffset = (building.x - playerXPos + 10) / 2;
                        clearGuardAlerts();

                    }                
        
                }

            }

            // else if (!(pressed & DOWN_BUTTON)) {

            //     switch (player.getMovement()) {

            //         case Movement::CrouchLeft:
            //             player.setMovement(Movement::IdleLeft);
            //             break;

            //         case Movement::CrouchRight:
            //             player.setMovement(Movement::IdleRight);
            //             break;

            //         default: break;

            //     }

            // }

        }

    }

    // Mission over ..

    else {
                    
        if (justPressed & A_BUTTON) {

            switch (mapGameVars.nextLocation) {

                case MapMenu::CommercialDistrict:
                    player.incThreatLevel(10);
                    break;

                default:
                    player.incThreatLevel(5);
                    break;

            }

            if (player.getThreatLevel() == 100) {
                #ifdef TV_TRANSITION
                    mapGameVars.setMessage(Message::Game_Over);
                    gotoMapNextState(GameState::TV_Init);
                    nextState = GameState::Message_Game_Over_Init;
                #else
                    mapGameVars.setMessage(Message::Game_Over);
                    gameState = GameState::Message_Game_Over_Init;
                #endif
            }
            else {
                #ifdef TV_TRANSITION
                    gotoMapNextState(GameState::TV_Init);
                #else
                    gotoMap();
                #endif
            }

        }

    }

    stealthGameVars.resetReticleTargets();

    {
        #define SPACING_BACKGROUND 116
        int16_t x = stealthGameVars.world.backgroundXPos % SPACING_BACKGROUND;
        
        renderBackground(x - SPACING_BACKGROUND);
        renderBackground(x);
        renderBackground(x + SPACING_BACKGROUND);

    }

    for (uint8_t i = 0; i < BUILDING_COUNT / 2; i++)  {

        Building building = stealthGameVars.buildings[i];
        int16_t xPos = building.x + stealthGameVars.world.buildingXPos;

        switch (building.type) {

            case BuildingType::LowBuilding:

                if (xPos >= - BUILDING_LOW_WIDTH && xPos <= WIDTH) {
                    renderLowBuilding(xPos, building.getDoorFrame());
                }

                if (xPos >= - BUILDING_LOW_DOOR_X - (BUILDING_DOOR_WIDTH / 2) && xPos <= WIDTH - BUILDING_LOW_DOOR_X - (BUILDING_DOOR_WIDTH / 2)) {

                    createReticleTarget(i, xPos + BUILDING_LOW_DOOR_X - 1);

                }

                break;

            case BuildingType::MedBuilding:

                if (xPos >= - BUILDING_MED_WIDTH && xPos <= WIDTH) {
                    renderMedBuilding(xPos, building.getDoorFrame());
                }

                if (xPos >= - BUILDING_MED_DOOR_X - (BUILDING_DOOR_WIDTH / 2) && xPos <= WIDTH - BUILDING_MED_DOOR_X - (BUILDING_DOOR_WIDTH / 2)) {

                    createReticleTarget(i, xPos + BUILDING_MED_DOOR_X - 1);

                }

                break;

            case BuildingType::HighBuilding:

                if (xPos >= - BUILDING_HIGH_WIDTH && xPos <= WIDTH) {
                    renderTallBuilding(xPos, building.getDoorFrame());
                }

                if (xPos >= - BUILDING_HIGH_DOOR_X - (BUILDING_DOOR_WIDTH / 2) && xPos <= WIDTH - BUILDING_HIGH_DOOR_X - (BUILDING_DOOR_WIDTH / 2)) {

                    createReticleTarget(i, xPos + BUILDING_HIGH_DOOR_X - 1);

                }

                break;

            default: break;

        }

    }


    {
        #define SPACING_FOREGROUND 243
        int16_t x = (stealthGameVars.world.foregroundXPos % SPACING_FOREGROUND) - SPACING_FOREGROUND;
        renderForeground(x);
        
    }


    renderGauges();


    // Player + Guards

    for (uint8_t i = 0; i < Constants::Max_Guards; i++) {
        renderGuard(stealthGameVars.guards[i]);
    }

    renderPlayer();
    renderTerminals();


    // If the reticle is being displayed ..

    if (player.getShowReticle() && arduboy.getFrameCountHalf(16)) {

        ReticleTarget &reticleTarget = stealthGameVars.getReticleTarget(stealthGameVars.reticleTargetIdx);
        Sprites::drawPlusMask(reticleTarget.x, reticleTarget.y, Images::Reticle, 0);

    }


    // Mission over?

    if (player.getHealth() == 0) {

        arduboy.fillRect(16, 20, 96, 19, BLACK);
        arduboy.drawRect(17, 21, 94, 17, WHITE);
        font3x5.setCursor(22, 26);
        font3x5.print(F("YOU~HAVE~BEEN~CAPTURED!"));

    }
    
    #ifdef USE_LED_ANALOGUE
        if (distToGuard < 48) {
            arduboy.setRGBled(RED_LED, 48 - distToGuard);
        }
        else {
            arduboy.setRGBled(RED_LED, 0);
        }
    #endif

}

void clearGuardAlerts() {

    for (uint8_t i = 0; i < Constants::Max_Guards; i++) {
        
        Guard &guard = stealthGameVars.guards[i];
        guard.setAlerted(false);

    }

}

void createReticleTarget(uint8_t idx, int16_t x) {

    ReticleTarget &target = stealthGameVars.getReticleTarget(stealthGameVars.reticleTargetCount);
    target.x = x;
    target.y = 36;
    target.buildingIdx = idx;
    target.type = TargetType::Door;
    stealthGameVars.reticleTargetCount++;

}

void gotoMap() {

    gameState = GameState::ShowMap;

    #ifdef BYTE_BEAT_SOUNDS_3
    byteBeatIdx = ByteBeatIndex::Map;
    #endif

}

void gotoMapNextState(GameState next) {

    gameState = next;
    nextState = GameState::ShowMap;

    #ifdef BYTE_BEAT_SOUNDS_3
    byteBeatIdx = ByteBeatIndex::Map;
    #endif

}
#include <Arduboy2.h>


void renderPlayer() {

    uint8_t frame = player.getHealth() > 0 ? (arduboy.getFrameCount(24) / 6) : 0;
    uint8_t headImage = 255;
    uint8_t y = 36;

    switch (player.getMovement()) {

        case Movement::IdleLeft:
            frame = 0;
            /*-fallthrough*/

        case Movement::WalkingLeft:
            headImage = 0;
            break;

        case Movement::IdleRight:
            frame = 0;
            /*-fallthrough*/

        case Movement::WalkingRight:
            headImage = 1;
            frame = frame + 4;
            break;

        case Movement::CrouchLeft:
        case Movement::CrouchRight:
            frame = (player.getMovement() == Movement::CrouchRight);
            Sprites::drawPlusMask(player.getX(), 37, Images::Player_Crouch, frame);
            frame = 255;
            break;

        case Movement::Hidden:
            Sprites::drawSelfMasked(player.getX(), 37, Images::Player_Hidden, 0);
            frame = 255;
            break;

        default: break;

    }

    if (frame < 255) {

        Sprites::drawPlusMask(player.getX() + 1, y + (frame == 1 ? 1 : 0), Images::Player_Head, headImage);
        Sprites::drawPlusMask(player.getX(), y + 9, Images::Common, frame);

    }

}

void renderGuard(Guard &guard) {

    uint8_t frame = 0;
    uint8_t headImage = 255;

    int16_t xPosition = guard.getX() + stealthGameVars.world.buildingXPos;

    if (player.getHealth() > 0) {

        if (!player.getShowReticle()) {
            frame = arduboy.getFrameCount(24) / 6;
        }
        else {
            frame = arduboy.getFrameCount(96) / 24; 
        }

    }

    switch (guard.getMovement()) {

        case Movement::IdleLeft:
            frame = 0;
            /*-fallthrough*/

        case Movement::WalkingLeft:
            headImage = 0;
            break;

        case Movement::IdleRight:
            frame = 0;
            /*-fallthrough*/

        case Movement::WalkingRight:
            headImage = 1;
            frame = frame + 4;
            break;

        case Movement::LookingUpLeft:
        case Movement::LookingUpRight:
            frame = (guard.getMovement() == Movement::LookingUpRight);
            Sprites::drawPlusMask(xPosition, 36, Images::Guard_LookUp, frame);
            frame = 255;
            break;

        default: break;

    }

    if (frame < 255 && headImage < 255) {
        Sprites::drawPlusMask(xPosition + 1, 36 + (frame == 1 ? 1 : 0), Images::Guard_Head, headImage);
        Sprites::drawPlusMask(xPosition, 45, Images::Common, frame);
    }

    if (guard.getAlertCount() > 0) {

        Sprites::drawPlusMask(xPosition + 5, 25, Images::Guard_Alert, 0);

    }

    
    frame = (frame % 4) / 2;

    switch (guard.getMovement()) {

        case Movement::IdleLeft:
        case Movement::WalkingLeft:
            Sprites::drawSelfMasked(xPosition - 16, 44, Images::Guard_Flashlight, frame);
            break;

        case Movement::IdleRight:
        case Movement::WalkingRight:
            Sprites::drawSelfMasked(xPosition + 14, 44, Images::Guard_Flashlight, 2 + frame);
            break;

        case Movement::LookingUpLeft:
        case Movement::LookingUpRight:
            if (guard.getSteps() > 64 || arduboy.getFrameCountHalf(32)) {
                Sprites::drawPlusMask(xPosition + 4, 24, Images::Guard_Question, 0);
            }                    
            break;

        default: break;

    }

}

void renderForeground(const int16_t xOffset) {


    // Ground ..

    for (int16_t x = 6 + xOffset; x < 243; x = x + 32) {
        Sprites::drawOverwrite(x, 53, Images::Footpath, 0);
    }


    // LightPoles

    uint8_t lpi = 0;
    bool flickeringLight = false;

    for (int16_t x = 6 + xOffset; x < (SPACING_FOREGROUND * 3); x = x + 81) {

        if (x > -BUILDING_LIGHTPOLE_WIDTH && x < WIDTH) {

            Sprites::drawPlusMask(x, 12, Images::LightPole_Top, 0);
            Sprites::drawOverwrite(x, 20, Images::LightPole_Base, 0);

            if (stealthGameVars.questionCount > 0) {

                if (lpi == stealthGameVars.lampPoleIdx) {

                    uint8_t flicker = random(18, 40);

                    if (arduboy.getFrameCountHalf(flicker)) {

                        arduboy.drawFastHLine(x + 10, 16, 8, BLACK);
                        flickeringLight = true;

                        #ifdef USE_LED_ANALOGUE
                        arduboy.setRGBled(BLUE_LED, flicker - 18);
                        #endif

                    }

                }

            }

        }

        if (x >= - BUILDING_LIGHTPOLE_WIDTH + BUILDING_LIGHTPOLE_BULB_X && x <= WIDTH - BUILDING_LIGHTPOLE_BULB_X - BUILDING_LIGHTPOLE_BULB_WIDTH) {

            ReticleTarget &target = stealthGameVars.getReticleTarget(stealthGameVars.reticleTargetCount);
            target.x = x + 7;
            target.y = 10;
            target.type = TargetType::LightPole;
            target.buildingIdx = lpi;
            stealthGameVars.reticleTargetCount++;

        }

        lpi++;

    }

    if (!flickeringLight) {
        #ifdef USE_LED_ANALOGUE
        arduboy.setRGBled(BLUE_LED, 0);
        #endif
    }

    if (stealthGameVars.questionCount > 0) {
        stealthGameVars.questionCount--;
    }

}


void renderBackground(const int16_t xOffset) {

    for (uint8_t i = 0; i < 14; i++) {

        int16_t x = pgm_read_byte(&coords[i * 3]) + xOffset;
        int16_t y = pgm_read_byte(&coords[i * 3 + 1]);
        int16_t z = pgm_read_byte(&coords[i * 3 + 2]);

        if (i % 2 == 0) {
            arduboy.drawFastHLine(x, y, z);
        }
        else {
            arduboy.drawFastVLine(x, y, z);
        }

    }

}

void renderTallBuilding(const int16_t x, const int8_t door) {

    arduboy.drawFastVLine(x, 0, 52, BLACK);    
    arduboy.fillRect(x + 1, 0, 23, 52, WHITE);
    arduboy.drawFastVLine(x + 24, 0, 52, BLACK);    
    arduboy.drawFastVLine(x + 25, 0, 52, WHITE);    
    arduboy.fillRect(x + 26, 0, 19, 52, BLACK);
    arduboy.drawFastVLine(x + 45, 0, 52, WHITE);    

    Sprites::drawOverwrite(x + 3, 23, Images::Building_Sign_00, 0);
    Sprites::drawOverwrite(x + 4, 33, Images::Building_Door, door);
}

void renderMedBuilding(const int16_t x, const int8_t door) {

    Sprites::drawPlusMask(x, 18, Images::Building_Med, 0);
    Sprites::drawOverwrite(x + 6, 33, Images::Building_Door, door);

}

void renderLowBuilding(const int16_t x, const int8_t door) {

    Sprites::drawOverwrite(x, 18, Images::Building_Roof, 0);
    arduboy.fillRect(x + 3, 25, 20, 27, WHITE);
    arduboy.fillRect(x, 22, 59, 3, BLACK);
    arduboy.fillRect(x + 23, 26, 33, 26, BLACK);
    arduboy.drawFastHLine(x + 1, 23, 57);    
    arduboy.drawFastHLine(x + 23, 25, 33);    
    arduboy.drawFastVLine(x + 56, 25, 27);    
    Sprites::drawOverwrite(x + 25, 32, Images::Building_DarkSide_00, 0);
    Sprites::drawOverwrite(x + 39, 32, Images::Building_DarkSide_01, 0);
    Sprites::drawOverwrite(x + 6, 27, Images::Building_Sign_01, 0);
    Sprites::drawOverwrite(x + 6, 33, Images::Building_Door, door);

}

void renderGauges() {


    // Health ..
    
    arduboy.fillRect(0, 1, 14 + (player.getHealth() / 32), 7, BLACK);    
    Sprites::drawSelfMasked(1, 1, Images::Alert, 0);
    
    for (uint8_t x = 0; x < player.getHealth() / 32; x++) {

        arduboy.drawFastVLine(8 + (x * 2), 2, 5);

    }



    // Memory ..
    
    arduboy.fillRect(106, 1, 9 + (player.getMemory() * 2), 7, BLACK);    
    Sprites::drawSelfMasked(107, 2, Images::Memory, 0);
    for (uint8_t x = 0; x < player.getMemory(); x++) {

        arduboy.drawFastVLine(115 + (x * 2), 2, 5);

    }

}

void renderTerminals() {


    // Terminals ..

    for (uint8_t i = BUILDING_COUNT / 2; i < BUILDING_COUNT; i++)  {

        Building building = stealthGameVars.buildings[i];
        int16_t xPos = building.x + stealthGameVars.world.foregroundXPos;

        if (xPos >= - BUILDING_TERMINAL_WIDTH && xPos <= WIDTH) {

            Sprites::drawOverwrite(xPos, 39, Images::Terminal, 0);

        }

    }

}
#include <Arduboy2.h>

#ifdef TV_TRANSITION

void tv_Init() {

    gameState = GameState::TV;
    tvSequenceVars.counter = tvSequenceVars.mode == TVMode::TurnOff ? 0 : 34;

}   

void tv() {

    const uint8_t offsets[] = { 0, 10, 25, 54, 62 };

    switch (tvSequenceVars.counter) {

        case 0:
        case 23:
            arduboy.fillScreen(WHITE);
            break;

        case 1 ... 2:
        case 28 ... 34:
            arduboy.fillScreen(BLACK);
            break;

        case 3 ... 18:
            {
                const uint8_t offset = offsets[(tvSequenceVars.counter - 3) / 4];
                arduboy.drawFastHLine(offset, 32, 128 - offset - offset + 1);
            }
            break;

        case 19 ... 22:
            {
                const uint8_t offset = offsets[4];
                arduboy.drawFastHLine(offset, 32, 128 - offset - offset + 1);
                arduboy.drawFastVLine( 64, 31, 3);
            }
            break;

        case 24 ... 27:
            Sprites::drawSelfMasked(63, 31, Images::Cathode, 0);
            break;

        case 35:
        case 255:
            gameState = nextState;
            break;

    }

    tvSequenceVars.counter = tvSequenceVars.mode == TVMode::TurnOff ? tvSequenceVars.counter + 1 : tvSequenceVars.counter - 1;

}
#endif
#include <Arduboy2.h>

void title_Init() {

    gameState = GameState::Title;
    arduboy.resetFrameCount();

    clearLEDs();

    #ifdef ARDUBOY_TONES_SOUNDS
        sound.tones(Sounds::Score);
    #endif

}   

void title() {

    Sprites::drawOverwrite(0, 0, Images::Title_00, 0);
    Sprites::drawOverwrite(73, 13, Images::Title_01, 0);
    Sprites::drawOverwrite(51, 46, Images::Title_01, 0);
    Sprites::drawOverwrite(5, 17, Images::Title_02, 0);

    flickering(17);

    arduboy.fillRect(20, 4, WIDTH, 4, WHITE);
    arduboy.drawFastHLine(0, 13, 73);
    arduboy.drawFastHLine(13, 9, WIDTH);
    arduboy.drawFastHLine(55, 46, WIDTH);
    arduboy.drawFastHLine(0, 50, 25);
    arduboy.fillRect(0, 52, 25, 4, WHITE);
    arduboy.drawRect(25, 50, 77, 11);
    font3x5.setCursor(29, 52);

    if (arduboy.getFrameCountHalf(254)) {
        font3x5.print(F("PRESS~'A'~TO~BREACH"));
    }
    else {
        font3x5.print(F("HOLD~SEL+UP~FOR~MENU")); /* was: hold Up for the AVR bootloader */
    }

    Sprites::drawSelfMasked(51, 46, Images::Title_01, 0);

    if (arduboy.justPressed(A_BUTTON)) { 

        player.reset();
        mapGameVars.setMessage(Message::Intro_1);

        gameState = GameState::Message_Intro_Init;

        #ifdef ARDUBOY_TONES_SOUNDS
            sound.noTone();
        #endif

        #ifdef BYTE_BEAT_SOUNDS
            enableByteBeat();
        #endif

    }

}

#include "src/utils/Arduboy2Ext.h"

void clearLEDs() {

    #ifdef USE_LED_ANALOGUE
    arduboy.setRGBled(RED_LED, 0);
    arduboy.setRGBled(GREEN_LED, 0);
    arduboy.setRGBled(BLUE_LED, 0);
    #endif

}

void printHex(uint8_t data) {

  if (data < 16) { font3x5.print("0"); }
  font3x5.print(data, HEX);

}

void initByteBeat() {

}

void disableByteBeat() {

    byteBeatOn = false;

}

void enableByteBeat() {

    byteBeatOn = true;

}

uint8_t isSubArray(uint8_t fullArray[], uint8_t subArray[], uint8_t fullArrayLength, uint8_t subArrayLength)  { 

    // Two pointers to traverse the arrays ..

    uint8_t i = 0;
    uint8_t j = 0; 
  
    // Traverse both arrays simultaneously ..

    while (i < fullArrayLength && j < subArrayLength) { 
  
        // If element matches increment both pointers ..

        if (fullArray[i] == subArray[j]) { 
  
            i++; 
            j++; 
  
            // If fullArrayLength is completely traversed ?

            if (i == fullArrayLength) return j; 
  
            // If subArray is completely traversed ?

            if (j == subArrayLength) return subArrayLength; 
        } 

        // If not, increment i and reset j 

        else { 
            i = i - j + 1; 
            j = 0; 
        } 

    } 
  
    return 0; 

} 

void flickering(uint8_t y) {

    uint8_t frameCount = arduboy.getFrameCount(192);
    uint8_t rectHeight = 0;

#ifdef FLICKERING

    switch (frameCount) {

        case 0:
        case 1:
        case 5:
        case 7:
        case 16:
        case 61:
            rectHeight = 24;
            break;

        case 64 ... 65:
            rectHeight = 3;
            break;

        case 66:
        case 75:
            rectHeight = 5;
            y = y + 16;
            break;

        case 69 ... 70:
            rectHeight = 8;
            y = y + 5;
            break;

        case 132:
        case 136:
            arduboy.drawFastHLine(0, y + 2, 40, BLACK);
            arduboy.drawFastHLine(30, y + 17, 50, BLACK);
            arduboy.drawFastHLine(60, y + 25, 50, BLACK);
            break;

        case 134:
            arduboy.drawFastHLine(60, y + 7, 40, BLACK);
            arduboy.drawFastHLine(30, y + 17, 50, BLACK);
            arduboy.drawFastHLine(0, y + 25, 50, BLACK);
            break;

        default:
            break;

    }

    arduboy.fillRect(0, y, WIDTH, rectHeight, BLACK);

#endif

}
