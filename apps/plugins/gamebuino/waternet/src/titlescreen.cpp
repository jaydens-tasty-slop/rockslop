#include <stdint.h>
#include <Gamebuino-Meta.h>

#include "../res/fulltitlescreenmap.h"
#include "commonvars.h"
#include "helperfuncs.h"
#include "sound.h"
#include "printfuncs.h"
#include "savestate.h"
#include "level.h"
#include "palettes.h"

void drawTitleScreen()
{
    //gb.display.clear(INDEX_BLACK);
    gb.display.drawImage(0, 0, fullTitlescreenMap);

    switch (titleStep)
    {
        case tsMainMenu:
            printMessage(6, 8, F("MAIN MENU"));
            printMessage(7, 10, F("START"));
            printMessage(7, 11, F("HELP"));
            printMessage(7, 12, F("OPTIONS"));
            printMessage(7, 13, F("CREDITS"));
            break;
        case tsDifficulty:
            printMessage(6, 8, F("VERY EASY"));
            printMessage(6, 9, F("EASY"));
            printMessage(6, 10, F("NORMAL"));
            printMessage(6, 11, F("HARD"));
            printMessage(6, 12, F("VERY HARD"));
            printMessage(6, 13, F("RANDOM"));
            break;
        case tsGameMode:
            if (mainMenu == mmStartGame)
                printMessage(4, 8, F("SELECT  MODE"));
            else
                printMessage(6, 8, F("MODE HELP"));
            printMessage(7, 10, F("ROTATE"));
            printMessage(7, 11, F("SLIDE"));
            printMessage(7, 12, F("ROSLID"));
            break;
        case tsCredits:
            printMessage(7, 8, F("CREDITS"));            
            printMessage(5, 10, F("CREATED BY"));
            printMessage(4, 11, F("WILLEMS DAVY"));
            printMessage(4, 12, F("JOYRIDER3774"));
            break;
        case tsOptions:
            printMessage(6, 8, F("OPTIONS"));            
         
            if(isMusicOn())
                printMessage(5, 10, F("MUSIC  ON"));
            else
                printMessage(5, 10, F("MUSIC  OFF"));

            if(isSoundOn())
                printMessage(5, 11, F("SOUND  ON"));
            else
                printMessage(5, 11, F("SOUND  OFF"));
            
            printMessage(5, 12, getPaletteName());
            
            if(getPalInverse())
                printMessage(5, 13, F("INVERT ON"));
            else
                printMessage(5, 13, F("INVERT OFF"));
            break;
    }

    //set menu tile
    switch (titleStep)
    {
        case tsMainMenu:
            set_bkg_tile_xy(6, 10 + mainMenu, leftMenu);
            break;
        case tsGameMode:
            set_bkg_tile_xy(6, 10 + gameMode, leftMenu);
            break;
        case tsDifficulty:
            set_bkg_tile_xy(5, 8 + difficulty, leftMenu);
            break;
        case tsOptions:
            set_bkg_tile_xy(4, 10 + option, leftMenu);
            break;
    }
}

void initTitleScreen()
{   
    setBlockTilesAsBackground();
    SelectMusic(musTitle);
    setPaletteTitle();
    needRedraw = 1;
}

