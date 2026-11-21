#include <stdint.h>
#include <Gamebuino-Meta.h>
#include "helperfuncs.h"
#include "commonvars.h"
#include "printfuncs.h"
#include "palettes.h"

#include "../res/titlescreenmap.h"
#include "../res/blocktiles.h"

uint16_t frames;
float titlePosY;
constexpr uint8_t frameDelay = 16 * frameRate / 15;    

void initIntro()
{
    setPaletteTitle();
    setBlockTilesAsBackground();
    titlePosY = gb.display.height();
    frames = 0;   
}

void intro() 
{
    if (gameState == gsInitIntro)
    {
        initIntro();
        gameState -= gsInitDiff;
    }
    
    frames++;
    gb.display.clear(INDEX_BLACK);
    if (frames < frameDelay)
    {
        //20-12
        printMessage(8 >> 1, 7, F("WILLEMS DAVY"));
    }
    else
    {
        if (frames < frameDelay *2)
        {
            //20-8
            printMessage(12 >> 1, 7, F("PRESENTS"));
        }
        else
        {
            gb.display.drawImage(0, (uint16_t)titlePosY, titlescreenMap);
            if(titlePosY > 0)
            {
                titlePosY -= (float)60/(float)frameRate;
            }
            else
            {
                gameState = gsInitTitle;
            }
        }
    }
    
    if (gb.buttons.released(BUTTON_A) || 
        gb.buttons.released(BUTTON_B)) 
    {            
        gameState = gsInitTitle;
    }
    if(gb.buttons.released(BUTTON_UP))
    {
        debugMode = 1;
    }
}
