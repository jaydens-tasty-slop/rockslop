#include <Arduino.h>

#include "src/utils/Arduboy2Ext.h"
#include <ArduboyFX.h>

#include "fxdata/Images.h"
#include "src/images/Images.h"
#include "src/utils/Constants.h"
#include "src/utils/Enums.h"
#include "src/utils/EEPROM_Utils.h"
#include "src/utils/Structs.h"

Arduboy2Ext arduboy;
TitleScreenVars titleScreenVars;
StatisticsScreenVars statisticsScreenVars;
GamePlayVars gamePlayVars;

GameState gameState = GameState::SplashScreen_Init;

uint16_t man_delay = random(Constants::Delay_Low, Constants::Delay_High);
uint8_t man_Idx = 0;
AnimationSequence animation = AnimationSequence::Walk_RL;

/* Arduino IDE auto-prototypes */
void game_Init();
void game();
void drawSolution();
void drawGuesses(int8_t xOffset, int8_t yOffset);
void drawMan(uint8_t tmpManX, uint8_t tmpManB, uint8_t tmpManI, uint8_t tmpManP, int8_t tmpManK);
void drawKeyboard(uint8_t xOffset, uint8_t yOffset);
void splashScreen_Init();
void splashScreen();
void statistics_Init();
void statistics();
uint8_t widthOfNumber(uint16_t number);
void title_Init();
void title();
void moveCursor(Direction direction);
CheckState checkWord();

void setup() {

    arduboy.boot();
    arduboy.flashlight();
    arduboy.systemButtons();
    arduboy.setFrameRate(60);
    arduboy.initRandomSeed();

    FX::disableOLED();  
    FX::begin(FX_DATA_PAGE); 

    EEPROM_Utils::initEEPROM(false, GameMode::English, true);
    gamePlayVars.mode = EEPROM_Utils::getMode();

}

void loop() {
   
    if (!arduboy.nextFrame()) return; 
    arduboy.pollButtons();

    switch (gameState) {

        case GameState::SplashScreen_Init:

            splashScreen_Init();
            splashScreen();
            break;

        case GameState::SplashScreen:

            splashScreen();
            break;

        case GameState::Title_Init:

            gameState = GameState::Title;
            title_Init();
            title();
            break;

        case GameState::Title:

            title();
            break;

        case GameState::Game_Init:

            game_Init();
            game();
            break;

        case GameState::Game:

            game();
            break;

        case GameState::Stats_Init:

            statistics_Init();
            statistics();
            break;

        case GameState::Stats:

            statistics();
            break;

    }

    FX::enableOLED();              
    arduboy.display(CLEAR_BUFFER); 
    FX::disableOLED();             

}


#include <Arduboy2.h>     // required to build for Arduboy
#include <ArduboyFX.h>    // required to access the FX external flash
#include "fxdata/Images.h"
#include "src/utils/Constants.h"


void game_Init() {

    gamePlayVars.checkState = CheckState::Normal;
    gamePlayVars.cancelButton = 0;
    gamePlayVars.keyboard.reset();
    gamePlayVars.guesses.reset();

    gameState = GameState::Game;
    

    // Pick a random word ..

    #ifndef USE_BRINE

        switch (gamePlayVars.mode) {

            case GameMode::English:
                {
                    while (true) {

                        uint24_t numberOfWords = (Dictionary::English_EndOfWordData - Dictionary::English_Words) / 6;
                        FX::seekData(Dictionary::English_Words + (random(0, numberOfWords) * 6));
                        uint8_t status = FX::readPendingUInt8();

                        for (uint8_t i = 0; i < 5; i++) {
                            gamePlayVars.selectedWord[i] = FX::readPendingUInt8();
                        }

                        FX::readEnd();

                        if (status == 1) break;

                    }
                }
                break;

            case GameMode::French:
                {
                    uint24_t numberOfWords = (Dictionary::French_EndOfWordData - Dictionary::French_Words) / 6;
                    FX::seekData(Dictionary::French_Words + (random(0, numberOfWords) * 6));

                    uint8_t status = FX::readPendingUInt8();

                    for (uint8_t i = 0; i < 5; i++) {
                        gamePlayVars.selectedWord[i] = FX::readPendingUInt8();
                    }

                    FX::readEnd();
                }
                break;

        }                

    #endif

}

