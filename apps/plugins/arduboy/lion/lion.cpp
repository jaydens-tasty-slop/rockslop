#include <Arduino.h>
#include "src/utils/Arduboy2Ext.h"
#include "src/images/images.h"
#include "src/utils/consts.h"
#include "src/utils/EEPROM_Utils.h"
#include "src/entities/Entities.h"
#include "src/sounds/Sounds.h"
#include "src/ArduboyNewTones/ArduboyNewTones.h"
#include "src/ArduboyNewTones/ArduboyNewTonesPitches.h"

Arduboy2Ext arduboy;

#ifdef SOUNDS
    ArduboyTones sound(arduboy.audio.enabled);
#endif

Player player1;
Player player2;
Lion lion1;
Lion lion2;
Chair chair;

#ifdef SHOW_ROAR
    Explosions explosions;
    #ifdef SOUNDS
        SoundVars soundVars;
    #endif
#endif

Direction lionAttacking = Direction::None;
uint8_t lionAttackingIndex = 0;

#ifdef SPLASH
    GameState gameState = GameState::Splash_Init;
#else
    GameState gameState = GameState::Title_Init;
#endif

GameMode gameMode = GameMode::Easy;

uint8_t frameRate = 50;
int16_t counter = 10;
uint16_t score = 0;
uint8_t numberOfLives = 3;
uint8_t ledDelay = 0;
uint8_t marqueeCounter = 0;
uint8_t resetCounter = 0;

bool gameOver = false;

#ifdef SOUNDS
    uint8_t soundCounter = 0;
    bool sounds = arduboy.audio.enabled();
#endif

/* NewTones had a pitch-sweeping NOTE_ROAR; approximated by a growl of short low tones */
static const uint16_t roarTones[] PROGMEM = { 120,30,95,30,130,30,100,30,145,30,105,30,125,30,90,30,135,30,100,30,120,30,95,30,130,30,100,30,145,30,105,30,125,30,90,30,135,30,100,30,120,30,95,30,130,30,100,30,145,30, TONES_END };

/* Arduino IDE auto-prototypes */
void instructions_Init();
void instructions();
void moveLion(Lion &thisLion, Lion &otherLion);
void changeLevel(Lion &thisLion, Lion &otherLion, Direction newDirection);
void changeOneLevel(Lion &thisLion, Lion &otherLion, Direction newDirection);
void increaseScore();
void die();
void playGame_Init();
void lifeReset();
void playGame(void);
void renderBackground();
void renderScoreBoards(uint16_t score, uint8_t numberOfLives);
void renderPlayer(Player &player, const uint8_t *playerImages, const uint8_t *playerMask);
void renderLion(Lion &lion);
void splashScreen_Init();
void splashScreen();
void title();
void drawElements(bool title, uint8_t lionFrame);

void setup(void) {

	arduboy.boot();
	arduboy.flashlight();
	arduboy.systemButtons();

    #ifdef SOUNDS    
	    arduboy.audio.begin();
    #endif

	arduboy.initRandomSeed();
	arduboy.setFrameRate(40);
    arduboy.setRGBled(0, 0, 0);

    EEPROM_Utils::initEEPROM(false);
	
}

void loop(void) {

	if (!arduboy.nextFrame()) return;

	arduboy.pollButtons();

    switch (gameState) {

        #ifdef SPLASH

            case GameState::Splash_Init:

                splashScreen_Init();
                gameState = GameState::Splash;
                [[fallthrough]]

            case GameState::Splash:

                splashScreen();
                arduboy.display();
                break;

        #endif

        #ifdef SHOW_INSTUCTIONS

            case GameState::Instructions_Init:
                instructions_Init();
                gameState = GameState::Instructions;
                [[fallthrough]]

            case GameState::Instructions:
                instructions();
                arduboy.displayClearToWhite();
                break;

        #endif

        case GameState::Title_Init:

            gameState = GameState::Title;
            counter = -1;
            marqueeCounter = 0;
            numberOfLives = 3;
            gameOver = false;
            arduboy.setFrameRate(60);

            #ifdef SOUNDS
                soundCounter = 0;
                sound.tones(Sounds::Title);
            #endif

            #ifdef SHOW_ROAR
                explosions.reset();
            #endif
            [[fallthrough]]

        case GameState::Title:

            title();
            arduboy.displayClearToWhite();
            break;

        case GameState::PlayGame_Init:

            arduboy.setFrameRate(40);
            playGame_Init();
            score = 0;
            [[fallthrough]]

        case GameState::PlayGame:

            playGame();

            if (frameRate != 40 + (score / 8)) {
                frameRate = 40 + (score / 8);
                arduboy.setFrameRate(frameRate);
            }
            arduboy.displayClearToWhite();
            break;

    }


}




