#pragma once
#include "zarduboy.h"
#include "bitmaps.h"
#include "sounds.h"

ZArduboy Z;

// for each level (line)
// - nb mole
// - total nb bad mole
// - level sleep frames
// - level sleep frames between each mole, for level start
// - level wake frames
// - level going out frames

uint8_t constexpr dataLevels[] PROGMEM =
{
    1, 0, 50, 0,  100, 80,
    1, 1, 40, 0,  90,  70,
    1, 3, 30, 0,  80,  70,
    2, 4, 50, 60, 90,  80,
    2, 5, 50, 50, 80,  70,
    3, 6, 50, 50, 70,  70,
};
auto constexpr kDataLevelSize = sizeof(uint8_t)*6;
auto constexpr kMaxTunedLevels = uint8_t{6};

////////////////////////////////////////////////////////////////////////////
// globals

auto constexpr kFrameTime = int32_t{1000/60};
auto constexpr kEEPROMMagicNumber = uint8_t{0xF0};
auto constexpr kEEPROMMagicNumber_pos = uint16_t{32};
auto constexpr kEEPROMSound_pos = uint16_t{33};
auto constexpr kEEPROMHiScore_pos = uint16_t{34};

auto globalTime = uint32_t{0};
auto frame = uint32_t{0};
auto previousButtonState = uint8_t{0};

////////////////////////////////////////////////////////////////////////////
// game data

using FuncGameState = void(*)();
auto gameFunc = FuncGameState{nullptr};
auto gameScore = uint16_t{0};
auto gameHiScore = uint16_t{0};
auto gameTime = int32_t{30000};

auto gameLevel = uint8_t{0};
auto gameLevelSleepingFrames = int16_t{0};
auto gameLevelSleepingFramesOnStartStep = int16_t{0};
auto gameLevelScoreToNext = uint16_t{0};
auto gameLevelNbMole = uint8_t{0};
auto gameLevelMinMoleRem = uint8_t{0};
auto gameLevelBadMoleRem = uint8_t{0};

struct Hole
{
    bool occupied = false;
    int16_t x = -1;
    int16_t y = -1;

    Hole() = default;
    Hole(int16_t x, int16_t y)
        : x{x}
        , y{y}
    {}
    void draw()
    {
        Z.drawBitmap(x - 10, y + 3, bmpHole, bmpHole_w, bmpHole_h);
    }
};
Hole gameHoles[] =
{
    Hole{24, 11}, Hole{48, 11}, Hole{72, 11},
    Hole{22, 32}, Hole{48, 32}, Hole{74, 32},
    Hole{20, 53}, Hole{48, 53}, Hole{76, 53}
};

struct Mole
{
    enum eState : uint8_t
    {
        Sleeping = 0,
        Waking,
        GoingOut,
        Hit,

        NbState
    };

    eState state = Sleeping;
    int16_t stateRFrame = 0;
    uint8_t holePos = 9;
    bool isBad = false;
    static int16_t stateWakingFrameT[4];
    static int16_t stateGoingOutFrameT[3];

    static void initLevel(int16_t wakeFrames, int16_t goingOutFrames)
    {
        stateWakingFrameT[0] = wakeFrames;
        stateWakingFrameT[1] = (wakeFrames*7);
        stateWakingFrameT[2] = (wakeFrames*3)/4;
        stateWakingFrameT[3] = (wakeFrames/2);
        for(auto n = 0; n < 4;++n)
        {
            if(stateWakingFrameT[n] <= 0)
                stateWakingFrameT[n] = 1;
        }
        stateGoingOutFrameT[0] = goingOutFrames;
        stateGoingOutFrameT[1] = (goingOutFrames*9)/10;
        stateGoingOutFrameT[2] = (goingOutFrames*7)/10;
        for(auto n = 0; n < 3;++n)
        {
            if(stateGoingOutFrameT[n] <= 0)
                stateGoingOutFrameT[n] = 1;
        }
    }

