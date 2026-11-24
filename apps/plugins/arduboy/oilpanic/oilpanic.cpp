#include <Arduino.h>
#include "src/utils/Arduboy2Ext.h"
#include <ArduboyTones.h>
#include "src/images/Images.h"
#include "src/utils/consts.h"
#include "src/utils/EEPROM_Utils.h"
#include "src/entities/Entities.h"
#include "src/sounds/Sounds.h"
#include "src/fonts/Font3x5.h"
#include "src/fonts/Font4x6.h"

/* Arduino IDE auto-prototypes */
void highScore_Init(void);
void highScore(void);
void renderHighScore(uint8_t y, const SaveEntry & saveEntry);
void instructions_Init(void);
void instructions(void);
void instructions_Scene1();
void instructions_Scene2();
void instructions_Scene3();
void instructions_Scene4();
void instructions_Scene5();
void instructions_Scene6();
void renderDottedLines(uint8_t x);
void renderArrows(uint8_t leftArrow, uint8_t rightArrow);
void renderOutside(int8_t xOffset, uint8_t outdoorsYOffset);
void playGame_Init();
void lifeReset();
void playGame(void);
void catchOil(XPosition xPosition);
bool updateThrowOil(ThrowOil &throwOil);
void renderPlayer_Indoors(int8_t xOffset);
void renderPlayer_Outdoors(int8_t xOffset, uint8_t yOffset);
void renderThrowingOil(int8_t xOffset, uint8_t yOffset);
void renderCatcher(int8_t xOffset, uint8_t yOffset);
void renderCatcherMap(int8_t xOffset);
void renderBystanders(int8_t xOffset, uint8_t yOffset);
void renderOils(GameScene &gameScene, uint8_t yOffset);
void renderOil(GameScene gameScene, Oil &oil, int8_t xOffset, uint8_t yOffset);
void renderScoreboard(GameScene gameScene);
void splashScreen_Init();
void splashScreen();
void title();

Arduboy2Ext arduboy;
#ifdef SOUNDS
ArduboyTones sound(arduboy.audio.enabled);
#endif

Font3x5 font3x5 = Font3x5();
Font4x6 font4x6 = Font4x6();

Player player;
Catcher catcher;
Oils oils;
Oil oil;

GameState gameState = GameState::Splash_Init;
GameScene gameScene = GameScene::Indoors;
ThrowOil throwOil = ThrowOil::None;

int16_t counter = 10;
uint8_t frameRate = 50;
uint16_t score = 0;
uint8_t numberOfLives_Indoors = 3;
uint8_t numberOfLives_Outdoors = 3;
uint8_t outdoorsYOffset = 0;
uint8_t gameOverCounter = 0;
uint8_t ledRedCounter = 0;
uint8_t ledGreenCounter = 0;

bool gameOver = false;
bool newHighScore = false;



// High scores ..

uint8_t winnerIdx = 0;
uint8_t charIdx = 0;
uint8_t clearScores = 0;
uint8_t pressACounter = 0;

SaveEntry entries[EEPROM_Utils::saveEntriesCount];


#ifdef SOUNDS
uint8_t soundCounter = 0;
bool soundsOnOff = arduboy.audio.enabled();
#endif

void setup(void) {

	arduboy.boot();
	arduboy.flashlight();
	arduboy.systemButtons();
	arduboy.initRandomSeed();
	arduboy.setFrameRate(50);
  arduboy.setRGBled(0, 0, 0);

    #ifdef SOUNDS
    arduboy.audio.begin();
    #endif

    EEPROM_Utils::initialiseEEPROM();
	
}

void loop(void) {

	if (!arduboy.nextFrame()) return;

	arduboy.pollButtons();

    switch (gameState) {

        case GameState::Splash_Init:

            splashScreen_Init();
            gameState = GameState::Splash;
            arduboy.setFrameRate(50);
            [[fallthrough]]

        case GameState::Splash:

            splashScreen();
            arduboy.display();
            break;

        case GameState::Title_Init:

            gameState = GameState::Title;
            counter = 0;
            arduboy.setFrameRate(50);
            #ifdef SOUNDS
            sound.tones(Sounds::score);
            #endif
            [[fallthrough]]

        case GameState::Title:

            title();
            arduboy.displayClearToWhite();
            break;

        case GameState::Instructions_Init:

            instructions_Init();
            gameState = GameState::Instructions_Scene1;
            #ifdef SOUNDS
            sound.noTone();
            #endif
            [[fallthrough]]

        case GameState::Instructions_Scene1 ... GameState::Instructions_Scene6:
            instructions();
            arduboy.displayClearToWhite();
            break;

        case GameState::PlayGame_Init:

            playGame_Init();
            #ifdef SOUNDS
            sound.noTone();
            #endif
            [[fallthrough]]

        case GameState::PlayGame:

            playGame();

            if (frameRate != 40 + (score / 8)) {
                frameRate = 40 + (score / 8);
                arduboy.setFrameRate(frameRate);
            }
            arduboy.displayClearToWhite();
            break;

        case GameState::HighScore_Init:

            highScore_Init();
            arduboy.setFrameRate(50);
            #ifdef SOUNDS
            sound.noTone();
            #endif
            [[fallthrough]]

        case GameState::HighScore:

            highScore();
            arduboy.displayClearToWhite();
            break;

    }


}

#include <Arduboy2.h>


#define HS_NAME_LEFT 81
#define HS_SCORE_LEFT HS_NAME_LEFT + 24
#define HS_CHAR_TOP 22
#define HS_CHAR_V_SPACING 8
#define HS_PRESS_A_DELAY 100
#define FLASH_FRAME_COUNT 40


// ----------------------------------------------------------------------------
//  High Score init ..
//
void highScore_Init(void) {

    charIdx = 0;
    clearScores = 0;
    pressACounter = HS_PRESS_A_DELAY;
    winnerIdx = (score > 0 ? EEPROM_Utils::saveScore(score) : Constants::no_Winner);
    gameState = GameState::HighScore;

    arduboy.clearButtonState();
    arduboy.setRGBled(GREEN_LED, 0);
    arduboy.setRGBled(RED_LED, 0);


    // Retrieve existing names and scores ..

    EEPROM_Utils::readSaveEntries(entries);
    font4x6.setTextColor(BLACK);

}


// ----------------------------------------------------------------------------
//  High Score ..
//
#define KEY_REPEAT 12