// ----------------------------------------------------------------------------
//  Initialise iinstruction screen ..
//
void instructions_Init() {

    ledDelay = 0;
    counter = 0;

}


// ----------------------------------------------------------------------------
//  Render instructoin screen ..
//
void instructions() {

    const uint8_t xCoord[53] = {  26, 28, 30, 32,   34, 36, 38, 40,   42, 44, 46, 48,   50, 52, 54, 56,   
                                  58, 60, 62, 64,   66, 68, 70, 72,   72, 72, 72, 70,   68, 66, 64, 62,  
                                  60, 58, 56, 54,   52, 50, 48, 46,   44, 42, 40, 38,   36, 34, 32, 30,
                                  28, 26, 26, 26,   26 };

    const uint8_t xPos[53]   = {   8,  9,  8,  9,    8,  9,  8,  9,    8,  9,  8,  9,    8,  9,  8,  9,    
                                   8,  9,  8,  9,    8,  9,  8, 15,   15, 15, 15, 11,   12, 11, 12, 11,
                                  12, 11, 12, 11,   12, 11, 12, 11,   12, 11, 12, 11,   12, 11, 12, 11,  
                                   12,  3,  3,  3,   3 };

    const uint8_t yCoord[53] = {  12, 12,17, 17,    17, 22, 22, 22,   27, 27, 27, 32,   32, 32, 32, 32,   
                                  32, 32, 32, 32,   32, 32, 32, 32,   32, 32, 32, 32,   32, 32, 27, 27,  
                                  27, 22, 22, 22,   17, 17, 17, 12,   12, 12, 12, 12,   12, 12, 12, 12,   
                                  12, 12, 12, 12,   12 };

    const uint8_t blink[53] = {    0,  0,  2,  0,    0,  2,  0,  0,    2,  0,  0,  0,    0,  0,  0,  0,
                                   0,  0,  0,  0,    0,  3,  0,  3,    0,  3,  0,  3,    0,  0,  1,  0,   
                                   0,  1,  0,  0,    1,  0,  0,  1,    0,  0,  0,  0,    3,  0,  3,  0,   
                                   3,  0,  3,  0,    2 };


    if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) { 

        gameState = GameState::PlayGame_Init;

    }


    ledDelay++;
    if (ledDelay == 235) ledDelay = 0;

    if (arduboy.isFrameCount(8)){
        counter++;
        if (counter == 53) counter = 0;
    }

    uint8_t counter2 = (counter + 26) % 53;

    if (blink[counter] != 1) { 
        Sprites::drawOverwrite(3, 1, Images::Button_Up, 0);
    } 
    else {
        Sprites::drawOverwrite(3, 1, Images::Button_Blank, 0);
    }

    if (blink[counter2] != 1) { 
        Sprites::drawOverwrite(115, 1, Images::Button_B, 0);
    } 
    else {
        Sprites::drawOverwrite(115, 1, Images::Button_Blank, 0);
    }

    Sprites::drawExternalMask(2, yCoord[counter], Images::Player_01, Images::Player_01_Mask, blink[counter] == 3, blink[counter] == 3);
    Sprites::drawExternalMask(102, yCoord[counter2], Images::Player_02, Images::Player_02_Mask, blink[counter2] == 3, blink[counter2] == 3);


    lion1.setXPosition(static_cast<XPosition>(xPos[counter]));
    if (counter <= 26) lion1.setDirection(Direction::Right);
    else lion1.setDirection(Direction::Left);

    lion2.setXPosition(static_cast<XPosition>(xPos[counter2]));
    if (counter2 <= 26) lion2.setDirection(Direction::Right);
    else lion2.setDirection(Direction::Left);


    Sprites::drawExternalMask(xCoord[counter] + 1, 3, Images::Lion, Images::Lion_Mask, lion1.getFrame(), lion1.getFrame());
    Sprites::drawExternalMask(xCoord[counter2] + 1, 27, Images::Lion, Images::Lion_Mask, lion2.getFrame(), lion2.getFrame());

    if (blink[counter] != 2) { 
        Sprites::drawOverwrite(3, 53, Images::Button_Down, 0);
    } 
    else {
        Sprites::drawOverwrite(3, 53, Images::Button_Blank, 0);
    }

    if (blink[counter2] != 2) { 
        Sprites::drawOverwrite(115, 53, Images::Button_A, 0);
    } 
    else {
        Sprites::drawOverwrite(115, 53, Images::Button_Blank, 0);
    }


    switch (ledDelay) {

        case 0 ... 14:
        case 75 ... 89:
        case 160 ... 174:
            break;

        case 15 ... 74:
            Sprites::drawOverwrite(23, 55, Images::Instr_00, 0);
            break;

        case 90 ... 159:
            Sprites::drawOverwrite(25, 55, Images::Instr_01, 0);
            break;

        case 175 ... 234:
            Sprites::drawOverwrite(18, 55, Images::Instr_02, 0);
            break;

    }

}
#include <Arduboy2.h>