    void sleep(int16_t nbSleepFrame)
    {
        if(holePos != 9)
        {
            gameHoles[holePos].occupied = false;
            holePos = 9;
        }
        state = Sleeping;
        stateRFrame = nbSleepFrame;
    }

    finline void sleep()
    {
        sleep(gameLevelSleepingFrames);
    }

    void wake()
    {
        // select hole
        auto n = 0;
        do
        {
            n = random(9);
        } while(gameHoles[n].occupied == true);
        holePos = n;
        gameHoles[n].occupied = true;
        // is bad mole?
        if(gameLevelBadMoleRem > 0)
        {
            isBad = (random(0, 4) > 2);
            if((isBad == false) && ((gameLevelMinMoleRem - gameLevelBadMoleRem) < 2))
                isBad = true;

            if(isBad == true)
                --gameLevelBadMoleRem;
        }
        else
            isBad = false;
        if(gameLevelMinMoleRem > 0)
            --gameLevelMinMoleRem;
        state = Waking;
        stateRFrame = stateWakingFrameT[0];
        // no need to wake up if not enough time to go out ...
        if((stateRFrame + 2) > (gameTime / kFrameTime))
            sleep();
    }

    void goOut()
    {
        state = GoingOut;
        stateRFrame = stateGoingOutFrameT[0];
    }

    void hit()
    {
        state = Hit;
        stateRFrame = 30;
    }

    void updateAndDraw()
    {
        --stateRFrame;
        switch(state)
        {
        case Sleeping:
            if(stateRFrame < 0)
                wake();
            break;

        case Waking:
            {
                auto& hole = gameHoles[holePos];
                auto x = hole.x;
                auto y = hole.y;
                if((stateRFrame <= stateWakingFrameT[1]) && (stateRFrame > stateWakingFrameT[2]))
                {
                    // opened eyes
                    Z.drawPixel(x - 3, y + 5, White);
                    Z.drawPixel(x - 3, y + 6, White);
                    Z.drawPixel(x + 2, y + 5, White);
                    Z.drawPixel(x + 2, y + 6, White);
                }
                else if((stateRFrame > stateWakingFrameT[1]) || (stateRFrame > stateWakingFrameT[3]))
                {
                    // closed eyes
                    Z.drawPixel(x - 3, y + 6, White);
                    Z.drawPixel(x + 2, y + 6, White);
                }
                if(stateRFrame < 0)
                    goOut();
            }
            break;

        case GoingOut:
            {
                auto& hole = gameHoles[holePos];
                auto x = hole.x - 6;
                auto y = hole.y - 8;
                auto bmp = (isBad == false)? bmpMole_out2 : bmpMoleBad_out2;
                if(stateRFrame >= stateGoingOutFrameT[1])
                    bmp = (isBad == false)? bmpMole_out0 : bmpMoleBad_out0;
                else if((stateRFrame < stateGoingOutFrameT[1]) && (stateRFrame > stateGoingOutFrameT[2]))
                    bmp = (isBad == false)? bmpMole_out1 : bmpMoleBad_out1;
                Z.drawBitmapOver(x, y, bmp, nullptr, bmpMole_w, bmpMole_h);
                if(stateRFrame < 0)
                    sleep();
            }
            break;

        case Hit:
            {
                auto& hole = gameHoles[holePos];
                auto x = hole.x;
                auto y = hole.y;
                auto bmp = bmpMole_hit0;
                if((stateRFrame%10) >= 5)
                    bmp = bmpMole_hit1;
                Z.drawBitmapOver(x - 6, y - 8, bmp, nullptr, bmpMole_w, bmpMole_h);
                if(stateRFrame < 0)
                    sleep();
            }
            break;
        default:
            break;
        }
    }
};
Mole gameMoles[5];
int16_t Mole::stateWakingFrameT[4];
int16_t Mole::stateGoingOutFrameT[3];