void game() {

    uint8_t tmpManX = 0;
    uint8_t tmpManB = 0;
    uint8_t tmpManI = 0;
    uint8_t tmpManP = 0;
    int8_t tmpManK = 0;


    if (man_delay == 0) {

        switch (animation) {

            case AnimationSequence::Think_L:

                FX::seekData(static_cast<uint24_t>(Animation::ManData_L + (man_Idx * 5)));
                tmpManX = FX::readPendingUInt8();
                tmpManK = FX::readPendingUInt8();
                tmpManB = FX::readPendingUInt8();
                tmpManI = FX::readPendingUInt8();
                tmpManP = FX::readPendingUInt8();
                FX::readEnd();
                break;

            case AnimationSequence::Think_R: 
                
                FX::seekData(static_cast<uint24_t>(Animation::ManData_R + (man_Idx * 5)));
                tmpManX = FX::readPendingUInt8();
                tmpManK = -FX::readPendingUInt8();
                tmpManB = FX::readPendingUInt8();
                tmpManI = FX::readPendingUInt8();
                tmpManP = FX::readPendingUInt8();
                FX::readEnd();
                break;

            case AnimationSequence::Walk_LR: 

                tmpManX = man_Idx;
                tmpManK = 0;
                tmpManB = 0;
                tmpManI = ((man_Idx % 6) / 2) + 3;
                tmpManP = 0;
                break;

            case AnimationSequence::Walk_RL: 

                tmpManX = 128 + 16 - man_Idx;
                tmpManK = 0;
                tmpManB = 0;
                tmpManI = ((man_Idx % 6) / 2);
                tmpManP = 0;
                break;

        }

    }


    // Press B to quit ..

    if (arduboy.pressed(B_BUTTON)) {

        gamePlayVars.cancelButton++;

        if (gamePlayVars.cancelButton == 64) {
            gamePlayVars.checkState = CheckState::Quit;
        }

    }
    else {

        gamePlayVars.cancelButton = false;

    }


    switch (gamePlayVars.checkState) {

        case CheckState::InvalidWord:
            if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
                gamePlayVars.checkState = CheckState::Normal;
            }
            break;

        case CheckState::CorrectWord:
            if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
                EEPROM_Utils::increaseCorrectWords(gamePlayVars.mode, gamePlayVars.guesses.yCursor);
                statisticsScreenVars.numberOfAttempts = gamePlayVars.guesses.yCursor + 1;
                gameState = GameState::Stats_Init;
            }
            break;

        case CheckState::TooManyAttempts:
            if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
                EEPROM_Utils::resetWiningStreak(gamePlayVars.mode);
                gameState = GameState::Stats_Init;
            }
            break;

        case CheckState::Quit:
            if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
                if (gamePlayVars.guesses.yCursor > 0) EEPROM_Utils::resetWiningStreak(gamePlayVars.mode);
                statisticsScreenVars.numberOfAttempts = 7;
                gameState = GameState::Stats_Init;
            }
            break;

        default:

            if (arduboy.justPressed(LEFT_BUTTON))   moveCursor(Direction::Left);
            if (arduboy.justPressed(RIGHT_BUTTON))  moveCursor(Direction::Right);

            if (arduboy.pressed(UP_BUTTON)) {
                
                switch (gamePlayVars.keyboard.state) {

                    case KeyboardState::Showing:
                    case KeyboardState::StartHiding:
                    case KeyboardState::StartShowing:
                        break;

                    default:

                        if (gamePlayVars.guesses.listY < 0) {
                            gamePlayVars.guesses.listY = gamePlayVars.guesses.listY + 1;
                        }
                        break;

                }

            }        

            if (arduboy.justPressed(UP_BUTTON)) {
                
                switch (gamePlayVars.keyboard.state) {

                    case KeyboardState::Showing:
                        moveCursor(Direction::Up);
                        break;

                    default:
                        break;

                }

            }        

            if (arduboy.pressed(DOWN_BUTTON))   {
                
                switch (gamePlayVars.keyboard.state) {

                    case KeyboardState::Showing:
                    case KeyboardState::StartHiding:
                    case KeyboardState::StartShowing:
                        break;

                    default:

                        if (gamePlayVars.guesses.listY > Constants::scroll_Limits[gamePlayVars.guesses.yCursor]) {
                            gamePlayVars.guesses.listY = gamePlayVars.guesses.listY - 1;
                        }
                        else{
                            gamePlayVars.keyboard.state = KeyboardState::StartShowing;
                        }

                        break;

                }

            }

            if (arduboy.justPressed(DOWN_BUTTON))   {
                
                switch (gamePlayVars.keyboard.state) {

                    case KeyboardState::Showing:
                        moveCursor(Direction::Down);
                        break;

                    default:
                        break;

                }

            }

            if (arduboy.justPressed(B_BUTTON)) {

                if (gamePlayVars.guesses.xCursor > 0) {

                    gamePlayVars.guesses.xCursor--;
                    gamePlayVars.guesses.chars[gamePlayVars.guesses.yCursor][gamePlayVars.guesses.xCursor] = ' ';
                    gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][gamePlayVars.guesses.xCursor] = GuessState::Dashed;

                }

            }

            if (arduboy.justPressed(A_BUTTON)) {

                if (gamePlayVars.keyboard.xCursor == 0 && gamePlayVars.keyboard.yCursor == 2) {

                    if (gamePlayVars.guesses.xCursor > 0) {

                        gamePlayVars.guesses.xCursor--;
                        gamePlayVars.guesses.chars[gamePlayVars.guesses.yCursor][gamePlayVars.guesses.xCursor] = ' ';
                        gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][gamePlayVars.guesses.xCursor] = GuessState::Dashed;

                    }

                }
                else if (gamePlayVars.keyboard.xCursor == 8 && gamePlayVars.keyboard.yCursor == 2) {

                    // Are there any spaces?

                    bool hasSpace = false;

                    for (uint8_t i = 0; i < 5; i++) {

                        if (gamePlayVars.guesses.chars[gamePlayVars.guesses.yCursor][i] == ' ') {

                            hasSpace = true;
                            break;

                        }

                    }

                    if (!hasSpace) {

                        gamePlayVars.checkState = checkWord();

                        switch (gamePlayVars.checkState) {

                            case CheckState::CorrectWord:
                                gamePlayVars.showInvalidWord_Count = 19;
                                break;

                            case CheckState::InvalidWord:
                                gamePlayVars.showInvalidWord_Count = 15;
                                break;

                            case CheckState::RealWord:

                                if (gamePlayVars.guesses.yCursor == 5) {

                                    gamePlayVars.checkState = CheckState::TooManyAttempts;

                                }
                                else {
                                    
                                    gamePlayVars.guesses.xCursor = 0;
                                    gamePlayVars.guesses.yCursor = gamePlayVars.guesses.yCursor + 1;

                                    if (gamePlayVars.guesses.yCursor >= 2) {
                                        gamePlayVars.guesses.listY = -((gamePlayVars.guesses.yCursor - 1) * Constants::guess_Spacing);
                                    }

                                }
                                break;

                            default: break;

                        }

                    }

                }
                else {
                    
                    //if (keyboard[Constants::key_Map[gamePlayVars.keyboard.yCursor][gamePlayVars.keyboard.xCursor]] != KeyState::Invisible) {

                        if (gamePlayVars.guesses.xCursor < 5) {

                            gamePlayVars.guesses.chars[gamePlayVars.guesses.yCursor][gamePlayVars.guesses.xCursor] = 65 + Constants::key_Map[gamePlayVars.keyboard.yCursor][gamePlayVars.keyboard.xCursor];

                        }

                        if (gamePlayVars.guesses.xCursor < 5) {

                            gamePlayVars.guesses.xCursor++;

                            if (gamePlayVars.guesses.xCursor == 5) {

                                gamePlayVars.keyboard.xCursor = 8;
                                gamePlayVars.keyboard.yCursor = 2;

                            }

                        }

                    //}

                }

            }

            break;

    }

    drawMan(tmpManX, tmpManB, tmpManI, tmpManP, tmpManK);
    drawGuesses(tmpManK, gamePlayVars.guesses.listY);

    switch (gamePlayVars.checkState) {

        case CheckState::InvalidWord:
            if (gamePlayVars.mode == GameMode::English) {
                FX::drawBitmap(23, 48, Images::InvalidWord_EN, 0, dbmNormal);
            }
            else {
                FX::drawBitmap(5, 48, Images::InvalidWord_FR, 0, dbmNormal);
            }
            break;

        case CheckState::CorrectWord:
            if (gamePlayVars.mode == GameMode::English) {
                FX::drawBitmap(41, 48, Images::Correct_EN, 0, dbmNormal);
            }
            else {
                FX::drawBitmap(41, 48, Images::Correct_FR, 0, dbmNormal);
            }
            break;

        case CheckState::TooManyAttempts:
            drawSolution();
            if (gamePlayVars.mode == GameMode::English) {
                FX::drawBitmap(18, 37, Images::TooManyAttempts_EN, 0, dbmNormal);
            }
            else {
                FX::drawBitmap(10, 37, Images::TooManyAttempts_FR, 0, dbmNormal);
            }
            break;

        case CheckState::Quit:
            arduboy.fillRect(0, 35, WIDTH, 32, BLACK);
            if (gamePlayVars.mode == GameMode::English) {
                FX::drawBitmap(29, 37, Images::Quit_EN, 0, dbmNormal);
            }
            else {
                FX::drawBitmap(32, 37, Images::Quit_FR, 0, dbmNormal);
            }
            drawSolution();
            break;

        default:
            drawKeyboard(19 + tmpManK, gamePlayVars.keyboard.yPos);
            break;

    }


    // Show or hide keyboard ..

    switch (gamePlayVars.keyboard.state) {

        case KeyboardState::StartHiding:

            gamePlayVars.keyboard.yPos++;
            if (gamePlayVars.guesses.yCursor >= 2) gamePlayVars.guesses.listY++;

            if (gamePlayVars.keyboard.yPos == 64) {
                gamePlayVars.keyboard.state = KeyboardState::Hiding;
            }
            break;

        case KeyboardState::StartShowing:

            gamePlayVars.keyboard.yPos--;
            if (gamePlayVars.guesses.yCursor >= 2) gamePlayVars.guesses.listY--;

            if (gamePlayVars.keyboard.yPos == 33) {
                gamePlayVars.keyboard.state = KeyboardState::Showing;
            }
            break;
        
        default:
            break;
    }



    // Housekeeping ..

    if (gamePlayVars.showInvalidWord_Count > 0 && arduboy.getFrameCount(2) == 0) gamePlayVars.showInvalidWord_Count--;

    if (arduboy.getFrameCount(3) == 0) {

        if (man_delay > 0) {
            man_delay--;

            if (man_delay == 0) {
                man_Idx = 0;
            }
        }
        else {

            switch (animation) {

                case AnimationSequence::Think_L:

                    man_Idx++;
                    if (man_Idx > 155) {
                        man_delay = random(Constants::Delay_Low, Constants::Delay_High);
                        animation = AnimationSequence::Walk_LR;
                    }

                    break;

                case AnimationSequence::Think_R:

                    man_Idx++;
                    if (man_Idx > 155) {
                        man_delay = random(Constants::Delay_Low, Constants::Delay_High);
                        animation = AnimationSequence::Walk_RL;
                    }

                    break;

                case AnimationSequence::Walk_LR:

                    man_Idx++;
                    if (man_Idx > 128 + 16) {
                        man_delay = random(Constants::Delay_Low, Constants::Delay_High);
                        animation = AnimationSequence::Think_R;
                    }

                    break;

                case AnimationSequence::Walk_RL:

                    man_Idx++;
                    if (man_Idx > 128 + 16) {
                        man_delay = random(Constants::Delay_Low, Constants::Delay_High);
                        animation = AnimationSequence::Think_L;
                    }

                    break;


            }

        }

    }

}