// ----------------------------------------------------------------------------
//  Mpve the a lion relative to the grid and the other lion ..
//
void moveLion(Lion &thisLion, Lion &otherLion) {

    // Handle lion moves ..

    switch (thisLion.getDirection()) {

        case Direction::Up:
        case Direction::Down:
            thisLion.decSteps();
            break;

        case Direction::Left:

            switch (thisLion.getXPosition()) {

                case XPosition::LH_Position3 ... XPosition::LH_Position1:
                    {
                        bool rnd = (random(0, 6) == 0);
                        uint8_t thisLionX = static_cast<uint8_t>(thisLion.getXPosition());
                        uint8_t otherLionX = static_cast<uint8_t>(otherLion.getXPosition());

                        if (rnd && ((thisLionX - otherLionX > 3 && otherLion.getDirection() == Direction::Right) ||
                                    (thisLionX - otherLionX > 5 && otherLion.getDirection() == Direction::Left))) {

                            changeLevel(thisLion, otherLion, Direction::Right);

                        }
                        else if (otherLion.getYPosition() != YPosition::Level_2 && !otherLion.isMovingUpDown() && rnd) {

                            changeOneLevel(thisLion, otherLion, Direction::Right);

                        }
                        else {

                            thisLion.decXPosition();

                        }

                    }
                    break;

                case XPosition::LH_Position4:

                    if (player1.getYPosition() == thisLion.getYPosition()) {

                        bool rnd = (random(0, 3) == 0);

                        if (otherLion.getXPosition() >= XPosition::Centre && !otherLion.isMovingUpDown() && rnd) {

                            changeLevel(thisLion, otherLion, Direction::Right);

                        }
                        else if (otherLion.getYPosition() != YPosition::Level_2 && !otherLion.isMovingUpDown() && rnd) {

                            changeOneLevel(thisLion, otherLion, Direction::Right);

                        }
                        else {

                            thisLion.decXPosition();
                            
                        }

                    }
                    else {

                        if (otherLion.getXPosition() > XPosition::LH_Attack) {

                            thisLion.decXPosition();

                        }
                        else {

                            thisLion.incXPosition();
                            thisLion.setDirection(Direction::Right);

                        }

                    }

                    break;

                case XPosition::LH_Attack: 

                    if (player1.getYPosition() == thisLion.getYPosition() && (score < 100 || random(0, 2) == 0)) {

                        thisLion.incXPosition();
                        thisLion.setDirection(Direction::Right);
                        increaseScore();

                    }
                    else {

                        thisLion.decXPosition();

                    }

                    break;

                case XPosition::LH_Attack_OutofCage: 

                    if (player1.getYPosition() == thisLion.getYPosition()) {

                        thisLion.incXPosition();
                        thisLion.setDirection(Direction::Right);
                        increaseScore();
                        
                    }
                    else {

                        thisLion.decXPosition();

                    }

                    break;

                case XPosition::LH_Attacking:

                    if (player1.getYPosition() < thisLion.getYPosition()) {

                        thisLion.setXPosition(XPosition::LH_Attacking_Up);

                    }
                    else if (player1.getYPosition() > thisLion.getYPosition()) {

                        thisLion.setXPosition(XPosition::LH_Attacking_Down);

                    }
                    else {

                        thisLion.setXPosition(XPosition::LH_Attacking_Down);

                    }

                    break;

                case XPosition::LH_Attacking_Up:
                case XPosition::LH_Attacking_Down:
                case XPosition::LH_Attacking_Left:

                    lionAttacking = Direction::Left;
                    lionAttackingIndex = thisLion.getIndex();

                    if (!thisLion.getRunning()) {

                        player1.setRunning(true, thisLion.getXPosition());
                        chair.setPosition(player1.getXDisplay() + 14, player1.getYDisplay() + 2, Direction::Left);
                        thisLion.setRunning(true, player1.getYPosition() == thisLion.getYPosition());
                        die();

                    }

                    break;

                default:

                    thisLion.decXPosition();
                    break;

            }

            break;

        case Direction::Right:

            switch (thisLion.getXPosition()) {

                case XPosition::RH_Position1 ... XPosition::RH_Position3:
                    {
                        bool rnd = (random(0, 6) == 0);
                        uint8_t thisLionX = static_cast<uint8_t>(thisLion.getXPosition());
                        uint8_t otherLionX = static_cast<uint8_t>(otherLion.getXPosition());

                        if (rnd && ((thisLionX - otherLionX < -3 && otherLion.getDirection() == Direction::Left) ||
                                    (thisLionX - otherLionX < -5 && otherLion.getDirection() == Direction::Right))) {

                            changeLevel(thisLion, otherLion, Direction::Left);

                        }
                        else if (otherLion.getYPosition() != YPosition::Level_2 && !otherLion.isMovingUpDown() && rnd) {

                            changeOneLevel(thisLion, otherLion, Direction::Left);

                        }
                        else {

                            thisLion.incXPosition();

                        }

                    }
                    break;

                case XPosition::RH_Position4:
                    {
                        bool rnd = (random(0, 3) == 0);

                        if (gameMode == GameMode::Hard) {

                            if (player2.getYPosition() == thisLion.getYPosition()) {

                                if (otherLion.getXPosition() <= XPosition::Centre && !otherLion.isMovingUpDown() && rnd) {

                                    changeLevel(thisLion, otherLion, Direction::Left);

                                }
                                else if (otherLion.getYPosition() != YPosition::Level_2 && !otherLion.isMovingUpDown() && rnd) {

                                    changeOneLevel(thisLion, otherLion, Direction::Left);

                                }
                                else {

                                    thisLion.incXPosition();

                                }

                            }
                            else {

                                if (otherLion.getXPosition() < XPosition::RH_Attack) {

                                    thisLion.incXPosition();

                                }
                                else {

                                    thisLion.decXPosition();
                                    thisLion.setDirection(Direction::Left);

                                }

                            }

                        }
                        else {

                            if (otherLion.getXPosition() <= XPosition::Centre && !otherLion.isMovingUpDown() && rnd) {

                                changeLevel(thisLion, otherLion, Direction::Left);

                            }
                            else if (otherLion.getYPosition() != YPosition::Level_2 && !otherLion.isMovingUpDown() && rnd) {

                                changeOneLevel(thisLion, otherLion, Direction::Left);

                            }
                            else {

                                thisLion.decXPosition();
                                thisLion.setDirection(Direction::Left);

                            }

                        }

                    }

                    break;

                case XPosition::RH_Attack: 

                    if (player2.getYPosition() == thisLion.getYPosition() && (score < 100 || random(0, 2) == 0)) {

                        thisLion.decXPosition();
                        thisLion.setDirection(Direction::Left);
                        increaseScore();

                    }
                    else {

                        thisLion.incXPosition();

                    }

                    break;

                case XPosition::RH_Attack_OutofCage: 

                    if (player2.getYPosition() == thisLion.getYPosition()) {

                        thisLion.decXPosition();
                        thisLion.setDirection(Direction::Left);
                        increaseScore();

                    }
                    else {

                        thisLion.incXPosition();

                    }

                    break;

                case XPosition::RH_Attacking:

                    if (player2.getYPosition() < thisLion.getYPosition()) {

                        thisLion.setXPosition(XPosition::RH_Attacking_Up);

                    }
                    else if (player2.getYPosition() > thisLion.getYPosition()) {

                        thisLion.setXPosition(XPosition::RH_Attacking_Down);

                    }
                    else {

                        thisLion.setXPosition(XPosition::RH_Attacking_Down);

                    }

                    break;

                case XPosition::RH_Attacking_Up:
                case XPosition::RH_Attacking_Down:
                case XPosition::RH_Attacking_Left:

                    lionAttacking = Direction::Right;
                    lionAttackingIndex = thisLion.getIndex();

                    if (!thisLion.getRunning()) {

                        player2.setRunning(true, thisLion.getXPosition());
                        chair.setPosition(player2.getXDisplay(), player2.getYDisplay() + 2, Direction::Right);
                        thisLion.setRunning(true, player1.getYPosition() == thisLion.getYPosition());
                        die();

                    }

                    break;

                default:

                    thisLion.incXPosition();
                    break;

            }

            break;

        default: break;

    }

}


