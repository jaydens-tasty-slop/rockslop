#include <Arduino.h>
#include <stdint.h>
#include "src/utils/Constants.h"
#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Utils.h"
#include "src/utils/Structs.h"
#include "src/utils/HighScoreEditor.h"
#include "src/entities/Entities.h"
#include "src/utils/EEPROM_Utils.h"
#include "src/images/Images.h"
#include <FixedPointsCommon.h>

#ifdef SOUNDS
#include <ArduboyTones.h>
#include "src/sounds/Sounds.h"
#endif

Arduboy2Ext arduboy;

#ifndef DEBUG
ARDUBOY_NO_USB
#endif

#ifdef SOUNDS
ArduboyTones sound(arduboy.audio.enabled);
#endif

Vec3 cameraPos;

#ifdef CACTII
Vec3 cactii[Constants::NumberOfCactii];
#endif

Car car;
OtherCar otherCars[Constants::NumberOfOtherCars];
World world;
GameState gameState = GameState::Splash_Init;
HighScore highScore;


// State variables -------------------------------------------------

HighScoreVars highScoreVars;
GamePlayVars gamePlayVars;
GameOverVars gameOverVars;
TitleVars titleVars;

/* Arduino IDE auto-prototypes */
void moveCactii();
void placeCactii();
void gameOver_Init();
void gameOver();
void HighScore_Init();
void renderHighScore(HighScore &highScore);
void renderChar(uint8_t xOffset, uint8_t yOffset, uint8_t charIndex, HighScore &highScore);
void game_Init();
void game();
void draw(bool drawOtherCars, bool noCurves);
void renderHud();
void renderPlayerCar();
void drawFastDottedLine(int16_t startPos, int16_t endPos, int16_t y, bool &color);
void drawFastSolidLine(int16_t startPos, int16_t endPos, int16_t y, bool color);
void renderRoadEdge_Black(int16_t &startPos, int16_t endPos, int16_t y, bool &color);
void renderRoadEdge_Dark(int16_t &startPos, int16_t endPos, int16_t y, bool &color);
void renderRoadEdge_Light(int16_t &startPos, int16_t endPos, int16_t y);
void renderRoad_Black(int16_t &startPos, int16_t endPos, int16_t y, bool &color);
void renderRoad_Dither_Dark(int16_t &startPos, int16_t endPos, int16_t y, bool &dither, bool &color);
void renderRoad_Dither_Light(int16_t &startPos, int16_t endPos, int16_t y, bool &color);
void renderDayBanner();
void renderGameOver();
void renderLights();
void splashScreen_Init();
void splashScreen();
void title_Init();
void title();
void swap(OtherCar *xp, OtherCar *yp);
void sort(OtherCar arr[], uint8_t n);
int8_t moveCar(int16_t xMovement, int16_t zMovement);
void moveOtherCars();
void houseKeeping();
bool nudgeCarForward(uint8_t carIdx);

void setup() {

    arduboy.begin();
    arduboy.setFrameRate(30);

    #ifdef SOUNDS
    arduboy.audio.begin();
    #endif

    EEPROM_Utils::initEEPROM(false);

}

void loop() {

    if (!arduboy.nextFrame()) return;    
	arduboy.pollButtons();

    switch (gameState) {

        case GameState::Splash_Init:

            splashScreen_Init();
            splashScreen();
            arduboy.display(true);
            break;

        case GameState::Splash:

            splashScreen();
            arduboy.display(true);
            break;

        case GameState::Title_Init:

            title_Init();
            title();
            arduboy.display(true);
            break;

        case GameState::Title:

            title();
            arduboy.displayClearToPattern(gamePlayVars.getTimeOfDay_Pattern());
            break;

        case GameState::Game_Init:

            game_Init();
            game();
            arduboy.display(true);
            break;

        case GameState::Game:
            game();
            arduboy.displayClearToPattern(gamePlayVars.getTimeOfDay_Pattern());
            break;

        case GameState::GameOver_Init:

            gameOver_Init();
            gameOver();
            arduboy.displayClearToPattern(gamePlayVars.getTimeOfDay_Pattern());
            break;            

        case GameState::GameOver:

            gameOver();
            arduboy.displayClearToPattern(gamePlayVars.getTimeOfDay_Pattern());
            break;

        case GameState::SaveScore:

            highScore.reset();
            highScore.setSlotNumber(EEPROM_Utils::saveScore(gamePlayVars.distance));
            gameState = GameState::HighScore_Init;
            [[fallthrough]];

        case GameState::HighScore_Init:

            HighScore_Init();
            [[fallthrough]];

        case GameState::HighScore:

            renderHighScore(highScore);
            arduboy.display(true);
            break;

    }

}


#include <Arduboy2.h>

// ----------------------------------------------------------------------------
//  Initialise state ..
//
void gameOver_Init() {

    gameState = GameState::GameOver;
    
}   


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void gameOver() {

    draw(false, true);

    renderPlayerCar();
    renderHud();
    renderGameOver();


    if (arduboy.justPressed(A_BUTTON)) { 

        gameState = GameState::SaveScore;

    }

    houseKeeping();

    #ifdef CACTII
        moveCactii();
    #endif
    
    cameraPos.setZ(cameraPos.getZ() + gameOverVars.finalSpeed);
    cameraPos.setY(world.roadHeightAt(cameraPos.getZ() + 2 * UPM) + UPM);

    gamePlayVars.timeOfDay = 700;

}

#include "src/utils/Arduboy2Ext.h"

uint8_t clearScores = 0;


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void HighScore_Init() {

    cameraPos.setX(0);
    cameraPos.setY(UPM);
    cameraPos.setZ(-40);

    world.reset();
    gamePlayVars.reset();
    gameState = GameState::HighScore;

    #ifdef CACTII
        placeCactii();
    #endif
    
}   