void titleScreen()
{
    if(gameState == gsInitTitle)
    {
        initTitleScreen();
        gameState -= gsInitDiff;
    }
    
    if(needRedraw)
    {
        drawTitleScreen();
        needRedraw = 0;
    }

    if (gb.buttons.released(BUTTON_UP))
    {
        switch (titleStep)
        {
            case tsMainMenu:
                if(mainMenu > mmStartGame)
                {
                    playMenuSelectSound();
                    mainMenu--;
                    needRedraw = 1;
                }
                break;
            case tsGameMode:
                if(gameMode > gmRotate)
                {
                    playMenuSelectSound();
                    gameMode--;
                    needRedraw = 1;
                }
                break;
            case tsDifficulty:
                if(difficulty > diffVeryEasy)
                {
                    playMenuSelectSound();
                    difficulty--;
                    needRedraw = 1;
                }
                break;
            case tsOptions:
                if(option > opMusic)
                {
                    playMenuSelectSound();
                    option--;
                    needRedraw = 1;
                }
                break;
        }
    }

    if (gb.buttons.released(BUTTON_DOWN))
    {
        switch (titleStep) 
        {
            case tsMainMenu:
                if(mainMenu < mmCount-1)
                {
                    playMenuSelectSound();
                    mainMenu++;
                    needRedraw = 1;
                }
                break;
            case tsGameMode:
                if(gameMode < gmCount-1)
                {
                    playMenuSelectSound();
                    gameMode++;
                    needRedraw = 1;
                }
                break; 
            case tsDifficulty:
                if(difficulty < diffCount-1)
                {
                    playMenuSelectSound();
                    difficulty++;
                    needRedraw = 1;
                }
                break;
            case tsOptions:
                if(option < opCount-1)
                {
                    playMenuSelectSound();
                    option++;
                    needRedraw = 1;
                }
                break;
        }
    }

    if (gb.buttons.released(BUTTON_B))
    {
        switch (titleStep)
        {
            case tsOptions:
            case tsCredits:
                titleStep = tsMainMenu;
                playMenuBackSound();
                needRedraw = 1;
                break;
            case tsGameMode:
            case tsDifficulty:
                titleStep--;
                playMenuBackSound();
                needRedraw = 1;
                break;
        }
    }

    if (gb.buttons.released(BUTTON_A))
    {
        playMenuAcknowlege();
        switch(mainMenu)
        {
            case mmOptions:
                if(titleStep != tsOptions)
                {
                    titleStep = tsOptions;
                    needRedraw = 1;
                }
                else
                {
                    switch(option)
                    {
                        case opMusic:
                            setMusicOn(!isMusicOn());
                            setMusicOnSaveState(isMusicOn());
                            needRedraw = 1;
                            break;
                        case opSound:
                            setSoundOn(!isSoundOn());
                            setSoundOnSaveState(isSoundOn());
                            needRedraw = 1;
                            break;
                         case opColorInvert:
                            setPalInverse(!getPalInverse());
                            setInverseColorSaveState(getPalInverse());
                            needRedraw = 1;
                            break;
                        case opColor:
                            uint8_t i = getPalIndex();
                            if (i < getMaxPalettes())
                                i++;
                            else
                                i = 0;
                            setPalIndex(i);
                            setActiveColorSaveState(i);
                            needRedraw = 1;
                            break;
                    }
                }
                break;

            case mmCredits:
                if(titleStep != tsCredits)
                {
                    titleStep = tsCredits;
                    needRedraw = 1;
                }
                else
                {
                    titleStep = tsMainMenu;
                    needRedraw = 1;
                }
                break;

            case mmHelp:
                if (titleStep < tsGameMode)
                {
                    titleStep++;
                    needRedraw = 1;
                }
                else
                {
                    switch (gameMode)
                    {
                        case gmRotate:
                            gameState = gsInitHelpRotate;
                            break;
                        case gmSlide:
                            gameState = gsInitHelpSlide;
                            break;
                        case gmRotateSlide:
                            gameState = gsInitHelpRotateSlide;
                            break; 
                    }
                }
                break;

            case mmStartGame:
                if (titleStep < tsDifficulty)
                {
                    titleStep++;
                    needRedraw = 1;
                }
                else
                {
                    if (difficulty == diffRandom)
                        selectedLevel = 1;
                    else
                        selectedLevel = lastUnlockedLevel(gameMode, difficulty);
                    
                    if (gameMode == gmRotate)
                        posAdd = 0;
                    else
                        posAdd = 1;
                    //set randomseet to systime here
                    //it will be reused all the time
                    //with the level generating
                    //but not when going back from
                    //level playing to level selector
                    //when calling init level there
                    randomSeedGame = getRandomSeed();
                    initLevel(randomSeedGame);

                    gameState = gsInitLevelSelect;
                }  
                break;
        }
    }
}
