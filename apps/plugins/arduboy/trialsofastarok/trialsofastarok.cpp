#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>

#include "AstarokGame.h"
#include "src/images/Images.h"
#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Constants.h"
#include "src/utils/Enums.h"
#include "src/utils/Structs.h"
#include "src/font/Font4x6.h"
#include <ArduboyPlaytune.h>

constexpr SQ7x8 Particle::rThresh;
constexpr SQ7x8 Particle::cF;
constexpr SQ7x8 Particle::gravity;
Arduboy2Ext arduboy;
#ifdef SOUNDS
ArduboyPlaytune tunes(arduboy.audio.enabled);
AstarokGame game(&arduboy, &tunes);
#else
AstarokGame game(&arduboy);
#endif
GameState gameState = GameState::SplashScreen_Init;

TitleScreenVars titleScreenVars;
HighScoreVars highScoreVars;
IntroTextVars introTextVars;
SeedVars seedVars;

#ifndef DEBUG
    ARDUBOY_NO_USB 
#endif

/* Arduino IDE auto-prototypes */
void initEEPROM(bool forceOverwrite);
int16_t checkSum(bool update);
void checkHighScoreSlot(uint16_t score);
void writeHighScoreEntry(HighScoreVars &highScoreVars);
void highScores();
void introText_Init();
void introText();
void seed_Init();
void seed();
void splashScreen_Init();
void splashScreen();
void drawBackground();
void titleScreen();

void setup() {

    arduboy.boot();
    arduboy.setFrameRate(30);
	arduboy.flashlight();
	arduboy.systemButtons();

    #ifdef SOUNDS
    arduboy.audio.begin();
    tunes.initChannel(PIN_SPEAKER_1);
    tunes.initChannel(PIN_SPEAKER_2);    
    #endif

    initEEPROM(false);

}

void loop() {

    if (!arduboy.nextFrame()) return;
    arduboy.pollButtons();

    switch (gameState) {

        case GameState::SplashScreen_Init:

            splashScreen_Init();
            /*-fallthrough*/

        case GameState::SplashScreen:

            splashScreen();
            arduboy.display(false);
            break;

        case GameState::Title_Init:

            #ifdef SOUNDS
            tunes.playScore(Sounds::Theme);
            #endif
            gameState = GameState::Title;
            /*-fallthrough*/

        case GameState::Title:

            titleScreen();
            arduboy.display(true);
            break;

        case GameState::IntroText_Init:

            introText_Init();
            /*-fallthrough*/

        case GameState::IntroText:

            introText();
            arduboy.display(true);
            break;

        case GameState::Seed_Init:

            seed_Init();
            /*-fallthrough*/
            
        case GameState::Seed:

            seed();
            arduboy.display(true);
            break;

        case GameState::Game_Init:

            game.newGame();
            gameState = GameState::Game_Play;
            /*-fallthrough*/

        case GameState::Game_Play:

            game.cycle(gameState);
            game.draw();

            if (game.event == EventType::Death) {
                game.drawScorePanel();
            }

            arduboy.displayWithBackground(game.mapNumber % 2 ? MapLevel::AboveGround : MapLevel::BelowGround);
            break;

        case GameState::Game_Mini:

            game.draw();
            game.playMiniGame(gameState);
            arduboy.displayWithBackground(game.mapNumber % 2 ? MapLevel::AboveGround : MapLevel::BelowGround);
            break;

        case GameState::HighScore_Check:

            highScoreVars.reset();
            checkHighScoreSlot(game.score + game.player.x / Constants::TileSize);

            if (highScoreVars.slot != Constants::EEPROM_No_Slot) {
                game.arduboy->resetFrameCount();
                gameState = GameState::HighScore_Flash;
            } 
            else {
                gameState = GameState::HighScore_NoFlash;
            }

            /*-fallthrough*/

        case GameState::HighScore_Flash:
        case GameState::HighScore_NoFlash:

            highScores();
            arduboy.display(true);
            break;

    }

}

#include "src/utils/Arduboy2Ext.h"
#include <EEPROM.h>