// ----------------------------------------------------------------------------
//  Mve the lion one or two levels up or down ..
//
void changeLevel(Lion &thisLion, Lion &otherLion, Direction newDirection) {

    switch (thisLion.getYPosition()) {

        case YPosition::Level_1:

            thisLion.setDirection(Direction::Down);

            switch (otherLion.getYPosition()) {

                case YPosition::Level_1:
                case YPosition::Level_3:

                    thisLion.setSteps(3);
                    break;

                case YPosition::Level_2:

                    thisLion.setSteps(6);
                    break;

            }

            break;

        case YPosition::Level_2:

            thisLion.setSteps(3);

            switch (otherLion.getYPosition()) {

                case YPosition::Level_1:
                case YPosition::Level_2:

                    thisLion.setDirection(Direction::Down);
                    break;

                case YPosition::Level_3:

                    thisLion.setDirection(Direction::Up);
                    break;

            }

            break;

        case YPosition::Level_3:

            thisLion.setDirection(Direction::Up);

            switch (otherLion.getYPosition()) {

                case YPosition::Level_1:
                case YPosition::Level_3:

                    thisLion.setSteps(3);
                    break;

                case YPosition::Level_2:

                    thisLion.setSteps(6);
                    break;

            }

            break;

    }

    thisLion.setNextDirection(newDirection);

}