void highScore(void) {


    auto justPressed = arduboy.justPressedButtons();
    auto pressed = arduboy.pressedButtons();


    // Is the new score a high score ?

    if (winnerIdx < Constants::no_Winner) {
                
        char * player = entries[winnerIdx].name;

        if (justPressed == 0 && arduboy.frameCount % KEY_REPEAT == frameRate) {

            if (pressed & UP_BUTTON) {

                player[charIdx]++;
                if (player[charIdx] > 90)  player[charIdx] = 65;
                if (player[charIdx] == 64) player[charIdx] = 65;

            }

            if (pressed & DOWN_BUTTON) {

                player[charIdx]--;
                if (player[charIdx] < 65)  player[charIdx] = 90;

            }

            if (pressed & LEFT_BUTTON && charIdx > 0) {
                charIdx--;
            }

            if (pressed & RIGHT_BUTTON && charIdx < 2) {
                charIdx++;
            }

            if (pressed & A_BUTTON) {

                if (player[0] != 63 && player[1] != 63 && player[2] != 63) {

                    for (uint8_t i = 0; i < 3; i++) {
                        EEPROM_Utils::saveChar(winnerIdx, i, player[i]);
                    }

                    winnerIdx = Constants::no_Winner;
                    pressACounter = HS_PRESS_A_DELAY;

                }

            }

        }   
        else {

            if (justPressed & UP_BUTTON) {

                player[charIdx]++;
                if (player[charIdx] > 90)  player[charIdx] = 65;
                if (player[charIdx] == 64) player[charIdx] = 65;
                frameRate = arduboy.frameCount % KEY_REPEAT;

            }

            if (justPressed & DOWN_BUTTON) {

                player[charIdx]--;
                if (player[charIdx] < 65)  player[charIdx] = 90;
                frameRate = arduboy.frameCount % KEY_REPEAT;

            }

            if (justPressed & LEFT_BUTTON && charIdx > 0) {
                charIdx--;
                frameRate = arduboy.frameCount % KEY_REPEAT;
            }

            if (justPressed & RIGHT_BUTTON && charIdx < 2) {
                charIdx++;
                frameRate = arduboy.frameCount % KEY_REPEAT;
            }
            
        }

    }
    else {



        // Handle other input ..

        if ((justPressed & A_BUTTON || justPressed & B_BUTTON) && pressACounter == 0) {

            gameState = GameState::Title_Init;

        }

    }


    // Clear scores ..

    if ((pressed & LEFT_BUTTON) && (pressed & RIGHT_BUTTON)) {

        clearScores++;

        switch (clearScores) {

            case 21 ... 60:

                arduboy.setRGBled(128 - (clearScores * 2), 0, 0);
                break;

            case 61:

                clearScores = 0;
                arduboy.setRGBled(0, 0, 0);
                EEPROM_Utils::clearEEPROM();
                EEPROM_Utils::readSaveEntries(entries);

                break;

        }

    }
    else {

        if (clearScores > 0) {

            arduboy.setRGBled(0, 0, 0);
            clearScores = 0;

        }

    }


    // Decrement the 'Press A' counter if it has been set ..

    if (pressACounter > 0) pressACounter--;



    // Render screen -------------------------------------------------------------------------


    const bool flash = arduboy.getFrameCountHalf(FLASH_FRAME_COUNT);

    Sprites::drawOverwrite(0, 0, Images::HighScore_LHS, 0);
    Sprites::drawOverwrite(54, 42, Images::HighScore_BOT, 0);
    Sprites::drawOverwrite(54, 8, Images::HighScore_TXT, 0);

    arduboy.drawLine(54, 1, 128, 1, BLACK);
    arduboy.drawLine(78, 62, 128, 62, BLACK);
    

    // Render scores ..

    for (uint8_t index = 0; index < EEPROM_Utils::saveEntriesCount; ++index) {

        renderHighScore(HS_CHAR_TOP + (HS_CHAR_V_SPACING * index), entries[index]);

    }


    // Render edit field if the slot is being editted ..

    if (winnerIdx < Constants::no_Winner && flash) {

        char *player = entries[winnerIdx].name;

        arduboy.fillRect(HS_NAME_LEFT + (charIdx * 6) - 1, HS_CHAR_TOP + (winnerIdx * HS_CHAR_V_SPACING), 6, 8, BLACK);
        font4x6.setCursor(HS_NAME_LEFT + (charIdx * 6), HS_CHAR_TOP + (HS_CHAR_V_SPACING * winnerIdx));
        font4x6.print(player[charIdx] == 63 ? ' ' : player[charIdx]);

    }


    // Display Press A message?

    if (winnerIdx == Constants::no_Winner && pressACounter == 0) {

        font4x6.setCursor(HS_NAME_LEFT + 3, 51);
        font4x6.print("Press");
        Sprites::drawOverwrite(HS_NAME_LEFT + 33, 50, Images::PressA, 0);

    }

}



void renderHighScore(uint8_t y, const SaveEntry & saveEntry) {

    for (uint8_t i = 0; i < 3; i++) {
        font4x6.setCursor(HS_NAME_LEFT  + (i * 6) , y);
        font4x6.print(saveEntry.name[i]);
    }


    uint8_t digits[4] = {};
    extractDigits(digits, saveEntry.score);

    for (uint8_t j = 4, x2 = HS_SCORE_LEFT; j > 0; --j, x2 += 5) {

        font4x6.setCursor(x2, y);
        font4x6.print(digits[j - 1]);

    }

}
#include <Arduboy2.h>


#define SCENE1_ELEMENT_COUNT 93
#define SCENE1_OFFSET -70
#define SCENE2_OFFSET 70
#define SCENE2_ELEMENT_COUNT 76
#define SCENE3_OFFSET -70
#define SCENE3_ELEMENT_COUNT 56
#define SCENE4_OFFSET 34
#define SCENE4_BORDER 70
#define SCENE4_ELEMENT_COUNT 100
#define SCENE6_DELAY 32
#define SCENE5_OFFSET -70

const XPosition PROGMEM scene1_PlayerXPosition[SCENE1_ELEMENT_COUNT] = {         
    XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_16, XPosition::Position_16, 
    XPosition::Position_17, XPosition::Position_17, XPosition::Position_16, XPosition::Position_16, 
    XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_16, XPosition::Position_16, 
    XPosition::Position_17, XPosition::Position_17, XPosition::Position_16, XPosition::Position_16, 
    XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_15_Oil, 
        
    XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_16, XPosition::Position_16, 
    XPosition::Position_17, XPosition::Position_17, XPosition::Position_16, XPosition::Position_16, 
    XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_16, XPosition::Position_16, 
    XPosition::Position_17, XPosition::Position_17, XPosition::Position_16, XPosition::Position_16, 
    XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_15_Oil, 
        
    XPosition::Position_15_Oil, XPosition::Position_15_Oil, XPosition::Position_14, XPosition::Position_14, 
    XPosition::Position_13, XPosition::Position_13, XPosition::Position_12, XPosition::Position_12, 
    XPosition::Position_11, XPosition::Position_11, XPosition::Position_10_Oil, XPosition::Position_10_Oil, 
    XPosition::Position_9, XPosition::Position_9, XPosition::Position_8, XPosition::Position_8, 
    XPosition::Position_7, XPosition::Position_7, XPosition::Position_7, XPosition::Position_7, 

    XPosition::Position_8, XPosition::Position_8, XPosition::Position_9, XPosition::Position_9, 
    XPosition::Position_10_Oil, XPosition::Position_10_Oil, XPosition::Position_11, XPosition::Position_12, 
    XPosition::Position_13, XPosition::Position_14, XPosition::Position_15_Oil, XPosition::Position_16, 
    XPosition::Position_17, XPosition::Position_18, XPosition::Position_19, XPosition::Position_Tipping_RH, 
    XPosition::Position_Tipping_RH, XPosition::Position_Tipping_RH, XPosition::Position_Tipping_RH, XPosition::Position_Tipping_RH, 

    XPosition::Position_Tipping_RH, XPosition::Position_Tipping_RH, XPosition::Position_Tipping_RH, XPosition::Position_Tipping_RH, 
    XPosition::Position_19, XPosition::Position_18, XPosition::Position_17, XPosition::Position_16, 
    XPosition::Position_15_Oil, XPosition::Position_16, XPosition::Position_17, XPosition::Position_18, 
};