void initEEPROM(bool forceOverwrite) {

    uint8_t c1 = EEPROM.read(Constants::EEPROM_Start_C1);
    uint8_t c2 = EEPROM.read(Constants::EEPROM_Start_C2);

    if (c1 != 'A' || c2 != 'T' || forceOverwrite) {

        EEPROM.update(Constants::EEPROM_Start_C1, 'A');
        EEPROM.update(Constants::EEPROM_Start_C2, 'T');  

        uint16_t score = 30;
        uint8_t sounds = 1;

        EEPROM.put(Constants::EEPROM_Sounds, sounds);

        for (uint8_t i = 0; i < 5; i++) {
            EEPROM.update(Constants::EEPROM_Seeds + i, 0);
        }

        for (uint8_t i = 0; i < Constants::NumberOfScores; i++) {

            EEPROM.put(Constants::EEPROM_Scores + (7 * i), score = score - 10);
            EEPROM.update(Constants::EEPROM_Scores + (7 * i) + 2, 0);
            EEPROM.update(Constants::EEPROM_Scores + (7 * i) + 3, 0);
            EEPROM.update(Constants::EEPROM_Scores + (7 * i) + 4, 0);
            EEPROM.update(Constants::EEPROM_Scores + (7 * i) + 5, 0);
            EEPROM.update(Constants::EEPROM_Scores + (7 * i) + 6, 0);

        }

    } 
    else {

        int16_t checkSumOld = 0;
        int16_t checkSumNow = checkSum(false);
        EEPROM.get(Constants::EEPROM_Checksum, checkSumOld);

        if (checkSumNow != checkSumOld) {

            initEEPROM(true);

        }
        
    }    
   
}


/* -----------------------------------------------------------------------------
 *   Generate and optionally save a check sum .. 
 */
int16_t checkSum(bool update) {

    int16_t checksum = 0;

    for (uint8_t i = 0; i < (Constants::EEPROM_End - Constants::EEPROM_Start); i++) {

        checksum = checksum + ((i % 2 == 0 ? 1 : -1) * eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Start + i)));

    }

    if (update) {
        EEPROM.put(Constants::EEPROM_Checksum, static_cast<uint16_t>(checksum));
    }

    return checksum;

}


// Returns the new slot the provided high score should occupy ..
void checkHighScoreSlot(uint16_t score) {

    uint16_t tmpScore = 0;

    for (uint8_t i = 0; i < Constants::NumberOfScores; i++) {

        EEPROM.get(Constants::EEPROM_Scores + (7 * i), tmpScore);

        if (tmpScore < score) {
            
            highScoreVars.slot = i;
            highScoreVars.score = game.score + game.player.x / Constants::TileSize;

            for (uint8_t i = 0; i < 5; i++) {
                highScoreVars.seeds[i] = seedVars.seed[i];
            }
            
            writeHighScoreEntry(highScoreVars);
            return;

        }

    }

    highScoreVars.slot = Constants::EEPROM_No_Slot;

}

void writeHighScoreEntry(HighScoreVars &highScoreVars) {


    // Move the scores down one ..

    for (uint8_t j = Constants::NumberOfScores - 1; j > highScoreVars.slot; j--) {          

        for (uint8_t k = 0; k < 7; k++) {

            EEPROM.write(Constants::EEPROM_Scores + (7 * j) + k, (char)EEPROM.read(Constants::EEPROM_Scores + (7 * (j - 1)) + k));

        }

    }


    // Write score and initials to the nominated slot ..

    EEPROM.put(Constants::EEPROM_Scores + (7 * highScoreVars.slot), game.score + game.player.x / Constants::TileSize);

    for (uint8_t i = 0; i < 5; i++) {

        EEPROM.update(Constants::EEPROM_Scores + (7 * highScoreVars.slot) + 2 + i, highScoreVars.seeds[i]);

    }

    checkSum(true);

}

