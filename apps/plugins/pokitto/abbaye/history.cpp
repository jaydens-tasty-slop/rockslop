/* history.c */

#include "history.h"

void history(uint8_t *state)
{
    rect hysrcjean = {320, 88, 16, 24};
    rect hydesjean = {0, 85, 16, 24};
    rect hysrcenem = {64, 64, 16, 24};
    rect hydesenem = {0, 85, 16, 24};
    float hyposjean = -16;
    float hyposenem[4] = {-17, -17, -17, -17};
    uint8_t animation = 0;
    uint8_t musicload = 0;
    uint8_t exit = 0;

    //LOOP
    while (Pokitto::Core::isRunning() && exit != 1)
    {
        if (Pokitto::Core::update())
        {
            /* Play music at start */
            if (musicload == 0)
            {
                musicload = 1;
                Mix_PlayMusic(9);
            }

            /* Show text */
            Pokitto::Display::setColor(15);
            Pokitto::Display::print(10,10,msg_history_1);
            Pokitto::Display::print(10,120,msg_history_2);

            for(int i=0; i<3; i++)
            {
                /* Animation control */
                if (animation < 13)
                    animation++;
                else
                    animation = 0;

                /* Jean running */
                if (hyposjean < 257)
                {
                    hyposjean += 0.75;
                    hydesjean.x = hyposjean;
                    hysrcjean.x = 320 + ((animation / 7) * 16); /* Walking animation */
                    hysrcjean.y = 88;		/* 8 or 16 bits sprite */
                    if(i==0)
                        SDL_RenderCopyEx(&hysrcjean, &hydesjean, 1);
                }

                /* Crusaders running */
                /* When start running */
                for (uint16_t i = 0; i < 4; i++)
                {
                    if (hyposjean > (35 + (30 * i)))
                        hyposenem[i] += 0.65;
                }
            }

            /* Draw */
            for (uint16_t i = 0; i < 4; i++)
            {
                if ((hyposenem[i] > -17) && (hyposenem[i] < 257))
                {
                    hydesenem.x = hyposenem[i];
                    hysrcenem.x = 64 + ((animation / 7) * 16);
                    hysrcenem.y = 64 ;
                    SDL_RenderCopyEx(&hysrcenem, &hydesenem, 1);
                }
            }

            /* Check keyboard */
            if (Pokitto::Buttons::pressed(BTN_A))
            {
                /* Start game */
                exit = 1;
                *state = 2;
            }
            if (hyposenem[3] > 256)
            {
                /* Ending history */
                exit = 1;
                *state = 2;
            }

        }
    }
}
