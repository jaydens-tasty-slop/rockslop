#include<avr/io.h> 
#include <Arduboy2.h>
#include <EEPROM.h>
Arduboy2 arduboy;

// sprites game
const unsigned char PROGMEM sprite[] = {8,8, 
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x55,0x00,0xAA,0x00,0x55,0x00,0xAA,0x00,
 0x18,0x7E,0x52,0xFF,0xDB,0x56,0x76,0x18,
 0x00,0x77,0x77,0x77,0x00,0x77,0x77,0x77,
 0x0C,0x1E,0x39,0xF9,0xFF,0x3F,0x1E,0x0C,
 0x0C,0x1E,0x3F,0xFF,0xC7,0x27,0x1E,0x0C,
 0x00,0x7E,0x42,0x5A,0x5A,0x42,0x7E,0x00,
 0xFF,0x81,0xBD,0xA5,0xA5,0xBD,0x81,0xFF,
 0xA0,0x92,0x55,0x7F,0x7F,0x55,0x92,0x88,
 0x88,0x92,0x55,0x7F,0x7F,0x55,0x92,0xA0,
 0x00,0x04,0x04,0xFF,0x04,0x04,0x00,0x00,
};

const int adresEEPROM = 503;
byte level;
byte grot[8][16];
byte fly_X[4];
byte fly_Y[4];
byte richting[4];
byte vlieg;
unsigned long knipper;
unsigned long vliegen;
bool spriteINV;
bool spriteON;
byte knop;
byte mapX;
byte mapY;
byte diamant;
byte knopB = 0;
byte spel;