void highScores() {

    uint16_t score;

    // Rnadomly spin a rune ..

    if (gameState == GameState::HighScore_NoFlash && highScoreVars.spinCountdown == 0) {

        highScoreVars.index = hash(game.seed) % 14;
        highScoreVars.spinIndex = 16;
        highScoreVars.spinCountdown = 100;

    }


    // Render screen ..

    Sprites::drawOverwrite(1, 0, Images::Underground_Chain, 0);
    Sprites::drawOverwrite(9, 1, Images::TopThree, 0);
    Sprites::drawOverwrite(65, 0, Images::Underground_Chain, 0);

    for (uint8_t x = 74; x < 128; x = x + 4) {

        Sprites::drawOverwrite(x, 1, Images::HighScore_Pattern, 0);

    }

    for (uint8_t i = 0; i < Constants::NumberOfScores; i++) {

        // If we are not editing this slot, the render it normally ..
        
        if (gameState == GameState::HighScore_NoFlash || i != highScoreVars.slot || arduboy.getFrameCountHalf(32)) {

            EEPROM.get(Constants::EEPROM_Scores + (7 * i), score);

            for (uint8_t j = 0; j < 5; j++) {

                if ((i * 5) + j == highScoreVars.index && highScoreVars.spinIndex > 0) {
                    Sprites::drawOverwrite(1 + (j * 14), 15 + (i * 17), Images::Rune_Frame, 16 - highScoreVars.spinIndex);
                }
                else {
                    Sprites::drawOverwrite(1 + (j * 14), 15 + (i * 17), Images::Rune_Frame, 0);
                    Sprites::drawOverwrite(4 + (j * 14), 15 + (i * 17) + 4, Images::Runes, EEPROM.read(Constants::EEPROM_Scores + (7 * i) + 2 + j));
                }

            }

            // Render score always ..

            uint8_t digits[6] = {};
            extractDigits(digits, score);
            
            for (uint8_t j = 0; j < 6; j++) {
                Sprites::drawOverwrite(69 + ((6 - j) * 5), 15 + (i * 17) + 4, Images::Numbers, digits[j]);
            }

        }

    }

    Sprites::drawOverwrite(105, 16, Images::Outside_Exit_00, 0);
    Sprites::drawOverwrite(108, 53, Images::HighScore_Pattern_2, 0);


    // Handle player input ..

    uint8_t justPressed = arduboy.justPressedButtons();
    uint8_t pressed = arduboy.pressedButtons();

    if ((pressed & LEFT_BUTTON) && (pressed & RIGHT_BUTTON)) {
        
        highScoreVars.resetCounter++;

        if (highScoreVars.resetCounter == 64) {
            initEEPROM(true);
            highScoreVars.resetCounter = 0;
        }

    }
    else if ((justPressed & A_BUTTON) || (justPressed & B_BUTTON)) {

        gameState = GameState::Title_Init;
        highScoreVars.resetCounter = 0;

    }
    else {

        highScoreVars.resetCounter = 0;

    }

    if (arduboy.frameCount >= 126 && gameState == GameState::HighScore_Flash) gameState = GameState::HighScore_NoFlash;
    highScoreVars.decSpinIndex();

}

#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Constants.h"
#include "src/images/Images.h"


const uint8_t textLengths[] = { 88, 81, 84 };

const uint8_t PROGMEM IntroText_00[] = {
    3,120/*E*/,'a','c','h',' ','y','e','a','r',' ','t','h','e',' ','p','e','o','p','l','e',' ','o','f',
    0,'t','h','e',' ','v','i','l','l','a','g','e',' ','c','h','o','o','s','e',' ','a',' ','n','e','w',
    12,'c','h','a','m','p','i','o','n',' ','t','o',' ','p','r','o','t','e','c','t',
    15,'t','h','e','m',' ','f','r','o','m',' ',106/*A*/,'s','t','a','r','o','k','.'
};

const uint8_t PROGMEM IntroText_01[] = {
    7, 113/*C*/,'o','m','p','e','t','e',' ','i','n',' ','t','h','e',' ','t','r','i','a','l','s',
    6,'t','o',' ','s','e','e',' ','h','o','w',' ','f','a','r',' ','y','o','u',' ','c','a','n',
    11,'g','e','t',' ','a','n','d',' ','y','o','u',' ','c','o','u','l','d',' ','b','e',
    25,'t','h','a','t',' ','c','h','a','m','p','i','o','n','!'
};

