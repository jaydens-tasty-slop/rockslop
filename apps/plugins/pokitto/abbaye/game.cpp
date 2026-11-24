/* game.c */

#include "game.h"
// #define DEBUG_SATAN

void game(uint8_t *state)
{
    uint16_t roomdata[26][35];
    struct roomnums room = {5, 5}; /* Room, previous room */
    uint8_t exit = 0;
    uint8_t changeflag = 1;			/* Screen change */
    uint8_t counter[3] = {0, 0, 0}; /* Counters */
    int16_t proyec[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; /* Enemiess shoots */
    uint8_t parchment = 0;
    uint8_t n = 0, invincible = 0;
    int16_t cameraoffsetx=0;
    int16_t cameraoffsety=0;
    bool skipdraw=false;


    /* Sounds */
    //TODO:Music
    //Mix_Music *bso[8];
    //Mix_Chunk *fx[7];
    /* Loading musics */
    //loadingmusic(bso,fx);

#ifdef DEBUG_SATAN
    room.current = 24;
    room.previous = 24;
    invincible = 1;
#endif

    /* Load data */
    //loaddata(stagedata, enemydata);
    loadroom(room.current,roomdata);

    /* Init struct */
    struct enem enemies =
    {
        .type = {0, 0, 0, 0, 0, 0, 0},
        .x = {0, 0, 0, 0, 0, 0, 0},
        .y = {0, 0, 0, 0, 0, 0, 0},
        .direction = {0, 0, 0, 0, 0, 0, 0},
        .tilex = {0, 0, 0, 0, 0, 0, 0},
        .tiley = {0, 0, 0, 0, 0, 0, 0},
        .animation = {0, 0, 0, 0, 0, 0, 0},
        .limleft = {0, 0, 0, 0, 0, 0, 0},
        .limright = {0, 0, 0, 0, 0, 0, 0},
        .speed = {0, 0, 0, 0, 0, 0, 0},
        .fire = {0, 0, 0, 0, 0, 0, 0},
        .adjustx1 = {0, 0, 0, 0, 0, 0, 0},
        .adjustx2 = {0, 0, 0, 0, 0, 0, 0},
        .adjusty1 = {0, 0, 0, 0, 0, 0, 0},
        .adjusty2 = {0, 0, 0, 0, 0, 0, 0}
    };

    struct hero jean =
    {
        .x = 72,
        .y = 136,
        .direction = 1,
        .jump = 0,
        .height = 0,
        .animation = 0,
        .gravity = 1.9,
        .points = {0, 0, 0, 0, 0, 0, 0, 0},
        .ground = 0,
        .collision = {0, 0, 0, 0},
        .ducking = 0,
        .checkpoint = {5, 5, 72, 136},
        .state = {9, 0},
        .flags = {0, 0, 0, 0, 0, 0, 0},
        .death = 0,
        .push = {0, 0, 0, 0},
        .temp = 0
    };

#ifdef DEBUG_SATAN
    jean.state[1] = 12;
#endif

    /* Game loop */
    while (Pokitto::Core::isRunning() && exit != 1)
    {
        if (Pokitto::Core::update())
        {
            for(int i=0; i<3; i++)
            {
                skipdraw=(i!=0);

#ifdef PROJ_DEVELOPER_MODE
                // for debug
                if (Pokitto::Buttons::timeHeld(BTN_C) > 0)
                {
                    if(Pokitto::Buttons::timeHeld(BTN_UP) == 1)
                    {
                        jean.state[1] = 12;
                        room.previous = room.current;
                        room.current += 1;
                        changeflag = 1;
                    }
                    else if(Pokitto::Buttons::timeHeld(BTN_DOWN) == 1)
                    {
                        room.previous = room.current;
                        room.current -= 1;
                        changeflag = 1;
                    }
                }
#else
                // in release version use C for pause
                if (Pokitto::Buttons::pressed(BTN_C)&&!skipdraw)
                {
                    pausemenu();
                }
#endif

                //Manage camera shift to allow see all level
                cameraoffsetx=0;

                if(jean.x>64)
                    cameraoffsetx=(64-jean.x);
                if(cameraoffsetx<-32)
                    cameraoffsetx=-32;

                if(jean.y>120)
                    cameraoffsety=-8;
                if(jean.y<80)
                    cameraoffsety=0;

                /* Rockbox port: the LCD shows a window that follows Jean */
                Pokitto::Display::setView((int)jean.x + cameraoffsetx + 8, (int)jean.y + cameraoffsety + 8);

                /* Manage counters */
                counters(counter);

                /* Animation of fire and water */
                animation(&room, counter, roomdata);

                /* Draw screen */
                drawscreen(&room, counter, changeflag, skipdraw, roomdata,cameraoffsetx,cameraoffsety);

                /* Draw statusbar */
                if (room.current != 4)
                    statusbar(&room, jean.state[0], jean.state[1], skipdraw);

                /* Draw Jean */
                if (jean.flags[6] < 8)
                    drawjean(&jean, counter, skipdraw,cameraoffsetx,cameraoffsety);

                /* Enemies */
                if (enemies.type[0] > 0)
                {
                    if (room.current != 4)
                        movenemies(&enemies, counter, proyec, jean);
                    if ((room.current == 5) || (room.current == 6))
                        crusaders(&enemies, counter, &room, skipdraw,cameraoffsetx,cameraoffsety);
                    if (room.current == 10)
                        dragon(&enemies, counter, proyec, skipdraw,cameraoffsetx,cameraoffsety);
                    if (room.current == 11)
                        fireball(&enemies, counter, jean,roomdata, skipdraw,cameraoffsetx,cameraoffsety);
                    if (room.current == 14)
                        plants(&enemies, counter, proyec, skipdraw,cameraoffsetx,cameraoffsety);
                    if (room.current == 9)
                        drawrope(enemies, skipdraw,cameraoffsetx,cameraoffsety);
                    if (room.current == 18)
                        death(&enemies, counter, proyec,roomdata,skipdraw,cameraoffsetx,cameraoffsety);
                    if ((room.current == 24) && (enemies.type[0] == 18))
                        satan(&enemies, counter, proyec, skipdraw,cameraoffsetx,cameraoffsety);
                    if ((room.current == 24) && (jean.flags[6] == 5))
                        crusaders(&enemies, counter, &room, skipdraw,cameraoffsetx,cameraoffsety);

                    drawenemies(&enemies, skipdraw,cameraoffsetx,cameraoffsety);
                }

                /* Shoots */
                if ((proyec[0] > 0) && ((room.current == 17) || (room.current == 20) || (room.current == 21) || (room.current == 22)))
                    drawshoots(proyec, &enemies, skipdraw,cameraoffsetx,cameraoffsety);

                /* Jean management */
                if( invincible )
                    jean.death = 0;

                if (jean.death == 0)
                {
                    if (jean.flags[6] < 5)
                    {
                        if (jean.temp == 0)
                            control(&jean);
                        if (jean.temp == 1)
                            jean.temp = 0;
                        collisions(&jean, &room, roomdata);
                        movejean(&jean);
                    }
                    if (room.current != 4)
                    {
                        touchobj(&jean, &room, &parchment, &changeflag, &enemies, proyec, roomdata);
                        contact(&jean, enemies, proyec, &room);
                    }
                    events(&jean, &room, counter, &enemies, roomdata);
                }

                /* Jean death */
                if (jean.death == 98)
                {
                    if (room.current != 4)
                    {
                        room.previous = room.current;
                        room.current = jean.checkpoint.room;
                        jean.x = jean.checkpoint.x;
                        jean.y = jean.checkpoint.y;
                        jean.jump = 0;
                        jean.height = 0;
                        jean.push[0] = 0;
                        jean.push[1] = 0;
                        jean.push[2] = 0;
                        jean.push[3] = 0;
                        changeflag = 2;
                        jean.state[0]--;
                        jean.death = 0;
                        jean.temp = 1;
                        music(&room, &changeflag, jean.flags[6]);
                    }
                    else
                    {
                        jean.death = 0;
                        jean.flags[6] = 8;
                    }
                }
                /* Using flag 6 as counter, to make time */
                if (jean.flags[6] > 7)
                    jean.flags[6]++;
                /* Reaching 15, jump to ending sequence */
                if (jean.flags[6] == 15)
                {
                    *state = 4;
                    exit = 1;
                }
                changescreen(&jean, &room, &changeflag);
                if (changeflag > 0)
                {
                    //Save room data
                    saveroom(room.previous,roomdata);

                    //Load room data
                    if( room.current != room.previous )
                        loadroom(room.current, roomdata);

                    if ((jean.flags[6] < 4) || (jean.flags[6] > 5))
                        searchenemies(&room, &enemies, &changeflag);
                    music(&room, &changeflag, jean.flags[6]);
                    for (n = 0; n < 24; n++)
                    {
                        /* Reset enemyshoots */
                        proyec[n] = 0;
                    }
                    counter[0] = 0;
                    changeflag = 0;
                }
                /* Parchments */
                if (parchment > 0)
                    showparchment(&parchment);
                if (jean.flags[6] == 3)
                {
                    redparchment(&jean);
                    parchment = 1;
                }
                if (jean.flags[6] == 6)
                {
                    blueparchment(&jean);
                    parchment = 1;
                    invincible = 0;
                }

                if (parchment > 0)
                {
                    Mix_PlaySound(2);

                    /* Stop all */
                    Pokitto::Display::update();
                    Pokitto::Core::wait(100);
                    Mix_PauseMusic();
                    Pokitto::Buttons::pollButtons();

                    /* Waiting a key */
                    while(!Pokitto::Buttons::aBtn())
                    {
                        Pokitto::Core::wait(100);
                        Pokitto::Buttons::pollButtons();
                        parchment = 0;
                        jean.push[2] = 0;
                        jean.push[3] = 0;
                    }

                    Mix_ResumeMusic();

                }
                if (jean.flags[6] == 4)
                {
                    Mix_PlaySound(2);
                    jean.flags[6] = 5;
                    jean.direction = 0;
                    music(&room, &changeflag, jean.flags[6]);
                }
                if (jean.flags[6] == 6)
                {
                    Pok_sleep(5);
                    jean.death = 0;
                    changeflag = 1;
                    room.previous = room.current;
                    room.current = 4;
                    jean.flags[6] = 7;
                    jean.x = 125;
                    jean.y = 115;
                    jean.jump = 1;
                }

                // let devs have infinite lives
#ifndef PROJ_DEVELOPER_MODE

                if (jean.state[0] == 0)
                {
                    Mix_HaltMusic();
                    /* Mix_FreeMusic(sonido); */
                    *state = 3;
                    exit = 1;
                }
#endif

                /* For debugging only (remove space->)* /

                if( !skipdraw ){
                    Pokitto::Display::setCursor(0,0);
                    Pokitto::Display::print( room.current );
                    Pokitto::Display::setCursor(30,0);
                    Pokitto::Display::print( room.previous );
                }

                /* */

            }
        }
    }
}

void animation(struct roomnums *room, uint8_t counter[], uint16_t roomdata[26][35])
{
    uint16_t data = 0;
    for (int8_t i = 0; i < 22; i++)
    {
        for (int8_t j = 0; j < 32; j++)
        {
            data = roomdata[i][j];

            /* Fire animation */
            if ((data == 59) || (data == 60))
            {
                if ((counter[0] == 1) || (counter[0] == 11) || (counter[0] == 21))
                {
                    if (data == 59)
                        data = 60;
                    else
                        data = 59;
                }
            }

            /* Water animation */
            if ((data > 500) && (data < 533))
            {
                if (data < 532)
                    data++;
                else
                    data = 501;
            }

            roomdata[i][j] = data;
        }
    }
}

void counters(uint8_t counter[])
{

    if (counter[0] < 29)
        counter[0]++;
    else
        counter[0] = 0;

    if (counter[1] < 90)
        counter[1]++;
    else
        counter[1] = 0;

    if (counter[2] < 8)
        counter[2]++;
    else
        counter[2] = 0;
}

void control(struct hero *jean)
{
    jean->ducking = 0;
    if (Pokitto::Buttons::timeHeld(BTN_B) > 0 || Pokitto::Buttons::downBtn())
    {
        if ((jean->jump == 0) && (jean->ducking == 0))
        {
            jean->push[1] = 1;
            jean->ducking = 1;
        }
    }

    //if (Pokitto::Buttons::aBtn())
    if (Pokitto::Buttons::pressed(BTN_A))
    {
        if ((jean->jump == 0) && (jean->ducking == 0))
            jean->jump = 1;
    }
    if (Pokitto::Buttons::released(BTN_A))
    {
        //jean->jump = 0;
    }

    jean->push[0] = 0;
    jean->push[1] = 0;
    jean->push[2] = 0;
    jean->push[3] = 0;
    if (Pokitto::Buttons::leftBtn())
    {
        jean->push[2] = 1;
        jean->push[3] = 0;
    }
    if (Pokitto::Buttons::rightBtn())
    {
        jean->push[3] = 1;
        jean->push[2] = 0;
    }

}

void events(struct hero *jean, struct roomnums *room, uint8_t counter[], struct enem *enemies, uint16_t roomdata[26][35])
{
    int8_t x = 0;
    int8_t y = 0;

    if (room->current == 4)
    {
        if (jean->temp < 7)
        {
            /* Moving Jean */
            if (counter[1] == 45)
            {
                switch (jean->direction)
                {
                case 0:
                    jean->direction = 1;
                    break;
                case 1:
                    jean->direction = 0;
                    break;
                }
                jean->temp++;
            }
        }
        else
        {
            jean->direction = 0;
            jean->death = 1;
        }
    }

    if (room->current == 7)
    {
        /* Close door */
        if ((jean->x > 24) && (jean->flags[0] == 0))
        {
            roomdata[14][0] = 347;
            roomdata[15][0] = 348;
            roomdata[16][0] = 349;
            roomdata[17][0] = 350;
            jean->flags[0] = 1;
            Mix_PlaySound(1);
        }
    }

    if (room->current == 8)
    {
        /* Open ground */
        if ((jean->x > 15) && (jean->flags[1] == 1) && (roomdata[20][26] == 7))
        {
            roomdata[20][26] = 38;
            roomdata[20][27] = 0;
            roomdata[21][26] = 0;
            roomdata[21][27] = 0;
            Mix_PlaySound(1);
        }
        /* Open door */
        if ((jean->x > 211) && (jean->flags[2] == 1) && (roomdata[14][31] == 343))
        {
            roomdata[14][31] = 0;
            roomdata[15][31] = 0;
            roomdata[16][31] = 0;
            roomdata[17][31] = 0;
            Mix_PlaySound(1);
        }
    }
    if (room->current == 10)
    {
        /* Dragon fire kills Jean */
        if (((jean->x > 127) && (jean->x < 144)) && (jean->y < 89))
        {
            if ((enemies->speed[0] > 109) && (enemies->speed[0] < 146))
                jean->death = 1;
        }
    }
    if (room->current == 19)
    {
        /* Open door */
        if ((jean->y > 16) && (jean->flags[3] == 1) && (roomdata[16][0] == 347))
        {
            roomdata[16][0] = 0;
            roomdata[17][0] = 0;
            roomdata[18][0] = 0;
            roomdata[19][0] = 0;
            Mix_PlaySound(1);
        }
    }
    if (room->current == 20)
    {
        /* Open door */
        if ((jean->x > 208) && (jean->flags[4] == 1) && (roomdata[14][31] == 343))
        {
            roomdata[14][31] = 0;
            roomdata[15][31] = 0;
            roomdata[16][31] = 0;
            roomdata[17][31] = 0;
            Mix_PlaySound(1);
        }
    }

    if (room->current == 24)
    {
        if ((jean->state[1] >= 12) && (jean->x > 8) && (jean->flags[6] == 0))
        {
            /* Block entry */
            roomdata[16][0] = 99;
            roomdata[17][0] = 99;
            roomdata[18][0] = 99;
            roomdata[19][0] = 99;
            jean->flags[6] = 1;
            /* Mark checkpoint */
            jean->checkpoint.room = 24;
            jean->checkpoint.x = jean->x;
            jean->checkpoint.y = jean->y;
        }
        if ((jean->flags[6] == 1) && (jean->state[1] > 0) && (counter[0] == 0))
        {
            /* Putting crosses */
            switch (jean->state[1])
            {
            case 1:
                x = 11;
                y = 5;
                break;
            case 2:
                x = 10;
                y = 5;
                break;
            case 3:
                x = 8;
                y = 8;
                break;
            case 4:
                x = 5;
                y = 8;
                break;
            case 5:
                x = 4;
                y = 8;
                break;
            case 6:
                x = 2;
                y = 12;
                break;
            case 7:
                x = 5;
                y = 14;
                break;
            case 8:
                x = 6;
                y = 14;
                break;
            case 9:
                x = 9;
                y = 14;
                break;
            case 10:
                x = 10;
                y = 14;
                break;
            case 11:
                x = 13;
                y = 13;
                break;
            case 12:
                x = 15;
                y = 16;
                break;
            }
            roomdata[y][x] = 84;
            jean->state[1]--;
            Mix_PlaySound(1);
        }
        if ((jean->flags[6] == 1) && (jean->state[1] == 0) && (counter[0] == 29))
        {
            /* Draw cup */
            roomdata[3][15] = 650;
            jean->flags[6] = 2;
            Mix_PlaySound(1);
        }
        /* Killed Satan, Smoke appears */
        if (enemies->type[0] == 88)
        {
            if (enemies->speed[0] < 90)
                enemies->speed[0]++;
            else
            {
                enemies->speed[0] = 0;
                enemies->type[0] = 0;
                enemies->x[0] = 0;
                enemies->y[0] = 0;
                enemies->type[0] = 17;
                /* Putting red parchment */
                roomdata[14][28] = 339;
                roomdata[14][29] = 340;
                roomdata[15][28] = 341;
                roomdata[15][29] = 342;
            }
        }
    }
}

void music(struct roomnums *room, uint8_t *changeflag, int flag)
{
    if (room->current == 1)
    {
        Mix_HaltMusic();
        Mix_PlayMusic(0);
    }
    if ((room->current == 2) && (room->previous == 1))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(1);
    }
    if (room->current == 4)
    {
        Mix_HaltMusic();
        Mix_PlayMusic(2);
    }
    if ((room->current == 5) /* && (room->previous != 6) */ )
    {
        Mix_HaltMusic();
        Mix_PlayMusic(7);
    }
    if ((room->current == 6) && (room->previous == 7))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(7);
    }
    if ((room->current == 7) && (room->previous == 6))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(1);
    }
    if (((room->current == 8) && (room->previous == 9)) || ((room->current == 8) && (*changeflag == 2)))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(1);
    }
    if (room->current == 9)
    {
        Mix_HaltMusic();
        Mix_PlayMusic(3);
    }
    if (((room->current == 11) && (room->previous == 12)) || ((room->current == 11) && (*changeflag == 2)))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(4);
    }
    if (((room->current == 12) && (room->previous == 11)) || ((room->current == 12) && (*changeflag == 2)))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(1);
    }
    if ((room->current == 13) && (room->previous == 14))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(1);
    }
    if (((room->current == 14) && (room->previous == 13)) || ((room->current == 14) && (*changeflag == 2)))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(4);
    }
    if ((room->current == 15) && (*changeflag == 2))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(4);
    }
    if ((room->current == 16) && (room->previous == 17))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(4);
    }
    if ((room->current == 17) && (room->previous == 16))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(1);
    }
    if (room->current == 18)
    {
        Mix_HaltMusic();
        Mix_PlayMusic(5);
    }
    if (((room->current == 19) && (room->previous == 18)) || ((room->current == 19) && (*changeflag == 2)))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(4);
    }
    if ((room->current == 20) && (room->previous == 21))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(4);
    }
    if ((room->current == 21) && (room->previous == 20))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(6);
    }
    if ((room->current == 23) && (room->previous == 24))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(6);
    }
    if ((room->current == 24) && (flag != 5))
    {
        Mix_HaltMusic();
        Mix_PlayMusic(5);
    }
    if ((room->current == 24) && (flag == 5))
        Mix_PlayMusic(7);

    *changeflag -= 1;
}