void drawSolution() {

    // Render words ..

    arduboy.setTextColor(BLACK);

    for (uint8_t x = 0; x < 5; x++) {

        arduboy.fillRect(Constants::guess_Left + (x * Constants::guess_Spacing), 50 - 2, Constants::guess_Spacing - 2, Constants::guess_Spacing - 2);
        arduboy.setCursor(Constants::guess_Left + (x * Constants::guess_Spacing) + 3, 50);
        arduboy.print(gamePlayVars.selectedWord[x]);

    }

    arduboy.setTextColor(WHITE);

}


void drawGuesses(int8_t xOffset, int8_t yOffset) {

    for (uint8_t y = 0; y < gamePlayVars.guesses.yCursor + 1; y++) {


        // Animate last row?

        int8_t xMove = 0;
        int8_t yMove[5] = { 0, 0, 0, 0, 0 };
        
        switch (gamePlayVars.checkState) {

            case CheckState::InvalidWord:

                if (y == gamePlayVars.guesses.yCursor) {
                    if (gamePlayVars.showInvalidWord_Count > 0) {
                        xMove = Constants::shake[gamePlayVars.showInvalidWord_Count];
                    }
                }
                break;

            case CheckState::CorrectWord:

                if (y == gamePlayVars.guesses.yCursor) {

                    if (gamePlayVars.showInvalidWord_Count > 0 && gamePlayVars.showInvalidWord_Count < 15) yMove[0] = Constants::shake[gamePlayVars.showInvalidWord_Count];
                    if (gamePlayVars.showInvalidWord_Count > 1 && gamePlayVars.showInvalidWord_Count < 16) yMove[1] = Constants::shake[gamePlayVars.showInvalidWord_Count - 1];
                    if (gamePlayVars.showInvalidWord_Count > 2 && gamePlayVars.showInvalidWord_Count < 17) yMove[2] = Constants::shake[gamePlayVars.showInvalidWord_Count - 2];
                    if (gamePlayVars.showInvalidWord_Count > 3 && gamePlayVars.showInvalidWord_Count < 18) yMove[3] = Constants::shake[gamePlayVars.showInvalidWord_Count - 3];
                    if (gamePlayVars.showInvalidWord_Count > 4 && gamePlayVars.showInvalidWord_Count < 19) yMove[4] = Constants::shake[gamePlayVars.showInvalidWord_Count - 4];

                }

                break;

            default: break;

        }


        // Render words ..

        for (uint8_t x = 0; x < 5; x++) {

            switch (gamePlayVars.guesses.state[y][x]) {

                case GuessState::Correct:
                    arduboy.setTextColor(BLACK);
                    arduboy.fillRect(Constants::guess_Left + (x * Constants::guess_Spacing) + xMove + xOffset, y * Constants::guess_Spacing + yOffset + yMove[x], Constants::guess_Spacing - 2, Constants::guess_Spacing - 2);
                    arduboy.setCursor(Constants::guess_Left + (x * Constants::guess_Spacing) + 3 + xMove + xOffset, y * Constants::guess_Spacing + 2 + yOffset + yMove[x]);
                    arduboy.print(gamePlayVars.guesses.chars[y][x]);
                    break;

                case GuessState::WrongPosition:
                    arduboy.setTextColor(WHITE);
                    arduboy.setCursor(Constants::guess_Left + (x * Constants::guess_Spacing) + 3 + xMove + xOffset, y * Constants::guess_Spacing + 2 + yOffset + yMove[x]);
                    arduboy.print(gamePlayVars.guesses.chars[y][x]);
                    arduboy.drawRect(Constants::guess_Left + (x * Constants::guess_Spacing) + xMove + xOffset, y * Constants::guess_Spacing + yOffset + yMove[x], Constants::guess_Spacing - 2, Constants::guess_Spacing - 2);
                    break;

                case GuessState::Dashed:
                    arduboy.setTextColor(WHITE);
                    arduboy.setCursor(Constants::guess_Left + (x * Constants::guess_Spacing) + 3 + xMove + xOffset, y * Constants::guess_Spacing + 2 + yOffset + yMove[x]);
                    arduboy.print(gamePlayVars.guesses.chars[y][x]);
                    arduboy.drawPixel(Constants::guess_Left + (x * Constants::guess_Spacing) + 1 + xMove + xOffset, ((y + 1) * Constants::guess_Spacing) - 2 + yOffset + yMove[x]);
                    arduboy.drawPixel(Constants::guess_Left + (x * Constants::guess_Spacing) + 3 + xMove + xOffset, ((y + 1) * Constants::guess_Spacing) - 2 + yOffset + yMove[x]);
                    arduboy.drawPixel(Constants::guess_Left + (x * Constants::guess_Spacing) + 5 + xMove + xOffset, ((y + 1) * Constants::guess_Spacing) - 2 + yOffset + yMove[x]);
                    arduboy.drawPixel(Constants::guess_Left + (x * Constants::guess_Spacing) + 7 + xMove + xOffset, ((y + 1) * Constants::guess_Spacing) - 2 + yOffset + yMove[x]);
                    arduboy.drawPixel(Constants::guess_Left + (x * Constants::guess_Spacing) + 9 + xMove + xOffset, ((y + 1) * Constants::guess_Spacing) - 2 + yOffset + yMove[x]);
                    break;


                default:
                    arduboy.setTextColor(WHITE);
                    arduboy.setCursor(Constants::guess_Left + (x * Constants::guess_Spacing) + 3 + xMove + xOffset, y * Constants::guess_Spacing + 2 + yOffset + yMove[x]);
                    arduboy.print(gamePlayVars.guesses.chars[y][x]);
                    break;

            }

        }

        arduboy.setTextColor(WHITE);

    }

}