const YPosition PROGMEM scene1_DripYPosition[SCENE1_ELEMENT_COUNT] = { 
    YPosition::None, YPosition::None, YPosition::None, YPosition::None, 
    YPosition::StartDrip_00, YPosition::StartDrip_00, YPosition::StartDrip_00, YPosition::StartDrip_00,
    YPosition::StartDrip_01, YPosition::StartDrip_01, YPosition::StartDrip_01, YPosition::StartDrip_01,
    YPosition::Falling_04, YPosition::Falling_05, YPosition::Falling_06, YPosition::Falling_07,
    YPosition::Falling_08, YPosition::Falling_09, YPosition::None, YPosition::None,

    YPosition::None, YPosition::None, YPosition::None, YPosition::None, 
    YPosition::StartDrip_00, YPosition::StartDrip_00, YPosition::StartDrip_00, YPosition::StartDrip_00,
    YPosition::StartDrip_01, YPosition::StartDrip_01, YPosition::StartDrip_01, YPosition::StartDrip_01,
    YPosition::Falling_04, YPosition::Falling_05, YPosition::Falling_06, YPosition::Falling_07,
    YPosition::Falling_08, YPosition::Falling_09, YPosition::None, YPosition::None,

    YPosition::None, YPosition::None, YPosition::None, YPosition::None, 
    YPosition::StartDrip_00, YPosition::StartDrip_00, YPosition::StartDrip_00, YPosition::StartDrip_00,
    YPosition::StartDrip_01, YPosition::StartDrip_01, YPosition::StartDrip_01, YPosition::StartDrip_01,
    YPosition::Falling_04, YPosition::Falling_05, YPosition::Falling_06, YPosition::Falling_07,
    YPosition::Falling_08, YPosition::Falling_09, YPosition::Falling_10, YPosition::Falling_11,

    YPosition::Falling_12, YPosition::Falling_13, YPosition::Fire_00, YPosition::Fire_01, 
    YPosition::Fire_02, YPosition::Fire_03, YPosition::Fire_04, YPosition::Fire_05, 
    YPosition::Fire_06, YPosition::Fire_07, YPosition::None, YPosition::None, 
    YPosition::None, YPosition::None, YPosition::None, YPosition::None, 
    YPosition::None, YPosition::None, YPosition::None, YPosition::None, 

    YPosition::None, YPosition::None, YPosition::None, YPosition::None, 
    YPosition::None, YPosition::None, YPosition::None, YPosition::None, 
    YPosition::None, YPosition::None, YPosition::None, YPosition::None, 
};

const uint8_t PROGMEM scene1_OilLevel[SCENE1_ELEMENT_COUNT] = {         
    0, 0, 0, 0,                                          
    0, 0, 0, 0,                                          
    0, 0, 0, 0,                                          
    0, 0, 0, 0,                                          
    0, 0, 1, 1,

    1, 1, 1, 1,
    1, 1, 1, 1, 
    1, 1, 1, 1, 
    1, 1, 1, 1, 
    1, 1, 2, 2,

    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,

    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,

    0, 0, 0, 0, 
    0, 0, 0, 0, 
    0, 0, 0, 0, 
};                                          

const uint8_t PROGMEM scene1_Instructions[SCENE1_ELEMENT_COUNT] = {         
    255, 255, 1, 1,                                          
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,

    1, 1, 1, 1,
    1, 1, 1, 1, 
    1, 1, 1, 1, 
    1, 1, 1, 1, 
    255, 255, 2, 2,

    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,

    2, 2, 255, 255,
    3, 3, 3, 3,
    3, 3, 3, 3,
    3, 3, 3, 3,
    3, 3, 3, 3,

    3, 3, 3, 3,
    3, 3, 3, 3,
    3, 3, 3, 3,
};      


const uint8_t PROGMEM scene1_YCursor[3] =   { 16, 17, 12 };                               

const uint8_t PROGMEM scene2_CatcherXPosition[SCENE2_ELEMENT_COUNT] = { 
    13, 12, 11, 10, 
    9, 8, 7, 6, 
    5, 4, 3, 2, 
    1, 0, 0, 0, 
    0, 0, 0, 1, 

    2, 3, 4, 5, 
    6, 7, 8, 9, 
    10, 11, 12, 13,
    14, 15, 16, 17, 
    18, 19, 20, 20,

    20, 20, 20, 20, 
    20, 20, 20, 20, 
    20, 20, 20, 20, 
    20, 20, 20, 20, 
    20, 20, 20, 20, 

    20, 20, 20, 20, 
    20, 20, 20, 20, 
    20, 20, 20, 20, 
    20, 20, 20, 20, 
};

const ThrowOil PROGMEM scene2_OilYPosition[SCENE2_ELEMENT_COUNT] = { 
    ThrowOil::None, ThrowOil::None, ThrowOil::None, ThrowOil::None,
    ThrowOil::None, ThrowOil::None, ThrowOil::None, ThrowOil::None,
    ThrowOil::None, ThrowOil::None, ThrowOil::None, ThrowOil::None,
    ThrowOil::LH_Top, ThrowOil::LH_Middle, ThrowOil::LH_Bottom, ThrowOil::None,
    ThrowOil::None, ThrowOil::None, ThrowOil::None, ThrowOil::None,

    ThrowOil::None, ThrowOil::None, ThrowOil::None, ThrowOil::None,
    ThrowOil::None, ThrowOil::None, ThrowOil::None, ThrowOil::None,
    ThrowOil::None, ThrowOil::None, ThrowOil::None, ThrowOil::None,
    ThrowOil::LH_Top, ThrowOil::LH_Middle, ThrowOil::LH_Miss_Down_Start, ThrowOil::LH_Miss_Down_01,
    ThrowOil::LH_Miss_Down_02, ThrowOil::LH_Miss_Down_03, ThrowOil::LH_Miss_Down_04, ThrowOil::LH_Miss_Down_05,

    ThrowOil::LH_Miss_Down_06, ThrowOil::LH_Miss_Down_07, ThrowOil::LH_Miss_Down_08, ThrowOil::LH_Miss_Bottom_Start,
    ThrowOil::LH_Miss_Bottom_01, ThrowOil::LH_Miss_Bottom_02, ThrowOil::LH_Miss_Bottom_03, ThrowOil::LH_Miss_Bottom_End_Of_Splash,
    ThrowOil::LH_Miss_Bottom_05, ThrowOil::LH_Miss_Bottom_06, ThrowOil::LH_Miss_Bottom_07, ThrowOil::LH_Miss_Bottom_08,
    ThrowOil::LH_Miss_Bottom_End, ThrowOil::LH_Miss_Bottom_End, ThrowOil::LH_Miss_Bottom_End, ThrowOil::LH_Miss_Bottom_End,
    ThrowOil::LH_Miss_Bottom_End, ThrowOil::LH_Miss_Bottom_End, ThrowOil::LH_Miss_Bottom_End, ThrowOil::LH_Miss_Bottom_End,

    ThrowOil::LH_Miss_Up_Start, ThrowOil::LH_Miss_Up_01, ThrowOil::LH_Miss_Up_02, ThrowOil::LH_Miss_Up_03,
    ThrowOil::LH_Miss_Up_04, ThrowOil::LH_Miss_Up_05, ThrowOil::LH_Miss_Up_06, ThrowOil::LH_Miss_Up_07,
    ThrowOil::LH_Miss_Up_08, ThrowOil::LH_Miss_Up_NearlyEnd, ThrowOil::LH_Miss_Up_End, ThrowOil::LH_Miss_Up_End,
    ThrowOil::LH_Miss_Up_End, ThrowOil::LH_Miss_Up_End, ThrowOil::LH_Miss_Up_End, ThrowOil::LH_Miss_Up_End,
};

const uint8_t PROGMEM scene2_YOffset[SCENE2_ELEMENT_COUNT] = { 
    0, 0, 0, 0, 
    0, 0, 0, 0, 
    0, 0, 0, 0, 
    0, 0, 0, 0, 
    0, 0, 0, 0, 

    0, 0, 0, 0, 
    0, 0, 0, 0, 
    2, 4, 6, 8, 
    10, 12, 14, 16,
    18, 20, 22, 24, 

    26, 28, 30, 32, 
    34, 36, 38, 38,
    38, 38, 38, 36, 
    34, 32, 30, 28, 
    26, 24, 22, 20,

    18, 16, 14, 12, 
    10, 8, 6, 4, 
    2, 0, 0, 0, 
    0, 0, 0, 0, 
};

const uint8_t PROGMEM scene2_Instructions[SCENE2_ELEMENT_COUNT] =   {         
    255, 255, 1, 1,                                          
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,

    1, 1, 1, 1,
    1, 1, 1, 1, 
    255, 255, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,

    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,

    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 255, 255,
};      

const uint8_t PROGMEM scene2_YCursor[3] =   { 19, 12 };                               