void changescreen(struct hero *jean, struct roomnums *room, uint8_t *changeflag)
{
    if ((jean->x < 1) && (room->current != 5))
    {
        room->previous = room->current;
        room->current -= 1;
        jean->x = 240;
        *changeflag = 1;
    }
    if ((jean->x + 8) > 256)
    {
        room->previous = room->current;
        room->current += 1;
        jean->x = 1;
        *changeflag = 1;
    }
    if ((jean->y + 12 < -16) && (jean->jump == 1))
    {
        room->previous = room->current;
        room->current -= 5;
        jean->y = 152;
        *changeflag = 1;
    }
    if ((jean->y > 175) && (jean->jump != 1))
    {
        room->previous = room->current;
        room->current += 5;
        jean->y = -16;
        *changeflag = 1;
    }
}

void pausemenu()
{
    int anim=0;
    char keysbuff[10]= {0,0,0,0,0,0,0,0,0,0};
    const char keyssecret[10]= {BTN_UP,BTN_UP,BTN_DOWN,BTN_DOWN,BTN_LEFT,BTN_RIGHT,BTN_LEFT,BTN_RIGHT,BTN_B,BTN_A};
    int msgseq=0;

    while (Pokitto::Core::isRunning())
    {
        if (Pokitto::Core::update())
        {
            //capture keys
            bool oksequence=true;
            for (int b=0; b<NUM_BTN; b++)
            {
                if(Pokitto::Buttons::pressed(b))
                {
                    for(int f=9; f>0; f--)
                    {
                        keysbuff[f]=keysbuff[f-1];
                    }
                    keysbuff[0]=b;
                    msgseq=0;
                }
            }
            //Check sequence
            for(int i=0; i<10; i++)
            {
                if(keysbuff[i]!=keyssecret[9-i])
                    oksequence=false;
            }
            //sequence ok
            if(oksequence)
            {
                Pokitto::Display::setColor(1);
                for(int m=0; m<msgseq+1; m++)
                {
                    Pokitto::Display::setCursor(60,10+m*16);
                    Pokitto::Display::print(msg_hell_mode[m]);
                }

                rect s1;
                rect s2 = {232,88,24,24};
                rect d1 = {10,6,32,24};
                rect d2 = {10,30,24,24};

                if(anim<10)
                {
                    s1 = {256,88,32,24};
                    SDL_RenderCopyEx(&s1,&d1,1);
                }
                else
                {
                    s1 = {192,88,32,24};
                    SDL_RenderCopyEx(&s1,&d1,1);
                }
                SDL_RenderCopyEx(&s2,&d2,1);
            }
            else
            {
                loadBackgroundImage("parchmentpause");
            }

            anim++;
            if(anim>20)
            {
                anim=0;
                if(msgseq<9)
                    msgseq++;
            }

            if (Pokitto::Buttons::pressed(BTN_C))
            {
                break;
            }

        }
    }
}