void drawMan(uint8_t tmpManX, uint8_t tmpManB, uint8_t tmpManI, uint8_t tmpManP, int8_t tmpManK) {


    // Draw man?

    if (man_delay == 0) {

        switch (animation) {

            case AnimationSequence::Think_L:

                if (tmpManP > 0) {
                    FX::drawBitmap(2, 28, Images::Man_Word[tmpManP - 1], 0, dbmNormal);
                }

                switch (tmpManB) {

                    case 1:
                        FX::drawBitmap(tmpManX - 16, 36, Images::LightBulb, 0, dbmNormal);
                        break;

                }

                FX::drawBitmap(tmpManX - 16, 45, Images::Man[tmpManI], 0, dbmNormal); 
                break;

            case AnimationSequence::Think_R:
                
                if (tmpManP > 0) {
                    FX::drawBitmap(106, 28, Images::Man_Word[tmpManP - 1], 0, dbmNormal);
                }

                switch (tmpManB) {

                    case 1:
                        FX::drawBitmap(136 - tmpManX, 36, Images::LightBulb, 0, dbmNormal);
                        break;

                }

                FX::drawBitmap(136 - tmpManX, 45, Images::Man[tmpManI], 0, dbmNormal);
                break;
        
            case AnimationSequence::Walk_LR:
            case AnimationSequence::Walk_RL:
                FX::drawBitmap(tmpManX - 16, 45, Images::Man[tmpManI], 0, dbmNormal); 
                break;

        }

    }

}
#include <Arduboy2.h>   
#include <ArduboyFX.h>   
#include "fxdata/Images.h"