// --------------------------------------------------------------------------------------
//  Render High Score screen ..
//
void renderHighScore(HighScore &highScore) {

    uint8_t xOffset = 35;
    uint8_t yOffset = 18;

    draw(false, true);

    arduboy.fillRect(23, 0, 82, 64, BLACK);
    arduboy.drawFastVLine(24, 16, 47, WHITE);
    arduboy.drawFastVLine(103, 16, 47, WHITE);
    arduboy.drawFastHLine(25, 63, 78, WHITE);

    Sprites::drawOverwrite(16, -2, Images::HighScore, 0);

    for (uint8_t x =0; x < Constants::MaxNumberOfScores; x++) {

        Slot slot = EEPROM_Utils::getSlot(x);

        Sprites::drawOverwrite(xOffset, yOffset, Images::Font4x6, slot.getChar0());
        Sprites::drawOverwrite(xOffset + 6, yOffset, Images::Font4x6, slot.getChar1());
        Sprites::drawOverwrite(xOffset + 12, yOffset, Images::Font4x6, slot.getChar2());


        // Score ..
        {
            uint8_t digits[6] = {};
            extractDigits(digits, slot.getScore());

            for (uint8_t i = 0, x2 = xOffset + 54; i < 6; ++i, x2 -= 5) {
                Sprites::drawOverwrite(x2, yOffset, Images::Font4x6, (digits[i] + 26));
            }

        }

        yOffset = yOffset + 9;

    }


    if (highScore.getSlotNumber() != Constants::DoNotEditSlot) {

        yOffset = 18;
        highScoreVars.alternate++;

        if (highScoreVars.alternate < 12) {

            Sprites::drawOverwrite(xOffset - 5, yOffset + (highScore.getSlotNumber() * 9) - 1, Images::Arrow, 0);
            Sprites::drawOverwrite(xOffset + 62, yOffset + (highScore.getSlotNumber() * 9) - 1, Images::Arrow, 0);

        }
        else if (highScoreVars.alternate > 24) {

            highScoreVars.alternate = 0;

        }

        if (highScore.getCharIndex() == 0) {
            renderChar(xOffset, yOffset, 0, highScore);
        }

        if (highScore.getCharIndex() == 1) {
            renderChar(xOffset, yOffset, 1, highScore);
        }

        if (highScore.getCharIndex() == 2) {
            renderChar(xOffset, yOffset, 2, highScore);
        }


        // Handle buttons ..

        uint8_t charIndex = highScore.getCharIndex();

        if (arduboy.justPressed(UP_BUTTON))       { highScore.incChar(charIndex); }
        if (arduboy.justPressed(DOWN_BUTTON))     { highScore.decChar(charIndex); }
        if (arduboy.justPressed(LEFT_BUTTON))     { highScore.decCharIndex(); } 
        if (arduboy.justPressed(RIGHT_BUTTON))    { highScore.incCharIndex(); } 

        if (arduboy.justPressed(A_BUTTON)) { 

            EEPROM_Utils::writeChars(highScore.getSlotNumber(), highScore);
            highScore.disableEditting(); 

        }

    }
    else {


        // Clear scores ..

        if (arduboy.pressed(LEFT_BUTTON) && arduboy.pressed(RIGHT_BUTTON)) {

            clearScores++;

            switch (clearScores) {

                case 21 ... 60:
                    arduboy.setRGBled(128 - (clearScores * 2), 0, 0);
                    break;

                case 61:
                    clearScores = 0;
                    arduboy.setRGBled(0, 0, 0);
                    EEPROM_Utils::initEEPROM(true);
                    //(NOTE_C6, 100);
                    return;

            }

        }
        else {

            if (clearScores > 0) {

                arduboy.setRGBled(0, 0, 0);
                clearScores = 0;

            }

        }

        if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) { 

            arduboy.setRGBled(0, 0, 0);
            gameState = GameState::Title_Init; 

        }

    }


    houseKeeping();

    #ifdef CACTII
        moveCactii();
    #endif
    
    cameraPos.setZ(cameraPos.getZ() + 6);
    cameraPos.setY(world.roadHeightAt(cameraPos.getZ() + 2 * UPM) + UPM);
    gamePlayVars.timeOfDay = 1700;

}

void renderChar(uint8_t xOffset, uint8_t yOffset, uint8_t charIndex, HighScore &highScore) {

    for (uint8_t x = 0; x < 2; x++) {
        Sprites::drawOverwrite(xOffset + (x * 6), yOffset + (highScore.getSlotNumber() * 9), Images::Font4x6, highScore.getChar(x));
    }

    arduboy.fillRect(xOffset + (charIndex * 6) - 1, yOffset + (highScore.getSlotNumber() * 9) - 1, 6, 8, WHITE);
    Sprites::drawErase(xOffset + (charIndex * 6), yOffset + (highScore.getSlotNumber() * 9), Images::Font4x6, highScore.getChar(charIndex));

}
#include <stdint.h>


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void game_Init() {

    #ifdef SOUNDS

        sound.noTone();

    #endif

    world.reset();
    car.reset();
    gamePlayVars.reset();
    gameOverVars.reset();

    cameraPos.setX(0);
    cameraPos.setY(UPM);
    cameraPos.setZ(-40);

    for (uint8_t i = 0; i < gamePlayVars.numberOfOtherCars; i++) {

        OtherCar &otherCar = otherCars[i];

        otherCar.setX(0);
        otherCar.setZ(400 + (i * 200));
        otherCar.setSpeed(random(7, 10) * Constants::SpeedDiv);

    }

    #ifdef CACTII
        placeCactii();
    #endif

    gameState = GameState::Game;

    #ifdef DEBUG_COLLISIONS
        gamePlayVars.showDayBannerCount = 1;
    #endif
}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void game() {

    #ifdef SOUNDS

        if (car.getGear() > 0 && !sound.playing()) {

            sound.tones(Sounds::Drive[car.getTacho()]);
        }

    #endif


    switch (car.getTransmissionType()) {
        
        case TransmissionType::Manual:

            if (arduboy.justPressed(UP_BUTTON)) {

                if (gamePlayVars.showDayBannerCount < 59 || gamePlayVars.days > 1) {
                    car.incGear();
                }
                else {

                    if (gamePlayVars.brakeCount == 0) {

                        gamePlayVars.brakeCount = Constants::BrakeCloud_DownGear;
                        gamePlayVars.brakeSide = Direction::Both;

                    }

                }

            }

            if (arduboy.justPressed(DOWN_BUTTON)) {

                bool brakes = car.decGear();

                if (brakes) {

                    gamePlayVars.brakeCount = Constants::BrakeCloud_DownGear;
                    gamePlayVars.brakeSide = Direction::Both;
                    
                }

            }

            break;

        case TransmissionType::Auto:
        
            if (car.getTacho() == 1 && car.getGear() > 1) {

                car.decGear();

            }
            
            if (!car.getOffroad()) {

                if (car.getTacho() == 8 && car.getGear() > 0) {

                    if (gamePlayVars.showDayBannerCount < 59 || gamePlayVars.days > 1) {
                        car.incGear();
                    }
                    else {

                        if (gamePlayVars.brakeCount == 0) {

                            gamePlayVars.brakeCount = Constants::BrakeCloud_DownGear;
                            gamePlayVars.brakeSide = Direction::Both;

                        }

                    }

                }

            }

            break;

    }


    // Render screen ..

    draw(true, false);
    renderPlayerCar();
    renderHud();

    if (gamePlayVars.days > 1) {
        renderDayBanner();
    }
    else {
       renderLights();
    }
    

    // Do some house keeping ..

    houseKeeping();

    #ifdef CACTII
        moveCactii();
    #endif
    

    // When moving left or right, the speed must be > 0 .. 
 
    uint8_t speed = car.getSpeed_Display();

    if (gamePlayVars.showDayBannerCount > 59 && gamePlayVars.days == 1) {

        speed = 0;


    }
    uint8_t speedForSteering = Constants::SpeedSteering[speed];


    // Steering ..
    
    Vec3 segClosest = world.getRoadSegment(0, car.getZ());
    Vec3 segNext = world.getRoadSegment(car.getZ(), car.getZ() + UPM);

    int16_t carOffsetOnRoad = car.getX() - ((segClosest.getX() + segNext.getX()) / 2);

    if (arduboy.pressed(LEFT_BUTTON) && carOffsetOnRoad > -300) {

        #ifdef DEBUG_COLLISIONS
            Serial.print("Left - ");
        #endif

        if (moveCar(-speedForSteering, 0) == Constants::NoCollision) {
            cameraPos.setX(cameraPos.getX() - speedForSteering);
            car.setX(car.getX() - speedForSteering);
        }

    }

    if (arduboy.pressed(RIGHT_BUTTON) && carOffsetOnRoad < 300) {

        #ifdef DEBUG_COLLISIONS
            Serial.print("Right -");
        #endif

        if (moveCar(speedForSteering, 0) == Constants::NoCollision) {
            cameraPos.setX(cameraPos.getX() + speedForSteering);
            car.setX(car.getX() + speedForSteering);
        }

    }



    if (speed > 0 && gamePlayVars.brakeCount == 0) {

        switch (carOffsetOnRoad) {

            case -1000 ... -191:

                gamePlayVars.brakeCount = Constants::BrakeCloud_OffRoad;
                gamePlayVars.brakeSide = Direction::Both;
                car.setOffroad(true);
                break;

            case -190 ... -130:

                gamePlayVars.brakeCount = Constants::BrakeCloud_OffRoad;
                gamePlayVars.brakeSide = Direction::Left;
                car.setOffroad(true);
                break;

            case 130 ... 190:

                gamePlayVars.brakeCount = Constants::BrakeCloud_OffRoad;
                gamePlayVars.brakeSide = Direction::Right;
                car.setOffroad(true);
                break;

            case 191 ... 1000:

                gamePlayVars.brakeCount = Constants::BrakeCloud_OffRoad;
                gamePlayVars.brakeSide = Direction::Both;
                car.setOffroad(true);
                break;

            default:
                car.setOffroad(false);
                break;

        }

    }


    int8_t collide = Constants::NoCollision;

    if (arduboy.isFrameCount(4)) {

        uint16_t tacho = car.getTacho();

        collide = moveCar(0, speed);

        CarMovement carMovement = CarMovement::NoMovement;

        if (collide == Constants::NoCollision) {

            if (!car.getOffroad() || car.getGear() == 1) {

                if (arduboy.pressed(A_BUTTON) && ((car.getGear() == 1 || tacho > 1) && tacho < 8)) {


                    // If accelerating from a stand still, render the dirt cloud ..

                    if (car.getSpeed().getInteger() == 0) {
                        gamePlayVars.brakeCount = Constants::BrakeCloud_Accelerate;
                        gamePlayVars.brakeSide = Direction::Both;       
                    }

                    carMovement = CarMovement::Accelerate;

                }
                else if (arduboy.pressed(A_BUTTON) && (tacho == 8 || (car.getGear() > 1 && tacho == 1))) {

                    carMovement = CarMovement::NoMovement;

                }
                else {

                    carMovement = CarMovement::Coast;

                }

            }
            else {

                carMovement = CarMovement::Coast;

            }

            if (arduboy.notPressed(A_BUTTON) && arduboy.pressed(B_BUTTON)) {

                carMovement = CarMovement::Decelerate;
                
            }

            car.changeSpeed(carMovement);

        }
        else {

            if (collide >= 0) {

                OtherCar &otherCar = otherCars[collide];

                if (arduboy.pressed(A_BUTTON)) {

                    car.setSpeed(otherCar.getSpeed());
                    otherCar.setZ(car.getZ() + Constants::PlayerCarLengthUnits);

                    #ifdef DEBUG_COLLISIONS
                        Serial.println("set speed 1");
                    #endif
                }
                else {

                    if (!car.getOffroad()) {
                        car.changeSpeed(CarMovement::NoMovement);
                    }
                    else{
                        car.changeSpeed(CarMovement::Coast);
                    }

                }

            }

        }

    }
    else {

        collide = moveCar(0, speed);

        if (collide != Constants::NoCollision) {
                    
                OtherCar &otherCar = otherCars[abs(collide)];

                if (otherCar.getZ() > car.getZ()) {

                    car.setSpeed(otherCar.getSpeed());


                    if (collide >= 0) {

                        otherCar.setZ(car.getZ() + Constants::PlayerCarLengthUnits);
                        #ifdef DEBUG_COLLISIONS
                            Serial.println("set speed 2");
                        #endif

                    }

                }

            

        }

    }
    

    // Now, after adjusting the car's speed for collisions, move the car forward ..

    if ((gamePlayVars.showDayBannerCount < 59 && gamePlayVars.days == 1) || gamePlayVars.days > 1) {

        speed = car.getSpeed_Display();

        car.setZ(car.getZ() + speed);
        cameraPos.setZ(cameraPos.getZ() + speed);
        cameraPos.setY(world.roadHeightAt(cameraPos.getZ() + 2 * UPM) + UPM);

        moveOtherCars();

    }

}