// ----------------------------------------------------------------------------
//  Mve the lion one level up or down ..
//
void changeOneLevel(Lion &thisLion, Lion &otherLion, Direction newDirection) {

    thisLion.setSteps(3);
    thisLion.setNextDirection(newDirection);

    if (thisLion.getYPosition() == YPosition::Level_2) {

        thisLion.setDirection(otherLion.getYPosition() == YPosition::Level_1 ? Direction::Down : Direction::Up);

    }
    else {

        thisLion.setDirection(thisLion.getYPosition() == YPosition::Level_1 ? Direction::Down : Direction::Up);

    }

}


// ----------------------------------------------------------------------------
//  Increase the score ..
//
void increaseScore() {

    #ifdef SOUNDS
    sound.tones(Sounds::LionBlocked);
    #endif
    arduboy.setRGBled(GREEN_LED, Constants::LED_Brightness);
    ledDelay = Constants::Led_Delay;
    score++;

}


// ----------------------------------------------------------------------------
//  Handle a player dead ..
//
void die() {

    #ifdef SOUNDS
    sound.tones(Sounds::LionEscaped);
    #endif
    arduboy.setRGBled(RED_LED, Constants::LED_Brightness);
    ledDelay = Constants::Led_Delay;
    numberOfLives--;

}
#include <Arduboy2.h>



// ----------------------------------------------------------------------------
//  Initialise game ..
//
void playGame_Init() {

    lifeReset();

    arduboy.setFrameRate(50);
    
    gameState = GameState::PlayGame;
    frameRate = 50;
    frameRate = 50;
    counter = 159;

}


// ----------------------------------------------------------------------------
//  Reset life at start of game and after each death ..
//
void lifeReset() {

    chair.reset();

    lionAttacking = Direction::None;
    lionAttackingIndex = 0;

    player1.reset(Constants::Player1_Index, Constants::Player1_YPos); 
    player2.reset(Constants::Player2_Index, Constants::Player2_YPos); 

    lion1.reset(Direction::Left, YPosition::Level_1, 8, Constants::Lion1_Index);
    lion2.reset(Direction::Right, YPosition::Level_3, 8, Constants::Lion2_Index);

}


