/* game.h */

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "Pokitto.h"
#include "sdlpok.h"
#include "structs.h"
#include "sdsave.h"

void keybpause(uint8_t *keyp);
void music(struct roomnums *room, uint8_t *changeflag, int flag);
void changescreen(struct hero *jean, struct roomnums *room, uint8_t *changeflag);
void events(struct hero *jean, struct roomnums *room, uint8_t counter[], struct enem *enemies, uint16_t roomdata[26][35]);
void control(struct hero *jean);
void counters(uint8_t counter[]);
void animation(struct roomnums *room, uint8_t counter[], uint16_t roomdata[26][35]);

/* enemies */
void blueparchment(struct hero *jean);
void redparchment(struct hero *jean);
void satan(struct enem *enemies, uint8_t counter[], int16_t proyec[], bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void death(struct enem *enemies, uint8_t counter[], int16_t proyec[], uint16_t roomdata[26][35],bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void fireball(struct enem *enemies, uint8_t counter[], struct hero jean,uint16_t roomdata[26][35], bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void plants(struct enem *enemies, uint8_t counter[], int16_t proyec[], bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void dragon(struct enem *enemies, uint8_t counter[], int16_t proyec[], bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void crusaders(struct enem *enemies, uint8_t counter[], struct roomnums *room, bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void movenemies(struct enem *enemies, uint8_t counter[], int16_t proyec[], struct hero jean);
void drawenemies(struct enem *enemies, bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void searchenemies(struct roomnums *room, struct enem *enemies, uint8_t *changeflag);

/* drawing */
void drawrope(struct enem enemies, bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void drawshoots(int16_t proyec[], struct enem *enemies, bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void drawscreen(struct roomnums *room, uint8_t counter[], uint8_t changeflag, bool skipdraw, uint16_t roomdata[26][35],int16_t cameraoffsetx,int16_t cameraoffsety);
void statusbar(struct roomnums *room, int lifes, int crosses, bool skipdraw);
void showparchment(uint8_t *parchment);

/* loading */
//void loadingmusic(Mix_Music *bso[],Mix_Chunk *fx[]);
//void loaddata(uint8_t stagedata[][22][32],int enemydata[][7][15]);

/* jean */
void drawjean(struct hero *jean, uint8_t counter[], bool skipdraw,int16_t cameraoffsetx,int16_t cameraoffsety);
void movejean(struct hero *jean);
void touchobj(struct hero *jean, struct roomnums *room, uint8_t *parchment, uint8_t *changeflag, struct enem *enemies, int16_t proyec[], uint16_t roomdata[26][35]);
void contact(struct hero *jean, struct enem enemies, int16_t proyec[], struct roomnums *room);
void collisions(struct hero *jean, struct roomnums *room, uint16_t roomdata[26][35]);

void pausemenu();
const char *msg_hell_mode[] = {
    "Hell mode enabled",
    "....",
    "Sit on your",
    "left hand,",
    "close your right",
    "eye",
    "and then..",
    "try finish",
    "this way now!",
    "!!!MUAHAHA!!!",
};
