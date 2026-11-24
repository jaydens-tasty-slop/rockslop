#include <Arduino.h>
#include "src/utils/Arduboy2Ext.h"
#include <Sprites.h>

#include "src/images/Images.h"
#include "src/puzzles/Puzzles.h"
#include "src/utils/Enums.h"
#include "src/utils/Utils.h"
#include "src/utils/EEPROM_Utils.h"
#include "src/entities/Entities.h"
#include "src/fonts/Font3x5.h"


#ifdef SOUNDS
#include <ArduboyTones.h>
#include "src/sounds/Sounds.h"
#endif


Arduboy2Ext arduboy;
Font3x5 font3x5 = Font3x5();

#ifdef SOUNDS
ArduboyTones sound(arduboy.audio.enabled);
#endif

GameState gameState = GameState::Splash_Init;
GameStats gameStats;
LevelSelectVars levelSelectVars;
Player player;
FallingTile fallingTiles[Constants::FallingTilesCount];
Arrow arrows[3];

uint8_t board[Constants::BoardHeight][Constants::BoardWidth];
constexpr SQ7x8 Particle::rThresh;
constexpr SQ7x8 Particle::cF;
constexpr SQ7x8 Particle::gravity;
Particle particles[Constants::ParticlesMax];

/* Arduino IDE auto-prototypes */
void game_Init();
void game();
void removeTile();
void initGame(uint8_t level);
bool endOfGame();
void instructions_Init();
void instructions();
void levelSelect_Init();
void levelSelect();
void launchParticles(int16_t x, int16_t y);
void updateAndRenderParticles();
void renderBoard_Zoom();
void renderBoard_Normal();
void renderHUD();
void splashScreen_Init();
void splashScreen();
void title_Init();
void title();

void setup() {

    arduboy.boot();
    arduboy.setFrameRate(30);
	arduboy.flashlight();
	arduboy.systemButtons();
    EEPROM_Utils::initEEPROM(false);

    #ifdef SOUNDS
    arduboy.audio.begin();
    #endif

}

void loop() {

    if (!arduboy.nextFrame()) return;    
	arduboy.pollButtons();

    switch (gameState) {

        case GameState::Splash_Init:
            splashScreen_Init();
            splashScreen();
            break;

        case GameState::Splash:
            splashScreen();
            break;

        case GameState::Title_Init:
            title_Init();
            title();
            break;

        case GameState::Title:
            title();
            break;

        case GameState::LevelSelect_Init:
            levelSelect_Init();
            levelSelect();
            break;

        case GameState::LevelSelect:
            levelSelect();
            break;

        case GameState::Instructions_Init:
            instructions_Init();
            instructions();
            break;

        case GameState::Instructions:
            instructions();
            break;

        case GameState::Game_Init:
            game_Init();
            game();
            arduboy.display(true);
            break;

        case GameState::Game:
            game();
            break;

    }

    arduboy.display(true);

}


#include <Arduboy2.h>