const uint8_t PROGMEM IntroText_02[] = { 
    10,122/*Y*/,'o','u','r',' ','c','h','o','i','c','e',' ','o','f',' ','r','u','n','e','s',
    7,'w','i','l','l',' ','s','e','l','e','c','t',' ','d','i','f','f','e','r','e','n','t',
    12,'c','o','u','r','s','e','s','.','.','.',' ','s','o','m','e',' ','a','r','e',
    13,'e','a','s','i','e','r',' ','t','h','a','n',' ','o','t','h','e','r','s','!'
};

const uint8_t * const IntroTexts[] = { IntroText_00, IntroText_01, IntroText_02 }; 

void introText_Init() {

    introTextVars.reset();
    gameState = GameState::IntroText;

}

void introText() {

    uint8_t line = 0;
    uint8_t x = 0;
    
    Sprites::drawOverwrite(  1,  1, Images::EnterRunes_Frame, 0);
    Sprites::drawOverwrite(  1, 54, Images::EnterRunes_Frame, 2);
    Sprites::drawOverwrite(120,  1, Images::EnterRunes_Frame, 1);
    Sprites::drawOverwrite(120, 54, Images::EnterRunes_Frame, 3);

    arduboy.drawFastHLine(1, 1, 125);
    arduboy.drawFastHLine(1, 61, 125);
    arduboy.drawFastHLine(3, 63, 122);


    for (uint8_t i = 0; i < introTextVars.index; i++) {

        uint8_t c = pgm_read_byte(&IntroTexts[introTextVars.panel][i]);

        switch (c) {

            case 0 ... 31:
                line++;
                x = 0 + c;
                break;

            case ' ':
                x = x + 4;
                break;
            
            default:
                Sprites::drawOverwrite(6 + x, 3 + (line * 10), Font::Images, Font::getIndex(c));
                x = x + 5;
                break;

        }
        
    }


    if (introTextVars.index < textLengths[introTextVars.panel] && arduboy.getFrameCount(2)) {

        introTextVars.index++;

    }


    uint8_t justPressed = arduboy.justPressedButtons();

    if (justPressed & A_BUTTON || justPressed & B_BUTTON) {
        
        if (introTextVars.index == textLengths[introTextVars.panel] && introTextVars.skipTypewriter == false) {
            introTextVars.panel++;
            introTextVars.index = 0;
        }
        else {
            if (introTextVars.skipTypewriter) introTextVars.panel++;
            introTextVars.skipTypewriter = true;
            introTextVars.index = textLengths[introTextVars.panel];
        }

        if (introTextVars.panel == 3) {
            gameState = GameState::Seed_Init;
        }

    }

}


#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Constants.h"
#include "src/images/Images.h"

void seed_Init() {

    #ifdef SOUNDS
    tunes.stopScore();
    #endif

    seedVars.reset();
    gameState = GameState::Seed;

}