// ----------------------------------------------------------------------------
//  Update game and render ..
//
void playGame(void) {

    uint8_t justPressedButton = arduboy.justPressedButtons();


    // --------------------------------------------------------------------------
    //  Update entity positions ..

    if (counter > 0) counter--;

    if (ledDelay > 0) {

        ledDelay--;

        if (ledDelay == 0) {

            arduboy.setRGBled(RED_LED, 0);
            arduboy.setRGBled(GREEN_LED, 0);

        }

    }


    // Handle other movements ..

    if (arduboy.getFrameCount(4)) chair.update();

    bool finished = (lion1.getRunning() && (lion1.getYDisplay() < -40 || lion1.getYDisplay() > 100)) ||
                    (lion2.getRunning() && (lion2.getYDisplay() < -40 || lion2.getYDisplay() > 100));


    // Return to main menu ..

    if (numberOfLives == 0 && finished && lionAttacking != Direction::None) {

        if (arduboy.pressed(A_BUTTON))              { gameState = GameState::Title_Init; }

    }


    // Update player positions ..

    if (counter == 0 && lionAttacking == Direction::None/* && arduboy.everyXFrames(4)*/) {

        if (justPressedButton & A_BUTTON)               { player2.incYPosition(); }
        if (justPressedButton & B_BUTTON)               { player2.decYPosition(); }

        if (justPressedButton & UP_BUTTON)              { player1.decYPosition(); }
        if (justPressedButton & DOWN_BUTTON)            { player1.incYPosition(); }

    }


    // Handle lion movements ..

    if (!gameOver) {

        lion1.updateRunning();
        lion2.updateRunning();

    }

    if (counter == 0) {

        if ((lionAttacking == Direction::None || lionAttackingIndex == Constants::Lion1_Index) && arduboy.everyXFrames(lion1.getSpeed()))     moveLion(lion1, lion2);
        if ((lionAttacking == Direction::None || lionAttackingIndex == Constants::Lion2_Index) && arduboy.everyXFrames(lion2.getSpeed()))     moveLion(lion2, lion1);

    }


    // --------------------------------------------------------------------------
    //  Render the screen ..

    renderBackground();


    // Lions ..

    renderLion(lion1);
    renderLion(lion2);


    // Player ..

    if (!gameOver) {

        player1.updateRunning();
        player2.updateRunning();

    }
    
    renderPlayer(player1, Images::Player_01, Images::Player_01_Mask);
    if (gameMode == GameMode::Hard) renderPlayer(player2, Images::Player_02, Images::Player_02_Mask);

    if (!player1.getRunning() && !player2.getRunning()) {

        renderScoreBoards(score, numberOfLives);
        
    }


    // Chair?

    if (chair.getDirection() == Direction::Left) {

        Sprites::drawExternalMask(chair.getXDisplay(), chair.getYDisplay(), Images::Chair_LH, Images::Chair_LH_Mask, chair.getFrame(), chair.getFrame());

    }

    if (chair.getDirection() == Direction::Right) {

        Sprites::drawExternalMask(chair.getXDisplay(), chair.getYDisplay(), Images::Chair_RH, Images::Chair_RH_Mask, chair.getFrame(), chair.getFrame());

    }


    // Is the game over ?

    if (finished && lionAttacking != Direction::None) {

        if (numberOfLives == 0) {

            Sprites::drawExternalMask(34, 24, Images::GameOver, Images::GameOver_Mask, 0, 0);
            gameOver = true;
            EEPROM_Utils::saveScore(gameMode, score);
            renderScoreBoards(score, numberOfLives);

        }
        else {

            lifeReset();
            counter = 159;            

        }

    }


    // Render counter?
    {
        uint8_t frame = 255;

        switch (counter) {

            case 5 ... 39:
                frame = 3;
                break;

            case 45 ... 79:
                frame = 2;
                break;

            case 85 ... 119:
                frame = 1;
                break;

            case 125 ... 159:
                frame = 0;
                break;

            default: break;

        }
     
        if (frame != 255) {
            Sprites::drawExternalMask(55, 24, Images::Count, Images::Count_Mask, frame, 0);
        }

    }

}

#include <Arduboy2.h>


// ----------------------------------------------------------------------------
//  Render background ..
//
void renderBackground() {

    Sprites::drawOverwrite(0, 0, Images::Scenery_LH, 0);
    Sprites::drawOverwrite(106, 0, Images::Scenery_RH, 0);
    Sprites::drawOverwrite(27, 0 , Images::Cage, 0);

    if (gameMode == GameMode::Easy) {

        Sprites::drawExternalMask(91, 0, Images::FenceRight, Images::FenceRight_Mask, 0, 0);
        Sprites::drawExternalMask(112, 30, Images::Player_Sitting, Images::Player_Sitting_Mask, 0, 0);

    }

}


// ----------------------------------------------------------------------------
//  Render scoreboards ..
//
void renderScoreBoards(uint16_t score, uint8_t numberOfLives) {

    Sprites::drawOverwrite(-1, 56, Images::Scoreboard_LH, 0);
    Sprites::drawOverwrite(114, 56, Images::Scoreboard_RH, 0);

    uint8_t digits[3] = {};
    extractDigits(digits, score);
    
    for (uint8_t i = 3, x = 1; i > 0; i--, x = x + 4) {

        Sprites::drawErase(x, 58, Images::Font, digits[i - 1]);

    }

    for (uint8_t x = 126, y = numberOfLives; y > 0; x = x - 2, y--) {

        arduboy.drawFastVLine(x, 58, 5, BLACK);

    }

}