// ----------------------------------------------------------------------------
//  Initialise state ..
//
void game_Init() {

    gameStats.reset();
    gameState = GameState::Game;
    gameStats.zoom = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Zoom)) > 0 ? 1 : 0;

    gameStats.xOffset = -132;
    gameStats.exit = 0;
    gameStats.endOfGame = false;
    
}   


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void game() {

    if (endOfGame() && !gameStats.endOfGame) {

        gameStats.endOfGame = true;

        #ifdef SOUNDS
        sound.tones(Sounds::Positive);
        #endif

    }


    // Update scrolling motion ..

    switch (gameStats.xOffset) {

        case -132:
            initGame(gameStats.level);
            gameStats.xOffset = gameStats.xOffset + 4;
            break;

        case -131 ... -1:
            gameStats.xOffset = gameStats.xOffset + 4;
            break;

        case 1 ... 127:
            gameStats.xOffset = gameStats.xOffset + 4;
            break;

        case 128 ... 256:
            gameStats.xOffset = -132;
            break;

        default: 
            break;

    }


    // Handle player movements ..

    if (arduboy.pressed(A_BUTTON)) { 

        gameStats.zoomCount++;

        if (gameStats.zoomCount == 8) {

            gameStats.zoom = gameStats.zoom > 0 ? 0 : 1;
            eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Zoom), gameStats.zoom);

        }
    }
    else {

        gameStats.zoomCount = 0;

    }


    // if (arduboy.justPressed(B_BUTTON)) gameStats.moves = 0;// SJH remove
    // if (arduboy.pressed(B_BUTTON) && arduboy.justPressed(UP_BUTTON)) { gameStats.level--; initGame(gameStats.level); }// SJH remove
    // if (arduboy.pressed(B_BUTTON) && arduboy.justPressed(DOWN_BUTTON)) { gameStats.level++; initGame(gameStats.level); }// SJH remove
    
    if (arduboy.pressed(B_BUTTON)) { // Exit

        gameStats.exit++;

        if (gameStats.exit == 32) {

            gameState = GameState::Title_Init;
            gameStats.exit = 0;

        }
    }
    else {

        gameStats.exit = 0;

    }

    if (gameStats.xOffset == 0 && !player.isMoving() && !gameStats.endOfGame && !player.isDying()) {

        if (arduboy.justPressed(LEFT_BUTTON) && player.getX() > -1)                             { player.moveLeft();   gameStats.moves++;   removeTile(); } 
        else if (arduboy.justPressed(RIGHT_BUTTON) && player.getX() < Constants::BoardWidth)    { player.moveRight();  gameStats.moves++;   removeTile(); } 
        else if (arduboy.justPressed(UP_BUTTON) && player.getY() > -1)                          { player.moveUp();     gameStats.moves++;   removeTile(); } 
        else if (arduboy.justPressed(DOWN_BUTTON) && player.getY() < Constants::BoardHeight)    { player.moveDown();   gameStats.moves++;   removeTile(); } 

    }


    // Is the player on a 'blank' tile?

    if (!player.isDying() && (player.getXNew() < 0 || player.getXNew() == Constants::BoardWidth || player.getYNew() < 0 || player.getYNew() == Constants::BoardHeight || board[player.getYNew()][player.getXNew()] == static_cast<uint8_t>(Tiles::None))) {

        player.kill();
        
        if (gameStats.xOffset == 0) {

            #ifdef SOUNDS
            sound.tones(Sounds::Death);
            #endif

        }

    }
    
            
    // Render board ..

    if (!gameStats.endOfGame) {

        if (gameStats.zoom) {
            renderBoard_Zoom();
        }
        else {
            renderBoard_Normal();
        }

    }
    renderHUD();


    // Update the game state ..

    if (player.update(gameStats.zoom) && gameStats.xOffset == 0) {
    
        gameStats.xOffset = 4;

    }

               
    for (FallingTile &fallingTile : fallingTiles) {

        if (fallingTile.isActive()) {

            fallingTile.update();
            
        }

    }

    if (arduboy.isFrameCount(6)) {
                
        for (Arrow &arrow : arrows) {

            arrow.update();

        }
        
    }

    if (gameStats.endOfGame) {

        uint8_t stars = 0;

        if (gameStats.moves < gameStats.minimumMoves + 2)           { stars = 3; }
        else if (gameStats.moves < gameStats.minimumMoves * 1.5)    { stars = 2; }
        else if (gameStats.moves < gameStats.minimumMoves * 3)      { stars = 1; }
        else                                                        { stars = 0; }

        Sprites::drawExternalMask(45, 20, (stars > 0 ? Images::Star_Filled : Images::Star_Hollow), Images::Star_Mask, 0, 0);
        Sprites::drawExternalMask(59, 20, (stars > 1 ? Images::Star_Filled : Images::Star_Hollow), Images::Star_Mask, 0, 0);
        Sprites::drawExternalMask(73, 20, (stars > 2 ? Images::Star_Filled : Images::Star_Hollow), Images::Star_Mask, 0, 0);

        if (gameStats.level + 1 == Puzzles::Count) {

            Sprites::drawOverwrite(37, 32, Images::EndOfGame, 0);
            updateAndRenderParticles();

            if (arduboy.isFrameCount(32)) launchParticles(random(32, 97), random(16, 48));

        }
        else {

            Sprites::drawOverwrite(22, 32, Images::Congratulations, 0);
            eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Current), gameStats.level + 1);
            if (gameStats.maxLevel < gameStats.level + 1) {
                eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Max), gameStats.level + 1);
            }

        }

        eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Rating + gameStats.level), stars);

        if (arduboy.justPressed(A_BUTTON)) {

            if (gameStats.level + 1 == Puzzles::Count) {

                gameState = GameState::Title_Init;

            }
            else {

                gameStats.xOffset = 4;
                gameStats.level++;
                gameStats.moves = 0;
                gameStats.endOfGame = false;

            }

        }

    }

}

