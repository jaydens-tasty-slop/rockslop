#include <Arduino.h>
#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Enums.h"
#include "src/utils/Utils.h"
#include "src/utils/EEPROM_Utils.h"
#include "src/entities/Entities.h"
#include "src/map/Coordinates.h"
#include "src/images/Images.h"
#include "src/sounds/Sounds.h"
#include "src/utils/Structs.h"
#include "ArduboyTones.h"

void setup(void);
void loop(void);
void splashScreen_Activate();
void splashScreen_Update();
void splashScreen_Render();
void titleScreen_Activate();
void titleScreen_Update();
void titleScreen_Render();
void playGame_Activate();
void playGame_Update();
void playGame_CheckLastKey(uint8_t chain);
void playGame_ScrollToBottom(bool renderKong);
void playGame_Render();
void playGame_RenderKong(uint8_t yOffset);
void playGame_RenderKey(uint8_t yOffset);
void playGame_DrawScenery(uint8_t yOffset);
void playGame_KillPlayer();
void playGame_ResetLevel(uint8_t introDelay);
void playGame_RenderScore(uint8_t yOffset);
bool playGame_GetPaused();
void playGame_SetPaused(bool value);
void playGame_HandleCommonButtons();
void playGame_RenderGameOverOrPause();
void highScore_Activate();
void highScore_Update();
void highScore_RenderHighScore(uint16_t score);
void highScore_Render();

#ifdef NO_USB
ARDUBOY_NO_USB 
#endif


Arduboy2Ext arduboy;
ArduboyTones sound(arduboy.audio.enabled);

SplashScreenStateVars splashScreenVars;
TitleScreenStateVars titleScreenVars;
PlayGameStateVars playGameVars;
HighScoreStateVars highScoreVars;

GameStats gameStats;
GameStateType gameState = GameStateType::SplashScreen_Activate; 

void setup(void) {

	arduboy.boot();
	arduboy.flashlight();
	arduboy.systemButtons();
	arduboy.audio.begin();
	arduboy.initRandomSeed();
	arduboy.setFrameRate(70);

	EEPROM_Utils::initialiseEEPROM();
	
}

void loop(void) {

	if (!arduboy.nextFrame()) return;

	arduboy.pollButtons();

		switch (gameState) {

		case GameStateType::SplashScreen_Activate:
			splashScreen_Activate();
			gameState = GameStateType::SplashScreen;
			[[fallthrough]];

		case GameStateType::SplashScreen:
			splashScreen_Update();
			splashScreen_Render();
			arduboy.displayWithBackground(Background::Black);
			break;

		case GameStateType::TitleScreen_Activate:
			titleScreen_Activate();
			gameState = GameStateType::TitleScreen;
			[[fallthrough]];

		case GameStateType::TitleScreen:
			titleScreen_Update();
			titleScreen_Render();
			arduboy.displayWithBackground(Background::Black);
			break;

		case GameStateType::PlayGame_Activate:
			playGame_Activate();
			gameState = GameStateType::PlayGame;
			[[fallthrough]];

		case GameStateType::PlayGame:
			playGame_Update();
			playGame_Render();
			arduboy.displayWithBackground(Background::None);
			break;

		case GameStateType::HighScore_Activate:
			highScore_Activate();
			gameState = GameStateType::HighScore;
			[[fallthrough]];

		case GameStateType::HighScore:
			highScore_Update();
			highScore_Render();
			arduboy.displayWithBackground(Background::White);
			break;

		default: break;

	}

}