// ----------------------------------------------------------------------------
//  Render player ..
//
void renderPlayer(Player &player, const uint8_t *playerImages, const uint8_t *playerMask) {

    uint8_t playerFrame = 0;

    if (arduboy.frameCount % 8 < 4) {

        YPosition playerY = player.getYPosition();
        playerFrame = ((playerY == lion1.getYPosition() && lion1.getXPosition() <= XPosition::LH_Attack) || (playerY == lion2.getYPosition() && lion2.getXPosition() >= XPosition::RH_Attack));

    }

    if (!player.getRunning()) {

        Sprites::drawExternalMask(player.getXDisplay(), player.getYDisplay(), playerImages, playerMask, playerFrame, playerFrame);

    }
    else {

        uint8_t frame = 0;

        switch (player.getXPosition()) {

            case XPosition::LH_Attacking_Up:
            case XPosition::RH_Attacking_Up:
                frame = 0;
                break;

            case XPosition::LH_Attacking_Down:
            case XPosition::RH_Attacking_Down:
                frame = 1;
                break;

            default: break;

        }

        Sprites::drawExternalMask(player.getXDisplay() + (player.getIndex() == Constants::Player2_Index ? 12 : 0), player.getYDisplay(), Images::Player_Up_Down, Images::Player_Up_Down_Mask, frame, 0);

    }

}



// ----------------------------------------------------------------------------
//  Render lion ..
//
void renderLion(Lion &lion) {

    Sprites::drawExternalMask(lion.getXDisplay(), lion.getYDisplay(), Images::Lion, Images::Lion_Mask, lion.getFrame(), lion.getFrame());

    if (lion.getRunning() && (lion.getXPosition() == XPosition::LH_Attacking_Up || lion.getXPosition() == XPosition::RH_Attacking_Up)) {
        uint8_t frame = (arduboy.frameCount % 4 < 2);
        Sprites::drawExternalMask(lion.getXDisplay(), lion.getYDisplay() + 24, Images::Lion_Cloud, Images::Lion_Cloud_Mask, frame, frame);

    }

}