void removeTile() {

    switch (static_cast<Tiles>(board[player.getY()][player.getX()])) {

        case Tiles::NormalFloor:

            #ifdef SOUNDS
            sound.tones(Sounds::Tone_01);
            #endif

            board[player.getY()][player.getX()] = static_cast<uint8_t>(Tiles::None);
           
            for (FallingTile &fallingTile : fallingTiles) {

                if (!fallingTile.isActive()) {

                    fallingTile.init(player.getX(), player.getY());
                    break;

                }

            }

            break;

        case Tiles::DoubleFloor:

            #ifdef SOUNDS
            sound.tones(Sounds::Tone_02);
            #endif

            board[player.getY()][player.getX()] = static_cast<uint8_t>(Tiles::NormalFloor);
            break;

        case Tiles::Button1:

            #ifdef SOUNDS
            sound.tones(Sounds::Tone_03);
            #endif

            board[player.getY()][player.getX()] = static_cast<uint8_t>(Tiles::Button2);
            break;

        case Tiles::Button2:

            #ifdef SOUNDS
            sound.tones(Sounds::Tone_03);
            #endif

            board[player.getY()][player.getX()] = static_cast<uint8_t>(Tiles::Button1);
            break;

        default: 
            #ifdef SOUNDS
            sound.tones(Sounds::Tone_01);
            #endif
            break;

    }
    
}

void initGame(uint8_t level) {

    const uint8_t *puzzle = pgm_read_ptr(&Puzzles::puzzles[level]);

    uint16_t idx = 0;


    // Retrieve player starting position ..

    uint8_t startX = pgm_read_byte(&puzzle[idx++]);
    uint8_t startY = pgm_read_byte(&puzzle[idx++]);

    player.init(startX, startY);


    // Retrieve arrows (if defined) ..

    startX = pgm_read_byte(&puzzle[idx++]);
    startY = pgm_read_byte(&puzzle[idx++]);

    arrows[0].init(startX, startY);

    startX = pgm_read_byte(&puzzle[idx++]);
    startY = pgm_read_byte(&puzzle[idx++]);


    // Minimium number of moves

    gameStats.minimumMoves = pgm_read_byte(&puzzle[idx++]);

    arrows[1].init(startX, startY);

    for (uint16_t y = 0; y < Constants::BoardHeight; y++){

        for (uint16_t x = 0; x < Constants::BoardWidth; x++){

            uint8_t data = pgm_read_byte(&puzzle[idx++]);

            board[y][x] = data;

        }

    }


    // Clear falling tiles ..

    for (FallingTile &fallingTile : fallingTiles) {

        fallingTile.setActive(false);

    }


    // Scroll in ..

    gameStats.xOffset = -132;
    gameStats.endOfGame = false;


    // If last row is all zeroes we can render the board down 6px ..

    uint8_t count = 0;

    for (uint16_t x = 0; x < Constants::BoardWidth; x++){

        count = count + (board[Constants::BoardHeight - 1][x] == 0 ? 1 : 0);

    }

    gameStats.yOffset = count == Constants::BoardWidth ? 6 : 0;

}

bool endOfGame() {

    if (gameStats.xOffset == 0 && !player.isMoving()) {

        for (int16_t y = 0; y < Constants::BoardHeight; y++) {

            for (int16_t x = 0; x < Constants::BoardWidth; x++) {

                if (x == player.getX() && y == player.getY()) {

                    if (board[y][x] == static_cast<uint8_t>(Tiles::Button2)) {

                        return false;

                    }

                }
                else {

                    if (board[y][x] == static_cast<uint8_t>(Tiles::Button1)) {

                        return false;

                    }

                }

            }

        }

        return true;

    }
    else {

        return false;
    }

}
#include <Arduboy2.h>