struct Hammer
{
    enum eState : uint8_t
    {
        StandBy = 0,
        Hit,
        HitMiss,
        IsHit,

        NbState
    };

    eState state = StandBy;
    int16_t stateRFrame = 0;
    uint8_t holePos = 4;

    void standBy()
    {
        state = StandBy;
    }

    void hit()
    {
        Z.tones(sndHammerHit);
        state = Hit;
        stateRFrame = 10;
    }

    void hitMiss()
    {
        Z.tones(sndHammeMiss);
        state = HitMiss;
        stateRFrame = 10;
    }

    void isHit()
    {
        Z.tones(sndHammerIsHit);
        state = IsHit;
        stateRFrame = 20;
    }

    void updateAndDraw()
    {
        --stateRFrame;
        auto& hole = gameHoles[holePos];
        switch(state)
        {
        case StandBy:
            Z.drawBitmapOver(hole.x, hole.y - 19, bmpHammer_0, bmpHammer_0_mask, bmpHammer_w, bmpHammer_h);
            break;
        case HitMiss:
        case Hit:
            Z.drawBitmapOver(hole.x - 5, hole.y - 17, bmpHammer_1, bmpHammer_1_mask, bmpHammer_w, bmpHammer_h);
            if(stateRFrame < 0)
                standBy();
            break;
        case IsHit:
            if((stateRFrame%10) >= 5)
                Z.drawBitmapOver(hole.x - 5, hole.y - 19, bmpHammer_1, bmpHammer_1_mask, bmpHammer_w, bmpHammer_h);
            if(stateRFrame < 0)
                standBy();
            break;
        }
    }
};
Hammer hammer;

////////////////////////////////////////////////////////////////////////////
// game func

void setNextLevel(uint8_t pos)
{
    gameLevel = pos + 1;
    if(gameLevel >= kMaxTunedLevels)
        pos = kMaxTunedLevels - 1;
    uint8_t const* dl = dataLevels + (pos * kDataLevelSize);
    gameLevelNbMole = pgm_read_byte(dl);
    dl += sizeof(uint8_t);
    gameLevelBadMoleRem = pgm_read_byte(dl);
    dl += sizeof(uint8_t);
    gameLevelSleepingFrames = static_cast<int16_t>(pgm_read_byte(dl));
    dl += sizeof(uint8_t);
    gameLevelSleepingFramesOnStartStep = static_cast<int16_t>(pgm_read_byte(dl));
    dl += sizeof(uint8_t);
    auto levelWakeFrames = static_cast<int16_t>(pgm_read_byte(dl));
    dl += sizeof(uint8_t);
    auto levelGoingOutFrames = static_cast<int16_t>(pgm_read_byte(dl));
    if(gameLevel >= kMaxTunedLevels)
    {
        ++gameLevelBadMoleRem;
        auto tmp = (gameLevel - kMaxTunedLevels + 1) * 2;
        gameLevelSleepingFrames -= tmp;
        gameLevelSleepingFramesOnStartStep -= tmp;
        levelWakeFrames -= tmp;
        levelGoingOutFrames -= tmp;
    }
    // additional checks
    if(gameLevelSleepingFrames < 20)
        gameLevelSleepingFrames = 20;
    if(gameLevelSleepingFramesOnStartStep < 20)
        gameLevelSleepingFramesOnStartStep = 20;
    if(levelWakeFrames < 20)
        levelWakeFrames = 20;
    if(levelGoingOutFrames < 20)
        levelGoingOutFrames = 20;

    // setup
    Mole::initLevel(levelWakeFrames, levelGoingOutFrames);
    gameTime = 30000;
    auto minMole = int16_t{0};
    auto maxMole = int16_t{0};
    for(auto n = 0; n < gameLevelNbMole; ++n)
    {
        auto ft = kFrameTime + 1;
        auto tmp = gameTime - (gameLevelSleepingFramesOnStartStep * n * ft);
        maxMole += tmp / ((gameLevelSleepingFrames + Mole::stateWakingFrameT[0] + 30) * ft);
        minMole += tmp / ((gameLevelSleepingFrames + Mole::stateWakingFrameT[0] + Mole::stateGoingOutFrameT[0]) * ft);
    }
    gameLevelMinMoleRem = static_cast<uint8_t>(minMole);
    gameLevelScoreToNext += (minMole - gameLevelNbMole - gameLevelBadMoleRem);
}

