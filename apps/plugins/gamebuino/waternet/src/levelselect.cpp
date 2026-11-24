#include <stdint.h>

#include "helperfuncs.h"
#include "commonvars.h"
#include "printfuncs.h"
#include "savestate.h"
#include "sound.h"
#include "level.h"
#include "palettes.h"


void drawLevelSelect() 
{
    gb.display.setColor(INDEX_BLACK);
    gb.display.fillRect(maxBoardBgWidth * 8, 0, gb.display.width() - (maxBoardBgWidth * 8), gb.display.height());
    //LEVEL:
    printMessage(maxBoardBgWidth  , 0 , F("LEVEL:"));
    
    //[LEVEL NR] 2 chars
    printNumber(maxBoardBgWidth + 4 , 1 , selectedLevel, 2);
    
    //B:BACK
    printMessage(maxBoardBgWidth  , 6 , F("b:"));
    printMessage(maxBoardBgWidth  , 7 , F("BACK"));
    
    //A:PLAY
    printMessage(maxBoardBgWidth  , 4 , F("a:"));
    printMessage(maxBoardBgWidth  , 5 , F("PLAY"));
    
    //Locked & Unlocked keywoard
    uint8_t tmpUnlocked = levelUnlocked(gameMode, difficulty, selectedLevel -1);
    if(!tmpUnlocked)
        printMessage(maxBoardBgWidth , 2 , F("LOCKED"));
    else
        printMessage(maxBoardBgWidth , 2 , F("OPEN"));
    
    drawLevel();

}


void initLevelSelect()
{
    setBlockTilesAsBackground();
    SelectMusic(musTitle);
    setPaletteGame();
    needRedraw = 1;
}

void levelSelect()
{
    if(gameState == gsInitLevelSelect)
    {
        initLevelSelect();
        gameState -= gsInitDiff;
    }
        
    if(needRedraw)
    {
        drawLevelSelect();
        needRedraw = 0;
    }
    
    uint8_t tmpUnlocked = levelUnlocked(gameMode, difficulty, selectedLevel -1);


    if (gb.buttons.released(BUTTON_B))
    {
        playMenuBackSound();
        gameState = gsInitTitle;
    }
    if (gb.buttons.released(BUTTON_A))
    {
        if(tmpUnlocked)
        {
            gameState = gsInitGame;
            playMenuAcknowlege();
        }
        else
        {
            playErrorSound();
        }
    }
    if (gb.buttons.released(BUTTON_LEFT))
    {
        if (difficulty == diffRandom)
        {
            playMenuSelectSound();
            randomSeedGame = getRandomSeed();
            initLevel(randomSeedGame);
            needRedraw = 1;
        }
        else
        {
            if (selectedLevel > 1)
            {
                playMenuSelectSound();
                selectedLevel--;
                initLevel(randomSeedGame);
                needRedraw = 1;
            }
        } 
    }
    if (gb.buttons.released(BUTTON_RIGHT))
    {
        if (difficulty == diffRandom)
        {
            playMenuSelectSound();
            //need new seed based on time
            randomSeedGame = getRandomSeed();
            initLevel(randomSeedGame);
            needRedraw = 1;
        }
        else
        {
            if (selectedLevel < maxLevel)
            {
                playMenuSelectSound();
                selectedLevel++;
                initLevel(randomSeedGame);
                needRedraw = 1;
            }
        }
    }
}