// ----------------------------------------------------------------------------
//  Initialise state ..
//
void instructions_Init() {
    
    gameState = GameState::Instructions;
    gameStats.start = 0;
    
}   


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void instructions() {

    // Delay for 'good luck' ..

    if (gameStats.start < 56) { gameStats.start++; }


    // Handle player actions ..

    if (arduboy.justPressed(LEFT_BUTTON) && gameStats.instruction > 0) { 

        gameStats.instruction--;

    }         

    if (arduboy.justPressed(RIGHT_BUTTON) || arduboy.justPressed(A_BUTTON)) {

        if (gameStats.instruction < 2) { 

            gameStats.instruction++;
            gameStats.start = 0;

        }
        else {

            gameState = GameState::Game_Init;

        }

    }         


    // Render page ..

    Sprites::drawOverwrite(0, 0, Images::Instructions, 0);

    switch (gameStats.instruction) {

        case 0:
            Sprites::drawOverwrite(123, 0, Images::Arrow_Right, 0);

            font3x5.setCursor(0, 15);
            font3x5.print("Move~between~the~floor~tiles~to");

            font3x5.setCursor(0, 24);
            font3x5.print("turn~on~all~the~button~switches.");

            font3x5.setCursor(0, 38);
            font3x5.print("But~be~careful~though~as~some");

            font3x5.setCursor(0, 47);
            font3x5.print("floor~tiles~will~fall~when~crossed");

            font3x5.setCursor(0, 56);
            font3x5.print("preventing~you~from~back-tracking!");

            break;

        case 1:
            Sprites::drawOverwrite(116, 0, Images::Arrow_Left, 0);
            Sprites::drawOverwrite(123, 0, Images::Arrow_Right, 0);

            font3x5.setCursor(0, 13);
            font3x5.print("Floor Tiles");

            Sprites::drawOverwrite(0, 27, Images::Tiles, 1 + (gameStats.tileSet * 7));
            font3x5.setCursor(10, 27);
            font3x5.print("Normal:~falls~away~once~crossed.");

            Sprites::drawOverwrite(0, 39, Images::Tiles, 4 + (gameStats.tileSet * 7));
            font3x5.setCursor(10, 39);
            font3x5.print("Double:~can~be~crossed~twice.");

            Sprites::drawOverwrite(0, 51, Images::Tiles, 3 + (gameStats.tileSet * 7));
            font3x5.setCursor(10, 51);
            font3x5.print("Solid:~never~falls~when~crossed.");
            break;

        case 2:
            Sprites::drawOverwrite(116, 0, Images::Arrow_Left, 0);
            Sprites::drawOverwrite(123, 0, Images::Arrow_Right, 0);

            font3x5.setCursor(0, 13);
            font3x5.print("Button Switches");

            Sprites::drawOverwrite(0, 27, Images::Tiles, 5 + (gameStats.tileSet * 7));
            font3x5.setCursor(10, 27);
            font3x5.print("Off:~switch~is~in~OFF~position.");

            Sprites::drawOverwrite(0, 39, Images::Tiles, 6 + (gameStats.tileSet * 7));
            font3x5.setCursor(10, 39);
            font3x5.print("On:~switch~is~in~ON~position.");

            if (gameStats.start >= 56) {
            
                font3x5.setCursor(33, 56);
                font3x5.print("Press~A~to~start~!");

            }
            else {
            
                font3x5.setCursor(13, 56);
                font3x5.print("Hold~A~to~zoom~and~B~to~quit.");

            }

            break;

    }

    arduboy.drawFastHLine(0, 8, 128, WHITE);

}