void seed() {

    drawBackground();
    
    Sprites::drawOverwrite(34, 8, Images::EnterRunes_Text, 0);
    Sprites::drawOverwrite(24, 21, Images::EnterRunes_Frame, 0);
    Sprites::drawOverwrite(24, 47, Images::EnterRunes_Frame, 2);
    Sprites::drawOverwrite(96, 21, Images::EnterRunes_Frame, 1);
    Sprites::drawOverwrite(96, 47, Images::EnterRunes_Frame, 3);

    arduboy.drawFastHLine(24, 21, 72);
    arduboy.drawFastHLine(24, 54, 72);
    arduboy.drawFastHLine(26, 56, 75);

    for (uint8_t i = 0; i < 5; i++) {

        if (i == seedVars.index && seedVars.spinIndex > 0) {
            Sprites::drawOverwrite(30 + (i* 14), 31, Images::Rune_Frame, 16 - seedVars.spinIndex);
        }
        else {
            Sprites::drawOverwrite(30 + (i* 14), 31, Images::Rune_Frame, 0);
            Sprites::drawOverwrite(33 + (i* 14), 35, Images::Runes, seedVars.seed[i]);
        }

    }

    Sprites::drawSelfMasked(33 + (seedVars.index * 14), 26, Images::Arrows, 0);
    Sprites::drawSelfMasked(33 + (seedVars.index* 14), 47, Images::Arrows, 1);

    uint8_t justPressed = arduboy.justPressedButtons();

    if (seedVars.spinIndex == 0) {

        if (justPressed & UP_BUTTON) {
            seedVars.incValue();
            
            #ifdef SOUNDS
            tunes.playScore(Sounds::ButtonPress);
            #endif
        }

        if (justPressed & DOWN_BUTTON) { 
            seedVars.decValue();
            
            #ifdef SOUNDS
            tunes.playScore(Sounds::ButtonPress);
            #endif
        }

        if (justPressed & LEFT_BUTTON) { 
            if (seedVars.decIndex()) {
                #ifdef SOUNDS
                tunes.playScore(Sounds::ButtonPress);
                #endif
            }
        }

        if (justPressed & RIGHT_BUTTON) { 
            if (seedVars.incIndex()) {
                #ifdef SOUNDS
                tunes.playScore(Sounds::ButtonPress);
                #endif
            }
        }

        if ((justPressed & A_BUTTON) || (justPressed & B_BUTTON)) {

            game.seed = seedVars.getSeedValue();
            for (uint8_t a = 0; a < Constants::GameSeeds; a++ ) {
                game.seeds[a] = hash(game.seed);
            }

            #ifdef SOUNDS
            tunes.playScore(Sounds::ButtonPress);
            #endif
            gameState = GameState::Game_Init;

        }

    }

    seedVars.decSpinIndex();

}


#include "src/utils/Arduboy2Ext.h"

static constexpr uint8_t ppotFrames = (1.6 *30); // 1.6s animation loop


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
#include "src/utils/Arduboy2Ext.h"

void drawBackground() {


    Sprites::drawOverwrite(4, 12, Images::Underground_Brick, 0);
    Sprites::drawOverwrite(105, 12, Images::Underground_Brick, 0);

    Sprites::drawOverwrite(-10, 27, Images::Underground_Brick, 0);
    Sprites::drawOverwrite(119, 27, Images::Underground_Brick, 0);

    Sprites::drawOverwrite(4, 41, Images::Underground_Brick, 0);
    Sprites::drawOverwrite(105, 41, Images::Underground_Brick, 0);

    Sprites::drawOverwrite(11, 27, Images::Torch, arduboy.getFrameCount(16) / 4);
    Sprites::drawOverwrite(111, 27, Images::Torch, arduboy.getFrameCount(16) / 4);

}

void titleScreen() {

    uint8_t justPressed = arduboy.justPressedButtons();

    Sprites::drawOverwrite(24, 4, Images::Title, 0);
    Sprites::drawSelfMasked(35, 0, Images::Underground_Chain, 0);
    Sprites::drawSelfMasked(90, 0, Images::Underground_Chain, 0);    
    drawBackground();

    Sprites::drawOverwrite(titleScreenVars.index == TitleScreenMode::Play ? 35 : 65, 59, Images::Title_Highlight, 0);

    Sprites::drawOverwrite(119, 56, Images::SoundToggle, arduboy.audio.enabled());

    if ((justPressed & A_BUTTON) || (justPressed & B_BUTTON)) {

        switch (titleScreenVars.index) {

            case TitleScreenMode::Play:
                if (introTextVars.seenIntroText)  {
                    gameState = GameState::Seed_Init;
                }
                else {
                    gameState = GameState::IntroText_Init;
                    introTextVars.seenIntroText = true;
                }
                break;

            default:
                gameState = GameState::HighScore_NoFlash;
                break;

        }

    }

    if ((justPressed & UP_BUTTON) || (justPressed & DOWN_BUTTON)) {
        
        if (arduboy.audio.enabled()) {

            arduboy.audio.off(); 
            arduboy.audio.saveOnOff();

        }
        else {

            arduboy.audio.on(); 
            arduboy.audio.saveOnOff();

        }

    }

    if (justPressed & LEFT_BUTTON) {

        titleScreenVars.index = TitleScreenMode::Play;

    }

    if (justPressed & RIGHT_BUTTON) {

        titleScreenVars.index = TitleScreenMode::HighScore;

    }

}