void drawKeyboard(uint8_t xOffset, uint8_t yOffset) {

                                  // A  B  C  D  E  F  G  H  I  J  K  L  M  N  O   P  Q  R  S  T  U  V  Q  X  Y  Z
    constexpr uint8_t key_PosX[] = { 1, 6, 4, 3, 3, 4, 5, 6, 8, 7, 8, 9, 8, 7, 9, 10, 1, 4, 2, 5, 7, 5, 2, 3, 6, 2 }; 
    constexpr uint8_t key_PosY[] = { 2, 3, 3, 2, 1, 2, 2, 2, 1, 2, 2, 2, 3, 3, 1,  1, 1, 1, 2, 1, 1, 3, 1, 3, 1, 3 }; 

    constexpr uint8_t key_Width = 10;
    constexpr uint8_t key_Height = 11;

    arduboy.fillRect(xOffset - 1, yOffset + 1, 93, 50, BLACK);

    for (uint8_t i = 0; i < 26; i++) {

        uint8_t xPos = key_PosX[i] - 1;
        uint8_t yPos = key_PosY[i] - 1;

        uint8_t x = xPos * (key_Width - 1) + (key_PosY[i] >= 2 ? 4 : 0) + (key_PosY[i] == 3 ? -2 : 0);
        uint8_t y = yPos * (key_Height - 1); 

        if (xPos == gamePlayVars.keyboard.xCursor && yPos == gamePlayVars.keyboard.yCursor && gamePlayVars.keyboard.state == KeyboardState::Showing) {

            FX::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_B, i, dbmWhite);

            if (gamePlayVars.keyboard.keys[Constants::key_Map[gamePlayVars.keyboard.yCursor][gamePlayVars.keyboard.xCursor]] != KeyState::Invisible) {
                FX::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_B_Cursor, 0, dbmMasked);
            }
            else {
                FX::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_B_Cursor_Half, 0, dbmMasked);
            }

        }
        else {

            if (gamePlayVars.keyboard.keys[i] == KeyState::Visible) {
                FX::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_W, i, dbmWhite);
            }
            else {
                FX::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_B, i, dbmWhite);
            }

        }

    }


    // Delete key ..

    if (gamePlayVars.keyboard.xCursor == 0 && gamePlayVars.keyboard.yCursor == 2) {
        FX::drawBitmap(xOffset, 2 * (key_Height - 1) + 1 + yOffset, Images::Keyboard_B_BackSpace, 0, dbmWhite);
    }
    else {
        FX::drawBitmap(xOffset, 2 * (key_Height - 1) + 1 + yOffset, Images::Keyboard_W_BackSpace, 0, dbmWhite);
    }


    // Enter Key ..

    if (gamePlayVars.keyboard.xCursor == 8 && gamePlayVars.keyboard.yCursor == 2) {
        FX::drawBitmap(7 * (key_Width - 1) + (key_Width - 1) + xOffset + 2, 2 * (key_Height - 1) + 1 + yOffset, Images::Keyboard_B_Enter, 0, dbmWhite);
    }
    else {
        FX::drawBitmap(7 * (key_Width - 1) + (key_Width - 1) + xOffset + 2, 2 * (key_Height - 1) + 1 + yOffset, Images::Keyboard_W_Enter, 0, dbmWhite);
    }

    arduboy.drawFastHLine(xOffset, yOffset, 91);
    arduboy.drawFastVLine(xOffset - 1, yOffset + 1, 50);
    arduboy.drawFastVLine(xOffset + 91, yOffset + 1, 50);

    switch (gamePlayVars.keyboard.state) {

        case KeyboardState::Showing:
        case KeyboardState::StartShowing:
            FX::drawBitmap(xOffset + 86, yOffset - 4, Images::Arrow_Up, 0, dbmWhite);
            break;

        case KeyboardState::Hiding:
        case KeyboardState::StartHiding:
            FX::drawBitmap(xOffset + 86, yOffset - 4, Images::Arrow_Down, 0, dbmWhite);
            break;

    }

}
#include "src/utils/Arduboy2Ext.h"