const uint8_t PROGMEM scene3_Instructions[SCENE3_ELEMENT_COUNT] =   {         
    255, 255, 1, 1,                                          
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,

    1, 1, 1, 1, 
    1, 1, 1, 1, 
    255, 255, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,

    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 255, 255,
};      

const uint8_t PROGMEM scene3_YCursor[3] =   { 12, 12 };                               

const uint8_t PROGMEM scene4_YCursor[3] =   { 16, 7 };                               

const uint8_t PROGMEM scene5_YCursor[3] =   { 16, 20 };                               


// ----------------------------------------------------------------------------
//  Initialise game ..
//
void instructions_Init(void) {

    outdoorsYOffset = 0;
    counter = 0;

    player.setOilLevel(0);
    arduboy.setFrameRate(38);

}


// ----------------------------------------------------------------------------
//  Render instructions ..
//
void instructions(void) {

    switch (gameState) {

        case GameState::Instructions_Scene1:
            instructions_Scene1();
            break;

        case GameState::Instructions_Scene2:
            instructions_Scene2();
            break;

        case GameState::Instructions_Scene3:
            instructions_Scene3();
            break;

        case GameState::Instructions_Scene4:
            instructions_Scene4();
            break;

        case GameState::Instructions_Scene5:
            instructions_Scene5();
            break;

        case GameState::Instructions_Scene6:
            instructions_Scene6();
            break;

        default: break;

    }

    if (arduboy.justPressed(A_BUTTON)) {
        gameState = GameState::PlayGame_Init;
    }

}


