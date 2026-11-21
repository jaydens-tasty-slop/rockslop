#pragma once

#include "Pokitto.h"
#include "Compat.h"
#include "GameCookie.h"
#include "Game.h"
#include "Strings.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

#define WEMAX   8     //9
#define ARMAX   7     //8
#define POMAX   14    //14
#define SCMAX   12    //14
#define RGMAX   11    //14
#define WDMAX   10    //14
#define RMAX    2
#define IMAX    26
#define TMAX    40
#define MMAX    16


struct Score {        //my first 'structure'
  uint16_t gold;
  uint8_t depth;
};

struct Hero {
  uint8_t hx;
  uint8_t hy;
  uint8_t im;
  uint8_t dlv;
  uint8_t st;
  uint8_t stm;
  uint8_t lv;
  uint8_t hconf;
  uint8_t hblnd;
  uint8_t hhall;
  uint8_t hfast;
  uint8_t hslep;
  uint8_t hmdet;
  uint8_t hisee;
  uint8_t hlevi;
  uint8_t hheld;
  char rdex;
  char rstr;
  uint16_t au;
  uint16_t hp;
  uint16_t hpm;
  uint16_t ex;
  uint16_t ht;
  uint16_t hh;
  uint16_t nl;
};

struct item {
  uint8_t ii;
  uint8_t i1;
  char i2;
  char i3;
  uint8_t i4;
};

class Game {
    
    public:

        void setup(GameCookie *cookie);
        void loop(void);

        // Actions

        void eat(uint8_t r);
        void wield(uint8_t r);
        void ringPut(uint8_t r, char i);
        void drink(uint8_t r);
        void readScroll(uint8_t r);
        void zap(uint8_t vari);
        uint8_t equip(uint8_t type, uint8_t n);   //type=3(weapon),4(armor),7(ring), n=1 or 2(for ring)...rtab[equip(7,2)]
        uint8_t hasRing(uint8_t vari);
        int checkHit(uint8_t dir, uint8_t str);
        uint8_t findPlace(uint8_t x, uint8_t y, uint8_t tm);   //tm=0:thing, 1:monst


        // Invent

        uint8_t inputWait();
        void drawInventry(uint8_t st, uint8_t mode);
        uint8_t inventry(uint8_t mode);
        uint8_t action(uint8_t st);
        void showStatus();
        void search();


        // Attack

        void hitMonst(uint8_t mm, uint8_t r, char dx, char dy); //mm=monst[x][y](1 to 16) r=kind(0 to 25
        void checkMonst(uint8_t m, uint8_t r, uint8_t dmg, uint8_t x, uint8_t y); //m=id(0...15), r=vari, dmg, hitpos x,y
        void hitHero(uint8_t i, uint8_t r); //i=kind(0 to 25 r=ID(0 to 15)
        void specialAttack(uint8_t mon, uint8_t id);  //mon=0 to 25 mon vari, @Pharas sharp eye

        // Game

        void title();
        void landing();
        void gameover();
        uint8_t digits(uint16_t num);
        void winner();
        void score();
        uint8_t checkHiScore();
        void gameloop();
        void buildDungeon();
        void heroMove(uint8_t dir);
        void traped(uint8_t vari);
        void charon(uint8_t dmg, uint8_t reason);
        void rustArmor(uint8_t eq);

        // Sub
        void setKnown(uint8_t x, uint8_t y);
        uint8_t getKnown(uint8_t x, uint8_t y);
        void clearKnown();
        void allKnown();
        void makeKnown();
        void clearDungeon();
        void locate(int x, int y);
        void drawMap();
        void drawHero();
        void drawMonst();
        void drawMonstSub(uint8_t i);
        uint8_t canBless();
        void moveMonst();
        void placeMonst();
        void generateMon(uint8_t m);
        void placeMonXY( uint8_t i, uint8_t x, uint8_t y);
        void showMsg();
        void placeThing();
        void placeThingXY(uint8_t ix, uint8_t iy, uint8_t r);
        void drawThing();
        void checkThing(uint8_t x, uint8_t y);
        void deleteThing(uint8_t i);
        uint8_t initState(uint8_t mon);
        void tweatHero();


        // Message
        void setActiveMessage(uint8_t id);
        void itmToGitm(uint8_t type, uint8_t r, uint8_t k);
        const char * getItemName(uint8_t type, uint8_t r, uint8_t k);

        // Flash
        void flashHero(char sym);
        void flashMonst(uint8_t num, char sym);
        void initHero();
        void initTable();


        // thing
        uint8_t randThing();
        uint8_t randPS(uint8_t i);
        uint8_t freeSlot();
        uint8_t sortItem();
        uint8_t findScroll(uint8_t t);
        void dropItem(uint8_t x, uint8_t y, uint8_t st);
        void useItem(uint8_t st);
        uint8_t randMonst(uint8_t d);
        void wakeUp();
        uint8_t strToDmg(uint8_t str);
        void deleteItem(uint8_t i);
        void throwItem(uint8_t i);
        void dropThrown(uint8_t x, uint8_t y, uint8_t fi, uint8_t f1, char f2, char f3, uint8_t f4);
        void hitPotion(uint8_t vari, uint8_t mon);
        void hitWand(uint8_t vari, uint8_t mon, uint8_t x, uint8_t y);
        uint8_t askDir();

        // Temp3

