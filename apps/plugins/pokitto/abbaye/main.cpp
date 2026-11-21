/* Abbaye des Morts */
/* Version 2.0 */

/* (c) 2010 - Locomalito & Gryzor87 */
/* 2013 - David "Nevat" Lara */

/* GPL v3 license */

#include "main.h"

int main()
{
    uint8_t stateinit = 0;
    uint8_t state = 0;   /* 0-intro,1-history,2-game,3-gameover,4-ending,5-exit */

    Pokitto::Core::begin();
    Pokitto::Display::setFont(fontAdventurer);
    Pokitto::Display::load565Palette(palette);
    Pokitto::Display::setInvisibleColor(0);
    Pokitto::Display::bgcolor = 0;
    Pokitto::Core::setFrameRate(30);

    initsd();

    while(true)
    {
        switch (state)
        {
        case 0:
            startscreen(&state);
            break;
        case 1:
            history(&state);
            break;
        case 2:
        {
            int vol = Pokitto::Sound::getVolume();
            Pokitto::Sound::setVolume(0); // prevent an awful sound
            initrooms();
            Pokitto::Sound::setVolume(vol);
            game(&state);
            break;
        }
        case 3:
            gameover(&state);
            break;
        case 4:
            ending(&state);
            break;
        case 6:
            break;
        }
    }

    closesd();

    /* Exiting normally */
    return 0;
}