// ----------------------------------------------------------------------------
// Render the screen .. 
//
void draw(bool drawOtherCars, bool noCurves) {
    
    Vec3 startPos, endPos;

    uint16_t indexFrom = cameraPos.getZ() / UPM;
    uint16_t indexTo = indexFrom + DRAW_DISTANCE; 


    // draw road segments starting with the backmost one first ..
    // IndexTo is the last position and is an ever increasing value.

    if (gamePlayVars.oldIndexTo != indexTo + 5) {
        world.addRoadSegment(indexTo + 5, noCurves);
        gamePlayVars.oldIndexTo = indexTo + 5;
    }

    for (uint16_t i = indexTo; i > indexFrom; --i) {

        Vec3 startVec = world.getRoadSegment(i);
        Vec3 endVec = world.getRoadSegment(i + 1);

        startPos = world.perspective(startVec, cameraPos);
        startPos.setY(startPos.getY() - (((i -1) - indexFrom) ));
        endPos   = world.perspective(endVec, cameraPos);
        endPos.setY(endPos.getY() - ((i - indexFrom) ));

        int16_t x1Bottom = startPos.getX() - startPos.getZ() * 1.25;
        int16_t x2Bottom = x1Bottom + startPos.getZ() * 2.5;
        int16_t x1Top = endPos.getX() - endPos.getZ() * 1.25;
        int16_t x2Top = x1Top + endPos.getZ() * 2.5;

        int16_t x1Diff = x1Top - x1Bottom;
        int16_t x2Diff = x2Top - x2Bottom;


        // Draw Horizon ..

        if (i == indexTo) {

            Vec3 frontMost = world.getRoadSegment(indexFrom);
            gamePlayVars.horizonX = 64 + frontMost.getX() / 4;

            uint8_t idx = gamePlayVars.getTimeOfDay() == TimeOfDay::Day ? 0 : 1;

            for (int16_t i = gamePlayVars.horizonX - 256; i < WIDTH; i = i + 128) {

                Sprites::drawExternalMask(i, endPos.getY() - 9, Images::Horizons, Images::Horizons_Mask, idx, idx);

            }

        }


        bool odd = i % 2;

        // if visible (ie leaning towards camera) ..

        if (startPos.getY() > endPos.getY() && startPos.getZ() > 0 && endPos.getZ() > 0) {

            int16_t segmentHeight = startPos.getY() - endPos.getY();
            int16_t y = startPos.getY();

            for (int16_t j = 0; j < segmentHeight; ++j) {

                int16_t pos1 = x1Bottom + (j * x1Diff) / segmentHeight;
                int16_t pos2 = x2Bottom + (j * x2Diff) / segmentHeight;

                if (y >= DISPLAY_HEIGHT) {
                    y--;
                    continue;
                }

                pos1 = max(0, pos1);
                pos2 = min(DISPLAY_WIDTH, pos2);

                bool dither = j % 2;
                bool color = y % 2;

                // Odd line
                if (odd) {


                    int16_t k = 0;

                    switch (gamePlayVars.getTimeOfDay()) {

                        case TimeOfDay::Day:
                            renderRoadEdge_Light(k, pos1, y);
                            renderRoad_Dither_Light(k, pos2, y, color);
                            renderRoadEdge_Light(k, DISPLAY_WIDTH, y);
                            break;

                        case TimeOfDay::Dawn:
                            renderRoadEdge_Black(k, pos1, y, color);
                            renderRoad_Dither_Light(k, pos2, y, color);
                            renderRoadEdge_Black(k, DISPLAY_WIDTH, y, color);
                            break;

                        case TimeOfDay::Night:
                            color = y % 2;
                            renderRoadEdge_Dark(k, pos1, y, color);                        
                            renderRoad_Black(k, pos2, y, color);
                            renderRoadEdge_Dark(k, DISPLAY_WIDTH, y, color);
                            break;

                    }

                }

                // Even line
                else {
                    
                    int16_t k = 0;
                    
                    switch (gamePlayVars.getTimeOfDay()) {

                        case TimeOfDay::Day:
                            color = y % 2;
                            renderRoadEdge_Dark(k, pos1, y, color);
                            dither = j % 2;
                            renderRoad_Dither_Dark(k, pos2, y, dither, color);
                            renderRoadEdge_Dark(k, DISPLAY_WIDTH, y, color);
                            break;

                        case TimeOfDay::Dawn:
                            color = y % 2;
                            renderRoadEdge_Dark(k, pos1, y, color);
                            dither = j % 2;
                            renderRoad_Dither_Dark(k, pos2, y, dither, color);
                            renderRoadEdge_Dark(k, DISPLAY_WIDTH, y, color);
                            break;

                        case TimeOfDay::Night:
                            renderRoadEdge_Black(k, pos1, y, color);
                            dither = j % 2;
                            renderRoad_Dither_Dark(k, pos2, y, dither, color);
                            renderRoadEdge_Black(k, DISPLAY_WIDTH, y, color);
                            break;

                    }

                }

                y--;
            }


            // Draw lines ..

            if (!odd) {

                int16_t pos1Top = x1Bottom + x1Diff;
                int16_t pos2Top = x2Bottom + x2Diff;

                int16_t pBottom = (x2Bottom - x1Bottom) / 6; 
                int16_t pTop = (pos2Top - pos1Top) / 6;

                int16_t centreTop = (x1Top + x2Top) / 2;
                int16_t centreBottom = (x1Bottom + x2Bottom) / 2;

                arduboy.drawLine(centreBottom - pBottom,startPos.getY(),centreTop - pTop,endPos.getY(),WHITE);
                arduboy.drawLine(centreBottom + pBottom,startPos.getY(),centreTop + pTop,endPos.getY(),WHITE);

            }

        }


    
        if (drawOtherCars) {


            // draw other cars ..


            for (uint8_t j = 0; j < gamePlayVars.numberOfOtherCars; j++) {

                OtherCar &otherCar = otherCars[j];


                // If the other car is within the road segment being rendered then render it ..

                if (otherCar.getZ() > startVec.getZ() && otherCar.getZ() <= endVec.getZ()) {


                    // Render cars based on time of day ..
                    
                    uint8_t imageIndex = gamePlayVars.getTimeOfDay() == TimeOfDay::Night ? 1 : 0;


                    
                    // Add the average road segement x value to the car ..

                    otherCar.setXWorld((startVec.getX() + endVec.getX()) / 2);
                    otherCar.setY(world.roadHeightAt(otherCar.getZ()));


                    #ifndef DEBUG_COLLISIONS

                        Vec3 otherCarNonRef = otherCar.clone();
                        otherCarNonRef.setX(otherCar.getX() + otherCar.getXWorld());

                        Vec3 carPerspective = world.perspective(otherCarNonRef, cameraPos);
                        carPerspective.setY(carPerspective.getY() - (i - indexFrom));
                        int16_t distToCar = otherCar.getZ() - cameraPos.getZ();


                        // Only render the cars if they are visible ..

                        uint8_t index = 0;

                        if (distToCar > -50 && distToCar < 650) {
                            
                            switch (distToCar) {

                                case 460 ... 650: // 100
                                    index = 5;
                                    break;

                                case 360 ... 459: // 90
                                    index = 4;
                                    break;

                                case 290 ... 359: // 90
                                    index = 3;
                                    break;

                                case 220 ... 289: // 80
                                    index = 2;
                                    break;

                                case 180 ... 219: // 50
                                    index = 1;
                                    break;

                                default:
                                    index = 0;
                                    break;

                            }
                    
                            Sprites::drawPlusMask(carPerspective.getX() - (pgm_read_byte(&Images::OtherCar_Width[index]) / 2), carPerspective.getY() - pgm_read_byte(&Images::OtherCar_Height[index]) + 1, Images::OtherCar[index], imageIndex);

                        }

                    #else

                        Vec3 leftTopCorner = otherCar.clone();
                        leftTopCorner.setX(leftTopCorner.getX() + otherCar.getXWorld() - (Constants::OtherCarWidthUnits / 2));
                        leftTopCorner.setZ(leftTopCorner.getZ() + Constants::OtherCarLengthUnits);

                        Vec3 rightTopCorner = otherCar.clone();
                        rightTopCorner.setX(rightTopCorner.getX() + otherCar.getXWorld() + (Constants::OtherCarWidthUnits / 2));
                        rightTopCorner.setZ(rightTopCorner.getZ() + Constants::OtherCarLengthUnits);
                        
                        Vec3 leftBottomCorner = otherCar.clone();
                        leftBottomCorner.setX(leftBottomCorner.getX() + otherCar.getXWorld() - (Constants::OtherCarWidthUnits / 2));

                        Vec3 rightBottomCorner = otherCar.clone();
                        rightBottomCorner.setX(rightBottomCorner.getX() + otherCar.getXWorld() + (Constants::OtherCarWidthUnits / 2));
                        
                        Vec3 leftTopCornerPersepective = world.perspective(leftTopCorner, cameraPos);
                        Vec3 rightTopCornerPersepective = world.perspective(rightTopCorner, cameraPos);
                        Vec3 leftBottomCornerPersepective = world.perspective(leftBottomCorner, cameraPos);
                        Vec3 rightBottomCornerPersepective = world.perspective(rightBottomCorner, cameraPos);

                        arduboy.drawLine(leftTopCornerPersepective.getX(), leftTopCornerPersepective.getY(), rightTopCornerPersepective.getX(), rightTopCornerPersepective.getY());
                        arduboy.drawLine(rightTopCornerPersepective.getX(), rightTopCornerPersepective.getY(), rightBottomCornerPersepective.getX(), rightBottomCornerPersepective.getY());
                        arduboy.drawLine(rightBottomCornerPersepective.getX(), rightBottomCornerPersepective.getY(), leftBottomCornerPersepective.getX(), leftBottomCornerPersepective.getY());
                        arduboy.drawLine(leftBottomCornerPersepective.getX(), leftBottomCornerPersepective.getY(), leftTopCornerPersepective.getX(), leftTopCornerPersepective.getY());

                    #endif

                }

            }

        }


        // Draw Cactii

        #ifdef CACTII

            for (uint8_t j = 0; j < Constants::NumberOfCactii; j++) {

                Vec3 &cactiiPos = cactii[j];

                if (cactiiPos.getZ() > startVec.getZ() && cactiiPos.getZ() <= endVec.getZ()) {

                    uint8_t cactiiIdx = 0;

                    if (i > indexFrom + 2 && i < indexFrom + 4) {
                        cactiiIdx = 4;
                    }

                    if (i >= indexFrom + 4 && i < indexFrom + 6) {
                        cactiiIdx = 3;
                    }

                    if (i >= indexFrom + 6 && i < indexFrom + 8) {
                        cactiiIdx = 2;
                    }

                    if (i >= indexFrom + 8 && i < indexFrom + 10) {
                        cactiiIdx = 1;
                    }

                    if (i >= indexFrom + 10) {
                        cactiiIdx = 0;
                    }

                    if (cactiiPos.getX() == 0) {

                        Sprites::drawPlusMask(x1Bottom - 12 - (pgm_read_byte(&Images::Cactii_Width[cactiiIdx]) / 2), startPos.getY() - pgm_read_byte(&Images::Cactii_Height[cactiiIdx]), Images::Cactii[cactiiIdx], 0);

                    }
                    else {

                        Sprites::drawPlusMask(x2Bottom + 13 - (pgm_read_byte(&Images::Cactii_Width[cactiiIdx]) / 2), startPos.getY() - pgm_read_byte(&Images::Cactii_Height[cactiiIdx]), Images::Cactii[cactiiIdx], 0);

                    }

                }

            }

        #endif


        // Sign posts / rocks ..

        if (odd && i > indexFrom + 2) {

            #ifdef ROCKS
                
                uint8_t rockIdx = 0;

                if (i > indexFrom + 2 && i < indexFrom + 4) {
                    rockIdx = 3;
                }

                if (i >= indexFrom + 4 && i < indexFrom + 6) {
                    rockIdx = 2;
                }

                if (i >= indexFrom + 6 && i < indexFrom + 8) {
                    rockIdx = 1;
                }

                if (i >= indexFrom + 8) {
                    rockIdx = 0;
                }

                int16_t spriteSize = Images::Rocks_Width[rockIdx];

                Sprites::drawPlusMask(x1Bottom - spriteSize, startPos.getY() - spriteSize, Images::Rocks[rockIdx], 0);
                Sprites::drawPlusMask(x2Bottom , startPos.getY() - spriteSize, Images::Rocks[rockIdx], 0);
                
            #endif

        }

    }
  
}

