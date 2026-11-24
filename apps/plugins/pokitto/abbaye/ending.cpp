/* ending.c */

#include "Pokitto.h"
#include "sdlpok.h"
#include <stdio.h>
#include <stdlib.h>
#include "structs.h"
#include "gfx/gfx.h"

const char* msg_ending_1 = "Your body has burned";
const char* msg_ending_2 = "in the flames,";
const char* msg_ending_3 = "but your soul has found";
const char* msg_ending_4 = "a place in Heaven";

void ending(uint8_t *state)
{

    //TODO:music
    //Mix_Music *bso = Mix_LoadMUS(DATADIR "/sounds/PrayerofHopeN.ogg");
    //Mix_PlayMusic(bso, 0);

    int16_t x = 0;
    int16_t i = 0;

    while (Pokitto::Core::isRunning() && *state!=0 )
    {
        if (Pokitto::Core::update())
        {
            if ( i < 951)
            {
                Pokitto::Display::clear();

                Pokitto::Display::setColor(15);//text color
                Pokitto::Display::print(30,20,msg_ending_1);
                Pokitto::Display::print(60,40,msg_ending_2);
                Pokitto::Display::print(25,125,msg_ending_3);
                Pokitto::Display::print(45,145,msg_ending_4);

                if (i > 90)
                    x = (i-90) / 15;
                if(x > 5)
                    x=5;

                if(i>90)
                    Pokitto::Display::drawBitmap(80, 70, gfxendingdoor[x]);
            }
            else
            {
                *state = 0;
            }
            i++;
        }
    }

}
