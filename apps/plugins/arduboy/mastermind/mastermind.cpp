// Mastermind for Arduboy by Frank Van De Ven (MIT). YP3 port: prototypes added, start-screen waits delay().
#include <Arduino.h>
void teken_startscherm(); void randomgetallen(); void teken_bord(); void invoer(); void antwoord(); void volgende_regel(); void kies_pion(); void kies_getal(); void teken_pionnen(byte); void teken_antwoord();
#include <Arduboy2.h>
#include "afbeeldingen.h"
  Arduboy2 arduboy;
#define ARDBITMAP_SBUF arduboy.getBuffer()
#include <ArdBitmap.h>
ArdBitmap<128,64> ardbitmap;

// answer circles: black (no. 0) and white (no. 1)
 const unsigned char PROGMEM rond[]= {0x04,0x04, 0x06,0x09,0x09,0x06, 0x06,0x0F,0x0F,0x06}; 
// the playing pawns nr. 0 = "grey" pawn + pawns with nr. 1 to 6
 const unsigned char PROGMEM pion[] = {0x09,0x09, 
 0x7C,0x82,0x55,0x29,0x55,0x29,0x55,0x82,0x7C,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00,//?
 0x7C,0x82,0x39,0x11,0x11,0x19,0x11,0x82,0x7C,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00,//1
 0x7C,0x82,0x39,0x09,0x39,0x21,0x39,0x82,0x7C,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00,//2
 0x7C,0x82,0x39,0x21,0x39,0x21,0x39,0x82,0x7C,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00,//3
 0x7C,0x82,0x21,0x21,0x39,0x29,0x29,0x82,0x7C,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00,//4
 0x7C,0x82,0x39,0x21,0x39,0x09,0x39,0x82,0x7C,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00,//5
 0x7C,0x82,0x39,0x29,0x39,0x09,0x39,0x82,0x7C,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00,//6
 0x38,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x38,0x00,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x00
 };// last sprite is for drawErase selection pawn
 
 // masks. prevents other pixels on the display from being overwritten.
 const unsigned char PROGMEM masker1[]= {0x04,0x04, 0x0F,0x0F,0x0F,0x0F};
 const unsigned char PROGMEM masker2[]= {0x09,0x09,
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
// variables used:
byte code[4];   // contains in input answer
byte toets[4];  // contains in input answer
byte ant[4];    // here are the positions of the black and white response pawns
byte code2[4];  // temporary storage code
byte regel;     // the line on the display 1 to 9 on which to play
byte knop1;     // the button pressed on the Arduboy
byte knop2;     // extra info pressed button: 1= left, top. 2= right, down
byte nr;        // the number of the pawn 0 to 3 (0 = 1st left pawn, 3 = 4th right pawn)
byte wit;       // number of white answer pawns (number is in answer)
byte zwart;     // number of black answer pawns (number is in answer and in correct place)

void setup() {
  arduboy.begin();           // initialiseerd de Arduboy2 bibliotheek
  arduboy.initRandomSeed();  // init. random numbers
  arduboy.setFrameRate(40);  // specify speed: 40 frames per sec. put on LCD
  teken_startscherm();       // draw the home screen + wait for the button to be pressed
  randomgetallen();          // pick 4 numbers and set line counter to 1
  teken_bord();              // draw the game board
  arduboy.display();         // puts the contents of the video memory on the display
}

// loop() function runs once per frame: reads buttons and controls game components
void loop() {
  if (!(arduboy.nextFrame())) {return;}
  arduboy.pollButtons();                  // check if a button has been pressed
  invoer();
   switch(knop1) {
   case 1: randomgetallen(); teken_bord(); knop1=0; break;  // new game
   case 2: antwoord();   volgende_regel(); knop1=0; break;  // respond to entered code
   case 3: kies_pion();  knop1=0; break;                    // select a pawn (0 to 3)
   case 4: kies_getal(); knop1=0; break;                    // select a number (1 to 6)
  }
  arduboy.display();
}

void antwoord(){
  zwart = 0; wit = 0;
  for (byte tel = 0; tel < 4; tel++){
    ant[tel] = toets[tel];
    code2[tel] = code[tel];
  } 
  for (byte tel = 0; tel <4; tel++) {
    if (code2[tel] == ant[tel]) {zwart++; code2[tel] = 8; ant[tel] = 9;}
    }
  for (byte tel1 = 0; tel1<4; tel1++) {
   for (byte tel2 = 0; tel2<4; tel2++) {
    if (code2[tel1] == ant[tel2]) {wit++; code2[tel1] = 8; ant[tel2] = 9;}
   }
  }
  teken_antwoord();                       // puts black and white answer pawns on screen
}

// draw the game board + the gray pawns + pawns 1st line + select 1st pawn
void teken_bord(){
  arduboy.clear(); 
  Arduboy2Base::drawRect(0,0,128,64,1);
  Arduboy2Base::drawFastVLine(113,0,64,1); 
  Arduboy2Base::drawFastHLine(0,14,113,1);
   for (byte tel = 0; tel < 4; tel ++){ 
   toets[tel] = 1; 
   Sprites::drawExternalMask(116, (tel*11+18), pion, masker2, 0, 0);}
  nr = 0;
  regel = 1; 
  teken_pionnen(1); 
}

// read out the buttons of the Arduboy
void invoer(){
  if (arduboy.justPressed(B_BUTTON))      {knop1 = 1; knop2 = 0;}  // new game
  if (knop2 != 3){                          // if the game is not over these buttons work
   if (arduboy.justPressed(A_BUTTON))     {knop1 = 2; knop2 = 0;}  // show answer
   if (arduboy.justPressed(UP_BUTTON))    {knop1 = 3; knop2 = 1;}  // choose pawn (move left)
   if (arduboy.justPressed(DOWN_BUTTON))  {knop1 = 3; knop2 = 2;}  // choose pawn (move right)
   if (arduboy.justPressed(LEFT_BUTTON))  {knop1 = 4; knop2 = 1;}  // choose number (1 lower)
   if (arduboy.justPressed(RIGHT_BUTTON)) {knop1 = 4; knop2 = 2;}  // choose number (1 up)
} }

// put next line of pawns on screen + end game at 4 black pawns
void volgende_regel(){
  nr = 0;
  if (regel < 9 && zwart != 4){ 
   teken_pionnen(0); 
   regel++;
   teken_pionnen(1);} 
  else {
   teken_pionnen(0);
   for (byte tel = 0; tel < 4; tel++){ 
    Sprites::drawExternalMask(116, (tel*11+18), pion, masker2, code[tel], 0);}
   knop2 = 3;} 
}

// choose the desired pawn (0 to 3) whose number you want to change
void kies_pion(){
  if (knop2 == 1) {nr--;} else {nr++;}
  nr=nr & 0x03;
  teken_pionnen(1); 
}

// choose the following numbers (1 to 6) and place them on the place of the selected pawn
void kies_getal(){
  if (knop2 == 1) {toets[nr]--; 
   if (toets[nr] == 0) {toets[nr] = 6;}}
  else {toets[nr]++;
   if (toets[nr] == 7) {toets[nr] = 1;}}
  teken_pionnen(1); 
}

// choose the code of 4 different numbers
void randomgetallen(){
  code[0]= random(1,7); 
  do {code[1] = random(1,7);}
   while (code[1]==code[0]); 
  do {code [2] = random(1,7);}
   while (code[2]==code[1] || code[2]==code[0]); 
  do {code [3] = random(1,7);}
   while (code[3]==code[2] || code[3]==code[1] || code[3]==code[0]);
}

// draw 4 pawns on the "line" (when "select" = 1, 1 pawn (in "nr") is selected)
void teken_pionnen(byte select) {
  byte posY = 101-(regel-1)*12; 
   for (byte tel = 0; tel < 4; tel ++){ 
   Sprites::drawExternalMask(posY, (tel*11+18), pion, masker2, toets[tel], 0);} 
  if (select == 1) { 
   Sprites::drawErase(posY, (nr*11+18), pion, 7);} 
}

// place the answer pawns anywhere (0= black, 1= white, 2= no pawn)
void teken_antwoord(){
  byte pos;
  for (byte tel = 0; tel < 4; tel++){ ant[tel] = 2;}
  for (byte tel = 0; tel < zwart; tel++) { 
   do
    pos = random(4);
   while (ant[pos] != 2);
   ant[pos] = 0;
  }
  for (byte tel = 0; tel < wit; tel++) {
   do
    pos = random(4);
   while (ant[pos] != 2); 
   ant[pos] = 1;
  }
   // put the answer pawns on the screen: 0= black (open circle). 1= white. 2= no pawn
   byte posY = 101-(regel-1)*12;
   if (ant[0] != 2) {Sprites::drawExternalMask(posY+5, 3,rond,masker1, ant[0], 0);}
   if (ant[1] != 2) {Sprites::drawExternalMask(posY, 8,rond,masker1, ant[1], 0);}
   if (ant[2] != 2) {Sprites::drawExternalMask(posY, 3,rond,masker1, ant[2], 0);}
   if (ant[3] != 2) {Sprites::drawExternalMask(posY+5, 8,rond,masker1, ant[3], 0);}
}

// draw the (compressed) start screen + wait for the button to be pressed + choose numbers
void teken_startscherm(){
  byte getal;
  arduboy.clear();  
  ardbitmap.drawCompressed(64,0,man,1,0,0); 
  ardbitmap.drawCompressed(0,0,bord,1,0,0);
  arduboy.display();
  while (arduboy.pressed(B_BUTTON) != true) { delay(10);
   getal = random(1,7);
   if (arduboy.pressed(A_BUTTON) == true){
    arduboy.clear();
    ardbitmap.drawCompressed(64,0,man,1,0,0);
    ardbitmap.drawCompressed(0,0,tekst,1,0,0);
    arduboy.display();}
  }
  while (arduboy.pressed(B_BUTTON) == true) {delay(10);}
}