#include <Arduboy2.h>

void renderHud() {


    // Gearbox ..

    switch (car.getTransmissionType()) {

        case TransmissionType::Manual:
            {
                uint8_t frame = arduboy.getFrameCount(8) / 2;
                Sprites::drawPlusMask(5, 1, Images::Gearbox, 0);

                switch (car.getGear()) {

                    case 0:
                        Sprites::drawPlusMask(11 + ((frame % 3) - 1), 7 + ((frame % 3) - 1), Images::Gearbox_Knob, 0);
                        break;

                    default: // Gears 1 ... 5
                        Sprites::drawPlusMask(pgm_read_byte(&Constants::GearboxX[car.getGear()]), (car.getGear() % 2) ? 1 : 13, Images::Gearbox_Knob, 0);
                        break;
                }
            }
            break;

        default: // TransmissionType::Auto
        
            Sprites::drawPlusMask(1, 0, Images::AutoUpper, 0); // Upper half of automatic gears (masked)
            Sprites::drawOverwrite(1, 8, Images::AutoLower, 0);// Lower half of automatic gears
            Sprites::drawOverwrite(18, 6, Images::AutoNumbers, car.getGear() - 1);
            break;

    }


    // Render Tacho ..

    {
        uint8_t tacho = car.getTacho();

        Sprites::drawOverwrite(28, 1, Images::Tacho, 0); // Render Tachometer gauge
        arduboy.drawFastHLine(28, 9, 27, BLACK); // Lower horizontal line of frame

        if (tacho == 1) {
            // Flash a solid bar to warn when under revving
            if (arduboy.getFrameCountHalf(32)) {
                arduboy.drawRect(30, 3, 2, 5, BLACK);
            }

        }
        else {

            for (uint8_t i = 0, x = 30; i < tacho; i++, x = x + 3) {
                switch (i) {
                    case 0 ... 5:
                        arduboy.drawFastVLine(x, 3, 5, BLACK); // Render low revs line
                        break;
                    default:
                        arduboy.drawRect(x, 3, 2, 5, BLACK); // Render high revs bar
                        break;
                }

            } 

        }

    }


    // Render distance travelled (score)..

    {
        uint8_t digits[5] = {};
        extractDigits(digits, gamePlayVars.distance / 10);
        
        for (uint8_t i = 0, x2 = 84 ; i < 5; ++i, x2 -= 6) {
            Sprites::drawOverwrite(x2, 1, Images::numbers_white, digits[i]);
        }

        Sprites::drawOverwrite(90, 1, Images::numbers_black, gamePlayVars.distance % 10);
        arduboy.drawFastHLine(60, 9, 37, BLACK); // Lower horizontal line of frame

        if (gamePlayVars.getTimeOfDay() != TimeOfDay::Day) {
            arduboy.drawFastVLine(96, 2, 7, WHITE); // Highlight right vertical line of frame
        }

    }



    // Render car counter ..

    Sprites::drawPlusMask(104, 1, Images::CarCounter, 0);

    #ifdef SOUNDS

        if (gamePlayVars.carsPassed > 0 && (gamePlayVars.timeOfDay > 3000) && arduboy.getFrameCount(32) == 0) {

            sound.tones(Sounds::Alert);

        }

    #endif

    if ((gamePlayVars.carsPassed > 0 && gamePlayVars.timeOfDay > 3000 && arduboy.getFrameCountHalf(32)) || gamePlayVars.timeOfDay <= 3000) {

		uint8_t digits[3] = {};
		extractDigits(digits, gamePlayVars.carsPassed);
		
		for (uint8_t i = 0, x2 = 118 ; i < 3; ++i, x2 -= 6) {
			Sprites::drawOverwrite(x2, 3, Images::numbers_white, digits[i]);
		}

	}

}