#include "src/utils/Arduboy2Ext.h"


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void levelSelect_Init() {

    gameState = GameState::LevelSelect;

    levelSelectVars.cursor = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Current));
    gameStats.maxLevel = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Max));

    if (gameStats.maxLevel <= 3)                                    { levelSelectVars.topRow = 0; }
    else if (levelSelectVars.cursor <= 1)                           { levelSelectVars.topRow = 0; }
    else if (levelSelectVars.cursor == gameStats.maxLevel)          { levelSelectVars.topRow = levelSelectVars.cursor - 3; }
    else if (levelSelectVars.cursor == gameStats.maxLevel - 1)      { levelSelectVars.topRow = levelSelectVars.cursor - 2; }
    else                                                            { levelSelectVars.topRow = levelSelectVars.cursor - 1; }
    
}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void levelSelect() { 

    if (levelSelectVars.cursorFlashTop > 0)     levelSelectVars.cursorFlashTop--;
    if (levelSelectVars.cursorFlashBottom > 0)  levelSelectVars.cursorFlashBottom--;

    if (arduboy.justPressed(A_BUTTON)) {

        gameStats.level = levelSelectVars.cursor;
        gameStats.moves = 0;
        eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Current), gameStats.level);
        gameState = GameState::Game_Init; 

    }

    if (arduboy.justPressed(B_BUTTON)) {

        gameState = GameState::Title_Init; 

    }

    if (arduboy.justPressed(UP_BUTTON) && levelSelectVars.cursor > 0) {

        levelSelectVars.cursor--;
        levelSelectVars.cursorFlashTop = 4;

        if (gameStats.maxLevel <= 3)                                    { levelSelectVars.topRow = 0; }
        else if (levelSelectVars.cursor <= 1)                           { levelSelectVars.topRow = 0; }
        else if (levelSelectVars.cursor == gameStats.maxLevel)          { levelSelectVars.topRow = levelSelectVars.cursor - 3; }
        else if (levelSelectVars.cursor == gameStats.maxLevel - 1)      { levelSelectVars.topRow = levelSelectVars.cursor - 2; }
        else                                                            { levelSelectVars.topRow = levelSelectVars.cursor - 1; }

    }

    if (arduboy.justPressed(DOWN_BUTTON) && levelSelectVars.cursor < gameStats.maxLevel) {

        levelSelectVars.cursor++;
        levelSelectVars.cursorFlashBottom = 4;

        if (gameStats.maxLevel <= 3)                                    { levelSelectVars.topRow = 0; }
        else if (levelSelectVars.cursor <= 1)                           { levelSelectVars.topRow = 0; }
        else if (levelSelectVars.cursor == gameStats.maxLevel)          { levelSelectVars.topRow = levelSelectVars.cursor - 3; }
        else if (levelSelectVars.cursor == gameStats.maxLevel - 1)      { levelSelectVars.topRow = levelSelectVars.cursor - 2; }
        else if (levelSelectVars.cursor == gameStats.maxLevel - 2)      { levelSelectVars.topRow = levelSelectVars.cursor - 1; }
        else                                                            { levelSelectVars.topRow = levelSelectVars.cursor - 1; }

    }



    if (levelSelectVars.topRow > 0) { 
        
        Sprites::drawOverwrite(61, levelSelectVars.cursorFlashTop > 0 ? 2 : 0, Images::Arrow_Up, 0); 
        arduboy.drawFastHLine(0, 0, 54);
        arduboy.drawFastHLine(75, 0, 54);        

    }
    else {

        arduboy.drawFastHLine(0, 0, 128);

    }


    uint8_t y = 10;

    for (uint8_t i = levelSelectVars.topRow; i <= (gameStats.maxLevel <= 3 ? gameStats.maxLevel : levelSelectVars.topRow + 3); i++) {

        Sprites::drawOverwrite(20, y, Images::Level, 0);

        {
            uint8_t digits[2] = {};
            uint8_t x = 61;

            extractDigits(digits, i);

            for (uint8_t i = 0; i < 2; ++i) {

                if (digits[i] == 1) x = x - 3;

            }

            for (uint8_t i = 0; i < 2; ++i) {
                Sprites::drawOverwrite(x, y, Images::Numbers, digits[i]);
                x = x - (digits[i] == 1 ? 5 : 8);
            }

        }

        uint8_t stars = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Rating) + i);
        Sprites::drawExternalMask(74, y - 1, (stars > 0 ? Images::Star_Filled : Images::Star_Hollow), Images::Star_Mask, 0, 0);
        Sprites::drawExternalMask(86, y - 1, (stars > 1 ? Images::Star_Filled : Images::Star_Hollow), Images::Star_Mask, 0, 0);
        Sprites::drawExternalMask(98, y - 1, (stars > 2 ? Images::Star_Filled : Images::Star_Hollow), Images::Star_Mask, 0, 0);

        if (i == levelSelectVars.cursor) {
            uint8_t frame = arduboy.getFrameCount(32) / 8;
            Sprites::drawSelfMasked(7, y, Images::Player, frame);
        }

        y = y + 12;

    }

    if (levelSelectVars.topRow + 3 < gameStats.maxLevel) { 
        
        Sprites::drawOverwrite(61, 58 - (levelSelectVars.cursorFlashBottom > 0 ? 2 : 0), Images::Arrow_Down, 0); 
        arduboy.drawFastHLine(0, 63, 54);
        arduboy.drawFastHLine(75, 63, 54);
        
    }
    else {

        arduboy.drawFastHLine(0, 63, 128);

    }

}