enum game {start,nieuw_spel,dash,dood,next};
const byte game[152][16]PROGMEM = {
 
 {1,1,1,1,1,2,2,1,1,1,1,2,2,2,1,1}, // Level 1
 {1,1,2,1,1,2,1,2,1,1,1,4,4,4,1,1},
 {1,1,4,1,1,4,1,1,2,1,1,1,1,1,1,1},
 {1,1,1,1,1,4,1,1,1,2,1,1,1,1,1,1},
 {1,1,1,1,1,4,1,1,1,2,2,2,1,2,1,1}, 
 {1,2,2,1,1,1,1,1,1,1,1,1,1,4,1,1},
 {1,2,2,1,1,1,1,8,1,1,1,1,1,4,1,1},
 {4,4,4,1,1,1,1,1,1,1,1,1,1,4,1,1},

 {1,2,1,1,1,1,1,1,1,2,2,1,1,1,1,1}, // Level 2
 {1,1,1,1,1,1,2,2,1,4,4,1,1,1,8,1}, 
 {1,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1},
 {1,4,4,4,1,1,2,1,1,3,1,1,1,1,1,1},
 {1,4,4,4,1,1,2,1,1,3,1,2,2,2,1,1},
 {1,1,1,1,1,1,1,1,1,3,1,1,2,2,1,1},
 {1,1,1,1,1,1,1,1,1,3,1,1,4,4,1,1},
 {1,1,1,1,2,2,1,1,1,3,1,1,1,1,1,1},
 
 {1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1}, // Level 3
 {1,1,4,4,4,1,1,0,1,2,2,2,2,1,1,1},
 {1,1,4,2,2,1,1,0,1,4,4,4,4,1,1,1},
 {1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1},
 {1,1,1,1,1,0,1,1,1,1,1,1,2,4,1,1}, 
 {1,1,1,0,6,0,1,1,1,1,2,2,2,4,1,1},
 {1,1,1,0,1,1,1,1,8,1,1,4,2,1,1,1},
 {1,1,1,0,1,1,1,1,1,1,1,1,2,1,1,1},
 
 {1,1,1,1,4,3,4,3,3,3,1,1,4,3,3,4}, // Level 4
 {1,3,3,3,3,3,1,1,1,1,1,3,3,1,1,1},
 {1,3,1,1,1,3,1,3,3,3,3,3,1,1,3,1},
 {1,1,1,3,1,3,1,3,1,1,1,1,1,3,1,1},
 {1,3,1,3,1,1,1,1,1,3,3,3,3,3,4,3}, 
 {1,3,1,3,3,3,3,3,1,1,1,4,3,4,3,3},
 {1,3,1,1,1,1,3,3,3,1,3,3,3,1,3,4},
 {8,3,4,3,3,4,3,4,1,1,1,1,1,1,1,1},

 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // Level 5
 {1,2,2,2,2,2,1,1,1,1,2,1,1,1,1,1},
 {1,4,4,4,4,4,1,1,2,1,2,1,1,3,3,1},
 {1,1,1,1,1,1,1,1,4,1,4,1,1,3,3,1},
 {1,0,6,0,1,1,1,1,4,1,4,1,1,1,1,1}, 
 {1,0,0,0,1,1,1,1,4,1,4,1,1,1,1,1},
 {1,4,0,0,1,1,1,1,4,1,1,1,2,2,2,1},
 {1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,1},
 
 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // Level 6
 {1,1,1,1,1,1,1,1,1,1,0,0,6,0,1,1},
 {1,1,0,6,0,0,1,1,1,1,4,4,4,4,1,1},
 {1,1,4,4,4,4,1,1,1,1,1,1,1,1,1,1},
 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 
 {1,1,0,6,0,0,1,1,1,1,3,3,2,3,1,1},
 {1,1,4,4,4,4,1,1,1,1,3,1,1,3,1,1},
 {1,1,1,1,1,1,1,1,1,1,3,8,1,3,1,1},

 {2,2,1,1,1,2,4,2,4,4,1,1,1,1,2,2}, // Level 7
 {4,4,1,2,1,1,1,1,1,1,1,1,1,1,4,4},
 {1,1,1,4,3,3,3,3,3,3,3,2,1,1,1,1},
 {1,1,1,4,3,1,1,1,2,2,3,4,1,1,8,1},
 {1,1,1,4,3,4,4,4,4,4,3,4,1,1,1,1},
 {1,1,1,1,3,1,1,1,1,1,3,4,1,1,1,1},
 {1,2,2,1,3,3,3,1,3,3,3,1,2,2,2,1},
 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},

 {1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1}, // Level 8
 {1,3,3,3,3,3,1,4,1,1,3,3,3,3,3,1},
 {1,3,6,0,0,3,1,4,1,1,1,6,0,0,3,1},
 {1,3,0,0,0,3,1,4,1,1,3,0,0,0,3,1},
 {1,3,0,0,4,3,1,4,1,1,3,4,4,4,3,1}, 
 {1,3,1,3,3,3,1,1,1,1,3,3,3,3,3,1},
 {1,1,1,1,1,1,1,1,1,1,1,4,2,2,2,1},
 {1,1,1,1,1,1,8,1,1,1,1,1,4,4,4,1},

 {2,1,1,1,3,4,4,4,4,4,4,3,1,1,1,2}, // Level 9
 {4,2,1,1,1,3,4,4,4,4,3,1,1,1,2,4},
 {2,4,2,1,1,1,3,4,4,3,1,1,1,2,4,4},
 {2,2,4,2,1,1,1,1,1,1,1,1,2,4,4,4},
 {2,2,2,4,2,1,1,1,1,1,1,2,4,4,4,4}, 
 {4,4,4,4,4,4,1,1,1,1,4,4,4,4,4,4},
 {1,1,1,1,1,1,1,8,1,1,1,1,1,1,1,1},
 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
 
 {4,4,4,4,4,4,4,3,3,4,4,4,4,4,4,4}, // Level 10
 {3,4,4,4,4,4,3,1,1,3,4,4,4,4,4,3},
 {1,3,4,4,4,3,1,1,1,1,3,4,4,4,3,1},
 {1,1,3,1,3,1,1,0,0,1,1,3,1,3,1,1},
 {0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0}, 
 {0,0,0,0,1,0,0,4,4,0,0,1,0,0,0,0},
 {0,0,0,1,0,0,4,4,4,4,0,0,1,0,0,0},
 {8,0,1,0,0,4,4,4,4,4,4,0,6,1,0,0},

 {1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1}, // Level 11
 {3,4,3,1,3,4,3,1,3,4,3,1,3,4,3,1},
 {3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1},
 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
 {1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1},
 {3,4,3,1,3,4,3,1,3,4,3,1,3,4,3,1},
 {3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1},
 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8},

 {1,2,1,1,1,1,1,8,1,1,1,1,1,2,2,2}, // Level 12
 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
 {1,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2},
 {1,1,1,1,3,2,2,2,2,2,3,1,1,1,1,1},
 {1,2,1,1,3,4,4,4,4,4,3,1,1,1,1,2},
 {1,2,1,1,3,1,1,1,1,1,3,1,1,1,1,2},
 {1,1,1,1,3,3,1,1,1,3,3,1,1,4,1,1},
 {1,1,1,1,4,4,1,1,4,4,4,1,1,4,1,1},

 {0,0,0,0,4,3,4,3,3,3,0,0,4,3,3,4}, // Level 13
 {0,3,3,3,3,3,1,0,0,0,0,3,3,0,0,1},
 {0,3,0,0,0,3,0,3,3,3,3,3,0,0,3,0},
 {0,0,0,3,0,3,0,3,0,0,0,0,0,3,0,0},
 {0,3,0,3,0,0,0,0,0,3,3,3,3,3,4,3}, 
 {0,3,0,3,3,3,3,3,0,0,0,4,3,4,3,3},
 {0,3,0,0,0,0,3,3,3,0,3,3,3,1,3,4},
 {8,3,4,3,3,4,3,4,6,0,0,0,0,0,0,0},

 {2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,2}, // Level 14
 {4,4,4,4,4,1,1,1,1,1,4,4,4,4,4,3},
 {4,4,4,4,4,3,1,1,1,1,4,4,4,4,3,1},
 {4,4,4,4,3,1,1,3,1,1,4,4,4,3,1,1},
 {4,4,4,3,1,1,1,1,3,1,4,4,3,1,1,2}, 
 {4,4,3,1,1,8,1,1,1,1,4,3,1,1,2,4},
 {4,3,1,1,1,1,1,1,1,1,3,1,1,4,4,4},
 {3,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4},

 {1,1,1,1,1,1,3,3,1,1,1,1,1,1,1,1}, // Level 15
 {1,1,1,1,3,3,4,4,3,3,1,1,1,1,1,1},
 {1,1,3,3,4,4,1,1,4,4,3,3,1,1,1,1},
 {3,3,4,1,1,1,4,4,1,1,1,3,2,3,3,3},
 {3,3,4,0,0,0,1,1,0,0,0,1,1,1,1,3}, 
 {1,1,3,3,6,0,0,0,0,0,3,3,3,3,1,3},
 {1,1,1,1,3,3,4,4,3,3,1,1,1,3,1,3},
 {1,1,1,1,1,1,3,3,1,1,1,1,1,3,1,8},

 {2,2,1,1,1,3,1,1,1,1,1,3,1,1,2,2}, // Level 16
 {4,1,1,1,3,4,3,1,1,1,3,4,3,1,1,4},
 {1,1,1,3,4,4,4,3,1,3,4,4,4,3,1,1},
 {1,1,3,4,4,2,4,4,3,4,4,2,4,4,3,1},
 {1,1,3,2,3,3,3,1,3,2,3,3,3,1,3,1}, 
 {1,1,3,1,1,3,1,1,3,1,1,3,1,1,3,1},
 {1,1,3,1,1,3,1,1,3,1,1,3,1,1,3,1},
 {8,1,1,1,1,3,1,1,1,1,1,3,1,1,1,1},

 {0,0,0,0,0,0,3,0,0,0,0,0,6,0,0,0}, // Level 17
 {0,1,4,2,1,0,1,0,1,4,4,1,2,4,1,0},
 {0,1,1,1,1,0,3,0,1,1,1,1,1,1,1,0},
 {0,0,6,0,0,0,3,0,0,0,0,4,4,0,0,0},
 {3,1,3,3,3,3,3,3,3,1,3,3,3,3,3,3}, 
 {0,6,4,4,0,0,3,0,0,0,0,0,0,8,0,6},
 {0,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0},
 {0,0,0,0,0,0,3,0,0,0,4,4,0,0,0,0},

 {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // Level 18
 {1,0,0,0,0,0,0,0,0,0,0,0,0,0,6,1},
 {1,4,4,4,1,4,4,1,1,4,1,4,4,4,1,1},
 {1,4,1,1,1,4,4,1,1,4,1,4,1,1,4,1},
 {1,4,4,4,1,4,1,4,1,4,1,4,1,1,4,1},
 {1,4,1,1,1,4,1,1,4,4,1,4,1,1,4,1}, 
 {1,4,4,4,1,4,1,1,4,4,1,4,4,4,1,1},
 {8,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},

 {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, // 19: Title screen
 {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
 {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
 {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
 {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
 {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
 {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4},
 {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

void lees_knoppen();
void welkom();
void info();
void bestuur_man();
void bestuur_fly();
void vallen();
void next_level();
void teken_achtergrond();
void startpositie();
void laad_level(byte level2);
void hervatSpel();
void EEPROM_schrijf (int adres_eeprom, byte data_eeprom);
byte EEPROM_lees (int adres_eeprom);

void setup() {
 arduboy.begin();
 arduboy.setFrameRate(8);
 knipper = millis();
 vliegen = millis();
 spel = start;
}

void loop() {
 if(!(arduboy.nextFrame())) {return;}
 arduboy.clear();
  lees_knoppen();
  switch (spel) {
  case start: 
    if (arduboy.notPressed(B_BUTTON) == true){ knopB = 1;}
   welkom();
   hervatSpel();
   laad_level(level);
   break;
  case nieuw_spel:
    if (arduboy.notPressed(B_BUTTON) == true){ knopB = 2;} 
   info();
   level = 1;
   EEPROM_schrijf(adresEEPROM+2,0x01);
   laad_level(level);
   break;
  case dash:
    if (arduboy.notPressed(B_BUTTON) == true){ knopB = 0;}
   bestuur_man();
   vallen();
   bestuur_fly();
   teken_achtergrond();
   if (diamant == 0) {spel =  next;}
   break;
  case dood:
   teken_achtergrond();
   break;
  case next:
   next_level();
   break;
 }
 arduboy.display();
}

// Input buttons. If you're on the edge of the cave, the button doesn't work
void lees_knoppen() { 
 if (arduboy.pressed(DOWN_BUTTON))  {if (mapY < 7)  {knop = 1;}}
 if (arduboy.pressed(UP_BUTTON))    {if (mapY > 0)  {knop = 2;}}
 if (arduboy.pressed(RIGHT_BUTTON)) {if (mapX < 15) {knop = 3;}}
 if (arduboy.pressed(LEFT_BUTTON))  {if (mapX > 0)  {knop = 4;}}
 if (arduboy.pressed(A_BUTTON))     {knop = 0; laad_level(level); spel = dash;}
 if (arduboy.pressed(B_BUTTON))     {knop = 0;
  if (knopB == 0) {spel = start;} 
  if (knopB == 1) {spel = nieuw_spel;}
  if (knopB == 2) {spel = dash;}
  } 
}

// Openings screen
void welkom() {
 arduboy.clear();
 laad_level(19);
 teken_achtergrond();
  arduboy.setTextSize(2);
  arduboy.setCursor(10, 12);
  arduboy.print("Tiny Dash");
  arduboy.setTextSize(1);
  arduboy.setCursor(10, 30);
  arduboy.print("A continu old game");
  arduboy.setCursor(10, 39);
  arduboy.print("B new game");
  arduboy.setCursor(2, 57);
  arduboy.print("code:Frank van de Ven");
}

// Print game explanation on the screen
void info() {
 arduboy.clear();
  arduboy.setCursor(0, 05);
  arduboy.print("Collect the diamonds");
  Sprites::drawOverwrite(120, 05, sprite,4);
  arduboy.setCursor(0, 15);
  arduboy.print("in the cave with");
  Sprites::drawOverwrite(100, 15, sprite,8);
  arduboy.setCursor(0, 25);
  arduboy.print("by digging sand  away");
  Sprites::drawOverwrite(92, 25, sprite,1);
  arduboy.setCursor(0, 35);
  arduboy.print("Falling stones   and");
  Sprites::drawOverwrite(89, 35, sprite,2);
  arduboy.setCursor(0, 45);
  arduboy.print("diamonds   or the fly");
  Sprites::drawOverwrite(50, 45, sprite,4);
  arduboy.setCursor(10, 55);
  arduboy.print("can kill you ( ).");
  Sprites::drawOverwrite(0, 55, sprite,6);
  Sprites::drawOverwrite(93, 55, sprite,10);
}

// Controls the man on the screen and moves him. also takes care of moving a rock
void bestuur_man() {
 switch (knop) {
 case 1:
  knop = 0;
  if (grot[mapY+1][mapX] == 0 || grot[mapY+1][mapX] == 1) {
   grot[mapY][mapX] = 0; grot[mapY+1][mapX] = 8; mapY++;
  break;}                                                       
  if (grot[mapY+1][mapX] == 4 || grot[mapY+1][mapX] == 5) {
       grot[mapY][mapX] = 0; grot[mapY+1][mapX] = 8; mapY++;
       diamant--;
  break;}   
  break;
 case 2:
  knop = 0;
  if (grot[mapY-1][mapX] == 0 || grot[mapY-1][mapX] == 1) {
   grot[mapY][mapX] = 0; grot[mapY-1][mapX] = 8; mapY--;
  break;}  
  if (grot[mapY-1][mapX] == 4 || grot[mapY-1][mapX] == 5) {
       grot[mapY][mapX] = 0; grot[mapY-1][mapX] = 8; mapY--;
       diamant--;
  break;} 
  break;
 case 3:
  knop = 0;
  if (grot[mapY][mapX+1] == 0 || grot[mapY][mapX+1] == 1) {
   grot[mapY][mapX] = 0; grot[mapY][mapX+1] = 8; mapX++;
  break;}  
  if (grot[mapY][mapX+1] == 4 || grot[mapY][mapX+1] == 5) {
       grot[mapY][mapX] = 0; grot[mapY][mapX+1] = 8; mapX++;
       diamant--;
   break;}  
   if (grot[mapY][mapX+1] == 2 && grot[mapY][mapX+2] == 0 && mapX < 14){
    grot[mapY][mapX] = 0; grot[mapY][mapX+1] = 8; grot[mapY][mapX+2] = 2;
    mapX++;
   break;} 
 break;
 case 4:
  knop = 0;
  if (grot[mapY][mapX-1] == 0 || grot[mapY][mapX-1] == 1) {
   grot[mapY][mapX] = 0; grot[mapY][mapX-1] = 8; mapX--;
  break;}  
  if (grot[mapY][mapX-1] == 4 || grot[mapY][mapX-1] == 5) {
       grot[mapY][mapX] = 0; grot[mapY][mapX-1] = 8; mapX--;
       diamant--;
  break;}  
  if (grot[mapY][mapX-1] == 2 && grot[mapY][mapX-2] == 0 && mapX > 1) {
    grot[mapY][mapX] = 0; grot[mapY][mapX-1] = 8; grot[mapY][mapX-2] = 2; 
    mapX--;
  break;}                                              
 break;
}}

// Control all Fly's 
// The route that is a fly moves is following the edge of a open spaces, turning clockwise.
void bestuur_fly(){
const byte X[8] = {1,2,1,0,1,2,1,0};
const byte Y[8] = {0,1,2,1,0,1,2,1};
const byte Z[8] = {3,0,1,2,3,0,2,3};
byte xx;
byte yy;
 if (millis() > (vliegen + 200)) {
  vliegen = millis();
  for (byte tel = 0; tel < vlieg; tel++) {
   for (byte tel2 = 0; tel2 < 4; tel2++){
    yy = fly_Y[tel]+Y[tel2+richting[tel]]-1;
    xx = fly_X[tel]+X[tel2+richting[tel]]-1;
    if (xx < 0 || xx > 15 || yy < 0 || yy > 7) {continue;}
     if (grot[yy][xx] == 0 || grot[yy][xx] == 8 || grot[yy][xx] == 9) {
      grot[yy][xx] = grot[fly_Y[tel]][fly_X[tel]];
      grot[fly_Y[tel]][fly_X[tel]] = 0;
      fly_Y[tel] = yy;
      fly_X[tel] = xx; 
      richting[tel] = Z[richting[tel]+tel2];
      break;
     }
   }
  if (mapX == xx && mapY == yy){grot[mapY][mapX] = 10; spel = dood; fly_Y[tel] = 100; fly_X[tel] = 100;}   
  }
 }
}

// Drop stones and diamonds. If the man is underneath, he is dead
void vallen() {
 for (int y = 6; y >= 0; y--) {
 for (int x = 15; x >= 0; x--) {
  if ((grot[y][x] == 4 || grot[y][x] == 5 || grot[y][x] == 2) && grot[y+1][x] == 0) {
   grot[y+1][x] = grot[y][x]; grot[y][x] = 0;
   if (mapY == y+2 && mapX == x) {grot[mapY][mapX] = 10; spel = dood;}
   } 
  if ((grot[y][x] == 4 || grot[y][x] == 5 || grot[y][x] == 2) && (grot[y][x-1] == 0 && grot[y+1][x-1] == 0) && x > 0) {
   grot[y+1][x-1] = grot[y][x]; grot[y][x] = 0;
   if (mapY == y+2 && mapX == x-1) {grot[mapY][mapX] = 10; spel  =  dood;}
   }
  if ((grot[y][x] == 4 || grot[y][x] == 5 || grot[y][x] == 2) && (grot[y][x+1] == 0 && grot[y+1][x+1] == 0) && x < 15) {
   grot[y+1][x+1] = grot[y][x]; grot[y][x] = 0;
   if (mapY == y+2 && mapX == x+1)  {grot[mapY][mapX] = 10; spel = dood;}
   }
 }} 
}

// When all diamonds have been cleared: next level
void next_level() {
 level++;
  arduboy.setCursor(10, 10);
   if (level < 19){
    arduboy.print("Level ");
    arduboy.print(level);
    spel =  dash;
  }
   else {
    level = 1;
    arduboy.print("Game Over");
    spel = start;
  }
  arduboy.display();
  EEPROM_schrijf(adresEEPROM+2,level);
  delay(2000);
 laad_level(level);
 knop = 0;
}

// Draw the background. Also provides animation man, diamond, fly
void teken_achtergrond(){
 if (millis() > (knipper + 100)) {
  knipper = millis();
  spriteINV ^= 1;
  spriteON = 1;}
 for (int y = 0; y < 8; y++) {
 for (int x = 0; x < 16; x++) {
  if ((spriteON && (grot[y][x] & 0x04)) == 1) {grot[y][x] ^= spriteINV;}
  if (grot[y][x] == 8) {
   grot[y][x] = grot[y][x] + ((y * 17 + x) & 0x01);}
  Sprites::drawOverwrite(x*8, y*8, sprite,grot[y][x]);
 }}
 spriteON = 0;
}

// Initialization: search the cave and determine position man (mapX, mapY) (sprite 8) 
// and count all diamonds (sprite 4) and tel fly's (sprite 6) also save their position
void startpositie() {              
 diamant = 0;
 vlieg = 0;
 for (int y = 0; y < 8; y++) {
 for (int x = 0; x < 16; x++) {
  if (grot[y][x] == 8) {mapX = x; mapY = y;}
  if (grot[y][x] == 4) {diamant++;}
  if (grot[y][x] == 6) {fly_X[vlieg] = x; fly_Y[vlieg] = y;
   vlieg++;}
 }} 
}

// Load a level from program memory into RAM
void laad_level(byte level2) {
 for (int y = 0; y < 8; y++) {
 for (int x = 0; x < 16; x++) {
  grot[y][x] = pgm_read_byte (& game[y+((level2-1)*8)][x]);
 }}
 richting[0]=0; richting[1]=0; richting[2]=0; richting[3]=0; richting[4]=0;
 startpositie();
}

// Read level number from EEPROM. When the 1st game startup: load EEPROM with level 1.
void hervatSpel() {
 byte tel = 0;
 if (EEPROM_lees(adresEEPROM) == 0x42) {tel++;}
 if (EEPROM_lees(adresEEPROM+1) == 0x44) {tel++;}
 level = EEPROM_lees(adresEEPROM+2);
 if (level > 0 && level < 19) {tel++;}
 if (tel != 3) {
  EEPROM_schrijf(adresEEPROM,0x42);
  EEPROM_schrijf(adresEEPROM+1,0x44);
  level = 1;
  EEPROM_schrijf(adresEEPROM+2,level);
 }
}

// Routine writes data to the specified address in the EEPROM (only if it has changed)
void EEPROM_schrijf (int adres_eeprom, byte data_eeprom) {
 EEPROM.update(adres_eeprom, data_eeprom);   // Rockbox: EEPROM is a per-game file
}
byte EEPROM_lees (int adres_eeprom) {
 return EEPROM.read(adres_eeprom);
}