void renderPlayerCar() {


    // Calculate an index for the wheel graphic using the speed ..

    uint8_t index = 0;
    uint8_t carSpeed = car.getSpeed_Display();

    switch (carSpeed) {

        case 0 ... 1:
            index = 1;
            break;

        case 2 ... 14:
            index = 9 - (carSpeed / 2);
            break;

        default:
            index = 2;
            break;

    }

    index = arduboy.getFrameCountHalf(index);
  
    
    // Set car X to camera X (as car is always in the middle of screen) ..

    car.setX(cameraPos.getX());
    car.setY(world.roadHeightAt(car.getZ()));



    // Render the car ..

    #ifndef DEBUG_COLLISIONS

        Sprites::drawPlusMask(46, 64 - Constants::PlayerCarHeightPX, Images::Player, index);

    #else

        Vec3 leftTopCorner = car.clone();
        leftTopCorner.setX(leftTopCorner.getX() - (Constants::PlayerCarWidthUnits / 2));
        leftTopCorner.setZ(leftTopCorner.getZ() + Constants::PlayerCarLengthUnits);

        Vec3 rightTopCorner = car.clone();
        rightTopCorner.setX(rightTopCorner.getX() + (Constants::PlayerCarWidthUnits / 2));
        rightTopCorner.setZ(rightTopCorner.getZ() + Constants::PlayerCarLengthUnits);
        
        Vec3 leftBottomCorner = car.clone();
        leftBottomCorner.setX(leftBottomCorner.getX() - (Constants::PlayerCarWidthUnits / 2));

        Vec3 rightBottomCorner = car.clone();
        rightBottomCorner.setX(rightBottomCorner.getX() + (Constants::PlayerCarWidthUnits / 2));
        
        Vec3 leftTopCornerPersepective = world.perspective(leftTopCorner, cameraPos);
        Vec3 rightTopCornerPersepective = world.perspective(rightTopCorner, cameraPos);
        Vec3 leftBottomCornerPersepective = world.perspective(leftBottomCorner, cameraPos);
        Vec3 rightBottomCornerPersepective = world.perspective(rightBottomCorner, cameraPos);

        arduboy.drawLine(leftTopCornerPersepective.getX(), leftTopCornerPersepective.getY(), rightTopCornerPersepective.getX(), rightTopCornerPersepective.getY());
        arduboy.drawLine(rightTopCornerPersepective.getX(), rightTopCornerPersepective.getY(), rightBottomCornerPersepective.getX(), rightBottomCornerPersepective.getY());
        arduboy.drawLine(rightBottomCornerPersepective.getX(), rightBottomCornerPersepective.getY(), leftBottomCornerPersepective.getX(), leftBottomCornerPersepective.getY());
        arduboy.drawLine(leftBottomCornerPersepective.getX(), leftBottomCornerPersepective.getY(), leftTopCornerPersepective.getX(), leftTopCornerPersepective.getY());

    #endif


    // Render brake cloud if braking ..

    if (gamePlayVars.brakeCount > 0) {

        gamePlayVars.brakeCount--;

        uint8_t idx = (gamePlayVars.brakeCount / 2) % 3;

        if (gamePlayVars.brakeSide != Direction::Right)     Sprites::drawPlusMask(40, 58, Images::Dirt_Cloud_Left, idx);
        if (gamePlayVars.brakeSide != Direction::Left)      Sprites::drawPlusMask(71, 58, Images::Dirt_Cloud_Right, idx);

    }

}