#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Utils.h"


void launchParticles(int16_t x, int16_t y) {

    for (int i = 0; i < Constants::ParticlesMax; i++) {

        particles[i].setX(x);
        particles[i].setY(y);
        particles[i].setVelX(random(-4, 5));
        particles[i].setVelY(random(-1, 6));
        particles[i].setCounter(random(10, 46));
        particles[i].setSize(random(1, 3));
    
    }

}

void updateAndRenderParticles() {

    for ( int i = 0; i < Constants::ParticlesMax; i++) {

        particles[i].update();

        if ( particles[i].render() ) { // the dot should be rendered

            int pSize = particles[i].getSize();

            if (pSize == 1) {
                arduboy.drawPixel( particles[i].getX(), particles[i].getY(), 1);
            } 
            else {
                arduboy.drawRect( particles[i].getX(), particles[i].getY(), pSize, pSize, 1);
            }

        }

    }

}

#include <Arduboy2.h>

void renderBoard_Zoom() {


    int16_t worldX = 64 - (player.getX() * Constants::CellWidth_Zoom_PlusBorder) + player.getXOffset() - (Constants::CellWidth_Zoom_PlusBorder / 2) - player.getXOffset() + gameStats.xOffset;
    int16_t worldY = 32 - (player.getY() * Constants::CellHeight_Zoom_PlusBorder) + player.getYOffset() + (Constants::CellHeight_Zoom_PlusBorder / 2) - player.getYOffset();

    for (uint16_t y = 0; y < Constants::BoardHeight; y++){

        for (uint16_t x = 0; x < Constants::BoardWidth; x++){

            if (board[y][x] > static_cast<uint8_t>(Tiles::None)) {
                Sprites::drawExternalMask(worldX + (x * Constants::CellWidth_Zoom_PlusBorder), 
                                        worldY + (y * Constants::CellHeight_Zoom_PlusBorder), 
                                        Images::Tiles_Zoom, Images::Tile_Zoom_Mask, board[y][x] + (gameStats.tileSet * 7), 0);
            }

        }

    }


    // Render falling tiles ..
                
    for (FallingTile &fallingTile : fallingTiles) {

        if (fallingTile.isActive()) {

            Sprites::drawExternalMask(worldX + (fallingTile.getX() * Constants::CellWidth_Zoom_PlusBorder), 
                                      worldY + (fallingTile.getY() * Constants::CellHeight_Zoom_PlusBorder) + fallingTile.getYOffset(), 
                                      Images::Player_Zoom_Dying, Images::Player_Zoom_Dying_Mask, 0, 0);
            
        }

    }


    // Render arrows .. (if defined)

    for (Arrow &arrow : arrows) {

        if (arrow.getX() > 0 || arrow.getY() > 0) {
    
            Sprites::drawExternalMask(worldX + (arrow.getX() * Constants::CellWidth_Zoom_PlusBorder) - 2, 
                                      worldY + (arrow.getY() * Constants::CellHeight_Zoom_PlusBorder) + arrow.getYOffset() + 3, 
                                      Images::Arrow_Zoom, Images::Arrow_Zoom_Mask, 0, 0);


        }

    }

    // Render player ..

    if (gameStats.xOffset == 0) {

        uint8_t frame = arduboy.getFrameCount(32) / 8;

        if (player.isDying()) {

            if (player.isMoving()) {

                Sprites::drawExternalMask(worldX + (player.getX() * Constants::CellWidth_Zoom_PlusBorder) + player.getXOffset(), 
                                          worldY + (player.getY() * Constants::CellHeight_Zoom_PlusBorder) + player.getYOffset() - 1, 
                                          Images::Player_Zoom, Images::Player_Zoom_Mask, frame, frame);

            }
            else {

                Sprites::drawExternalMask(worldX + (player.getX() * Constants::CellWidth_Zoom_PlusBorder), 
                                          worldY + (player.getY() * Constants::CellHeight_Zoom_PlusBorder) + player.getYDyingOffset_1() - 1, 
                                          Images::Player_Zoom_Dying, Images::Player_Zoom_Dying_Mask, frame, frame);

            }
    
            Sprites::drawExternalMask(worldX + (player.getXOld() * Constants::CellWidth_Zoom_PlusBorder), 
                                      worldY + (player.getYOld() * Constants::CellHeight_Zoom_PlusBorder) + player.getYDyingOffset_2() - 1, 
                                      Images::Player_Zoom_Dying, Images::Player_Zoom_Dying_Mask, frame, frame);

        }
        else {

            Sprites::drawExternalMask(worldX + (player.getX() * Constants::CellWidth_Zoom_PlusBorder) + player.getXOffset(), 
                                      worldY + (player.getY() * Constants::CellHeight_Zoom_PlusBorder) + player.getYOffset() - 1, 
                                      Images::Player_Zoom, Images::Player_Zoom_Mask, frame, frame);

        }
        
    }

}