        void box(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t kind);
        void teleportHero();
        void makeRoom(uint8_t i);
        void makeDungeon4();
        void buildStructure(uint8_t room, uint8_t structure);

        // EEPROM
        void saveHiScore();
        void loadHiScore();
        void clearHiScore();
        void saveStatus();
        void loadStatus();

    private:

        GameCookie *cookie;

        Score glory[5];
        Score tglory;

        const char tsym[9] = {'*', ':', ')', '[', '!', '?', '=', '/', ','};
        const char tprob[9] = {0, 10, 15, 24, 33, 63, 93, 96, 100};

        const char psprob[2][POMAX + 1] = {
        {0, 5, 10, 20, 25, 35, 40, 50, 60, 65, 75, 85, 90, 95, 100},
        {0, 5, 10, 15, 20, 35, 45, 55, 65, 75, 85, 90,100,100, 100}
        //  {0, 0, 0, 0, 0, 100, 100, 100, 100, 100, 100, 100,100,100, 100}
        };

        const char mname[26] = {
        'K', 'E', 'B', 'S', 'H', 'I', 'R', 'O',
        'Z', 'L', 'C', 'Q', 'A', 'N', 'Y', 'F',
        'T', 'W', 'P', 'X', 'U', 'M', 'V', 'G',
        'J', 'D'
        };

        const char mstat[26][7] = {
        //  hp,  r+XdY, dex, exp,item, state
        { 10,  1,  4,  60,   2,   0, 0b10000011},  // 0 K  76543210
        { 11,  1,  3,  65,   2,   0, 0b00000010},  // 1 E         wake
        { 10,  1,  3,  60,   2,   0, 0b00100101},  // 2 B        sleep (mimic is sleep and deep sleep)
        {  8,  1,  3,  50,   2,   0, 0b00000011},  // 3 S       deep sleep
        { 15,  2,  5,  67,   3,   0, 0b00000011},  // 4 H      NC
        { 15,  0,  0,  68,   5,   0, 0b00000100},  // 5 I     special attack(for medusa's grace)
        { 19,  2, 10,  70,  10,   0, 0b00000011},  // 6 R    wanderer
        { 25,  1,  5,  70,   5,  10, 0b00000011},  // 7 O   transparent
        { 21,  1,  7,  69,   8,   0, 0b00000011},  // 8 Z  fast
        { 25,  0,  0,  75,  21, 100, 0b00000100},  // 9 L
        { 32,  5, 19,  85,  15,  10, 0b00000101},  //10 C
        { 30,  3, 15,  78,  20,  20, 0b00000011},  //11 Q
        { 25,  0,  0, 100,  20,   0, 0b00000011},  //12 A
        { 25,  0,  0,  75,  39, 100, 0b00000100},  //13 N
        { 35,  3, 18,  80,  50,  20, 0b00000101},  //14 Y
        { 73,  0,  0,  80,  91,   0, 0b00000001},  //15 F
        { 75,  5, 28,  75, 125,  33, 0b00000011},  //16 T
        { 45,  2, 16,  75,  55,   0, 0b00000101},  //17 W
        { 76,  5, 20,  80, 120,  50, 0b01100101},  //18 P
        { 42,  4, 24,  75, 110,   0, 0b00000110},  //19 X
        { 90,  4, 40,  85, 200,  33, 0b00000011},  //20 U
        { 97,  7, 37,  85, 250,  25, 0b00000011},  //21 M
        { 55,  2, 18,  85,   4,  18, 0b00000011},  //22 V 350
        {115, 10, 50,  85,  20,  10, 0b10000011},  //23 G 2000
        {132,  7, 50, 100,  30,   0, 0b00000101},  //24 J 3000
        {145,  8, 60, 100,  50,  90, 0b00000010}   //25 D 5000
        };
        const uint8_t wstat[WEMAX * 2] = {
        2, 3 ,  3, 4 ,  4, 5 ,  1, 1 ,  1, 1 ,  1, 2 , 1, 3 , 1, 4 
        };
        const uint8_t astat[ARMAX] = {
        2, 3, 4, 5, 6, 6, 7
        };


        uint8_t dungeon[21][8];
        uint8_t known[3][8];
        uint8_t monst[21][8];
        uint8_t thing[21][8];


        uint8_t death=0;

        uint8_t rank=0;
        Hero hero={0,0,5,1,16,16,1,0,0,0,0,0,0,0,0,0,0,0,0,16,16,0,0,400,10};

        uint8_t ss = 0;      //show status flag
        char roomSX[RMAX * 2] = {}, roomSY[RMAX * 2] = {}, roomEX[RMAX * 2] = {}, roomEY[RMAX * 2] = {};
        uint8_t hasRoom[RMAX * 2] = {};
        uint8_t isDark[RMAX * 2] = {};
        uint8_t isBigRoom=0;
        uint8_t mx[MMAX] = {}, my[MMAX] = {}, ms[MMAX] = {}, mh[MMAX] = {}, m1[MMAX] = {}, m2[MMAX] = {};

        uint8_t ttab[4][POMAX] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}
        };

        item inv[IMAX]={};
        item tng[TMAX]={};
        item ftng={};

        unsigned int tknow[4] = {0, 0, 0, 0}; //0:potion, 1:scroll, 2:ring, 3:wand

        uint8_t adepth=26, wiz=0;

        const char * activeMessage = nullptr;

        //char gbuf[22];
        char gmon[10];
        //char gitm[8];
        char gmes[15];

        uint8_t gstate = 0;

};