void drawFastDottedLine(int16_t startPos, int16_t endPos, int16_t y, bool &color) {

    uint16_t rowOffset = (y / 8) * 128;
    uint8_t mask = 1 << (y % 8);

    for (int16_t x = startPos; x < endPos; ++x) {

        if (x >= 0 && x < WIDTH && y >=0 && y < HEIGHT) {
            if (color)
                arduboy.sBuffer[rowOffset + x] |= mask;
            else 
                arduboy.sBuffer[rowOffset + x] &= ~mask;

        }

        color = !color;

    }

}


void drawFastSolidLine(int16_t startPos, int16_t endPos, int16_t y, bool color) {

    uint16_t rowOffset = (y / 8) * 128;
    uint8_t mask = 1 << (y % 8);

    for (int16_t x = startPos; x < endPos; ++x) {

        if (x >= 0 && x < WIDTH && y >=0 && y < HEIGHT) {
            if (color)
                arduboy.sBuffer[rowOffset + x] |= mask;
            else 
                arduboy.sBuffer[rowOffset + x] &= ~mask;

        }

    }

}

void renderRoadEdge_Black(int16_t &startPos, int16_t endPos, int16_t y, bool &color) {

    drawFastSolidLine(startPos, endPos + 1, y, BLACK);
    startPos = endPos + 1;

}

void renderRoadEdge_Dark(int16_t &startPos, int16_t endPos, int16_t y, bool &color) {

    drawFastDottedLine(startPos, endPos + 1, y, color);
    startPos = endPos + 1;

}

void renderRoadEdge_Light(int16_t &startPos, int16_t endPos, int16_t y) {

    drawFastSolidLine(startPos, endPos + 1, y, WHITE);
    startPos = endPos + 1;

}

void renderRoad_Black(int16_t &startPos, int16_t endPos, int16_t y, bool &color) {

    drawFastSolidLine(startPos, endPos, y, BLACK);
    startPos = endPos;

}

void renderRoad_Dither_Dark(int16_t &startPos, int16_t endPos, int16_t y, bool &dither, bool &color) {

    if (dither) {
        drawFastDottedLine(startPos, endPos, y, color);
        startPos = endPos; 
    }
    else {
        drawFastSolidLine(startPos, endPos, y, BLACK);
        startPos = endPos;
    }

}

void renderRoad_Dither_Light(int16_t &startPos, int16_t endPos, int16_t y, bool &color) {

    drawFastDottedLine(startPos, endPos, y, color);
    startPos = endPos; 

}

void renderDayBanner() {

    uint8_t x = 53;

    if (gamePlayVars.showDayBannerCount > 0) {

        // Render 'New Day' banner from multiple image slices
        Sprites::drawPlusMask(16,    8+5, Images::StartOfDay_Side, 0);
        Sprites::drawPlusMask(16+91, 8+5, Images::StartOfDay_Side, 0);
        
        Sprites::drawPlusMask(16+3,  8, Images::StartOfDay, 0);
        Sprites::drawPlusMask(16+3,  8+16, Images::StartOfDay, 1);
        Sprites::drawOverwrite(16+2, 8+8, Images::StartOfDay_Mid, 0);
                
        Sprites::drawPlusMask(16+27, 8+24, Images::StartOfDay_Base, 0);
        Sprites::drawPlusMask(16+65, 8+24, Images::StartOfDay_Base, 0);


        if (gamePlayVars.days > 10) {
            x = x - 2;
        }
        Sprites::drawOverwrite(x, 18, Images::Day, 0);
        x = x + 18;

        {
            uint8_t newDay = gamePlayVars.days;
            if (newDay >= 100)  {
                Sprites::drawOverwrite(x, 18, Images::Font4x6, ((newDay / 100) + 26));
                newDay = newDay - ((newDay / 100) * 100);
                x = x + 5;
            }
            if (newDay >= 10)   {
                Sprites::drawOverwrite(x, 18, Images::Font4x6, ((newDay / 10) + 26));
                newDay = newDay % 10;
                x = x + 5;
            }
            Sprites::drawOverwrite(x, 18, Images::Font4x6, newDay + 26);
        }

    }

}

void renderGameOver() { // 0  to 108

    Sprites::drawPlusMask(WIDTH - gameOverVars.offset, 13, Images::GameOver_00, 0);
    Sprites::drawPlusMask(-59 + gameOverVars.offset, 37, Images::GameOver_01, 0);

}