void drawNumbers(int16_t x, int16_t y, uint16_t value)
{
    char buf[6];
    itoa(value, buf, 10);
    auto len = strlen(buf);
    for(auto n = size_t{0}; n < len; ++n)
    {
        auto digit = static_cast<uint8_t>(buf[n] - 48);
        Z.drawBitmap(x + n * 4, y, bmpNumbers + digit * 3, 3, 8);
    }
}

void drawText(int16_t x, int16_t y, char const* text)
{
    auto p = text;
    auto px = x;
    while(*p != '\0')
    {
        if(*p != ' ')
        {
            auto c = (*p - 'A') * 3;
            Z.drawBitmap(px, y, bmpLetters + c, 3, 5);
        }
        px += 4;
        ++p;
    }
}

finline bool buttonPressed(uint8_t button, uint8_t buttonState)
{
    return (!(button & buttonState)) && (button & previousButtonState);
}

finline bool buttonFirstPress(uint8_t button, uint8_t buttonState)
{
    return (button & buttonState) && (!(button & previousButtonState));
}

finline bool handleButtonAnyPress()
{
    auto buttonState = Z.buttonState();
    auto r = (buttonState != 0) && (previousButtonState == 0);
    previousButtonState = buttonState;
    return r;
}

void readHighScore()
{
    gameHiScore = static_cast<uint16_t>(Z.readFromEEPROM(kEEPROMHiScore_pos));
    gameHiScore |= Z.readFromEEPROM(kEEPROMHiScore_pos + 1) << 8;
}

void onNewHiScore(uint16_t score)
{
    gameHiScore = score;
    Z.writeToEEPROM(kEEPROMHiScore_pos, score & 0xff);
    Z.writeToEEPROM(kEEPROMHiScore_pos + 1, (score >> 8) & 0xff);
}

////////////////////////////////////////////////////////////////////////////
// game states

void gfTest();
void gfStart();
void gfPlay();
void gfLevelIntro();
void gfGameOver();
void gfInfo();

void toStart()
{
    previousButtonState = Z.buttonState();
    hammer.holePos = 0;
    gameTime = 0;
    gameScore = 0;
    gameLevelScoreToNext = 0;
    readHighScore();
    gameFunc = gfStart;
}

void toGameOver()
{
    previousButtonState = Z.buttonState();
    gameTime = 3000;
    gameFunc = gfGameOver;
    if(gameScore > gameHiScore)
        onNewHiScore(gameScore);
}

void toLevelIntro()
{
    gameTime = 3000;
    gameFunc = gfLevelIntro;
}

void toPlay()
{
    previousButtonState = 0;
    setNextLevel(gameLevel);
    hammer.holePos = 4;
    auto sleepNbFrame = gameLevelSleepingFrames;
    for(auto n = uint8_t{0}; n < gameLevelNbMole; ++n)
    {
        gameMoles[n].sleep(sleepNbFrame);
        sleepNbFrame += gameLevelSleepingFramesOnStartStep;
    }
    gameFunc = gfPlay;
}

void toInfo()
{
    previousButtonState = Z.buttonState();
    hammer.holePos = 0;
    gameFunc = gfInfo;
}

