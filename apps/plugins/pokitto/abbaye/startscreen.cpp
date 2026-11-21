/* startscreen.c */
#include <stdio.h>
#include <stdlib.h>
#include "Pokitto.h"
#include "gfx/gfx.h"
#include "sdlpok.h"

void startscreen(uint8_t *state)
{
    uint8_t ssmusicplay = 0;
    bool showInfo=false;
    bool disableInput = true;
    bool redraw = true;
    uint8_t exit = 0;
    uint16_t flashoff=Pokitto::Display::palette[8];
    uint16_t flashon=Pokitto::Display::palette[15];
    int16_t flashtime=0;

    Pokitto::Display::persistence = 1;

    while (Pokitto::Core::isRunning() && exit != 1)
    {
        if (Pokitto::Core::update())
        {
            /* Put image on renderer */
            if(redraw)
            {
#ifdef USE_HIGH_COLOR
                loadBackgroundImage( showInfo ? "intro2_hc" : "intro1_hc" );
#else
                loadBackgroundImage( showInfo ? "intro2_lc" : "intro1_lc" );
#endif // USE_HIGH_COLOR
                redraw=false;
            }

            //Lightning effects
            if(random(60)==1 && flashtime<1 && !showInfo)
            {
                flashtime=1+random(5);
		Mix_PlaySound(7);
            }
            if(flashtime>0)
            {
                Pokitto::Display::palette[8]=flashon;
                flashtime--;
            }
            else
            {
                Pokitto::Display::palette[8]=flashoff;
            }
#ifndef POK_SIM
            if(flashtime==1){
                Pokitto::setBacklight(0.1f);
	    }else if(flashtime==2){
		Pokitto::setBacklight(1.0f);
	    }else
                Pokitto::setBacklight(POK_BACKLIGHT_INITIALVALUE);
#endif // POK_SIM


            /* Play music if required */
            if (ssmusicplay == 0)
            {
                ssmusicplay = 1;
                Mix_PlayMusic(8);
            }


            if (Pokitto::Core::buttons.pressed(BTN_B))
            {
                redraw = true;
                showInfo=!showInfo;
                Pokitto::Display::load565Palette(palette);
#ifndef POK_SIM
                Pokitto::setBacklight(1.0f);
#endif // POK_SIM
            }
            if (Pokitto::Core::buttons.pressed(BTN_A))
            {
                if( !disableInput )
                {
                    /* Start game */
                    exit = 1;
                    Pokitto::Display::persistence = 0;
                    *state = 1;
                    Pokitto::Display::load565Palette(palette);
#ifndef POK_SIM
                    Pokitto::setBacklight(POK_BACKLIGHT_INITIALVALUE);
#endif // POK_SIM
                }
            }
            else
            {
                disableInput = false;
            }
        }
    }
}