// ----------------------------------------------------------------------------
//  Render instructions - Scene 1 ..
//
void instructions_Scene1() {

    gameScene = GameScene::Indoors;

    if (arduboy.isFrameCount(6)) {

        counter++;

        if (counter == SCENE1_ELEMENT_COUNT) {
            counter = 0;
            gameState = GameState::Instructions_Scene2;
        }

    }

    player.setXPosition(static_cast<XPosition>(pgm_read_byte(&scene1_PlayerXPosition[counter])));
    player.setOilLevel(pgm_read_byte(&scene1_OilLevel[counter]));
    oil.setYPosition(static_cast<YPosition>(pgm_read_byte(&scene1_DripYPosition[counter])));
    oil.setX(2);


    Sprites::drawOverwrite(SCENE1_OFFSET, 0, Images::Indoors, 0);
    arduboy.fillRect(SCENE1_OFFSET + 108, 24, 17, 6, WHITE);

    renderPlayer_Indoors(SCENE1_OFFSET);
    renderOil(gameScene, oil, SCENE1_OFFSET, outdoorsYOffset);
    renderDottedLines(SCENE1_OFFSET + 128 + 1);

    font3x5.setCursor(SCENE1_OFFSET + 128 + 6, pgm_read_byte(&scene1_YCursor[pgm_read_byte(&scene1_Instructions[counter]) - 1]));
    font3x5.setTextColor(BLACK);

    switch (pgm_read_byte(&scene1_Instructions[counter])) {

        case 1:
            font3x5.print(F("Catch~the~drops\nbefore~they~hit\nthe~floor~to~gain\npoints."));
            break;

        case 2:
            font3x5.print(F("Failing~to~catch\na~drop~will~start\na~fire~and~cost\nyou~a~life."));
            break;

        case 3:
            font3x5.print(F("Empty~the~bucket\nby~going~outside.\nIt~does~not~need\nto~be~full~for~it\nto~be~emptied."));
            break;

    }

    renderArrows(0, 123);
    
    if (arduboy.justPressed(RIGHT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene2;
    }

}


// ----------------------------------------------------------------------------
//  Render instructions - Scene 2 ..
//
void instructions_Scene2() {

    uint8_t frameCount = counter > 32 ? 3 : 6;
    gameScene = GameScene::Outdoors;

    if (arduboy.isFrameCount(frameCount)) {

        counter++;

        if (counter == SCENE2_ELEMENT_COUNT) {
            counter = 0;
            gameState = GameState::Instructions_Scene3;
        }

    }

    renderOutside(SCENE2_OFFSET, outdoorsYOffset);

    switch (counter) {

        case 12 ... 15:
        case 30 ... 33:
            player.setXPosition(XPosition::Position_Throwing_LH);
            break;

        default:
            player.setXPosition(XPosition::Position_Outside_LH);
            break;

    }

    catcher.setX(pgm_read_byte(&scene2_CatcherXPosition[counter]));
    catcher.setDirection(counter < 20 ? Direction::Left : Direction::Right);
    throwOil = static_cast<ThrowOil>(pgm_read_byte(&scene2_OilYPosition[counter]));
    outdoorsYOffset = pgm_read_byte(&scene2_YOffset[counter]);

    renderPlayer_Outdoors(SCENE2_OFFSET, outdoorsYOffset);
    renderCatcher(SCENE2_OFFSET, outdoorsYOffset);
    renderThrowingOil(SCENE2_OFFSET, outdoorsYOffset);
    renderBystanders(SCENE2_OFFSET, outdoorsYOffset);
    renderDottedLines(SCENE2_OFFSET - 3);


    font3x5.setCursor(2, pgm_read_byte(&scene2_YCursor[pgm_read_byte(&scene2_Instructions[counter]) - 1]));

    switch (pgm_read_byte(&scene2_Instructions[counter])) {

        case 1:
            font3x5.print(F("Throw~the~oil~to\nyour~colleague\nto~gain~points."));
            break;

        case 2:
            font3x5.print(F("If~he~fails~to\ncatch~the~oil~it\nwill~fall~on~the\ncustomers~below\nand~cost~a~life."));
            break;

    }

    renderArrows(1, 62);

    if (arduboy.justPressed(LEFT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene1;
    }

    if (arduboy.justPressed(RIGHT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene3;
    }

}


// ----------------------------------------------------------------------------
//  Render instructions - Scene 3 ..
//
void instructions_Scene3() {

    gameScene = GameScene::Indoors;

    if (arduboy.isFrameCount(6)) {
        counter++;
        if (counter == SCENE3_ELEMENT_COUNT) {
            counter = 0;
            outdoorsYOffset = 0;
            gameState = GameState::Instructions_Scene4;
        }

    }

    Sprites::drawOverwrite(SCENE3_OFFSET, 0, Images::Indoors, 0);
    arduboy.fillRect(SCENE3_OFFSET +108, 24, 17, 6, WHITE);

    renderDottedLines(SCENE1_OFFSET + 128 + 1);
    renderCatcherMap(SCENE3_OFFSET);

    if (arduboy.isFrameCount(3)) {
        catcher.update(999);
    }

    font3x5.setCursor(SCENE3_OFFSET + 128 + 6, pgm_read_byte(&scene3_YCursor[pgm_read_byte(&scene3_Instructions[counter]) - 1]));
    font3x5.setTextColor(BLACK);

    switch (pgm_read_byte(&scene3_Instructions[counter])) {

        case 1:
            font3x5.print(F("Hanging~on~the\ngarage~wall~is\na~map~that~shows\nyour~colleagues\nposition."));
            break;

        case 2:
            font3x5.print(F("Use~it~when\nyou~are~emptying\nyour~bucket~to\ndecide~which~way\nto~go."));
            break;

    }

    renderArrows(62, 123);

    if (arduboy.justPressed(LEFT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene2;
    }

    if (arduboy.justPressed(RIGHT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene4;
        outdoorsYOffset = 0;
        catcher.reset();
    }

}


// ----------------------------------------------------------------------------
//  Render instructions - Scene 4 ..
//
void instructions_Scene4() {


    // Create an oil drop ?

    outdoorsYOffset++;

    if (outdoorsYOffset == 80) {
        oil.setX(random(0, 3));
        oil.setYPosition(YPosition::StartDrip_00);
        outdoorsYOffset = 0;
    }

    if (arduboy.isFrameCount(6)) {

        counter++;

        if (counter == SCENE4_ELEMENT_COUNT) {
            counter = 0;
            gameState = GameState::Instructions_Scene5;
        }

    }

    renderOutside(SCENE4_OFFSET, 0);

    if (!gameOver && arduboy.isFrameCount(4)) {
        catcher.update(100);
        #ifndef SOUNDS
        oil.update();
        #else
        oil.update(sound);
        #endif
    }

    renderOil(GameScene::Outdoors, oil, SCENE4_OFFSET, 0);
    renderCatcher(SCENE4_OFFSET, 0);
    arduboy.fillRect(0, 0, SCENE4_BORDER, HEIGHT, WHITE);

    renderDottedLines(SCENE4_BORDER - 3);
    font3x5.setCursor(2, pgm_read_byte(&scene4_YCursor[counter <= 40 ? 0 : 1]));

    switch (counter) {

        case 2 ... 40:
            font3x5.print(F("Likewise~when~you\nare~outside~you\ncan~see~drips~\nforming~inside."));
            break;

        case 43 ... 100:
            font3x5.print(F("Do~not~spend~too\nlong~outside\nwaiting~for~your\ncolleague~as~you\nmay~cause~a\nfire~inside."));
            break;

    }

    renderArrows(1, 62);

    if (arduboy.justPressed(LEFT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene3;
    }

    if (arduboy.justPressed(RIGHT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene5;
    }

}


// ----------------------------------------------------------------------------
//  Render instructions - Scene 5 ..
//
void instructions_Scene5() {

    if (arduboy.isFrameCount(6)) {

        counter++;

        if (counter == 87) {
            counter = SCENE6_DELAY * 4;
            gameState = GameState::Instructions_Scene6;
        }

    }


    renderDottedLines(SCENE5_OFFSET + 128 + 1);

    font3x5.setCursor(SCENE5_OFFSET + 128 + 6, pgm_read_byte(&scene5_YCursor[counter <= 40 ? 0 : 1]));

    switch (counter) {

        case 0 ... 1:
        case 41 ... 42:
            Sprites::drawOverwrite(SCENE5_OFFSET, 0, Images::Indoors, 0);
            break;

        case 2 ... 40:

            Sprites::drawOverwrite(SCENE5_OFFSET, 0, Images::Indoors, 0);
            font3x5.print(F("The~game~is~over\nwhen~you~miss\nthree~oil~drips\ninside~or~.."));

            if ((counter / 8) % 2 == 0) {

                for (uint8_t i = 0, x = 120 + SCENE5_OFFSET; i < 3; i++, x = x - 8) {

                    Sprites::drawErase(x, 16, Images::Life, 0);

                }

                Sprites::drawErase(108 + SCENE5_OFFSET, 24, Images::Miss, 0);

            }

            break;

        case 45 ... 85:
            renderOutside(SCENE5_OFFSET, 0);
            font3x5.print(F("..~tip~the~oil~on\nthree~customers\ndown~below."));

            if ((counter / 8) % 2 == 0) {

                for (uint8_t i = 0, x = 123 + SCENE5_OFFSET; i < 3; i++, x = x - 5) {

                    Sprites::drawExternalMask(x, 1, Images::outsideMiss, Images::outsideMiss_Mask, 0, 0);

                }

            }

            break;

        default:
            renderOutside(SCENE5_OFFSET, 0);
            break;

    }


    renderArrows(62, 123);

    if (arduboy.justPressed(LEFT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene4;
        outdoorsYOffset = 0;
        catcher.reset();
    }

    if (arduboy.justPressed(RIGHT_BUTTON)) {
        counter = SCENE6_DELAY * 4;
        outdoorsYOffset = 0;
        gameState = GameState::Instructions_Scene6;
    }

}


// ----------------------------------------------------------------------------
//  Render instructions - Scene 6 ..
//
void instructions_Scene6() {

    counter--;

    Sprites::drawOverwrite(3, 26, Images::Ready, 0);

    if (counter > 3 * SCENE6_DELAY)  Sprites::drawExternalMask(100, 10, Images::Barrel, Images::Barrel_Mask, 0, 0);
    if (counter > 2 * SCENE6_DELAY)  Sprites::drawExternalMask(91, 33, Images::Barrel, Images::Barrel_Mask, 0, 0);
    if (counter > SCENE6_DELAY)  Sprites::drawExternalMask(109, 33, Images::Barrel, Images::Barrel_Mask, 0, 0);

    if  (counter == 0) gameState = GameState::PlayGame_Init;

    renderArrows(1, 0);

    if (arduboy.justPressed(LEFT_BUTTON)) {
        counter = 0;
        gameState = GameState::Instructions_Scene5;
        outdoorsYOffset = 0;
        catcher.reset();
    }

    if (arduboy.justPressed(RIGHT_BUTTON)) {
        counter = 0;
        outdoorsYOffset = 0;
        gameState = GameState::PlayGame_Init;
    }

}

// ----------------------------------------------------------------------------
//  Render dotted lines ..
//
void renderDottedLines(uint8_t x) {

    arduboy.drawVerticalDottedLine(0, HEIGHT, x, BLACK);
    arduboy.drawVerticalDottedLine(1, HEIGHT, x + 1, BLACK);

}


// ----------------------------------------------------------------------------
//  Render left and right arrows ..
//
void renderArrows(uint8_t leftArrow, uint8_t rightArrow) {

    if (leftArrow > 0) Sprites::drawOverwrite(leftArrow, 58, Images::LeftArrow, 0);
    if (rightArrow > 0) Sprites::drawOverwrite(rightArrow, 58, Images::RightArrow, 0);

}


// ----------------------------------------------------------------------------
//  Render outside scene ..
//
void renderOutside(int8_t xOffset, uint8_t outdoorsYOffset) {

    for (uint8_t x = 0; x < 13; x++) {

        Sprites::drawOverwrite(xOffset, 0 - outdoorsYOffset + (x * 8), Images::Outdoors, x);

    }

}
#include <Arduboy2.h>



// ----------------------------------------------------------------------------
//  Initialise game ..
//
void playGame_Init() {

    lifeReset();

    arduboy.setFrameRate(40);
    oils.reset();
    player.reset();
    
    gameState = GameState::PlayGame;
    gameScene = GameScene::Indoors;

    frameRate = 50;
    score = 0;
    gameOver = false;
    gameOverCounter = 0;
    numberOfLives_Indoors = 3;
    numberOfLives_Outdoors = 3;
    outdoorsYOffset = 0;
    throwOil = ThrowOil::None;

}


// ----------------------------------------------------------------------------
//  Reset life at start of game and after each death ..
//
void lifeReset() {

    counter = 10;
    outdoorsYOffset = 0;
    gameOverCounter = 0;

}


// ----------------------------------------------------------------------------
//  Update game and render ..
//
void playGame(void) {

    uint8_t justPressedButton = arduboy.justPressedButtons();


    // Handle LEDS ..

    if (ledGreenCounter > 0) {

        ledGreenCounter--;

        switch (ledGreenCounter) {

            case 1 ... Constants::led_Green_Delay: 
            case (Constants::led_Green_Delay * 2) + 1 ... (Constants::led_Green_Delay * 3): 
            case (Constants::led_Green_Delay * 4) + 1 ... (Constants::led_Green_Delay * 5):  
                arduboy.setRGBled(GREEN_LED, Constants::LED_Brightness);
                break;

            default:
                arduboy.setRGBled(GREEN_LED, 0);
                break;

        }

    }

    if (ledRedCounter > 0) {

        ledRedCounter--;

        if (ledRedCounter == 0) {

            arduboy.setRGBled(RED_LED, 0);

        }
        else {

            arduboy.setRGBled(RED_LED, Constants::LED_Brightness);

        }

    }

    if (gameOverCounter > 0) {
        gameOverCounter--;
    }


    // Update entity positions ..

    if (!gameOver && arduboy.isFrameCount(4)) {
        
        catcher.update(score);

    }

    if (arduboy.isFrameCount(2)) {

        if (updateThrowOil(throwOil)) {

            gameOver = true;
            gameOverCounter = 32;

        }

    }


    // Update oil positions

    for (uint8_t x = 0; x < Constants::number_Of_Oils; x++) {

        Oil &oil = oils.getOil(x);

        switch (oil.getYPosition()) {

            case YPosition::StartDrip_00 ... YPosition::Falling_13:
                {

                    uint8_t frameCount = 0;


                    // Drop slower if player is outside ..

                    switch (player.getXPosition()) {

                        case XPosition::Position_Throwing_LH ... XPosition::Position_Tipping_LH:
                        case XPosition::Position_Tipping_RH ... XPosition::Position_Throwing_RH:
                            frameCount = 16;
                            break;

                        default:
                            frameCount = 8;
                            break;

                    }

                    if (arduboy.isFrameCount(frameCount)) {

                        #ifndef SOUNDS
                        if (oil.update()) {
                        #else
                        if (oil.update(sound)) {
                        #endif

                            if (numberOfLives_Indoors > 0) {
                                
                                #ifdef SOUNDS
                                sound.tones(Sounds::fireStarted);
                                #endif
                                numberOfLives_Indoors--;
                                ledRedCounter = Constants::led_Red_Delay;

                                if (numberOfLives_Indoors == 0) {

                                    gameOver = true;
                                    gameOverCounter = 32;

                                }

                            }

                        }

                    }

                }

                break;

            case YPosition::Fire_00 ... YPosition::Fire_07:

                if (arduboy.isFrameCount(2)) {

                    #ifndef SOUNDS
                    oil.update();
                    #else
                    oil.update(sound);
                    #endif

                }
                break;

            default: break;

        }

    }


    // Launch Oil ..

    if (!gameOver) {

        oils.launchOil(score);

    }


    switch (gameScene) {

        case GameScene::Indoors:

            if (!gameOver) {

                if (arduboy.isFrameCount(2)) {

                    player.update();

                }

                if (justPressedButton & LEFT_BUTTON)               { player.decXPosition(); }
                if (justPressedButton & RIGHT_BUTTON)              { player.incXPosition(); }
                if (justPressedButton & B_BUTTON)                  { player.incXPosition(); }

            }
            else if (gameOver && gameOverCounter == 0) {

                if (justPressedButton & A_BUTTON)                   { gameState = GameState::HighScore_Init; }

            }


            // Change scene?

            if (player.getXPosition() == XPosition::Position_Outside_LH || player.getXPosition() == XPosition::Position_Outside_RH) {

                gameScene = GameScene::Outdoors;
                throwOil = ThrowOil::None;

            }


            // Did we catch some oil ?

            switch (player.getXPosition()) {

                case XPosition::Position_5_Oil:
                case XPosition::Position_10_Oil:
                case XPosition::Position_15_Oil:
                    catchOil(player.getXPosition());
                    break;

                default: break;

            }


            break;

        case GameScene::Outdoors:

            if (!gameOver) {

                if (justPressedButton & LEFT_BUTTON) {

                    switch (player.getXPosition()) {
                        
                        case XPosition::Position_Outside_LH:

                            if (catcher.isCatching(Direction::Left)) {

                                score = score + player.getOilLevel();
                                ledGreenCounter = pgm_read_byte(&Constants::ledGreenDelays[player.getOilLevel() - 1]);

                                #ifdef SOUNDS
                                sound.tones(Sounds::scoreAdded);
                                #endif

                            }

                            if (throwOil == ThrowOil::None && player.getOilLevel() > 0) throwOil = ThrowOil::LH_Top;
                            player.setXPosition(XPosition::Position_Throwing_LH);
                            player.setOilLevel(0);
                            break;

                        case XPosition::Position_Throwing_RH:
                            player.setXPosition(XPosition::Position_Outside_RH);
                            break;

                        case XPosition::Position_Outside_RH:
                            gameScene = GameScene::Indoors; 
                            player.decXPosition();
                            break;

                        default: break;

                    }

                }

                if ((justPressedButton & RIGHT_BUTTON) || (justPressedButton & B_BUTTON)) { 

                    switch (player.getXPosition()) {
                        
                        case XPosition::Position_Outside_RH:

                            if (catcher.isCatching(Direction::Right)) {

                                score = score + player.getOilLevel();
                                ledGreenCounter = pgm_read_byte(&Constants::ledGreenDelays[player.getOilLevel() - 1]);

                                #ifdef SOUNDS
                                sound.tones(Sounds::scoreAdded);
                                #endif

                            }

                            if (throwOil == ThrowOil::None && player.getOilLevel() > 0) throwOil = ThrowOil::RH_Top;
                            player.setOilLevel(0);
                            player.setXPosition(XPosition::Position_Throwing_RH);
                            break;

                        case XPosition::Position_Throwing_LH:
                            player.setXPosition(XPosition::Position_Outside_LH);
                            break;

                        case XPosition::Position_Outside_LH:
                            gameScene = GameScene::Indoors; 
                            player.incXPosition();
                            break;

                        default: break;

                    }

                }

            }
            else if (gameOver && gameOverCounter == 0) {

                if (justPressedButton & A_BUTTON)                   { gameState = GameState::HighScore_Init; }

            }

            break;

    }



    // --------------------------------------------------------------------------
    //  Render the screen ..

    switch (gameScene) {

        case GameScene::Indoors:

            Sprites::drawOverwrite(0, 0, Images::Indoors, 0);
            renderPlayer_Indoors(0);
            renderCatcherMap(0);
            break;
            

        case GameScene::Outdoors:
    
            for (uint8_t x = 0; x < 13; x++) {

                Sprites::drawOverwrite(0, 0 - outdoorsYOffset + (x * 8), Images::Outdoors, x);

            }

            renderPlayer_Outdoors(0, outdoorsYOffset);
            renderThrowingOil(0, outdoorsYOffset);
            renderCatcher(0, outdoorsYOffset);
            renderBystanders(0, outdoorsYOffset);
            break;

    }

    renderOils(gameScene, outdoorsYOffset);
    renderScoreboard(gameScene);


    // Game over ?

    if (gameOver && gameOverCounter == 0) {

        Sprites::drawExternalMask(34, 24, Images::GameOver, Images::GameOver_Mask, 0, 0);

    }

}


void catchOil(XPosition xPosition) {

    for (uint8_t x = 0; x < Constants::number_Of_Oils; x++) {

        Oil &oil = oils.getOil(x); 

        if (oil.getYPosition() != YPosition::None && oil.getXPosition() == xPosition) {

            switch (oil.getYPosition()) {

                case YPosition::Falling_09 ... YPosition::Falling_11:
                    if (player.incOilLevel()) {

                        oil.setYPosition(YPosition::None);
                        score++;
                        #ifdef SOUNDS
                        sound.tones(Sounds::catchOil);
                        #endif

                    }
                    break;

                default: break;
                
            }
 
        }       

    }

}


bool updateThrowOil(ThrowOil &throwOil) {

    switch (throwOil) {

        case ThrowOil::LH_Top:
        case ThrowOil::RH_Top:
        case ThrowOil::LH_Miss_Bottom_Start ... ThrowOil::LH_Miss_Bottom_End:
        case ThrowOil::RH_Miss_Bottom_Start ... ThrowOil::RH_Miss_Bottom_End:
            throwOil++;
            break;

        case ThrowOil::LH_Middle:
            if (catcher.isCatching(Direction::Left)) {
                throwOil = ThrowOil::LH_Bottom;
            }
            else {

                throwOil = ThrowOil::LH_Miss_Down_Start;
                #ifdef SOUNDS
                sound.tones(Sounds::falling);
                #endif

            }
            break;
            
        case ThrowOil::LH_Bottom:
        case ThrowOil::RH_Bottom:
            throwOil = ThrowOil::None;
            break;

        case ThrowOil::LH_Miss_Down_Start ... ThrowOil::LH_Miss_Down_End:
            
            throwOil++;
            outdoorsYOffset = outdoorsYOffset + 4;
            
            if (throwOil == ThrowOil::LH_Miss_Bottom_Start) {
            
                if (numberOfLives_Outdoors > 0) {

                    numberOfLives_Outdoors--;
                    ledRedCounter = Constants::led_Red_Delay;;

                    if (numberOfLives_Outdoors == 0) {

                        return true;
                        
                    }

                }

            }
            break;

        case ThrowOil::LH_Miss_Up_Start ... ThrowOil::LH_Miss_Up_NearlyEnd:
            if (!gameOver) {
                throwOil++;
                outdoorsYOffset = outdoorsYOffset - 4;
            }
            break;

        case ThrowOil::RH_Middle:
            if (catcher.isCatching(Direction::Right)) {
                throwOil = ThrowOil::RH_Bottom;
            }
            else {

                throwOil = ThrowOil::RH_Miss_Down_Start;
                #ifdef SOUNDS
                sound.tones(Sounds::falling);
                #endif

            }
            break;

        case ThrowOil::RH_Miss_Down_Start ... ThrowOil::RH_Miss_Down_End:

            throwOil++;
            outdoorsYOffset = outdoorsYOffset + 4;

            if (throwOil == ThrowOil::RH_Miss_Bottom_Start) {

                if (numberOfLives_Outdoors > 0) {      

                    numberOfLives_Outdoors--;
                    ledRedCounter = Constants::led_Red_Delay;;

                    if (numberOfLives_Outdoors == 0) {

                        return true;
                        
                    }

                }

            }
            break;

        case ThrowOil::RH_Miss_Up_Start ... ThrowOil::RH_Miss_Up_NearlyEnd:
            if (!gameOver) {
                throwOil++;
                outdoorsYOffset = outdoorsYOffset - 4;
            }
            break;

        default: break;

    }

    return false;

}
#include <Arduboy2.h>


// ----------------------------------------------------------------------------
//  Render player indoors ..
//
void renderPlayer_Indoors(int8_t xOffset) {

    Sprites::drawExternalMask(player.getXDisplay() + xOffset, 29, Images::Player, Images::Player_Mask, player.getFrame(), player.getFrame());

    switch (player.getXPosition()) {

        case XPosition::Position_Tipping_LH:

            if (player.getOilLevel() > 0) {

                Sprites::drawOverwrite(xOffset, 38, Images::Bucket_Tilt_LH, player.getOilLevel() - 1);

            }

            break;

        case XPosition::Position_Tipping_RH:

            if (player.getOilLevel() > 0) {

                Sprites::drawOverwrite(126 + xOffset, 37, Images::Bucket_Tilt_RH, player.getOilLevel() - 1);

            }

            break;

        default:

            if (player.getOilLevel() > 0) {

                Sprites::drawErase(player.getXDisplay() + 1 + xOffset, 33, Images::Bucket, player.getOilLevel() - 1);

            }

            break;

    }            

}


// ----------------------------------------------------------------------------
//  Render player outdoors ..
//
void renderPlayer_Outdoors(int8_t xOffset, uint8_t yOffset) {

    switch (player.getXPosition()) {

        case XPosition::Position_Outside_LH:
            Sprites::drawExternalMask(15 + xOffset, 14 - yOffset, Images::Player_Outside, Images::Player_Outside_Mask, 0, 0);
            break;

        case XPosition::Position_Throwing_LH:
            Sprites::drawExternalMask(15 + xOffset, 14 - yOffset, Images::Player_Outside, Images::Player_Outside_Mask, 1, 1);
            break;

        case XPosition::Position_Outside_RH:
            Sprites::drawExternalMask(88 + xOffset, 14 - yOffset, Images::Player_Outside, Images::Player_Outside_Mask, 2, 2);
            break;

        case XPosition::Position_Throwing_RH:
            Sprites::drawExternalMask(88 + xOffset, 14 - yOffset, Images::Player_Outside, Images::Player_Outside_Mask, 3, 3);
            break;

        default: break;

    }

}


// ----------------------------------------------------------------------------
//  Render player outdoors ..
//
void renderThrowingOil(int8_t xOffset, uint8_t yOffset) {

    switch (throwOil) {

        case ThrowOil::LH_Top:
            Sprites::drawErase(13 + xOffset, 31 - yOffset, Images::ThrowOil_LH, 0);
            break;

        case ThrowOil::LH_Middle:
            Sprites::drawErase(12 + xOffset, 35 - yOffset, Images::ThrowOil_LH, 1);
            break;

        case ThrowOil::LH_Bottom:
            Sprites::drawErase(13 + xOffset, 39 - yOffset, Images::ThrowOil_LH, 2);
            break;

        case ThrowOil::LH_Miss_Down_Start ... ThrowOil::LH_Miss_Down_End:
            Sprites::drawErase(12 + xOffset - ((static_cast<uint8_t>(throwOil) - static_cast<uint8_t>(ThrowOil::LH_Miss_Down_Start)) / 4), 39 + ((static_cast<uint8_t>(throwOil) - static_cast<uint8_t>(ThrowOil::LH_Miss_Down_Start)) * 4) - yOffset, Images::ThrowOil_LH, 1);
            break;

        case ThrowOil::LH_Miss_Bottom_Start ... ThrowOil::LH_Miss_Bottom_End_Of_Splash:
            Sprites::drawErase(-1 + xOffset, 70 - yOffset, Images::Oil_Splash, static_cast<uint8_t>(throwOil) - static_cast<uint8_t>(ThrowOil::LH_Miss_Bottom_Start));
            break;

        case ThrowOil::RH_Top:
            Sprites::drawErase(112 + xOffset, 31 - yOffset, Images::ThrowOil_RH, 0);
            break;

        case ThrowOil::RH_Middle:
            Sprites::drawErase(113 + xOffset, 35 - yOffset, Images::ThrowOil_RH, 1);
            break;

        case ThrowOil::RH_Bottom:
            Sprites::drawErase(112 + xOffset, 39 - yOffset, Images::ThrowOil_RH, 2);
            break;

        case ThrowOil::RH_Miss_Down_Start ... ThrowOil::RH_Miss_Down_End:
            Sprites::drawErase(113 + xOffset + ((static_cast<uint8_t>(throwOil) - static_cast<uint8_t>(ThrowOil::RH_Miss_Down_Start)) / 4), 39 + ((static_cast<uint8_t>(throwOil) - static_cast<uint8_t>(ThrowOil::RH_Miss_Down_Start)) * 4) - yOffset, Images::ThrowOil_LH, 1);
            break;

        case ThrowOil::RH_Miss_Bottom_Start ... ThrowOil::RH_Miss_Bottom_End_Of_Splash:
            Sprites::drawErase(105 + xOffset, 70 - yOffset, Images::Oil_Splash, static_cast<uint8_t>(throwOil) - static_cast<uint8_t>(ThrowOil::RH_Miss_Bottom_Start));
            break;


        default: break;

    }

}

// ----------------------------------------------------------------------------
//  Render catcher ..
//
void renderCatcher(int8_t xOffset, uint8_t yOffset) {

    Sprites::drawExternalMask(catcher.getXDisplay() + xOffset, 41 - yOffset, Images::Catcher, Images::Catcher_Mask, catcher.getFrame(), catcher.getFrame());

}


// ----------------------------------------------------------------------------
//  Render catcher map ..
//
void renderCatcherMap(int8_t xOffset) {

    arduboy.drawFastVLine(player.getXDisplayMap() - 4 + xOffset, 21, 3, BLACK);
    arduboy.drawFastVLine(catcher.getXDisplayMap() - 4 + xOffset, 27, 3, BLACK);

}


// ----------------------------------------------------------------------------
//  Render bystanders
//
void renderBystanders(int8_t xOffset, uint8_t yOffset) {

    uint8_t bystander1 = 0;
    uint8_t bystander2 = 0;
    
    switch (throwOil) {

        case ThrowOil::LH_Miss_Bottom_Start ... ThrowOil::LH_Miss_Bottom_End:
            bystander1 = 1;
            bystander2 = 2;
            break;

        case ThrowOil::RH_Miss_Bottom_Start ... ThrowOil::RH_Miss_Bottom_End:
            bystander1 = 0;
            bystander2 = 3;
            break;

        default: 
            bystander1 = 0;
            bystander2 = 2;
            break;
    
    }

    Sprites::drawExternalMask(2 + xOffset, 76 - yOffset, Images::Bystanders, Images::Bystanders_Mask, bystander1, bystander1);
    Sprites::drawExternalMask(108 + xOffset, 76 - yOffset, Images::Bystanders, Images::Bystanders_Mask, bystander2, bystander2);

}

// ----------------------------------------------------------------------------
//  Render oil droplets ..
//
void renderOils(GameScene &gameScene, uint8_t yOffset) {

    for (uint8_t x = 0; x < Constants::number_Of_Oils; x++) {

        Oil oil = oils.getOil(x); 

        renderOil(gameScene, oil, 0, outdoorsYOffset);

    }

}

// ----------------------------------------------------------------------------
//  Render oil droplets indoors ..
//
void renderOil(GameScene gameScene, Oil &oil, int8_t xOffset, uint8_t yOffset) {

    if (oil.getYPosition() != YPosition::None) {

        if (gameScene == GameScene::Indoors) {

            switch (oil.getYPosition()) {

                case YPosition::StartDrip_00 ... YPosition::StartDrip_01:
                    Sprites::drawExternalMask(oil.getXDisplay() + xOffset, oil.getYDisplay(), Images::Oil_Drop, Images::Oil_Drop_Mask, 0, 0);
                    break;

                case YPosition::StartDrip_02 ... YPosition::StartDrip_03:
                    Sprites::drawExternalMask(oil.getXDisplay() + xOffset, oil.getYDisplay(), Images::Oil_Drop, Images::Oil_Drop_Mask, 1, 1);
                    break;

                case YPosition::Falling_04 ... YPosition::Falling_13:
                    Sprites::drawExternalMask(oil.getXDisplay() + xOffset, oil.getYDisplay(), Images::Oil_Drop, Images::Oil_Drop_Mask, 2, 2);
                    break;

                case YPosition::Fire_00 ... YPosition::Fire_03:
                    Sprites::drawExternalMask(oil.getXDisplay() - 1 + xOffset, 24, Images::Oil_Fire, Images::Oil_Fire_Mask, static_cast<uint8_t>(oil.getYPosition()) - static_cast<uint8_t>(YPosition::Fire_00), static_cast<uint8_t>(oil.getYPosition()) - static_cast<uint8_t>(YPosition::Fire_00));
                    break;

                case YPosition::Fire_04 ... YPosition::Fire_07:
                    Sprites::drawExternalMask(oil.getXDisplay() - 1 + xOffset, 24, Images::Oil_Fire, Images::Oil_Fire_Mask, static_cast<uint8_t>(YPosition::Fire_07) - static_cast<uint8_t>(oil.getYPosition()), static_cast<uint8_t>(YPosition::Fire_07) - static_cast<uint8_t>(oil.getYPosition()));
                    break;
                
                default: break;

            }

        }
        else {

            switch (oil.getYPosition()) {

                case YPosition::StartDrip_00 ... YPosition::Falling_13:
                    arduboy.drawPixel(oil.getXDisplayMap() + xOffset, oil.getYDisplayMap() - yOffset, BLACK);
                    break;

                case YPosition::Fire_00 ... YPosition::Fire_03:
                    if (oil.getX() == 2) {
                        Sprites::drawErase(oil.getXDisplayMap() + xOffset - 1, 20 - yOffset + 8, Images::Oil_Fire_Map, static_cast<uint8_t>(oil.getYPosition()) - static_cast<uint8_t>(YPosition::Fire_00));
                    }
                    break;

                case YPosition::Fire_04 ... YPosition::Fire_07:
                    if (oil.getX() == 2) {
                        Sprites::drawErase(oil.getXDisplayMap() + xOffset - 1, 20 - yOffset + 8, Images::Oil_Fire_Map, static_cast<uint8_t>(YPosition::Fire_07) - static_cast<uint8_t>(oil.getYPosition()));
                    }
                    break;
                
                default: break;

            }

        }

    }    

}

// ----------------------------------------------------------------------------
//  Render scoreboards ..
//
void renderScoreboard(GameScene gameScene) {

    uint8_t digits[3] = {};
    extractDigits(digits, score);
    
    switch (gameScene) {

        case GameScene::Indoors:

            for (uint8_t i = 3, x = 111; i > 0; i--, x = x + 4) {

                Sprites::drawErase(x, 10, Images::Font, digits[i - 1]);

            }

            if (numberOfLives_Indoors < 3) {

                for (uint8_t i = 0, x = 120; i < 3 - numberOfLives_Indoors; i++, x = x - 8) {

                    Sprites::drawErase(x, 16, Images::Life, 0);

                }

                Sprites::drawErase(108, 24, Images::Miss, 0);

            }

            break;

        case GameScene::Outdoors:

            for (uint8_t i = 3, x = 1; i > 0; i--, x = x + 4) {

                Sprites::drawErase(x, 1, Images::Font, digits[i - 1]);

            }

            if (numberOfLives_Outdoors < 3) {

                for (uint8_t i = 0, x = 123; i < 3 - numberOfLives_Outdoors; i++, x = x - 5) {

                    Sprites::drawExternalMask(x, 1, Images::outsideMiss, Images::outsideMiss_Mask, 0, 0);

                }

            }

            break;

    }

}


static constexpr uint8_t ppotFrames = (1.6 *50); // 1.6s animation loop


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void splashScreen_Init() {

    counter = 0;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void splashScreen() { 

    auto justPressed = arduboy.justPressedButtons();

    if (justPressed > 0 && counter > 0) {

    counter = 32;  

    }

    if (justPressed > 0 && counter == 0) {

        #ifdef SOUNDS    
        sound.tone(NOTE_A2, 45);
        sound.tone(NOTE_F1, 45);
        #endif

        counter = 1;

    }

    if (counter > 0) {

        counter++;

        if (counter == 33) gameState = GameState::Title_Init; 

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
        // Draw directly to the screen buffer
        arduboy.sBuffer[45 +(3*WIDTH)] = 0xA5;
        arduboy.sBuffer[46 +(3*WIDTH)] = 0x95;
    }


}
#include <Arduboy2.h>

void title() {

    counter++;
    #ifdef SOUNDS
        if (soundCounter > 0) soundCounter--;
    #endif

    Sprites::drawOverwrite(0, 0, Images::Title, 0);

    switch (counter) {

        case 60 ... 63:
            Sprites::drawOverwrite(81, 18, Images::Title_Eyes, 0);
            break;

        case 64 ... 67:
            Sprites::drawOverwrite(81, 18, Images::Title_Eyes, 1);
            break;

        case 68 ... 71:
            Sprites::drawOverwrite(81, 18, Images::Title_Eyes, 2);
            break;


        case 120:
            counter = 0;
            break;
    }

    if (arduboy.justPressed(A_BUTTON)) { 

        gameState = GameState::Instructions_Init;

    }

    if (arduboy.justPressed(B_BUTTON)) { 

        score = 0;
        gameState = GameState::HighScore_Init;

    }

    #ifdef SOUNDS

        if (arduboy.justPressed(UP_BUTTON) || arduboy.justPressed(DOWN_BUTTON)) {
            soundsOnOff = EEPROM_Utils::toggleSoundSettings(arduboy);
            soundCounter = 32;
        }

        if (soundCounter > 0) {
            Sprites::drawOverwrite(116, 54, Images::Sound, !soundsOnOff);
        }

    #endif

}

