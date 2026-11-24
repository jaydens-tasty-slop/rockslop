/* drawing.c */

#include "drawing.h"


void drawscreen(struct roomnums *room, uint8_t counter[], uint8_t changeflag, bool skipdraw,uint16_t roomdata[26][35],int16_t cameraoffsetx,int16_t cameraoffsety)
{
    if(skipdraw)
        return;

    rect srctiles = {0, 0, 8, 8};
    rect destiles = {0, 0, 8, 8};

    for (uint8_t coordy = 0; coordy <= 21; coordy++)
    {
        for (uint8_t coordx = 0; coordx <= 31; coordx++)
        {
            uint16_t data = roomdata[coordy][coordx];
            if ((data > 0) && (data != 99))
            {
                destiles.x = coordx * 8 + cameraoffsetx;
                destiles.y = coordy * 8 + cameraoffsety;
                if (data < 200)
                {
                    srctiles.w = 8;
                    srctiles.h = 8;
                    if (data < 101)
                    {
                        srctiles.y = 0;
                        if (data == 84) /* Cross brightness */
                            srctiles.x = (data - 1) * 8 + (counter[0] / 8 * 8);
                        else
                            srctiles.x = (data - 1) * 8;
                    }
                    else
                    {
                        if (data == 154)
                        {
                            srctiles.x = 600 + ((counter[0] / 8) * 16);
                            srctiles.y = 0;
                            srctiles.w = 16;
                            srctiles.h = 24;
                        }
                        else
                        {
                            srctiles.y = 8;
                            srctiles.x = (data - 101) * 8;
                        }
                    }
                }
                if ((data > 199) && (data < 300))
                {
                    srctiles.x = (data - 201) * 48;
                    srctiles.y = 16;
                    srctiles.w = 48;
                    srctiles.h = 48;
                }
                if ((data > 299) && (data < 399))
                {
                    srctiles.x = 96 + ((data - 301) * 8);
                    srctiles.y = 16;
                    srctiles.w = 8;
                    srctiles.h = 8;
                    /* Door movement */
                    if ((room->current == 7) && ((counter[1] > 59) && (counter[1] < 71)))
                    {
                        if ((data == 347) || (data == 348) || (data == 349) || (data == 350))
                        {
                            destiles.x += 2;
                            if ((data == 350) && (counter[1] == 70))
                                Mix_PlaySound(6); /* Sound of door */
                        }
                    }
                }
                /* Hearts */
                if ((data > 399) && (data < 405))
                {
                    srctiles.x = 96 + ((data - 401) * 8) + (32 * (counter[0] / 15));
                    srctiles.y = 24;
                    srctiles.w = 8;
                    srctiles.h = 8;
                }
                /* Crosses */
                if ((data > 408) && (data < 429))
                {
                    srctiles.x = 96 + ((data - 401) * 8) + (32 * (counter[1] / 23));
                    srctiles.y = 24;
                    srctiles.w = 8;
                    srctiles.h = 8;
                }

                if ((data > 499) && (data < 599))
                {
                    srctiles.x = 96 + ((data - 501) * 8);
                    srctiles.y = 32;
                    srctiles.w = 8;
                    srctiles.h = 8;
                }
                if ((data > 599) && (data < 650))
                {
                    srctiles.x = 96 + ((data - 601) * 8);
                    srctiles.y = 56;
                    srctiles.w = 8;
                    srctiles.h = 8;
                }
                if (data == 650)
                {
                    /* Cup */
                    srctiles.x = 584;
                    srctiles.y = 87;
                    srctiles.w = 16;
                    srctiles.h = 16;
                }
                destiles.w = srctiles.w;
                destiles.h = srctiles.h;
                if ((data == 152) || (data == 137) || (data == 136))
                {
                    if (changeflag == 0)
                    {
                        srctiles.y = srctiles.y;
                        SDL_RenderCopySolid(&srctiles, &destiles);
                    }
                }
                else
                {
                    srctiles.y = srctiles.y;
                    SDL_RenderCopySolid(&srctiles, &destiles);
                }
            }
        }
    }
}