void gfLevelIntro()
{
    if(gameTime == 3000)
        Z.tones(sndLevelIntro2);

    gameTime -= kFrameTime;
    auto const y = int16_t{4};
    auto offset = int16_t{0};
    if(((gameTime < 2200) && (gameTime > 2000))
       || ((gameTime < 1200) && (gameTime > 1000)))
    {
        offset = 1;
        Z.drawBitmap(56, 10 + y, bmpHammer_1, bmpHammer_w, bmpHammer_h);
    }
    else
        Z.drawBitmap(60, 8 + y, bmpHammer_0, bmpHammer_w, bmpHammer_h);
    drawText(50, 26 + y + offset, "LEVEL\0");
    drawNumbers(74, 26 + y + offset, gameLevel+1);
    if((gameTime <= 2000) && gameTime > 1000)
        drawText(54, 34 + y, "READY\0");
    else if(gameTime <= 1000)
        drawText(54, 34 + y, "START\0");
    if(gameTime < 0)
        toPlay();
}

void gfGameOver()
{
    gameTime -= kFrameTime;
    if(gameTime < 2000)
        drawText(48, 12, "GAME OVER\0");
    if(gameTime < 1000)
    {
        if(gameScore == gameHiScore)
        {
            Z.drawBitmap(32, 34, bmpHole, bmpHole_w, bmpHole_h);
            Z.drawBitmapOver(36, 22, bmpMole_out2, nullptr, bmpMole_w, bmpMole_h);
            drawText(56, 26, "NEW HISCORE\0");
            drawNumbers(72, 34, gameScore);
        }
        else
        {
            Z.drawBitmap(40, 34, bmpHole, bmpHole_w, bmpHole_h);
            Z.drawBitmapOver(44, 22, bmpMole_hit0, nullptr, bmpMole_w, bmpMole_h);
            drawText(68, 26, "SCORE\0");
            drawNumbers(72, 34, gameScore);
        }
    }
    if(gameTime < 0)
    {
        if((gameTime/1000)%2 == 0)
            drawText(40, 56, "PRESS ANY KEY\0");
        if(handleButtonAnyPress() == true)
            toStart();
    }
}

void gfPlay()
{
    gameTime -= kFrameTime;
    // check for game over
    if(gameTime < 0)
    {
        if((gameScore >= gameLevelScoreToNext))// && (gameLevel < kMaxLevels))
        {
            toLevelIntro();
            return;
        }
        toGameOver();
        return;
    }

    // update input
    auto buttonState = Z.buttonState();
    auto tryHit = false;
    if(hammer.state == Hammer::StandBy)
    {
        auto& hp = hammer.holePos;
        if(buttonFirstPress(Z.InputLeft, buttonState) == true)
            hp = (hp%3 == 0)? hp : hp - 1;
        else if(buttonFirstPress(Z.InputRight, buttonState) == true)
            hp = (hp%3 == 2)? hp : hp + 1;
        if(buttonFirstPress(Z.InputUp, buttonState) == true)
            hp = (hp < 3)? hp : hp - 3;
        else if(buttonFirstPress(Z.InputDown, buttonState) == true)
            hp = (hp > 5)? hp : hp + 3;
        tryHit = buttonFirstPress(Z.InputA, buttonState);
    }
    previousButtonState = buttonState;

    // draw game
    for(auto n = uint8_t{0}; n < 9; ++n)
        gameHoles[n].draw();
    for(auto n = uint8_t{0}; n < gameLevelNbMole; ++n)
    {
        auto& mole = gameMoles[n];
        if((tryHit == true)
           && (hammer.holePos == mole.holePos)
           && (mole.state == Mole::GoingOut))
        {
            if(mole.isBad == false)
            {
                hammer.hit();
                mole.hit();
                ++gameScore;
            }
            else
            {
                hammer.isHit();
                gameScore = (gameScore == 0)? 0 : gameScore - 1;
            }
        }
        mole.updateAndDraw();
    }
    if((tryHit == true) && (hammer.state == Hammer::StandBy))
        hammer.hitMiss();
    hammer.updateAndDraw();

    // draw hud
    {
        // draw score
        drawText(95, 2, "SCORE\0");
        auto x = int16_t{0};
        if(gameScore > 99)
            x = 116;
        else if(gameScore > 9)
            x = 120;
        else
            x = 124;
        drawNumbers(x, 8, gameScore);
        // draw level
        drawText(95, 17, "LEVEL\0");
        if(gameLevel > 9)
            x = 120;
        else
            x = 124;
        drawNumbers(x, 23, gameLevel);
        // draw time
        drawText(95, 51, "TIME\0");
        x = 127 - (gameTime/1000) - 1;
        Z.drawRect(95, 57, 126, 62, White);
        Z.drawHLine(x, 58, 126, White);
        Z.drawHLine(x, 59, 126, White);
        Z.drawHLine(x, 60, 126, White);
        Z.drawHLine(x, 61, 126, White);
    }
}