void renderBoard_Normal() {

    for (uint16_t y = 0; y < Constants::BoardHeight; y++){

        for (uint16_t x = 0; x < Constants::BoardWidth; x++){

            if (board[y][x] > static_cast<uint8_t>(Tiles::None)) {
                Sprites::drawExternalMask(Constants::Board_XOffset + gameStats.xOffset + (x * Constants::CellWidth_PlusBorder), 
                                        Constants::Board_YOffset + (y * Constants::CellHeight_PlusBorder) + gameStats.yOffset, 
                                        Images::Tiles, Images::Tile_Mask, board[y][x] + (gameStats.tileSet * 7), 0);
            }

        }

    }


    // Render falling tiles ..
                
    for (FallingTile &fallingTile : fallingTiles) {

        if (fallingTile.isActive()) {

            Sprites::drawExternalMask(Constants::Board_XOffset + (fallingTile.getX() * Constants::CellWidth_PlusBorder), 
                                      Constants::Board_YOffset + (fallingTile.getY() * Constants::CellHeight_PlusBorder) + fallingTile.getYOffset() + gameStats.yOffset, 
                                      Images::Player_Dying, Images::Player_Mask, 0, 0);
            
        }

    }


    // Render arrows .. (if defined)

    for (Arrow &arrow : arrows) {

        if (arrow.getX() > 0 || arrow.getY() > 0) {
    
            Sprites::drawExternalMask(Constants::Board_XOffset + gameStats.xOffset + (arrow.getX() * Constants::CellWidth_PlusBorder) - 1, 
                                    Constants::Board_YOffset + (arrow.getY() * Constants::CellHeight_PlusBorder) + arrow.getYOffset() + gameStats.yOffset - 1, 
                                    Images::Arrow, Images::Arrow_Mask, 0, 0);


        }

    }


    // Render player ..

    if (gameStats.xOffset == 0) {

        uint8_t frame = arduboy.getFrameCount(32) / 8;

        if (player.isDying()) {

            if (player.isMoving()) {

                Sprites::drawExternalMask(Constants::Board_XOffset + (player.getX() * Constants::CellWidth_PlusBorder) + player.getXOffset(), 
                                          Constants::Board_YOffset + (player.getY() * Constants::CellHeight_PlusBorder) + player.getYOffset() + gameStats.yOffset - 1, 
                                          Images::Player, Images::Player_Mask, frame, frame);

            }
            else {

                Sprites::drawExternalMask(Constants::Board_XOffset + (player.getX() * Constants::CellWidth_PlusBorder), 
                                          Constants::Board_YOffset + (player.getY() * Constants::CellHeight_PlusBorder) + player.getYDyingOffset_1() + gameStats.yOffset - 1, 
                                          Images::Player_Dying, Images::Player_Mask, frame, frame);

            }
    
            Sprites::drawExternalMask(Constants::Board_XOffset + (player.getXOld() * Constants::CellWidth_PlusBorder), 
                                      Constants::Board_YOffset + (player.getYOld() * Constants::CellHeight_PlusBorder) + player.getYDyingOffset_2() + gameStats.yOffset - 1, 
                                      Images::Player_Dying, Images::Player_Mask, frame, frame);

        }
        else {

            Sprites::drawExternalMask(Constants::Board_XOffset + (player.getX() * Constants::CellWidth_PlusBorder) + player.getXOffset(), 
                                      Constants::Board_YOffset + (player.getY() * Constants::CellHeight_PlusBorder) + player.getYOffset() + gameStats.yOffset - 1, 
                                      Images::Player, Images::Player_Mask, frame, frame);

        }
        
    }

}