static constexpr uint8_t ppotFrames = (1.6 *60); // 1.6s animation loop


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void splashScreen_Init() {

    arduboy.setFrameRate(60);
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

void title() {

    uint8_t justPressed = arduboy.justPressedButtons();
    uint8_t pressed = arduboy.pressedButtons();

    if (counter >= 0) counter++;

    #ifdef SOUNDS
        if (soundCounter > 0) soundCounter--;
    #endif

    #ifdef THREE_IMAGES
        marqueeCounter++; 
        if (marqueeCounter == 195) marqueeCounter = 0;
    #else
        marqueeCounter++; 
        if (marqueeCounter == 166) marqueeCounter = 0;
    #endif

    Sprites::drawOverwrite(0, 0, Images::Title_LionTop, arduboy.getFrameCount(255) < 16 ? 1 : 0);

    if ((justPressed & A_BUTTON) || (justPressed & B_BUTTON)) { 

        gameMode = (justPressed & A_BUTTON) ? GameMode::Easy : GameMode::Hard;

        #ifdef SHOW_ROAR
            
            if (counter == -1) {

                #ifdef SOUNDS
                    sound.noTone();
                #endif
                counter = 0;
                
                #ifdef SOUNDS
                    soundVars.reset();
                #endif

            }
            else {

                #ifdef SHOW_INSTUCTIONS
                    gameState = GameState::Instructions_Init;
                #else
                    gameState = GameState::PlayGame_Init;
                #endif

            }

        #else

            #ifdef SHOW_INSTUCTIONS
                gameState = GameState::Instructions_Init;
            #else
                gameState = GameState::PlayGame_Init;
            #endif

            counter = 131;

        #endif

    }

    #ifdef SOUNDS
        if (counter == -1 && (justPressed & UP_BUTTON) || (justPressed & DOWN_BUTTON)) {

            sounds = EEPROM_Utils::toggleSoundSettings(arduboy);
            soundCounter = 64;
            
        }
    #endif

    #ifdef RESET
        if (counter == -1 && (pressed & LEFT_BUTTON) && (pressed & RIGHT_BUTTON)) {

            resetCounter++;
            arduboy.setRGBled(BLUE_LED, resetCounter);

            if (resetCounter == 128) {
                EEPROM_Utils::initEEPROM(true);
                arduboy.setRGBled(RED_LED, resetCounter);
                resetCounter = 0;

            }
            
        }
        else {

            resetCounter = 0;
            arduboy.setRGBled(RED_LED, 0);
            arduboy.setRGBled(BLUE_LED, 0);

        }
    #endif

    #ifdef SHOW_ROAR

        switch (counter) {

            case -1:
                drawElements(true, 0);
                break;

            case 0 ... 9:
                drawElements(false, 0);
                break;

            case 10:
                explosions.setExplosions(35, 35);
                drawElements(true, 1);
                break;

            case 11:

                #ifdef SOUNDS
                    sound.tones(roarTones);
                #endif

                drawElements(false, 1);
                Sprites::drawOverwrite(52 + ((counter % 3) - 1), 6 + ((counter % 3) - 1), Images::Roar, 0);
                break;

            case 12 ... 120:
                drawElements(false, 1);
                Sprites::drawOverwrite(52 + ((counter % 3) - 1), 6 + ((counter % 3) - 1), Images::Roar, 0);
                break;

            case 121 ... 130:
                drawElements(false, 1);
                break;

            case 131:
                drawElements(false, 0);

                if ((pressed & UP_BUTTON) || (pressed & DOWN_BUTTON) || (pressed & LEFT_BUTTON) || (pressed & RIGHT_BUTTON)) {

                    counter = 0;

                }
                else {
                    #ifdef SHOW_INSTUCTIONS
                        gameState = GameState::Instructions_Init;
                    #else
                        gameState = GameState::PlayGame_Init;
                    #endif
                }
                break;

        }

    #else
        drawElements(true, 0);
    #endif

    if (counter < 0) {

        #ifdef THREE_IMAGES

            switch (marqueeCounter) {

                case 0 ... 14:
                case 65 ... 79:
                case 130 ... 144:
                    break;

                case 15 ... 64:
                    Sprites::drawOverwrite(73, 54, Images::Mode, 0);
                    break;

                case 80 ... 129:
                    Sprites::drawOverwrite(73, 54, Images::Mode, 1);
                    break;

                case 145 ... 194:
                    {

                        uint8_t digits[3] = {};
                        extractDigits(digits, score);
                        Sprites::drawOverwrite(72, 54, Images::Hard, 0);
                        
                        for (uint8_t i = 3, x = 88; i > 0; i--, x = x + 4) {

                            Sprites::drawErase(x, 54, Images::Font, digits[i - 1]);

                        }

                    }
                    break;

            }

        #else

            switch (marqueeCounter) {

                case 0 ... 14:
                case 85 ... 99:
                    break;

                case 15 ... 84:
                    Sprites::drawOverwrite(59, 54, Images::Mode, 0);
                    Sprites::drawOverwrite(88, 54, Images::Mode, 1);
                    break;

                case 100 ... 165:
                    {

                        uint8_t digits[3] = {};
                        Sprites::drawOverwrite(64, 54, Images::High, 0);
                        
                        extractDigits(digits, EEPROM_Utils::getScore(GameMode::Easy));
                        for (uint8_t i = 3, x = 81; i > 0; i--, x = x + 4) {

                            Sprites::drawErase(x, 54, Images::Font, digits[i - 1]);

                        }
                        
                        extractDigits(digits, EEPROM_Utils::getScore(GameMode::Hard));
                        for (uint8_t i = 3, x = 96; i > 0; i--, x = x + 4) {

                            Sprites::drawErase(x, 54, Images::Font, digits[i - 1]);

                        }

                    }
                    break;

            }

        #endif

    }

    #ifdef SOUNDS

        if (soundCounter > 0) {
            Sprites::drawOverwrite(119, 55, Images::Sound, !sounds);
        }

    #endif


    // Render roar ..

    #ifdef SHOW_ROAR

        for (uint8_t i = 0; i < Constants::Particle_Count; i++) {

            Explosion explosion = explosions.getExplosion(i);

            if (explosion.getCounter() > 0) {
                Sprites::drawExternalMask(explosion.getX() + 1, explosion.getY(), Images::Pixel, Images::Pixel_Mask, 0, 0);
            }

        }

        explosions.update(arduboy.frameCount);

    #endif

}

void drawElements(bool title, uint8_t lionFrame) {

    if (title) Sprites::drawOverwrite(43, 10, Images::Title, 0);
    Sprites::drawOverwrite(0, 32, Images::Title_Lion_Bottom, lionFrame);

}