void gfStart()
{
    // update input
    auto buttonState = Z.buttonState();
    auto& hp = hammer.holePos;
    if(gameTime <= 0)
    {
        if(buttonFirstPress(Z.InputLeft, buttonState) == true)
            hp = (hp == 0)? hp : hp - 1;
        else if(buttonFirstPress(Z.InputRight, buttonState) == true)
            hp = (hp == 2)? hp : hp + 1;
        if(buttonFirstPress(Z.InputA, buttonState) == true)
        {
            gameTime = 400;
            if(hp == 1)
            {
                Z.enableSound(!Z.isSoundEnabled());
                Z.updateToEEPROM(kEEPROMSound_pos, (Z.isSoundEnabled() == true)? 1 : 0);
            }
            Z.tones(sndMenuSelect);
        }
    }
    else
    {
        gameTime -= kFrameTime;
        if(gameTime <= 0)
        {
            if(hp == 0)
            {
                gameLevel = 0;
                toLevelIntro();
                return;
            }
            else if(hp == 2)
            {
                toInfo();
                return;
            }
        }
    }
    previousButtonState = buttonState;

    // draw title
    Z.drawCompressed(18, 5, bmpTitleC, bmpTitleC_w, bmpTitleC_h);

    // draw select
    auto px = int16_t{0};
    auto py = int16_t{0};
    auto bmp = bmpHammer_0;
    auto mask = bmpHammer_0_mask;
    auto offset = int16_t{0};
    if(gameTime <= 200)
    {
        py = 40;
        if(hp == 0)
            px = 14;
        else if(hp == 1)
            px = 46;
        else if(hp == 2)
            px = 78;
    }
    else
    {
        offset = 1;
        py = 42;
        if(hp == 0)
            px = 12;
        else if(hp == 1)
            px = 44;
        else if(hp == 2)
            px = 76;
        bmp = bmpHammer_1;
        mask = bmpHammer_1_mask;
    }
    Z.drawBitmapOver(px, py, bmp, mask, bmpHammer_w, bmpHammer_h);

    // draw menu
    {
        drawText(10, (hp==0)? 58 + offset : 58, "PLAY\0");
        drawText(40, (hp==1)? 58 + offset : 58, "SOUND\0");
        if(Z.isSoundEnabled() == false)
            Z.drawHLine(38, (hp==1)? 60 + offset : 60, 60, White);
        drawText(74, (hp==2)? 58 + offset : 58, "INFO\0");
    }

    // draw hiscore
    {
        drawText(100, 50, "HISCORE\0");
        auto x = int16_t{0};
        if(gameHiScore > 99)
            x = 116;
        else if(gameHiScore > 9)
            x = 120;
        else
            x = 124;
        drawNumbers(x, 58, gameHiScore);
    }
}