void renderLights() {

    uint8_t x = 0;

    if (gamePlayVars.showDayBannerCount > 0) {

        Sprites::drawExternalMask(3, 18, Images::Lights, Images::Lights_Mask, 0, 0);

        switch (gamePlayVars.showDayBannerCount) {
        // NewDayBannerDelay count down from 130...

            // Get Ready...
            case 110:
                arduboy.setRGBled(32, 0, 0); // Red
                #ifdef SOUNDS
                    sound.tones(Sounds::Lights_Short);
                #endif
                /*-fallthrough*/
            case 81 ... 109:
                arduboy.fillRect(11, 24, 3, 3, WHITE); // Illuminate light 1 (top)
                break;

            // Get Set...
            case 80:
                arduboy.setRGBled(24, 24, 0); // Amber
                #ifdef SOUNDS
                    sound.tones(Sounds::Lights_Short);
                #endif
                /*-fallthrough*/
            case 51 ... 79:
                arduboy.fillRect(11, 30, 3, 3, WHITE); // Illuminate light 2 (middle)
                break;

            // Go! ...
            case 50:
                arduboy.setRGBled(0, 48, 0); // Green
                #ifdef SOUNDS
                    sound.tones(Sounds::Lights_Long);
                #endif
                /*-fallthrough*/
            case 20 ... 49:
                arduboy.fillRect(11, 36, 3, 3, WHITE); // Illuminate light 3 (bottom)
                break;
                
            case 0 ... 19:
                arduboy.setRGBled(0, 0, 0);
                arduboy.fillRect(11, 36, 3, 3, WHITE); // Illuminate light 3 (bottom)
                break;

            default:
                break;

        }

    }

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

    #ifdef SOUNDS
        sound.tones(Sounds::Theme);
    #endif
    
    cameraPos.setX(0);
    cameraPos.setY(UPM);
    cameraPos.setZ(-40);

    world.reset();
    gamePlayVars.reset();
    titleVars.reset();

    gameState = GameState::Title;

    #ifdef CACTII
        placeCactii();
    #endif

    #ifdef DEBUG_COLLISIONS
        titleVars.offset = Constants::Title_MaxOffset - 3;
    #endif
    
}   


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void title() {

    // Update position of text ..

    if (titleVars.flyIn && titleVars.offset < Constants::Title_MaxOffset) {

        titleVars.offset = titleVars.offset + 3;

    }

    if (!titleVars.flyIn) {

        if (titleVars.offset > 0) {
            titleVars.offset = titleVars.offset - 3;
        }
        else {

            switch (titleVars.cursor) {
        
                case 0:
                    if (titleVars.seenAim)  {
                        titleVars.titleState = TitleState::SelectTransmission;
                    }
                    else {
                        titleVars.titleState = TitleState::Aim;
                        titleVars.seenAim = true;
                    }
                    titleVars.flyIn = true;
                    break;
        
                case 1:
                    gameState = GameState::HighScore_Init;
                    break;

            }

        }

    }


    draw(false, true);

    switch (titleVars.titleState) {

        case TitleState::StartGame:

            Sprites::drawPlusMask(WIDTH - titleVars.offset, 8, Images::RoadTrip_00, 0);
            Sprites::drawPlusMask(-69 + titleVars.offset, 34, Images::RoadTrip_01, 0);
        
            if (titleVars.offset == Constants::Title_MaxOffset) {

                if (arduboy.justPressed(A_BUTTON)) { 

                    titleVars.flyIn = false;
                    #ifdef DEBUG_COLLISIONS
                        titleVars.offset = 3;
                    #endif

                }         
                
                if (arduboy.justPressed(UP_BUTTON)) { 

                    titleVars.cursor = 0;

                }

                if (arduboy.justPressed(DOWN_BUTTON)) { 

                    titleVars.cursor = 1;

                }

                arduboy.fillRect(84, 8, 44, 15, WHITE);
                arduboy.drawRect(85, 10 + (titleVars.cursor == 1 ? 7 : 0), 3, 3, BLACK);

                Sprites::drawErase(90, 9, Images::Title_Start, 0);
                Sprites::drawErase(90, 16, Images::Title_HighScore, 0);

            }

            break;

        case TitleState::Aim:


            arduboy.fillRect(22, 12, 84, 42, BLACK);
            Sprites::drawOverwrite(22, 12, Images::Corner_TL, 0);
            Sprites::drawOverwrite(22, 46, Images::Corner_BL, 0);
            Sprites::drawOverwrite(103, 12, Images::Corner_TR, 0);
            Sprites::drawOverwrite(103, 46, Images::Corner_BR, 0);

            arduboy.drawFastHLine(25, 13, 78, WHITE);
            arduboy.drawFastHLine(25, 52, 78, WHITE);

            arduboy.drawFastVLine(23, 20, 32, WHITE);
            arduboy.drawFastVLine(104, 20, 32, WHITE);

            Sprites::drawOverwrite(28, 16, Images::Aim, 0);

            if (arduboy.justPressed(A_BUTTON)) { 

                titleVars.titleState = TitleState::SelectTransmission;

            }  

            break;

        case TitleState::SelectTransmission:
            
            // Highlight transmission type selected
            uint8_t xOffset = (car.getTransmissionType() == TransmissionType::Manual ? 42 : 0);
            // Render upper horizontal edge
            Sprites::drawPlusMask(25 + xOffset, 14, Images::TransmissionType_HighlightHorizontal, 0);
            // Render lower horizontal edge
            Sprites::drawPlusMask(25 + xOffset, 42, Images::TransmissionType_HighlightHorizontal, 0);
            // Render vertical edges
            for (uint8_t y = 14+4; y < 42; y = y + 7) {
                Sprites::drawOverwrite(25 + xOffset, y, Images::TransmissionType_HighlightVertical, 0);
                Sprites::drawOverwrite(53 + xOffset, y, Images::TransmissionType_HighlightVertical, 0);
            };

            // Draw 'AT' title card - Automatic Transmission
            Sprites::drawPlusMask(27, 16, Images::TransmissionType_Auto, 0);

            // Draw 'MT' title card - Manual Transmission
            Sprites::drawPlusMask(69, 16, Images::TransmissionType_Auto, 0);
            Sprites::drawOverwrite(69+4, 16+2, Images::TransmissionType_Manual, 0); // Overwrite 'A' with 'M'


            if (arduboy.justPressed(A_BUTTON)) { 

                gameState = GameState::Game_Init;

            }         
            
            if (arduboy.justPressed(LEFT_BUTTON)) { 

                car.setTransmissionType(TransmissionType::Auto);

            }

            if (arduboy.justPressed(RIGHT_BUTTON)) { 

                car.setTransmissionType(TransmissionType::Manual);

            }

            break;

    }


    houseKeeping();

    #ifdef CACTII
        moveCactii();
    #endif
    
    cameraPos.setZ(cameraPos.getZ() + 6);
    cameraPos.setY(world.roadHeightAt(cameraPos.getZ() + 2 * UPM) + UPM);


    // Reset the time to ensure a white (day) screen ..

    gamePlayVars.timeOfDay = 700;

}

#include <stdint.h>

void swap(OtherCar *xp, OtherCar *yp) {  

    OtherCar temp = *xp;  
    *xp = *yp;  
    *yp = temp;  

}  
  
void sort(OtherCar arr[], uint8_t n) {  

    uint8_t i, j;  

    for (i = 0; i < n-1; i++) {     

        for (j = 0; j < n-i-1; j++) {

            if (arr[j].getZ() < arr[j+1].getZ()) {  
                swap(&arr[j], &arr[j+1]);  
            }

        }

    }

}  

int8_t moveCar(int16_t xMovement, int16_t zMovement) { // will return NoCollision constant or car index.

    #ifdef DEBUG_COLLISIONS

        Serial.print("moveCars(");
        Serial.print(xMovement);
        Serial.print(",");
        Serial.print(zMovement);
        Serial.println(")");

    #endif

    uint8_t collide = Constants::NoCollision;

    for (uint8_t i = 0; i < gamePlayVars.numberOfOtherCars; i++) {

        OtherCar &otherCar = otherCars[i];

        #ifdef DEBUG_COLLISIONS

            Serial.print("A P ");
            car.printPos();
            Serial.print(" = O(");
            Serial.print(i);
            Serial.print(") ");
            otherCar.printPos();
            Serial.print(" - ");

        #endif

        int16_t xOverlap = (car.getX() + xMovement) - (otherCar.getX() + otherCar.getXWorld());
        int16_t zOverlap = (car.getZ() + zMovement) - otherCar.getZ();

        if (abs(xOverlap) < Constants::OtherCarWidthUnits && abs(zOverlap) < Constants::OtherCarLengthUnits) {
                    
            #ifdef DEBUG_COLLISIONS
                Serial.print("COL x:");
                Serial.print(xMovement);
                Serial.print(", xO:");
                Serial.print(xOverlap);
                Serial.print(", z:");
                Serial.print(zMovement);
                Serial.print(", zO:");
                Serial.print(zOverlap);

            #endif

            if (xMovement != 0) { // Move left or right ..

                if (abs(zOverlap) < Constants::CollisionTollerance_Z) {

                    collide = i;
                                
                    #ifdef SOUNDS

                        if (!sound.playing()) {

                            sound.tones(Sounds::Crash);
                        }

                    #endif

                    #ifdef DEBUG_COLLISIONS
                        Serial.println("BLOCK LR");
                    #endif

                    break;

                }

            }

            if (zMovement != 0) { // Move front or back ..

                if (abs(xOverlap) < Constants::CollisionTollerance_X) {

                    collide = i;
                                
                    #ifdef SOUNDS

                        if (!sound.playing()) {

                            sound.tones(Sounds::Crash);

                        }

                    #endif

                    #ifdef DEBUG_COLLISIONS
                        Serial.println("BLOCK FB");
                    #endif

                    if (car.getZ() < otherCars[i].getZ() && nudgeCarForward(i)) {

                        collide = collide * -1;

                    }

                    break;
                    
                }

            }

            #ifdef DEBUG_COLLISIONS
                Serial.println("No Col");
            #endif

        }
        else {
            
            #ifdef DEBUG_COLLISIONS
                Serial.println("");
            #endif

        }

        // #ifdef DEBUG_COLLISIONS

        //     Serial.print("B P ");
        //     car.printPos();
        //     Serial.print(" = O(");
        //     Serial.print(i);
        //     Serial.print(") ");
        //     otherCar.printPos();
        //     Serial.println(" ");

        // #endif
    }


    return collide;

}