void renderHUD() {

    arduboy.fillRect(0, 0, 128, 10, BLACK);

    Sprites::drawOverwrite(0, 0, Images::Level, 0);

    {
        uint8_t digits[3] = {};
        uint8_t x = 45;

        extractDigits(digits, gameStats.level);

        for (uint8_t i = 0; i < 3; ++i) {

            if (digits[i] == 1) x = x - 3;

        }

        for (uint8_t i = 0; i < 3; ++i) {
            Sprites::drawOverwrite(x, 0, Images::Numbers, digits[i]);
            x = x - (digits[i] == 1 ? 5 : 8);
        }

    }


    Sprites::drawOverwrite(71, 0, Images::Moves, 0);

    {
        uint8_t digits[3] = {};
        uint8_t x = 121;

        extractDigits(digits, gameStats.moves < 999 ? gameStats.moves : 999);

        for (uint8_t i = 0; i < 3; ++i) {

            if (digits[i] == 1) x = x - 3;

        }
        
        for (uint8_t i = 0; i < 3; ++i) {
            Sprites::drawOverwrite(x, 0, Images::Numbers, digits[i]);
            x = x - (digits[i] == 1 ? 5 : 8);
        }

    }

    arduboy.drawFastHLine(0, 8, 128, WHITE);


}
#include "src/utils/Arduboy2Ext.h"

static constexpr uint8_t ppotFrames = (1.6 *30); // 1.6s animation loop


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
#include <Arduboy2.h>

// ----------------------------------------------------------------------------
//  Initialise state ..
//
void title_Init() {

    uint8_t maxLevel = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Max));
    uint8_t curlevel = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Current));
    gameStats.tileSet = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Tile_Set)) == 1 ? 0 : 1;

    gameStats.titleSel = maxLevel > 0 ? 1 : 0;
    gameStats.maxLevel = maxLevel;
    gameStats.level = curlevel;
    
    gameState = GameState::Title;
    
}   


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void title() {


    // Handle player actions ..

    if (arduboy.justPressed(LEFT_BUTTON)) { 

        gameStats.titleSel = 0;

    }         

    if (arduboy.justPressed(RIGHT_BUTTON) && gameStats.maxLevel > 0) { 

        gameStats.titleSel = 1;

    }         

    if (arduboy.justPressed(A_BUTTON)) { 

        if (gameStats.titleSel == 0) { 
            gameStats.level = 0;
            gameStats.instruction = 0;
            gameState = GameState::Instructions_Init;
            eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Level_Current), gameStats.level);
        }
        else {
            gameState = GameState::LevelSelect_Init;
        }

    }         


    // Change tilest ..
    
    if (arduboy.pressed(B_BUTTON)) { 

        gameStats.exit++;

        if (gameStats.exit == 32) {

            gameStats.tileSet = gameStats.tileSet == 1 ? 0 : 1;
            eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Tile_Set), gameStats.tileSet);
            gameStats.exit = 0;

        }
    }
    else {

        gameStats.exit = 0;

    }


    // Render page ..

    Sprites::drawOverwrite(9, 9, Images::Title, gameStats.tileSet);
    Sprites::drawOverwrite(25, 49, Images::Title_New, 0);

    if (gameStats.maxLevel > 0) {
     
        Sprites::drawOverwrite(68, 49, Images::Title_Continue, 0);

    }

    uint8_t frame = arduboy.getFrameCount(32) / 8;
    Sprites::drawSelfMasked(14 + (gameStats.titleSel * 44), 50, Images::Player, frame);

}