void statusbar(struct roomnums *room, int lifes, int crosses, bool skipdraw)
{
    if(skipdraw)
        return;

    rect srcbar = {448, 104, 13, 12};
    rect desbar = {0, 163, 13, 12};

    /* Show heart and crosses sprites */
    SDL_RenderCopy(&srcbar, &desbar);
    srcbar.x = 461;
    srcbar.w = 12;
    desbar.x = 32;
    SDL_RenderCopy(&srcbar, &desbar);

    Pokitto::Display::setColor(15);
    for (uint8_t i = 0; i <= 2; i++)
    {
        switch (i)
        {
        case 0:
            Pokitto::Display::print(15,163,lifes);
            break;
        case 1:
            Pokitto::Display::print(45,163,crosses);
            break;
        case 2:
            if ((room->current > 0) && (room->current < 4))
                Pokitto::Display::print(70,163,msg_levels[room->current-1]);
            if (room->current > 4)
                Pokitto::Display::print(70,163,msg_levels[room->current-2]);
            break;
        }
    }
}

void drawrope(struct enem enemies, bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety)
{
    if(skipdraw)
        return;

    rect srctile = {424, 8, 16, 8};
    rect destile = {0, 0, 16, 8};

    for (uint8_t i = 2; i < 6; i++)
    {
        int16_t blocks = (enemies.y[i] - (enemies.limleft[i] - 8)) / 8;
        for (uint8_t j = 0; j <= blocks; j++)
        {
            srctile.y = 8;
            destile.x = enemies.x[i]+cameraoffsetx;
            destile.y = (enemies.limleft[i] - 8) + (8 * j) + cameraoffsety;
            SDL_RenderCopy(&srctile, &destile);
        }
    }
}

void drawshoots(int16_t proyec[], struct enem *enemies, bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety)
{
    /* Shoots from skeletons & gargoyles */

    rect srctile = {656, 24, 16, 8};
    rect destile = {0, 0, 0, 0};

    srctile.y = 24;

    for (uint8_t n = 0; n <= 4; n += 2)
    {
        if (proyec[n] > 0)
        {
            uint8_t i = proyec[n + 1];
            if (enemies->type[i] == 15)
            {
                srctile.h = 16;
                srctile.x = 640 - (16 * enemies->direction[i]);
            }

            /* Move shoot */
            if (enemies->direction[i] == 1)
            {
                if (proyec[n] > enemies->limleft[i])
                    proyec[n] -= skipdraw * 2; // 2.5;
                else
                {
                    enemies->fire[i] = 0;
                    enemies->speed[i] = 0;
                    proyec[n] = 0;
                }
            }
            else
            {
                if (proyec[n] < enemies->limright[i])
                    proyec[n] += skipdraw * 2; // 2.5;
                else
                {
                    enemies->fire[i] = 0;
                    enemies->speed[i] = 0;
                    proyec[n] = 0;
                }
            }
            destile.w = srctile.w;
            destile.h = srctile.h;

            /* Draw shoot */
            if(!skipdraw)
            {
                switch (enemies->direction[i])
                {
                case 0:
                    if ((proyec[n] < (enemies->limright[i] - 8)) && (proyec[n] != 0))
                    {
                        destile.x = proyec[n]+cameraoffsetx;
                        destile.y = enemies->y[i] + 8 +cameraoffsety;
                        SDL_RenderCopy(&srctile, &destile);
                    }
                    break;
                case 1:
                    if (proyec[n] > (enemies->limleft[i] + 8))
                    {
                        destile.x = proyec[n]+cameraoffsetx;
                        destile.y = enemies->y[i] + 8 + cameraoffsety;
                        SDL_RenderCopy(&srctile, &destile);
                    }
                    break;
                }
            }
        }
    }
}

void showparchment(uint8_t *parchment)
{
    switch (*parchment)
    {
    case 3:
        loadBackgroundImage("parchment1");
        break;
    case 8:
        loadBackgroundImage("parchment2");
        break;
    case 12:
        loadBackgroundImage("parchment3");
        break;
    case 14:
        loadBackgroundImage("parchment4");
        break;
    case 16:
        loadBackgroundImage("parchment5");
        break;
    case 21:
        loadBackgroundImage("parchment6");
        break;
    }
}

void redparchment(struct hero *jean)
{
    loadBackgroundImage("redparch");
    jean->flags[6] = 4;
}

void blueparchment(struct hero *jean)
{
    loadBackgroundImage("blueparch");
}