void moveOtherCars() {

    // Sort the cars ..

    sort(otherCars, gamePlayVars.numberOfOtherCars);

    for (uint8_t i = 0; i < gamePlayVars.numberOfOtherCars; i++) {

        bool collide = false;
        OtherCar &otherCar = otherCars[i];

        int16_t otherCarX = otherCar.getX() + otherCar.getXWorld() + otherCar.getXMovement();


        #ifdef DEBUG_COLLISIONS

            Serial.print("C O(");
            Serial.print(i);
            Serial.print(") ");
            otherCar.printPos();
            Serial.println(" - ");

        #endif        

        for (uint8_t j = i; j < gamePlayVars.numberOfOtherCars; j++) {

            if (i != j) {

                OtherCar &testCar = otherCars[j];

                if (abs((testCar.getX() + testCar.getXWorld()) - otherCarX) < Constants::OtherCarWidthUnits && abs(testCar.getZ() - (otherCar.getZ() + otherCar.getSpeed_Display())) < Constants::OtherCarLengthUnits ) {

                    collide = true;

                }

            }

        }


        // What about into the player?

        if (abs(car.getX() - otherCarX) < Constants::OtherCarWidthUnits && abs((car.getZ() + car.getSpeed_Display()) - (otherCar.getZ() + otherCar.getSpeed_Display())) < Constants::OtherCarLengthUnits ) {

            collide = true;

        }


        // If no collision has occurred then move the car forward ..

        if (!collide) {

            otherCar.setZ(otherCar.getZ() + otherCar.getSpeed_Display());

            if (otherCar.getZ() < cameraPos.getZ() - Constants::DistToRenewCars) {

                if (gamePlayVars.numberOfOtherCars > 1) {

                    for (uint8_t j = 0; j < gamePlayVars.numberOfOtherCars; j++) {

                        if (j != i) {

                            int16_t dist = otherCar.getZ() - cameraPos.getZ();

                            if (dist > 0) {

                                otherCar.setZ(otherCars[j].getZ() + random(700 - dist, 1200 - dist));

                            }
                            else {

                                otherCar.setZ(cameraPos.getZ() + random(700, 1200));

                            }

                            break;

                        }

                    }

                }
                else {

                    otherCar.setZ(cameraPos.getZ() + random(700, 1200));

                }

                otherCar.setSpeed(random(9, 14) * Constants::SpeedDiv);
                
                if (gamePlayVars.carsPassed > 0 && !gamePlayVars.gameOver) {

                    gamePlayVars.carsPassed--;

                    #ifdef SOUNDS

                        if (gamePlayVars.carsPassed > 0) {
                            sound.tones(Sounds::PassCar);
                        }
                        else {
                            sound.tones(Sounds::Success);
                        }

                    #endif

                }

            }

            otherCar.update();

        }

    }

    // Has the game finished?

    if (gamePlayVars.gameOver) {

        if (gameOverVars.offset < Constants::GameOver_MaxOffset) {

            gameOverVars.offset = gameOverVars.offset + 3;
            renderGameOver();

        }
        else {

            renderGameOver();
            gameOverVars.finalSpeed = car.getSpeed_Display();
            gameState = GameState::GameOver_Init;

        }

    }

}

void houseKeeping() {

    if (cameraPos.getZ() > Constants::ZMax) {

        cameraPos.decZ();
        car.decZ();
        world.decZ();
        gamePlayVars.decZ();

        for (uint8_t i = 0; i < gamePlayVars.numberOfOtherCars; i++) {

            OtherCar &otherCar = otherCars[i];
            otherCar.decZ();

        }

        #ifdef CACTII

            for (uint8_t i = 0; i < Constants::NumberOfCactii; i++) {

                Vec3 &cactusPos = cactii[i];
                cactusPos.decZ();

            }

        #endif

    }
    else {

        // Are cars too far into the distance?

        for (uint8_t i = 0; i < gamePlayVars.numberOfOtherCars; i++) {

            OtherCar &otherCar = otherCars[i];
            if (otherCar.getZ() - otherCar.getZ() > 2500) {
                otherCar.setZ(otherCar.getZ() - 1500);
            }

        }

    }


    // Increase the odometer ..

    if (!gamePlayVars.gameOver && (cameraPos.getZ() - gamePlayVars.previousZ > 64)) {

        gamePlayVars.previousZ = cameraPos.getZ();
        gamePlayVars.distance++;


    }


    // Increase time of day ..

    bool newDay = gamePlayVars.incTimeOfDay();

    if (newDay) {

        if (gamePlayVars.carsPassed > 0) {

            gamePlayVars.gameOver = true;
    
            #ifdef SOUNDS
                sound.tones(Sounds::GameOver);
            #endif

        }
        else {

            gamePlayVars.showDayBannerCount = Constants::NewDayBannerDelay;
            gamePlayVars.carsPassed = Constants::CarsToPass_Init + (gamePlayVars.days * Constants::CarsToPass_Increase);

        }
        
    }


    // Decrease banner counter ..

    //if (gamePlayVars.days > 1) {
        gamePlayVars.decShowDayBannerCount();
    //}

}

#ifdef CACTII

    void moveCactii() {

        // Move cactii

        for (uint8_t i = 0; i < Constants::NumberOfCactii; i++) {

            Vec3 &cactiiPos = cactii[i];

            if (cactiiPos.getZ() < cameraPos.getZ() - Constants::DistToRenewCars) {
                cactiiPos.setZ(cameraPos.getZ() + random(600, 900));
                cactiiPos.setX(random(0,2));
            }

        }

    }

    void placeCactii() {

        for (uint8_t i = 1; i <= Constants::NumberOfCactii; i++) {

            Vec3 &cactiiPos = cactii[i - 1];

            cactiiPos.setX(i % 2);
            cactiiPos.setZ(400 + (i * 245));

        }

    }

#endif

bool nudgeCarForward(uint8_t carIdx) {

    OtherCar &otherCar = otherCars[carIdx];
    bool collide = false;

    int16_t otherCarZ = otherCar.getZ() + otherCar.getSpeed_Display() + Constants::NudgeDistance;
    int16_t otherCarX = otherCar.getX() + otherCar.getXWorld() + otherCar.getXMovement();

    for (uint8_t i = 0; i < gamePlayVars.numberOfOtherCars; i++) {

        if (i != carIdx) {

            OtherCar &testCar = otherCars[i];

            if (abs((testCar.getX() + testCar.getXWorld()) - otherCarX) < Constants::OtherCarWidthUnits && abs(testCar.getZ() - otherCarZ) < Constants::OtherCarLengthUnits ) {

                collide = true;

            }

        }

    }

    if (!collide) {

        otherCar.setZ(otherCar.getZ() + Constants::NudgeDistance);
        return true;

    }

    return false;

}

