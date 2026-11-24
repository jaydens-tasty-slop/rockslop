#include <stdint.h>

#include "helperfuncs.h"
#include "commonvars.h"
#include "printfuncs.h"
#include "sound.h"
#include "level.h"
#include "palettes.h"

#include "../res/congratsscreentiles.h"
#include "../res/congratsscreenmap.h"

void initLevelsCleared()
{
    setPaletteTitle();
    set_bkg_data(&congratsScreenTiles);
    gb.display.clear(INDEX_BLACK);
    gb.display.drawImage(0, 0, congratsMap);
    switch (difficulty)
    {
        case diffVeryEasy:
            printCongratsScreen(2, 7, F("VERY EASY LEVELS"));
            break;
        
        case diffEasy:
            printCongratsScreen(4, 7, F("EASY LEVELS"));
            break;
    
        case diffNormal:
            printCongratsScreen(3, 7, F("NORMAL LEVELS"));
            break;

        case diffHard:
            printCongratsScreen(4, 7, F("HARD LEVELS"));
            break;
    
        case diffVeryHard:
            printCongratsScreen(2, 7, F("VERY HARD LEVELS"));
            break;
    }
    SelectMusic(musAllLevelsClear);
}

void levelsCleared() 
{  
    if(gameState == gsInitLevelsCleared)
    {
        initLevelsCleared();
        gameState -= gsInitDiff;
    }

    if (gb.buttons.released(BUTTON_A) || 
        gb.buttons.released(BUTTON_B)) 
    {
        playMenuAcknowlege();
        titleStep = tsMainMenu;
        gameState = gsInitTitle;
    }
}