void gfInfo()
{
    gameTime -= kFrameTime;
    // handle info screens
    if(hammer.holePos == 0)
    {
        auto hx = int16_t{68};
        auto hy = int16_t{34};
        auto bmpMole = bmpMole_out2;
        auto bmpHammer = bmpHammer_0;
        auto bmpHammer_mask = bmpHammer_0_mask;
        auto bmpButton = bmpButton2_0;
        auto px = hx;
        auto py = hy - 19;
        auto score = 0;
        if((gameTime/1000)%2 == 0)
        {
            bmpMole = bmpMole_hit0;
            bmpButton = bmpButton2_1;
            bmpHammer = bmpHammer_1;
            bmpHammer_mask = bmpHammer_1_mask;;
            score = 1;
            px = hx - 5;
            py = hy - 17;
        }

        drawText(42, 2, "HOW TO PLAY\0");
        // controls
        Z.drawBitmap(4, hy - 8, bmpPad, bmpPad_w, bmpPad_h);
        Z.drawBitmap(24, hy, bmpButton, bmpButton2_w, bmpButton2_h);
        Z.drawBitmap(34, hy - 4, bmpButton2_0, bmpButton2_w, bmpButton2_h);
        // hole + mole
        Z.drawBitmap(hx - 10, hy + 3, bmpHole, bmpHole_w, bmpHole_h);
        Z.drawBitmapOver(hx - 6, hy - 8, bmpMole, nullptr, bmpMole_w, bmpMole_h);
        Z.drawBitmapOver(px, py, bmpHammer, bmpHammer_mask, bmpHammer_w, bmpHammer_h);
        // score
        drawText(96, hy - 10, "SCORE\0");
        drawNumbers(123, hy - 4, score);
    }
    else if(hammer.holePos == 1)
    {
        drawText(50, 2, "CREDITS\0");
        drawText(20, 15, "CODE GFX SND\0");
        drawText(64, 23, "ZEDUCKMASTER\0");
        drawText(30, 35, "LICENSE\0");
        drawText(64, 43, "GNU GPL V\0");
        drawNumbers(100, 43, 3);
        Z.drawBitmap(99, 52, bmpMole_out2, bmpMole_w, bmpMole_h);
        // version
        drawText(114, 58, "V\0");
        drawNumbers(118, 58, 1);
        Z.drawPixel(122, 62, White);
        drawNumbers(124, 58, 2);
    }

    if(handleButtonAnyPress() == true)
    {
        ++hammer.holePos;
        if(hammer.holePos > 1)
            toStart();
    }
}

void gfTest()
{
    //Z.drawBitmap(-4, 11, bmpHole, bmpHole_w, bmpHole_h);
    //Z.drawBitmapOver(0, 1, bmpMole1, bmpMole_w, bmpMole_h);
}

////////////////////////////////////////////////////////////////////////////
// main program

void zsetup()
{
    // boot
    //Serial.begin(9600);
    Z.begin();
    Z.initRandomSeed();
    // check eeprom
    auto c = Z.readFromEEPROM(kEEPROMMagicNumber_pos);
    if(c != kEEPROMMagicNumber)
    {
        Z.writeToEEPROM(kEEPROMMagicNumber_pos, kEEPROMMagicNumber);
        Z.writeToEEPROM(kEEPROMSound_pos, 0);
        Z.writeToEEPROM(kEEPROMHiScore_pos, 0);
        Z.writeToEEPROM(kEEPROMHiScore_pos + 1, 0);
    }
    else
    {
        auto snd = Z.readFromEEPROM(kEEPROMSound_pos);
        Z.enableSound(snd == 1);
    }

    // start
    gameLevel = 0;
    //toLevelIntro();
    //toGameOver();
    //toPlay();
    toStart();
    //toInfo();
}

void zloop()
{
    // control frame time
    {
        auto curtime = millis();
        auto elapsedTime = curtime - globalTime;
        if(elapsedTime < kFrameTime)
        {
            if((kFrameTime - elapsedTime) > 1)
                Z.idle();
            return;
        }
        globalTime = curtime;
        ++frame;
    }
    // game func
    gameFunc();
    Z.display();
}