static constexpr uint8_t ppotFrames = (1.6 *70); // 1.6s animation loop


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void splashScreen_Activate() {

  splashScreenVars.counter = 0;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void splashScreen_Update() { 

    auto justPressed = arduboy.justPressedButtons();

    if (justPressed > 0 && splashScreenVars.counter > 0) {

        splashScreenVars.counter = 20;  

    }

    if (justPressed > 0 && splashScreenVars.counter == 0) {

        #ifdef PLAY_SOUNDS    
        sound.tone(NOTE_A2, 45);
        sound.tone(NOTE_F1, 45);
        #endif

        splashScreenVars.counter = 1;

    }

    if (splashScreenVars.counter > 0) {

        splashScreenVars.counter++;

        if (splashScreenVars.counter == 21) gameState = GameStateType::TitleScreen_Activate; 

    }

}


// ----------------------------------------------------------------------------
//  Render the state ..
//
void splashScreen_Render() {

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
    //  if (p % 2 == 0) {
    if ((p & 0xA0) == 0) {
        // Render directly to the screen buffer 
        arduboy.sBuffer[45 +(3*WIDTH)] = 0xA5;
        arduboy.sBuffer[46 +(3*WIDTH)] = 0x95;
    }

}
constexpr const static uint8_t UPLOAD_DELAY = 16;


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void titleScreen_Activate() {

    gameStats.resetGame();
    sound.volumeMode(VOLUME_ALWAYS_NORMAL);

    playGameVars.frameRate = FRAME_RATE_MIN;
    arduboy.setFrameRate(playGameVars.frameRate);

    // titleScreenVars.restart = 0;
    titleScreenVars.counter = 0;
    titleScreenVars.position = Rotation::None;

    #ifdef PLAY_SOUNDS 
    sound.tones(Sounds::TitleMusic);
    #endif

}


// ----------------------------------------------------------------------------
//  Handle state updates ..
//
void titleScreen_Update() {

    auto justPressed = arduboy.justPressedButtons();
    auto pressed = arduboy.pressedButtons();


    // Restart ?

    // if (pressed & DOWN_BUTTON) {

    //     if (titleScreenVars.restart < UPLOAD_DELAY) {
    //         titleScreenVars.restart++;
    //     }
    //     else {
    //         arduboy.exitToBootloader();
    //     }

    // }
    // else {
    //     titleScreenVars.restart = 0;
    // }


    // Handle other input ..

    if (titleScreenVars.position == Rotation::None) {

        if (justPressed & A_BUTTON || justPressed & B_BUTTON) {
            gameStats.mode = (justPressed & A_BUTTON ? GameMode::Easy : GameMode::Hard);
            gameState = GameStateType::PlayGame_Activate; 
        }

    }
    else {

        if (justPressed & A_BUTTON) {
            gameStats.mode = (titleScreenVars.position == Rotation::Left ? GameMode::Easy : GameMode::Hard);
            gameState = GameStateType::PlayGame_Activate; 
        }

    }

    if (justPressed & UP_BUTTON) {
        gameState = GameStateType::HighScore_Activate; 
    }

    if (justPressed & LEFT_BUTTON) {
        titleScreenVars.position = Rotation::Left;
    }

    if (justPressed & RIGHT_BUTTON) {
        titleScreenVars.position = Rotation::Right;
    }

    if (arduboy.getFrameCount(24) == 0) {
        titleScreenVars.counter = (titleScreenVars.counter == 2 ? 0 : titleScreenVars.counter + 1);
    }

}


// ----------------------------------------------------------------------------
//  Render the state ..
//
void titleScreen_Render() {

    const uint8_t headPos[3] = { 18, 19, 19 };
    const uint8_t leftArm[3] = { 20, 20, 19 };
    const uint8_t rightArm[3] = { 22, 22, 21 };

    Sprites::drawOverwrite(25, 2, Images::Title_Kong_II, 0);
    Sprites::drawOverwrite(2, 0, Images::Title_Chain_Long, 0);
    Sprites::drawOverwrite(13, 0, Images::Title_Chain_Short, 0);
    Sprites::drawOverwrite(118, 0, Images::Title_Chain_Long, 0);
    Sprites::drawOverwrite(109, 0, Images::Title_Chain_Short, 0);

    if (titleScreenVars.position != Rotation::Left || (titleScreenVars.position == Rotation::Left && arduboy.getFrameCountHalf(64))) {
        Sprites::drawOverwrite(2, 56, Images::Title_Game, 0);
        Sprites::drawOverwrite(22, 53, Images::Game_Icon, 0);
    }

    if (titleScreenVars.position != Rotation::Right || (titleScreenVars.position == Rotation::Right && arduboy.getFrameCountHalf(64))) {
        Sprites::drawOverwrite(95, 56, Images::Title_Game, 0);
        Sprites::drawOverwrite(115, 53, Images::Game_Icon, 1);
    }

    Sprites::drawSelfMasked(42, 43, Images::Title_Kong_Body, 0);
    Sprites::drawSelfMasked(22, leftArm[titleScreenVars.counter], Images::Title_Kong_Arm_Left, 0);
    Sprites::drawSelfMasked(75, rightArm[titleScreenVars.counter], Images::Title_Kong_Arm_Right, 0);

    Sprites::drawSelfMasked(52, headPos[titleScreenVars.counter], Images::Title_Kong_Head, 0);

}



// ----------------------------------------------------------------------------
//  Initialise state ..
//
void playGame_Activate() {

    playGameVars.player.reset();

    playGameVars.clappersLowerDelay = (gameStats.mode == GameMode::Easy ? CLAPPERS_LOWER_MAX_EASY : CLAPPERS_LOWER_MAX_HARD);
    playGameVars.clappersUpperDelay = (gameStats.mode == GameMode::Easy ? CLAPPERS_UPPER_MAX_EASY : CLAPPERS_UPPER_MAX_HARD);
    playGameVars.sparksUpperDelay = (gameStats.mode == GameMode::Easy ? SPARKS_LOWER_MAX_EASY : SPARKS_LOWER_MAX_HARD);
    playGameVars.sparksUpperDelay = (gameStats.mode == GameMode::Easy ? SPARKS_UPPER_MAX_EASY : SPARKS_UPPER_MAX_HARD);
    playGameVars.birdsDelay = (gameStats.mode == GameMode::Easy ? BIRDS_MAX_EASY : BIRDS_MAX_HARD);

    playGameVars.lowerClappers.setDelayMax(playGameVars.clappersLowerDelay, true);
    playGameVars.lowerClappers.reset(107);

    playGameVars.upperClappers.setDelayMax(playGameVars.clappersUpperDelay, true);
    playGameVars.upperClappers.reset(54);

    playGameVars.lowerSparks.setDelayMax(playGameVars.sparksUpperDelay, true);
    playGameVars.lowerSparks.reset(Coordinates::LowerSpark);

    playGameVars.upperSparks.setDelayMax(playGameVars.sparksUpperDelay, true);
    playGameVars.upperSparks.reset(Coordinates::UpperSpark);

    playGameVars.birds.setDelayMax(playGameVars.birdsDelay, true);
    playGameVars.birds.reset();

    playGameVars.kong.reset();

    playGameVars.frameRate = FRAME_RATE_MIN;
    arduboy.setFrameRate(playGameVars.frameRate);

    playGame_ResetLevel(INTRO_DELAY_FROM_TITLE_SCROLL);
    playGameVars.key.setKeyLocation(KeyLocation::LowerPosition);  

    playGameVars.playing = false;
    playGameVars.exitSequence = false;
    
    playGameVars.paused = false;

}


// ----------------------------------------------------------------------------
//    Handle state updates .. 
//
void playGame_Update() { 

	auto pressed = arduboy.pressedButtons();
    auto justPressed = arduboy.justPressedButtons();

    if (!playGame_GetPaused() && !playGameVars.player.isDead()) {


        // Handle player movements ..

        if (playGameVars.playing && arduboy.everyXFrames(2)) {

            playGameVars.player.getXPosition(true);

            if (!playGameVars.player.isJumping() && !playGameVars.player.isFalling() && !playGameVars.exitSequence) {


                // If the player is running from right to left, the controls are reversed ..

                if (playGameVars.player.canMove(Movements::Reverse)) {

                    if ((pressed & LEFT_BUTTON) && playGameVars.player.canMove(Movements::Left)) {
                        playGameVars.player.incPlayerPosition();
                    }

                    if ((pressed & RIGHT_BUTTON) && playGameVars.player.canMove(Movements::Right)) {
                        playGameVars.player.decPlayerPosition(false);
                    }

                }
                else {

                    if ((pressed & RIGHT_BUTTON) && playGameVars.player.canMove(Movements::Right)) {
                        playGameVars.player.incPlayerPosition();
                    }

                    if ((pressed & LEFT_BUTTON) && playGameVars.player.canMove(Movements::Left)) {
                        playGameVars.player.decPlayerPosition(false);
                    }

                }

                if ((pressed & DOWN_BUTTON) && playGameVars.player.canMove(Movements::Down)) {
                    playGameVars.player.decPlayerPosition(false);
                }

                if ((pressed & UP_BUTTON) && playGameVars.player.canMove(Movements::Up)) {
                    
                    playGameVars.player.incPlayerPosition();

                    if (playGameVars.player.getPosition() == PLAYER_CHAIN_0_END - 1 && playGameVars.key.getKeyLocation() == KeyLocation::Position_00 && !playGameVars.kong.getFlashChain(0)) {

                        playGame_CheckLastKey(0);
                        
                    }
                    else if (playGameVars.player.getPosition() == PLAYER_CHAIN_1_END - 1 && playGameVars.key.getKeyLocation() == KeyLocation::Position_01 && !playGameVars.kong.getFlashChain(1)) {

                        playGame_CheckLastKey(1);
                        
                    }
                    else if (playGameVars.player.getPosition() == PLAYER_CHAIN_2_END - 1 && playGameVars.key.getKeyLocation() == KeyLocation::Position_02 && !playGameVars.kong.getFlashChain(2)) {

                        playGame_CheckLastKey(2);
                        
                    }
                    else if (playGameVars.player.getPosition() == PLAYER_CHAIN_3_END - 1 && playGameVars.key.getKeyLocation() == KeyLocation::Position_03 && !playGameVars.kong.getFlashChain(3)) {

                        playGame_CheckLastKey(3);
                        
                    }

                }


                if (playGameVars.preventJumpDelay == 0 && (pressed & A_BUTTON) && (playGameVars.player.canMove(Movements::Jump) || playGameVars.player.canMove(Movements::JumpUp))) {

                    if (playGameVars.key.getKeyLocation() == KeyLocation::LowerPosition && playGameVars.player.getPosition() >= PLAYER_POS_LOWERKEY_START && playGameVars.player.getPosition() <= PLAYER_POS_LOWERKEY_END) {

                        playGameVars.key.setPath(Coordinates::LowerKey);

                        #ifdef PLAY_SOUNDS 
                            sound.tones(Sounds::Key);
                        #endif

                    }

                    if (playGameVars.key.getKeyLocation() == KeyLocation::UpperPosition && playGameVars.player.getPosition() >= PLAYER_POS_UPPERKEY_START && playGameVars.player.getPosition() <= PLAYER_POS_UPPERKEY_END) {

                        while (true) {

                            uint8_t r = random(0, 4);

                            if (!playGameVars.kong.getChain(r)) {
                                continue;
                            }

                            if (playGameVars.kong.getChain(r)) {
                                
                                playGameVars.key.setPath(pgm_read_ptr_t(&Coordinates::Positions[r]));
                        
                                #ifdef PLAY_SOUNDS 
                                    sound.tones(Sounds::Key);
                                #endif

                                break;

                            }

                        }

                    }

                    playGameVars.player.startJump();

                    #ifdef PLAY_SOUNDS 
                        sound.tones(Sounds::Jump);
                    #endif                   

                }

            }

            // Player is jumping or falling ..

            else {

                if (playGameVars.player.isJumping()) {

                    playGameVars.player.updateJump();


                    // Have we jumped a clapper or a spark?

                    if (playGameVars.player.atTopOfJump()) {

                        uint16_t position = playGameVars.player.getPosition();
                        bool points = false;

                        if (position >= PLAYER_CLAPPER_LOWER_START && position < PLAYER_CLAPPER_LOWER_END) {

                            for (uint8_t i = 0; i < CLAPPERS_LOWER_COUNT; i++) {

                                auto &clapper = playGameVars.lowerClappers.getClapper(i);

                                if (clapper.isEnabled() && absT(playGameVars.player.getXPosition(false) - clapper.getXPosition()) < JUMP_POINT_SPAN) {

                                    points = true;
                                    break;

                                } 

                            }

                        }

                        if (position >= PLAYER_SPARK_START && position < PLAYER_SPARK_END) {

                            for (uint8_t i = 0; i < SPARKS_LOWER_COUNT; i++) {

                                auto &spark = playGameVars.lowerSparks.getSpark(i);

                                if (spark.isEnabled() && absT(playGameVars.player.getXPosition(false) - spark.getXPosition()) < JUMP_POINT_SPAN) {

                                    points = true;
                                    break;

                                } 

                            }

                        }

                        if (position >= PLAYER_CLAPPER_UPPER_START && position < PLAYER_CLAPPER_UPPER_END) {

                            for (uint8_t i = 0; i < CLAPPERS_UPPER_COUNT; i++) {

                                auto &clapper = playGameVars.upperClappers.getClapper(i);

                                if (clapper.isEnabled() && absT(playGameVars.player.getXPosition(false) - clapper.getXPosition()) < JUMP_POINT_SPAN) {

                                    points = true;
                                    break;

                                } 

                            }

                        }

                        if (points) {

                            #ifdef PLAY_SOUNDS 
                                sound.tones(Sounds::JumpOver);
                            #endif

                            gameStats.score++;

                        }

                    }

                    if (playGameVars.player.isJumping() && playGameVars.player.canGrabChain()) {

                        switch (playGameVars.player.getPosition()) {

                            case PLAYER_VINE_0_MIN ... PLAYER_VINE_0:
                                playGameVars.player.setJumpPosition(0);
                                playGameVars.player.setPosition(PLAYER_VINE_0_CLIMBING);
                                break;

                            case PLAYER_VINE_1_MIN ... PLAYER_VINE_1:
                                playGameVars.player.setJumpPosition(0);
                                playGameVars.player.setPosition(PLAYER_VINE_1_CLIMBING);
                                break;

                            case PLAYER_CHAIN_0_GROUND_MIN ... PLAYER_CHAIN_0_GROUND_MAX - 1:
                                playGameVars.player.setJumpPosition(0);
                                playGameVars.player.setPosition(PLAYER_CHAIN_0_CLIMBING);
                                break;

                            case PLAYER_CHAIN_1_GROUND_MIN ... PLAYER_CHAIN_1_GROUND_MAX - 1:
                                playGameVars.player.setJumpPosition(0);
                                playGameVars.player.setPosition(PLAYER_CHAIN_1_CLIMBING);
                                break;

                            case PLAYER_CHAIN_2_GROUND_MIN ... PLAYER_CHAIN_2_GROUND_MAX - 1:
                                playGameVars.player.setJumpPosition(0);
                                playGameVars.player.setPosition(PLAYER_CHAIN_2_CLIMBING);
                                break;

                            case PLAYER_CHAIN_3_GROUND_MIN ... PLAYER_CHAIN_3_GROUND_MAX - 1:
                                playGameVars.player.setJumpPosition(0);
                                playGameVars.player.setPosition(PLAYER_CHAIN_3_CLIMBING);
                                break;
                                

                        }

                    }

                }

                if (!playGameVars.exitSequence && playGameVars.player.isFalling()) {
                    playGameVars.player.decPlayerPosition(false);
                }

            }

        }

        uint8_t yOffset = playGameVars.player.getYOffset();


        // If we are in the middle of the exit sequence, move the player and kong automatically ..

        if (playGameVars.exitSequence) {

            if (playGameVars.playing && arduboy.everyXFrames(2)) {

                playGameVars.key.updatePosition();
                playGameVars.kong.updateChains();

                uint16_t position = playGameVars.player.getPosition();

                switch (position) {

                    case PLAYER_CHAIN_0_BACKTOGROUND + 1 ... PLAYER_CHAIN_0_END:
                    case PLAYER_CHAIN_1_BACKTOGROUND + 1 ... PLAYER_CHAIN_1_END:
                    case PLAYER_CHAIN_2_BACKTOGROUND + 1 ... PLAYER_CHAIN_2_END:
                    case PLAYER_CHAIN_3_BACKTOGROUND + 1 ... PLAYER_CHAIN_3_END:
                        if (arduboy.everyXFrames(4)) {
                            playGameVars.player.decPlayerPosition(true);
                        }
                        break;

                    case PLAYER_CHAIN_MOVE_TO_EXIT_0 ... PLAYER_CHAIN_MOVE_TO_EXIT_END:
                        if (arduboy.everyXFrames(2)) {
                            playGameVars.player.incPlayerPosition();
                        }
                        break;

                }

                bool exitComplete = playGameVars.kong.updatePosition();

                if (exitComplete) {

                    gameStats.score = gameStats.score + 20;
                    playGameVars.kong.reset();
                    playGameVars.player.setPosition(0);
                    playGameVars.exitSequence = false;

                    playGame_ScrollToBottom(false);

                }

            }

        }
        else {
                

            // Update entity positions ..

            if (playGameVars.playing && arduboy.everyXFrames(2)) {

                bool updateChains = playGameVars.kong.updateChains();

                playGameVars.lowerClappers.updatePositions();
                playGameVars.upperClappers.updatePositions();
                playGameVars.key.updatePosition();
                playGameVars.birds.updatePositions();

                if (updateChains) {

                    playGameVars.player.setExit(true);
                    
                }

            }

            if (playGameVars.playing && arduboy.everyXFrames(3)) {

                playGameVars.lowerSparks.updatePositions();

            }

            if (playGameVars.playing && arduboy.everyXFrames(4)) {

                playGameVars.upperSparks.updatePositions();

            }

        }


        // Have we been hit by anything?

        #ifndef IGNORE_COLLISIONS
                
            Rect playerRect = playGameVars.player.getRect();

            for (uint8_t i = 0; i < NUMBER_OF_ENTITIES; i++) {

                auto &lowerClapper = playGameVars.lowerClappers.getClapper(i);

                if (lowerClapper.isEnabled()) {

                    Rect clapperRect = lowerClapper.getRect(yOffset, gameStats.mode);

                    if (arduboy.collide(playerRect, clapperRect)) {

                        playGame_KillPlayer();

                    }

                }

                auto &upperClapper = playGameVars.upperClappers.getClapper(i);

                if (upperClapper.isEnabled()) {

                    Rect clapperRect = upperClapper.getRect(yOffset, gameStats.mode);

                    if (arduboy.collide(playerRect, clapperRect)) {

                        playGame_KillPlayer();
                        
                    }

                }


                if (playGameVars.player.getPosition() > PLAYER_SPARK_LOWER_IGNORE) {

                    auto &lowerSpark = playGameVars.lowerSparks.getSpark(i);

                    if (lowerSpark.isEnabled()) {

                        Rect sparkRect = lowerSpark.getRect(yOffset, gameStats.mode);

                        if (arduboy.collide(playerRect, sparkRect)) {

                            playGame_KillPlayer();

                        }

                    }

                }

                auto &upperSpark = playGameVars.upperSparks.getSpark(i);

                if (upperSpark.isEnabled()) {

                    Rect sparkRect = upperSpark.getRect(yOffset, gameStats.mode);

                    if (arduboy.collide(playerRect, sparkRect)) {

                        playGame_KillPlayer();

                    }

                }

                auto &bird = playGameVars.birds.getBird(i);

                if (bird.isEnabled()) {

                    Rect birdRect = bird.getRect(yOffset, gameStats.mode);

                    if (arduboy.collide(playerRect, birdRect)) {

                        playGame_KillPlayer();
                        
                    }

                }

            }

        #endif

    }


    // End of Game?

    if (playGameVars.playing && gameStats.numberOfLivesLeft == 0) {

        gameStats.gameOver = true;
        playGameVars.playing = false;

        #ifdef PLAY_SOUNDS 
            sound.tones(Sounds::GameOver);
        #endif

    }


    // Handle the transition to a new life ..

    switch (playGameVars.introDelay) {

        case INTRO_DELAY_FROM_TITLE_SCROLL:

            playGame_ScrollToBottom(true);

            if (playGameVars.clappersLowerDelay > CLAPPERS_LOWER_MIN) {

                playGameVars.clappersLowerDelay = playGameVars.clappersLowerDelay - CLAPPERS_LOWER_INC;
                playGameVars.lowerClappers.setDelayMax(playGameVars.clappersLowerDelay, true);

            }

            if (playGameVars.clappersUpperDelay > CLAPPERS_UPPER_MIN) {

                playGameVars.clappersUpperDelay = playGameVars.clappersUpperDelay - CLAPPERS_UPPER_INC;
                playGameVars.upperClappers.setDelayMax(playGameVars.clappersUpperDelay, true);

            }

            if (playGameVars.sparksLowerDelay > SPARKS_LOWER_MIN) {

                playGameVars.sparksLowerDelay = playGameVars.sparksLowerDelay - SPARKS_LOWER_INC;
                playGameVars.lowerSparks.setDelayMax(playGameVars.sparksLowerDelay, true);

            }

            if (playGameVars.sparksUpperDelay > SPARKS_UPPER_MIN) {

                playGameVars.sparksUpperDelay = playGameVars.sparksUpperDelay - SPARKS_UPPER_INC;
                playGameVars.upperSparks.setDelayMax(playGameVars.sparksUpperDelay, true);

            }

            if (playGameVars.birdsDelay > BIRDS_MIN) {

                playGameVars.birdsDelay = playGameVars.birdsDelay - BIRDS_INC;
                playGameVars.birds.setDelayMax(playGameVars.birdsDelay, true);

            }

            playGameVars.frameRate = playGameVars.frameRate + 2;
            arduboy.setFrameRate(playGameVars.frameRate);
            
            break;

        case 1:
            if (gameStats.numberOfLivesLeft > 0) {
                playGameVars.player.reset();
                playGameVars.playing = true;
                gameStats.numberOfLivesLeft--;
            }
            break;

        default:
            break;

    }

    decToZero(playGameVars.introDelay);


    // Handle other buttons ..

    if (playGameVars.preventJumpDelay > 0) playGameVars.preventJumpDelay--;

    if (!playGameVars.playing && !gameStats.gameOver) {

        if (playGameVars.introDelay < 100 && ((justPressed & A_BUTTON) || (justPressed & B_BUTTON))) {

            playGameVars.playing = true;
            playGameVars.introDelay = 0;
            playGameVars.player.reset();
            gameStats.numberOfLivesLeft--;

            if (justPressed & A_BUTTON) {
                playGameVars.preventJumpDelay = 20;
            }

        }

    }
    else {    

        playGame_HandleCommonButtons();

    }

}


void playGame_CheckLastKey(uint8_t chain) {

    playGameVars.kong.setFlashChain(chain, true);
    playGameVars.key.setFlash(true);

    gameStats.score = gameStats.score + random(8, 16);

    if (gameStats.mode == GameMode::Easy) {

        playGameVars.upperClappers.reset(54);
        playGameVars.birds.reset();                            
        
    }

    #ifdef PLAY_SOUNDS 
        sound.tones(Sounds::Unlock);
    #endif

    for (uint8_t i = 0; i < 4 ; i++) {

        if (playGameVars.kong.getDisplayChain(i)) return;

    }

    playGameVars.lowerClappers.reset(107);
    playGameVars.upperClappers.reset(54);
    playGameVars.lowerSparks.reset(Coordinates::LowerSpark);
    playGameVars.upperSparks.reset(Coordinates::UpperSpark);
    playGameVars.birds.reset();
    playGameVars.exitSequence = true;

}

void playGame_ScrollToBottom(bool renderKong) {

    playGame_DrawScenery(0);
    if (renderKong) playGame_RenderKong(0);
    arduboy.displayWithBackground(Background::None);
    delay(1000);

    for (uint8_t i = 0; i < 64 ; i++) {

        playGame_DrawScenery(i);
        playGame_RenderKey(i);
        playGame_RenderScore(i);
        if (renderKong) playGame_RenderKong(i);
        arduboy.displayWithBackground(Background::None);
        delay(20);
    
    }

}


// ----------------------------------------------------------------------------
//  Render the state .. 
//
void playGame_Render() {

    const uint8_t yOffset = playGameVars.player.getYOffset();


    // Draw Scenery ..

    playGame_DrawScenery(yOffset);


    // Draw player

    if (!gameStats.gameOver) {

        if (playGameVars.introDelay == 0) {

            uint8_t stance = playGameVars.player.getImage();
            int8_t x = playGameVars.player.getXPosition(false);
            int8_t y = playGameVars.player.getYPosition();

            Sprites::drawPlusMask(x, y, pgm_read_ptr_t(&Images::Junior[stance]), 0);

        }
        else {

            if ((playGameVars.introDelay / 20) % 2 == 0) {

                int8_t x = playGameVars.player.getXPosition(false);
                int8_t y = playGameVars.player.getYPosition();

                if (playGameVars.player.isDead()) {

                    uint8_t stance = playGameVars.player.getImage();
                    Sprites::drawPlusMask(x, y, pgm_read_ptr_t(&Images::Junior[stance]), 0);

                }
                else {
                    Sprites::drawPlusMask(x, y, Images::Junior_Idle_R, 0);
                }

            }

        }

    }


    // Draw Kong

    if (playGameVars.kong.getEnabled()) {

        playGame_RenderKong(yOffset);

    }


    // Draw sparks / birds / clappers ..

    for (uint8_t i = 0; i < NUMBER_OF_ENTITIES; i++) {

        auto &lowerClapper = playGameVars.lowerClappers.getClapper(i);

        if (lowerClapper.isEnabled()) {

            int16_t x = lowerClapper.getXPosition();
            int8_t y = lowerClapper.getYPosition(yOffset);
            uint8_t index = lowerClapper.getImage();

            Sprites::drawPlusMask(x, y, Images::Clappers, index);

        }

        auto &upperClapper = playGameVars.upperClappers.getClapper(i);

        if (upperClapper.isEnabled()) {

            int16_t x = upperClapper.getXPosition();
            int8_t y = upperClapper.getYPosition(yOffset);
            uint8_t index = upperClapper.getImage();

            Sprites::drawPlusMask(x, y, Images::Clappers, index);

        }

        auto &lowerSpark = playGameVars.lowerSparks.getSpark(i);

        if (lowerSpark.isEnabled()) {

            int8_t x = lowerSpark.getXPosition();
            int8_t y = lowerSpark.getYPosition(yOffset);
            uint8_t index = lowerSpark.getImage();

            if (static_cast<SparkImage>(index) != SparkImage::None) {
                Sprites::drawPlusMask(x, y, Images::Spark, index);
            }

        }

        auto &upperSpark = playGameVars.upperSparks.getSpark(i);

        if (upperSpark.isEnabled()) {

            int8_t x = upperSpark.getXPosition();
            int8_t y = upperSpark.getYPosition(yOffset);
            uint8_t index = upperSpark.getImage();

            if (static_cast<SparkImage>(index) != SparkImage::None) {
                Sprites::drawPlusMask(x, y, Images::Spark, index);
            }

        }

        auto &bird = playGameVars.birds.getBird(i);

        if (bird.isEnabled()) {

            int8_t x = bird.getXPosition();
            int8_t y = bird.getYPosition(yOffset);
            uint8_t index = bird.getImage();

            if (static_cast<BirdImage>(index) != BirdImage::None) {

                Sprites::drawPlusMask(x, y, pgm_read_ptr_t(&Images::Birds[index]), 0);

            }

        }

    }


    playGame_RenderKey(yOffset);
    playGame_RenderGameOverOrPause();


    // Render score ..

    playGame_RenderScore(yOffset);

}


// ----------------------------------------------------------------------------
//  Render Kong .. 
//
void playGame_RenderKong(uint8_t yOffset) {

    int8_t x = playGameVars.kong.getXPosition();
    int8_t y = playGameVars.kong.getYPosition(yOffset);
    KongImage image = playGameVars.kong.getImage();

    Sprites::drawPlusMask(x, y, pgm_read_ptr_t(&Images::Kong[static_cast<uint8_t>(image)]), 0);
    
    const uint8_t xPos[] = { 30, 47, 76, 83 };
    const uint8_t yPos[] = { 6, 11, 11, 6 };
    
    for (uint8_t i = 0; i < 4 ; i++) {

        if (playGameVars.kong.getDisplayChain(i)) {
            Sprites::drawPlusMask(xPos[i], yPos[i] - yOffset, pgm_read_ptr_t(&Images::Lock_Chains[i]), 0);
        }

    }

}


// ----------------------------------------------------------------------------
//  Render key .. 
//
void playGame_RenderKey(uint8_t yOffset) {

    // Render key ..

    if (playGameVars.key.getKeyLocation() != KeyLocation::None) {

        int8_t x = playGameVars.key.getXPosition();
        int8_t y = playGameVars.key.getYPosition(yOffset);
        uint8_t index = playGameVars.key.getImage();

        if (index == NO_IMAGE) {

            index = arduboy.getFrameCount() % 59 / 15;
            Sprites::drawPlusMask(x, y, Images::Key_Spin, index);

        }
        else {

            if (playGameVars.key.getDisplay()) {
                Sprites::drawPlusMask(x, y, Images::Key_Rotate, index);
            }

        }

    }

}

void playGame_DrawScenery(uint8_t yOffset) {

    for (uint8_t i = 0; i < 16; i++) {

        Sprites::drawOverwrite(2, (i * 8) - yOffset, Images::Background, i);

    }

}

void playGame_KillPlayer() {
    
    #ifdef PLAY_SOUNDS 
        sound.tones(Sounds::Death);
    #endif
    
    playGameVars.player.setDead(true);
    playGameVars.playing = false;
    playGameVars.introDelay = INTRO_DELAY_FROM_TITLE;
    playGameVars.key.setKeyLocation(KeyLocation::LowerPosition);

}

void playGame_ResetLevel(uint8_t introDelay) {

    playGameVars.introDelay = introDelay;
    playGameVars.playing = false;

}

void playGame_RenderScore(uint8_t yOffset) {
    
    if (gameStats.numberOfLivesLeft >= 3) {
        Sprites::drawPlusMask(27, 118 - yOffset, Images::Junior_Walking_L_1, 0);
    }

    if (gameStats.numberOfLivesLeft >= 2) {
        Sprites::drawPlusMask(44, 118 - yOffset, Images::Junior_Walking_L_1, 0);
    }

    uint8_t digits[4] = {};
    extractDigits(digits, gameStats.score);

    Sprites::drawExternalMask(81, 118 - yOffset, Images::Game_Icon, Images::Game_Icon_Mask, gameStats.mode == GameMode::Hard, 0);

    for (uint8_t j = 4; j > 0; --j) {

        Sprites::drawErase(122 - (j*7), 120 - yOffset, Images::Numbers, digits[j - 1]);

    }

}


bool playGame_GetPaused() {

  return playGameVars.paused;

}

void playGame_SetPaused(bool value) {
  
  playGameVars.paused = value;

}

void playGame_HandleCommonButtons() {

    auto justPressed = arduboy.justPressedButtons();

    if (gameStats.gameOver) {

        if (justPressed & A_BUTTON) {
            gameState = GameStateType::HighScore_Activate; 
        }

    }
    else {

        if (justPressed & B_BUTTON) {
            playGameVars.paused = !playGameVars.paused; 
        }

    }

}

void playGame_RenderGameOverOrPause() {


  // Game Over?

  if (gameStats.gameOver) {

    Sprites::drawExternalMask(28, 13, Images::GameOver, Images::GameOver_Mask, 0, 0); 

  }

  // Pause?

  if (playGameVars.paused) {

    Sprites::drawExternalMask(39, 16, Images::Pause, Images::Pause_Mask, 0, 0); 

  }

}


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void highScore_Activate() {
	
    uint16_t score = EEPROM_Utils::readSaveScore(gameStats.mode);

    playGameVars.frameRate = FRAME_RATE_MIN;
    arduboy.setFrameRate(playGameVars.frameRate);
    arduboy.clearButtonState();

    highScoreVars.counter = 50;
    highScoreVars.newHighScore = false;
    highScoreVars.selectedMode = gameStats.mode;

    if (gameStats.score >= score) {
        EEPROM_Utils::saveScore(gameStats.mode, gameStats.score);
        highScoreVars.newHighScore = true;
    }

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void highScore_Update() {

    auto pressed = arduboy.pressedButtons();
    auto justPressed = arduboy.justPressedButtons();


    // Handle other input ..

    if (highScoreVars.counter > 0) {
        highScoreVars.counter--;
    }
        else {

        if ((justPressed & A_BUTTON || justPressed & B_BUTTON)) {
            gameState = GameStateType::TitleScreen_Activate; 
        }

        if ((pressed & LEFT_BUTTON) != 0) {
            highScoreVars.selectedMode = GameMode::Easy;
        }

        if ((pressed & RIGHT_BUTTON) != 0) {
            highScoreVars.selectedMode = GameMode::Hard;
        }

    }


	// Clear scores ..

	if ((pressed & LEFT_BUTTON) && (pressed & RIGHT_BUTTON)) {

		highScoreVars.clearScores++;

		switch (highScoreVars.clearScores) {

			case 21 ... 60:

				arduboy.setRGBled(128 - (highScoreVars.clearScores * 2), 0, 0);
				break;

			case 61:

				highScoreVars.clearScores = 0;
				arduboy.setRGBled(0, 0, 0);
				EEPROM_Utils::clearEEPROM();

				break;

		}

	}
	else {

		if (highScoreVars.clearScores > 0) {
		
			arduboy.setRGBled(0, 0, 0);
			highScoreVars.clearScores = 0;

		}
		
	}

}


void highScore_RenderHighScore(uint16_t score) {

    for (uint8_t j = 4; j > 0; --j) {

        uint8_t digits[4] = {};
        extractDigits(digits, score);
        Sprites::drawErase(79 - (j*7), 41, Images::Numbers, digits[j - 1]);

    }

}


// ----------------------------------------------------------------------------
//  Render the state .. 
//
void highScore_Render() {

    Sprites::drawOverwrite(0, 0, Images::HighScore_Left, 0);
    Sprites::drawOverwrite(113, 0, Images::HighScore_Right, 0);
    Sprites::drawOverwrite(15, 56, Images::HighScore_Bottom, 0);
    Sprites::drawOverwrite(49, 38, Images::HighScore_Jnr, 0);
    Sprites::drawOverwrite(19, 30, Images::HighScore_Chains, 0);
    Sprites::drawOverwrite(103, 30, Images::HighScore_Chains, 0);
    Sprites::drawOverwrite(10, 10, Images::HighScore_Panel_Left, 0);
    Sprites::drawOverwrite(51, 0, Images::HighScore_Panel_Mid, 0);
    Sprites::drawOverwrite(76, 10, Images::HighScore_Panel_Right, 0);
    Sprites::drawOverwrite(9, 22, Images::HighScore_Chain_Left, 0);
    Sprites::drawOverwrite(111, 22, Images::HighScore_Chain_Right, 0);

    switch(highScoreVars.selectedMode) {

        case GameMode::Easy:
            Sprites::drawOverwrite(16, 50, Images::HighScore_Easy, 0);
            break;

        case GameMode::Hard:
            Sprites::drawOverwrite(95, 50, Images::HighScore_Hard, 0);
            break;

    }

    if (!highScoreVars.newHighScore || (highScoreVars.newHighScore & arduboy.getFrameCountHalf(64))) { 

        uint16_t score = EEPROM_Utils::readSaveScore(highScoreVars.selectedMode);
        highScore_RenderHighScore(score);

    }

}