// ----------------------------------------------------------------------------
//  Initialise state ..
//
void splashScreen_Init() {

    gameState = GameState::SplashScreen;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void splashScreen() { 

    auto justPressed = arduboy.justPressedButtons();

    if (justPressed > 0) {
        
        gameState = GameState::Title_Init; 

    }


    Sprites::drawOverwrite(32, 17, Images::PPOT, 0);

    uint8_t y = 17; // Default pixel position 1 is hidden in the top line of the image
    switch (arduboy.getFrameCount(48)) {

        case 12 ... 23:
            y = 30; // Move pixel down to position 2
            /*-fallthrough*/
        case 0 ... 11:
            Sprites::drawOverwrite(91, 25, Images::PPOT_Arrow, 0); // Flash 'Play' arrow
            break;

        case 24 ... 35:
            y = 31; // Move pixel down to position 3
            break;

        default: // 36 ... 47:
            y = 32; // Move pixel down to position 4
            break;

    }

    arduboy.drawPixel(52, y, WHITE); // Falling pixel represents the tape spooling
    if (y % 2 == 0) { // On even steps of pixel movement, update the spindle image
        Sprites::drawOverwrite(45, 28, Images::PPOT_Spindle, 0);
    }

}

#include "src/utils/Arduboy2Ext.h"

void statistics_Init() {

    statisticsScreenVars.reset();
    gameState = GameState::Stats;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void statistics() { 

    uint16_t gamesWon = 0;
    uint16_t gamesPlayed = 0;
    uint16_t currentStreak = 0;
    uint16_t maxStreak = 0;
    uint16_t percent = 0;

    if (gamePlayVars.mode == GameMode::English) {

        EEPROM.get(Constants::EEPROM_Games_Won_EN, gamesWon);
        EEPROM.get(Constants::EEPROM_Games_Played_EN, gamesPlayed);
        EEPROM.get(Constants::EEPROM_Current_Streak_EN, currentStreak);
        EEPROM.get(Constants::EEPROM_Max_Streak_EN, maxStreak);

    }
    else {

        EEPROM.get(Constants::EEPROM_Games_Won_FR, gamesWon);
        EEPROM.get(Constants::EEPROM_Games_Played_FR, gamesPlayed);
        EEPROM.get(Constants::EEPROM_Current_Streak_FR, currentStreak);
        EEPROM.get(Constants::EEPROM_Max_Streak_FR, maxStreak);

    }

    percent = (gamesPlayed == 0 ? 0 : (gamesWon * 100) / gamesPlayed);

    if (arduboy.justPressed(A_BUTTON)) {
                
        switch (statisticsScreenVars.mode) {

            case StatisticsMode::PageOne:
                statisticsScreenVars.mode = StatisticsMode::PageTwo;
                break;

            case StatisticsMode::PageTwo:
                gameState = GameState::Title_Init; 
                break;

        }

    }



    // Press B to reset ..

    if (arduboy.pressed(B_BUTTON)) {

        gamePlayVars.cancelButton++;

        if (gamePlayVars.cancelButton == 64) {
            EEPROM_Utils::initEEPROM(true, gamePlayVars.mode, false);
        }

    }
    else {

        gamePlayVars.cancelButton = false;

    }


    switch (statisticsScreenVars.mode) {

        case StatisticsMode::PageOne:

            if (gamePlayVars.mode == GameMode::English) {
                FX::drawBitmap(0, 0, Images::Statistics_Header_EN, 0, dbmNormal);
                FX::drawBitmap(0, 8, Images::Statistics_EN, 0, dbmNormal);
            }
            else {
                FX::drawBitmap(0, 0, Images::Statistics_Header_FR, 0, dbmNormal);
                FX::drawBitmap(0, 8, Images::Statistics_FR, 0, dbmNormal);
            }

            arduboy.setCursor(31, 14);
            if (gamesPlayed < 100) { arduboy.print("0"); }
            if (gamesPlayed < 10)  { arduboy.print("0"); }
            arduboy.print(gamesPlayed);

            arduboy.setCursor(84, 14);
            if (percent < 100) { arduboy.print("0"); }
            if (percent < 10)  { arduboy.print("0"); }
            arduboy.print(percent);

            arduboy.setCursor(31, 39);
            if (currentStreak < 100) { arduboy.print("0"); }
            if (currentStreak < 10)  { arduboy.print("0"); }
            arduboy.print(currentStreak);

            arduboy.setCursor(84, 39);
            if (maxStreak < 100) { arduboy.print("0"); }
            if (maxStreak < 10)  { arduboy.print("0"); }
            arduboy.print(maxStreak);

            break;

        case StatisticsMode::PageTwo:

            if (gamePlayVars.mode == GameMode::English) {
                FX::drawBitmap(0, 0, Images::Statistics_Header_EN, 0, dbmNormal);
            }
            else {
                FX::drawBitmap(0, 0, Images::Statistics_Header_FR, 0, dbmNormal);
            }

            FX::drawBitmap(0, 8, Images::Statistics_Vert, 0, dbmNormal);

            for (uint8_t i = 0; i < 6; i++) {

                uint8_t width = EEPROM_Utils::getPercent(gamePlayVars.mode, i);
                uint16_t val = EEPROM_Utils::getPercentVal(gamePlayVars.mode, i);
                uint8_t textWidth = widthOfNumber(val);

                if (val != 0 && textWidth > width) width = textWidth + 1;

                if (i + 1 == statisticsScreenVars.numberOfAttempts) {
                    arduboy.drawRect(12, 15 + (i * 8), width == 0 ? 1 : width, 7);
                }
                else {
                    arduboy.fillRect(12, 15 + (i * 8), width == 0 ? 1 : width, 7);
                }


                if (val > 0) {

                    uint8_t digits[5];
                    extractDigits(digits, val);
                    bool firstDigitRendered = false;

                    for (int8_t j = 4; j >= 0; j--) {

                        if (digits[j] != 0 || firstDigitRendered) {

                            if (i + 1 == statisticsScreenVars.numberOfAttempts) {
                                if (arduboy.getFrameCountHalf(32)) {
                                    FX::drawBitmap(12 + width - (j * 4) - 5, 16 + (i * 8), Images::Numbers3x5, digits[j], dbmNormal);
                                }
                            }
                            else {
                                FX::drawBitmap(12 + width - (j * 4) - 5, 16 + (i * 8), Images::Numbers3x5, digits[j], dbmReverse);
                            }

                            firstDigitRendered = true;

                        }

                    }

                }

            }

            break;

    }

    
}

uint8_t widthOfNumber(uint16_t number) {

    if (number >= 10000) return 5 * 4 + 2;
    if (number >= 1000) return 4 * 4 + 2;
    if (number >= 100) return 3 * 4 + 2;
    if (number >= 10) return 2 * 4 + 2;
    if (number >= 1) return 1 * 4 + 2;
    return 0;

}
#include "src/utils/Arduboy2Ext.h"

void title_Init() {

    gameState = GameState::Title;

    titleScreenVars.delay = random(16, 48);

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void title() { 

    if (arduboy.justPressed(A_BUTTON)) {
        
        gameState = GameState::Game_Init; 

    }

    if (arduboy.justPressed(LEFT_BUTTON) && gamePlayVars.mode == GameMode::French) {
        
        gamePlayVars.mode = GameMode::English; 
        EEPROM_Utils::setMode(gamePlayVars.mode);

    }

    if (arduboy.justPressed(RIGHT_BUTTON) && gamePlayVars.mode == GameMode::English) {
        
        gamePlayVars.mode = GameMode::French; 
        EEPROM_Utils::setMode(gamePlayVars.mode);

    }

    if (arduboy.justPressed(B_BUTTON)) {
        
        statisticsScreenVars.numberOfAttempts = 7;
        gameState = GameState::Stats_Init; 

    }

    uint8_t indexes[6] = { 0, 0, 0, 0, 0, 0, };

    if (titleScreenVars.delay == 0) {

        indexes[titleScreenVars.charIndex] = titleScreenVars.frameIndex - 1;

    }
    else {

        titleScreenVars.delay--;

        if (titleScreenVars.delay == 0) {

            titleScreenVars.charIndex = random(0, 6);
            titleScreenVars.frameIndex = 19;
            
        }

    }

    FX::drawBitmap(14, 11, Images::TitleScreen_L[indexes[0]], 0, dbmWhite);
    FX::drawBitmap(31, 11, Images::TitleScreen_E[indexes[1]], 0, dbmWhite);
    FX::drawBitmap(48, 11, Images::TitleScreen_W[indexes[2]], 0, dbmWhite);
    FX::drawBitmap(65, 11, Images::TitleScreen_O[indexes[3]], 0, dbmWhite);
    FX::drawBitmap(82, 11, Images::TitleScreen_R[indexes[4]], 0, dbmWhite);
    FX::drawBitmap(99, 11, Images::TitleScreen_D[indexes[5]], 0, dbmWhite);

    FX::drawBitmap(0, 29, Images::TitleScreen_Lower, 0, dbmWhite);
    FX::drawBitmap(gamePlayVars.mode == GameMode::English ? 16 : 74, 47, Images::Pointer, 0, dbmWhite);

    if (titleScreenVars.frameIndex > 0) {
    
        titleScreenVars.frameIndex--;
    
        if (titleScreenVars.frameIndex == 0) {

            titleScreenVars.delay = random(32, 200);

        }

    }

}

#include <Arduboy2.h>  
#include <ArduboyFX.h> 

void moveCursor(Direction direction) {

    switch (direction) {

        case Direction::Right:
    
            switch (gamePlayVars.keyboard.yCursor) {

                case 0:
                    if (gamePlayVars.keyboard.xCursor < 9) {
                        
                        gamePlayVars.keyboard.xCursor++;

                    }
                    else {

                        gamePlayVars.keyboard.xCursor = 0;

                    }
                    break;

                case 1:
                case 2:
                    if (gamePlayVars.keyboard.xCursor < 8) {
                        
                        gamePlayVars.keyboard.xCursor++;

                    }
                    else {

                        gamePlayVars.keyboard.xCursor = 0;

                    }
                    break;

            }

            break;

        case Direction::Left:
    
            switch (gamePlayVars.keyboard.yCursor) {

                case 0:
                    if (gamePlayVars.keyboard.xCursor > 0) {

                        gamePlayVars.keyboard.xCursor--;

                    }
                    else {

                        gamePlayVars.keyboard.xCursor = 9;

                    }

                    break;

                case 1:
                case 2:
                    if (gamePlayVars.keyboard.xCursor > 0) {

                        gamePlayVars.keyboard.xCursor--;

                    }
                    else {

                        gamePlayVars.keyboard.xCursor = 8;

                    }

                    break;

            }
             
            break;

        case Direction::Down:
    
            switch (gamePlayVars.keyboard.yCursor) {

                case 0:
                    if (gamePlayVars.keyboard.xCursor == 9) {
                        
                        gamePlayVars.keyboard.yCursor++;
                        gamePlayVars.keyboard.xCursor--;

                    }
                    else {
                        gamePlayVars.keyboard.yCursor++;
                    }
                    break;

                case 1:
                    gamePlayVars.keyboard.yCursor++;
                    break;

            }

            break;

        case Direction::Up:
    
            if (gamePlayVars.keyboard.yCursor == 0) {

                gamePlayVars.keyboard.state = KeyboardState::StartHiding;
                break;

            }
            else {

                switch (gamePlayVars.keyboard.yCursor) {

                    case 0:
                    case 2:
                        if (gamePlayVars.keyboard.xCursor == 9) {
                            gamePlayVars.keyboard.yCursor--;
                            gamePlayVars.keyboard.xCursor--;
                        }
                        else {
                            gamePlayVars.keyboard.yCursor--;
                        }
                        break;

                    case 1:
                        gamePlayVars.keyboard.yCursor--;
                        break;


                }


            }

            break;


    }
    
}

CheckState checkWord() {

    uint8_t guessWord[5];
    uint8_t testWord[5];
    uint8_t answer[5];
    
    for (uint8_t i = 0; i < 5; i++) {
        guessWord[i] = gamePlayVars.guesses.chars[gamePlayVars.guesses.yCursor][i];
        answer[i] = gamePlayVars.selectedWord[i];
        gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][i] = GuessState::Incorrect;
    }


    // Is the word correct?

    bool isCorrectWord = true;

    for (uint8_t i = 0; i < 5; i++) {
        if (guessWord[i] != answer[i]) {
            isCorrectWord = false;
        }
    }
    
    if (isCorrectWord) {

        for (uint8_t i = 0; i < 5; i++) {
            gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][i] = GuessState::Correct; 
        }

        return CheckState::CorrectWord;
    }
 

    // Get start of words using char one and two of guess ..

    switch (gamePlayVars.mode) {

        case GameMode::English:
            {
                uint24_t startPos = ((guessWord[0] - 65) * 26) + (guessWord[1] - 65);
                FX::seekData(Dictionary::English_AlphaMap + (startPos * 2));
                uint16_t alphaStart = FX::readPendingUInt16();
                FX::readEnd();


                // No matching combination?

                if (alphaStart == 0xFFFF) {
                    return CheckState::InvalidWord;
                }

                FX::seekData(Dictionary::English_Words + (alphaStart * 6));

            }
            break;

        case GameMode::French:
            {
                uint24_t startPos = ((guessWord[0] - 65) * 26) + (guessWord[1] - 65);
                FX::seekData(Dictionary::French_AlphaMap + (startPos * 2));
                uint16_t alphaStart = FX::readPendingUInt16();
                FX::readEnd();


                // No matching combination?

                if (alphaStart == 0xFFFF) {
                    return CheckState::InvalidWord;
                }

                FX::seekData(Dictionary::French_Words + (alphaStart * 6));

            }
            break;

    }

    while (true) {

        FX::readPendingUInt8(); // Status

        for (uint8_t i = 0; i < 5; i++) {
            testWord[i] = FX::readPendingUInt8();
        }

        if (testWord[0] > guessWord[0] || (testWord[0] == guessWord[0] && testWord[1] > guessWord[1])) {
            
            FX::readEnd();
            return CheckState::InvalidWord;

        }


        // Valid word!  What are correct?

        bool isValid = true;

        for (uint8_t i = 0; i < 5; i++) {

            if (testWord[i] != guessWord[i]) {
                isValid = false;
                break;
            };

        }

        if (isValid) {

            
            // Correct letters ..

            for (uint8_t i = 0; i < 5; i++) {

                if (guessWord[i] == answer[i]) { 
                    gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][i] = GuessState::Correct; 
                    answer[i] = ' ';
                }

            }

            for (uint8_t i = 0; i < 5; i++) {

                if (guessWord[i] != gamePlayVars.selectedWord[i]) { 

                    for (uint8_t j = 0; j < 5; j++) {

                        if (gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][j] != GuessState::Correct && testWord[i] == answer[j]) { 

                            gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][i] = GuessState::WrongPosition;
                            answer[j] = ' ';
                            break;
                    
                        }
                    
                    }


                }

            }


            // If the letter is not found but already exists, we cannot remove it from the keyboard ..

            for (uint8_t i = 0; i < 5; i++) {

                if (gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][i] == GuessState::Incorrect) {

                    for (uint8_t j = 0; j < 5; j++) {

                        if (guessWord[i] == gamePlayVars.selectedWord[j]) { 

                            gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][i] = GuessState::Duplicate;
                    
                        }
                    
                    }

                }

            }


            // Disable the keys for letters not found in solution ..

            for (uint8_t i = 0; i < 5; i++) {

                if (gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][i] == GuessState::Incorrect) { 

                    gamePlayVars.keyboard.keys[guessWord[i] - 65] = KeyState::Invisible;
            
                }

            }


            // Clear duplicate marks ..

            for (uint8_t j = 0; j < 5; j++) {

                if (gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][j] == GuessState::Duplicate) { 

                    gamePlayVars.guesses.state[gamePlayVars.guesses.yCursor][j] = GuessState::Incorrect;
            
                }
            
            }

            return CheckState::RealWord;

        }

    }

    FX::readEnd();

}

