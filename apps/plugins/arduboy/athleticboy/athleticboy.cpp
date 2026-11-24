#include <stdlib.h>
#include <math.h>
// Game:      Athletic Boy
// Copyright: 2024 Frank van de Ven 
// Licence:   MIT

#include <Arduboy2.h>              // installeer bibliotheek
#include "AthleticBoy_data.h"      // include data bestand met data, sprites en afbeeldingen
Arduboy2 arduboy;                  // Arduboy
#define READ(x,y) (!!(x & (1<<y))) // definieer bit lees macro

volatile unsigned char * videoRam = 0;//Variabele wisScherm
extern "C" {                       // laat compiler weten dat er externe routines zijn
// C version of the upstream AVR assembler routine (AthleticBoy.S): clears the
// changing parts of the screen buffer (skip, count, value triples).
void wisScherm() {
  static const uint8_t ops[][3] = {{22,39,0x80},{24,11,0x80},{19,12,0x80},{176,36,0},
    {48,122,0},{6,122,0},{6,122,0},{15,106,0}};
  uint8_t *p = (uint8_t *)videoRam;
  for (uint8_t o = 0; o < 8; o++) {
    p += ops[o][0];
    for (uint8_t n = 0; n < ops[o][1]; n++) *p++ = ops[o][2];
  }
}
}

// variabelen
 int slX1;                    // x-positie uiteinde slinger
 int balX = 116;              // positie bal op x-as
 int bijX = 116;              // positie bij op a-as
 int bootX = 70;              // positie van de boot x-as
 int bootR = 2;               // richting + verplaatsing boot
 bool bootS = 0;              // is 1 als man op boot staat
 byte appel1sp = 26;          // sprite nummer plaatje appel 1 (25 of 26)
 byte appel2sp = 26;          // sprite nummer plaatje appel 2 (25 of 26)

 byte visX = 22;              // 1e X-positie visje
 byte visY;                   // Y-positie visje
 byte visS;                   // nummer afbeelding visje uit grafisch array
 byte visNr = 0;              // teller (0 t/m 10) die door de animatie van het visje loopt

 int scene = 0;               // game level (scene)(wordt geprint, 2 cijfers) Start bij 1.
 int scene_old = -1;          // vorige game level (scene)(gebruikt om gras random te tonen)
 int scene2;                  // bewaard level
 unsigned int score = 0;      // score (wordt geprint, 3 cijfers)
 bool geteld1 = 0;            // is 1 als score is geteld
 bool geteld2 = 0;            // is 1 als score is geteld 
 static int tijd = 300;       // max. speeltijd = 300 sec. (wordt geprint, 3 cijfers)
 int tijd2 = 300;             // overgebleven speeltijd
 byte levens = 5;             // aantal levens (wordt geprint, max 5 gezichtjes)
 bool botsing = 0;            // is 1 als man iets raak
 byte spel;                   // beval alle spel onderdelen van de huidige scene

 byte stap = 0;               // bepaald welke stap de man zet / wat de man doet.
 byte loop1 = 2;              // bepaald welke richting de man op loopt (0=links, 2=rechts)
 byte loop2 = 0;              // correctie plaatje looprichting man
 int manX = 120;              // X positie man (X van 0 tot 120)
 int manY = 40;               // y positie man (loopt altijd 8px boven de onderkant)
 byte man2;                   // gewenste tekening man
 unsigned long wacht;         // voor wacht lus
 unsigned long speeltijd;     // speeltijd


void tekenAnimatie();
void testBotsing();
void tekenMan();
void punten();
void introscherm();
void inputKnoppen();
void testGeenLevens();
void reset_man();
void tekenAchtergrond();
void teken_appel();
void teken_bij();
void teken_bal();
void teken_rots();
void teken_slinger();
void teken_vissen();
void teken_vijver();
void teken_gras();
void tekenBoomstam();
void updateTekst();
void printGetal(unsigned int getal, byte x_as);
bool tekenMan2(byte nr);
void testNextScene();

void setup() {

 arduboy.begin();                      // arduboy setup
 arduboy.clear();                      // wis video geheugen
 arduboy.setFrameRate(60);             // instellen framerate 60 frames / sec.
 videoRam = arduboy.getBuffer();       // vraagt 1e RAM adres sBuffer op (voor in assembler)
 tekenAchtergrond();                   // teken de achtergrond (2 bomen + tekst)
 introscherm();                        // teken intro scherm
 arduboy.initRandomSeed();             // initialiseer random getallen
 speeltijd = millis();                 // sla huidige tijd op
}

void loop() {
 if (!(arduboy.nextFrame())) {return;} // wacht tot het tijd is voor volgende frame
 wisScherm();                          // wist enkel de delen van het scherm die veranderen
 testGeenLevens();                     // controleer of je alle levens hebt verbruikt
 testNextScene();                      // controleer of je de scene uitgespeeld hebt
 spel = pgm_read_byte (& level[scene]);// sla alle spelonderdelen van de huidige scene op
 tekenAnimatie();                      // teken voorwerp, animatie die op een level plaatsvind
 tekenMan();                           // vraag input knoppen op en teken man + test botsing
 testBotsing();                        // kijk of er botsingen zijn
 punten();                             // kijk of een hindernis genomen is en pas de score aan
 tekenBoomstam();                      // teken de linker en rechter boomstam
 updateTekst();                        // print aantal levens + getallen: scene, score, tijd
 arduboy.display();                    // zet video geheugen op lcd
}
//----- einde programma ---------------------------------------------------------------------






//===========================================================================================

// de 8 bits van "spel" bepalen welke hindernissen in deze scene actief zijn
void tekenAnimatie() {
 if (READ(spel,0) == 1) {teken_rots();}        // 1 teken een rots op het pad
 if (READ(spel,1) == 1) {teken_slinger();}     // 2 teken bewegende slinger
 if (READ(spel,2) == 1) {teken_bal();}         // 3 teken een rollende bal
 if (READ(spel,3) == 1) {teken_bij();}         // 4 teken vliegende bij
 if (READ(spel,4) == 1) {teken_gras();}        // 5 teken gras (schermvulling)
 if (READ(spel,5) == 1) {teken_vijver();}      // 6 teken vijver en boot
 if (READ(spel,6) == 1) {teken_vissen();}      // 7 teken springende vissen
 if (READ(spel,7) == 1) {teken_appel();}       // 8 teken vallende appels
}
//===========================================================================================

// als er een botsing is kijk naar de 8 bits van "spel" om te zijn welke hindernissen er op
// het scherm staan. Vergelijk de positie van man met die hindernis om te zijn of er een
// botsing met die specifieke hindernis is.
void testBotsing() {
 if ((botsing == 1) && (stap != 8) && (stap != 9)){ // is er een botsing en je ben niet dood?
  if (READ(spel,0) == 1) {                                     // controle botsing rots
   if ((man2 != 6) && (manX > 62) && (manX < 83)){stap = 7;}}     // is man bij rots
  if (READ(spel,1) == 1) {                                    // controle pakken slinger
   if ((slX1 - manX < 5) && (manY != 40) && (manX < 99)){stap = 10;} // bij spring:pak slinger
   if ((manX > 40) && (manX <80) && (manY == 40)) {               // bij in water: verdrink
     stap = 11; if (manX > 60) {manX = 60;}}}                     // verplaats man in water
  if (READ(spel,2) == 1) {                                    // controle botsing bal
   if ((balX - manX < 16) && (balX - manX > 0)){stap = 7;}}       // ligt bal 16px van man?
  if (READ(spel,3) == 1)  {                                   // controle botsing bij
   if ((bijX - manX < 16) && (bijX - manX > 0) && (man2 != 5)){   // positie vlieg
    if ((READ(spel,5) == 1) && (manX > 14) && (manX < 100))   // vijver en man op water?
    {stap = 11;}                                                  // man verdrinkt
     else {stap = 7;}                                             // anders man dood
   }}
  if (READ(spel,5) == 1) {                                    // controle man op boot
   if (arduboy.everyXFrames(4)){                                  // actie om 4 frames uit
    if ((bootX - manX < 4) && (bootX - manX > -12 ))              // in man op boot?
     {manX += bootR; bootS = 1;}                                  // pas manX aan + score
     else
      if ((manX > 14) && (manX < 100 ))                           // bij in water: verdrink
       {stap = 11; bootS = 0;}                                    // man verdrinkt + score
   }}
  if (READ(spel,6) == 1) {                                    // controle water en vissen
   if ((manX > 11) && (manX < 25)){manY = 40; manX = 18; stap = 11;}// verplaats man in water
   if ((manX > 38) && (manX < 52)){manY = 40; manX = 45; stap = 11;}// verplaats man in water
   if ((manX > 65) && (manX < 79)){manY = 40; manX = 72; stap = 11;}// verplaats man in water
   if ((manX > 92) && (manX < 112)){manY = 40; manX = 99; stap = 11;}//verplaats man in water
   }
  if (READ(spel,7) == 1) {                                    // controle botsing appel
   if ((manX > 0) && (manX < 110)) stap = 7;}                     // man in speelveld? dood
 }
}
//===========================================================================================

// Stuur de animatie van de man aan.
void tekenMan() {
 if (arduboy.everyXFrames(6)){                // vertraging. teken Man om de 6 frames.
  switch (stap) {                             // met "stap" doorloop je de animaties
  case 0:                                     // man staat stil (plaatje 1 en 7)
   inputKnoppen();                            // vraag invoer knoppen op
   man2 = 0;                                            // teken stilstaande man
   break;
  case 1:                                     // man loopt 1e stap (plaatje 1 en 8)
   stap = 2;                                            // laat man 2e stap zetten
   manX += (loop1-1)*3;                                 // verplaats X positie man 3px
   man2 = 1;                                            // teken lopende man
   break;
  case 2:                                     // man loopt 2e stap (plaatje 2 en 9)
   stap = 0;                                            // laat man weer stil staan
   manX += (loop1-1)*3;                                 // verplaats X positie man 3px
   man2 = 2;                                            // teken lopende man
   break;
  case 3:                                     // man springt (plaatje 3 en 10)
   manX += (loop1-1)*6;                                 // verplaats X positie man 3px
   manY=manY-6;                                         // teken man 6px hoger
   stap = 4;                                            // laat man iets verder springen
   man2 = 3;                                            // teken springende man
    break;
  case 4:                                     // vervolg sprong man
    manX += (loop1-1)*6;                                // verplaats X positie man 3px
   stap = 5;                                            // zet man weer op de grond
   man2 = 3;                                            // teken springende man
   break;
  case 5:                                     // vervolg sprong man
   manX += (loop1-1)*6;                                 // verplaats X positie man 3px
   manY += 6;                                           // zet Y pos man weer goed
   stap = 0;                                            // terug naar stilstaande man
   man2 = 0;                                            // teken stilstaande man
   break;
  case 6:                                     // man bukt
   man2 = 5;                                            // teken bukkende man
   if (arduboy.notPressed(DOWN_BUTTON)) {stap = 0;}     // knop los? terug naar staan
   break;
  case 7:                                    // man ligt (plaatje 6 en 13)
   if (loop1 == 2){manX -= 10;}else {manX += 8;}        // correctie manX positie
   manY = 40;                                           // positie man op de grond
   man2 = 6;                                            // teken liggende man
   wacht = millis();                                    // sla huidige tijd op
   stap = 8;                                            // ga naar stap 8 (einde)
   break;
  case 8:                                     // wacht 2 sec (laat vlieg, bal doorgaan)
    if (millis() > wacht + 2000) {stap = 9;}            // wacht 2 sec.
   break;
  case 9:                                     // man is dood. 1 leven minder en reset man
   levens--;                                            // 1 leven minder
   reset_man();                                         // zet parameters man en objecten goed
   break;
  case 10:                                    // man hangt aan slinger
   man2 = 4;                                            // teken man die aan slinger hangt
   manX = slX1 - 7;                                     // Verplaats Xman met slinger locatie
   if ((arduboy.notPressed(A_BUTTON)) &&
    (arduboy.notPressed(UP_BUTTON)))  {stap = 5;}       // laat pas los als je knop A loslaat
   break;
  case 11:                                    // man in het water
   man2 = 7;                                            // teken verdrinkende man
   wacht = millis();                                    // sla huidige tijd op
   stap = 8;                                            // ga naar stap 8 (einde)
   break;
  }
 if (manX <= 0) {manX = 0;}
 }
 botsing = tekenMan2(man2 + loop2);  // tekent de man op lcd + kijkt of er een botsing is
}

//===== klaar ===============================================================================

// kijk of een hindernis genomen is en pas de score aan
void punten(){
 if (READ(spel,0) == 1) {         // 1 rots              (10 punten als je over rots springt)
  if (man2 == 3 && geteld1 == 0) {                       // score staat aan en man springt?
   if ((manX > 73) && (manX < 80))                       // is man boven rots?
    {score +=10; geteld1 = 1;}                           // score +10 en score uitzetten
  }
  if (man2 == 0) {geteld1 = 0;}                          // als man staat: score inschakelen
 }
 if (READ(spel,1) == 1) {         // 2 slingers          (20 punten als je aan slinger hangt)
  if (man2 == 4 && geteld2 == 0) {                       // score staat aan, man aan slinger?
   score +=20; geteld2 = 1;}                             // score +20 en score uitzetten
  if (man2 == 0) {geteld2 = 0;}                          // als man staat: score inschakelen
 }
 if (READ(spel,2) == 1) {         // 3 rollende bal      (10 punten als je over bal springt)
  if (man2 == 3 && geteld2 == 0) {                       // score staat aan en man springt?
   if ((manX > balX - 10) && (manX < balX + 7))          // is man boven bal?
    {score +=10; geteld2 = 1;}                           // score +10 en score uitzetten
  }
  if (man2 == 0) {geteld2 = 0;}                          // als man staat: score inschakelen
 }
 if (READ(spel,5) == 1) {         // 6 vijver en boot    (20 punten als je op boot springt)
  if (bootS == 1 && geteld2 == 0) {                      // score aan? en man op boot?
   score +=20; geteld2 = 1;                              // score +20 en score uitzetten
  }
  if ((manX < 15) || (manX > 98 )) {bootS = 0;}          // staat de man op kade? bootS uit  
  if (bootS == 0) {geteld2 = 0;}                         // is man van boot score inschakelen
 }
 if (READ(spel,6) == 1) {         // 7 springende vissen (10 punten als je over plas springt)
  if (man2 == 3 && geteld2 == 0 && botsing == 0) {       // score aan? man springt? geen vis?
   for (byte tel = 14; tel < 96 ; tel+=27) {             // loep loopt positie 4 plassen af
    if ((manX > tel) && (manX < tel + 8))                // is man boven 1 van de 4 plassen?
     {score +=10; geteld2 = 1;}                          // score +10 en score uitzetten
    }
   }
  if (man2 == 0) {geteld2 = 0;}                          // als man staat: score inschakelen
 }
}
//-------------------------------------------------------------------------------------------

// Teken introductie scherm op display
void introscherm() {
 tekenBoomstam();                                // teken de linker en rechter boomstam
 teken_gras();                                   // teken gras
 arduboy.drawCompressed(9,30, tekst1, WHITE);    // print grafisch tekst "Athletic Boy"
 arduboy.drawCompressed(30,48, tekst2, WHITE);   // print grafisch "A: credits B: Play"
 arduboy.display();                              // zet video geheugen op lcd
 while (arduboy.notPressed(B_BUTTON)) {          // wacht tot er op B wordt gedrukt
  if (arduboy.pressed(A_BUTTON)) {               // als er op A wordt gedrukt dan print tekst
   arduboy.fillRect(30,48,63,5, BLACK);          // wis tekst met rechthoek
   arduboy.drawCompressed(22,48, tekst3, WHITE); // print grafisch "code: frank van de ven"
   arduboy.display();                            // zet video geheugen op lcd 
  }
 }
 while (arduboy.pressed(B_BUTTON)) {delay(1);}// wacht tot knop wordt losgelaten
}
//-------------------------------------------------------------------------------------------

// ophalen knoppen tijdens spel en stuur beweging man aan.
void inputKnoppen() {
 if ((arduboy.pressed(RIGHT_BUTTON)) && loop1 == 2) {stap=1; manX += (loop1-1)*3;} // rechts
 if ((arduboy.pressed(RIGHT_BUTTON)) && loop1 == 0) {loop1=2; loop2=0;}            // draai om
 if ((arduboy.pressed(LEFT_BUTTON)) && loop1 == 0) {stap=1; manX += (loop1-1)*3;}  // links
 if ((arduboy.pressed(LEFT_BUTTON)) && loop1 == 2) {loop1=0; loop2=8;}             // draai om
 if (arduboy.pressed(DOWN_BUTTON)) {stap = 6;}                                     // bukken
 if (arduboy.pressed(UP_BUTTON)) {stap = 3;}                                       // springen
 if (arduboy.pressed(A_BUTTON)) {stap = 3;}                                        // springen
// if (arduboy.pressed(B_BUTTON)){}              // knop B
}
//-------------------------------------------------------------------------------------------

// controleer of je alle levens hebt verbruikt / en of de speeltijd erop zit.
void testGeenLevens(){
 if ((levens == 0) || (tijd2 == 0)) {       // controleer alle levens verbruikt of tijd is om
  arduboy.setCursor(11, 35);                // stel cursor positie in
  arduboy.setTextSize(2);                   // stel letter grote in
  arduboy.print("Game over");               // print tekst "Game over"
  tekenBoomstam();                          // teken de linker en rechter boomstam
  updateTekst();                            // print teksten
  arduboy.display();                        // update display
  while (arduboy.pressed(B_BUTTON) == 0){delay(1);}// wacht totdat er op B wordt gedrukt
  scene = 1;                                // start nieuw spel bij scene 1
  score = 0;                                // reset score
  levens = 5;                               // reset aantal levens
  reset_man();                              // zet parameters man en objecten goed
  speeltijd = millis();                     // sla huidige tijd op
 }
}
//-------------------------------------------------------------------------------------------

// zet parameters man en objecten goed voor nieuwe poging / game
void reset_man() {
  stap = 0;                                  // Laat de man weer stilstaan
  loop1 = 2;                                 // richting die man op loopt is weer rechts
  loop2 = 0;                                 // correctie plaatje looprichting man
  manX = 0;                                  // X positie man (X van 0 tot 120)
  manY = 40;                                 // y positie man (loopt 8px boven de onderkant)
  man2 = 0;                                  // teken stilstaande man
  balX = 116;                                // reset positie bal
  bijX = 116;                                // reset positie bij
  bootX = 70;                                // reset positie van de boot x-as
  bootR = 2;                                 // reset richting + verplaatsing boot
  appel1sp = 26;                             // reset sprite nummer plaatje appel 1
  appel2sp = 26;                             // reset sprite nummer plaatje appel 2
  geteld1 = 0;                               // score weer inschakelen
  geteld2 = 0;                               // score weer inschakelen  
  bootS = 0;                                 // reset indicator man op boot
}
//-------------------------------------------------------------------------------------------

// teken achtergrond (2 bomen en tekst). Gebruikt gecomprimeerde tekening.
void tekenAchtergrond() {
  arduboy.drawCompressed(0,0, achtergrond, WHITE); // tekent de achtergrond en teksten
}
//-------------------------------------------------------------------------------------------

// teken 2 vallende appels. Appels vallen op verschillende (random) plaatsen en tijden
void teken_appel() {
 static byte appel1nr;                            // nummer appel 1 (tussen 0 en 6)
 static byte appel2nr;                            // nummer appel 2 (tussen 0 en 6)
 static byte appel1X;                             // X positie appel 1
 static byte appel1Y;                             // Y positie appel 1
 static byte appel2X;                             // X positie appel 2
 static byte appel2Y;                             // Y positie appel 2
 if (appel1sp == 26) {                            // kijk of appel 1 gevallen is (onderaan)
  do                                              // herhaal tot je 2 verschillende nr's hebt
   {appel1nr = random(0,7);}                      // kies een willekeurige appel positie
  while (appel1nr == appel2nr);                   // kijk of andere appel hetzelfde nr heeft
  appel1X = pgm_read_byte(&appel[appel1nr]);      // haal start X-pos. appel 1 uit data array
  appel1Y = pgm_read_byte(&appel[appel1nr + 7]);  // haal start Y-pos. appel 1 uit data array
  appel1sp = 25;                                  // pas sprite nr aan in die van hele appel
 }
 if (appel2sp == 26) {                            // kijk of appel 2 gevallen is (onderaan)
  do                                              // herhaal tot je 2 verschillende nr's hebt
   {appel2nr = random(0,7);}                      // kies een willekeurige appel positie
  while (appel2nr == appel1nr);                   // kijk of andere appel hetzelfde nr heeft
  appel2X = pgm_read_byte(&appel[appel2nr]);      // haal start X-pos. appel 2 uit data array
  appel2Y = pgm_read_byte(&appel[appel2nr + 7]);  // haal start Y-pos. appel 2 uit data array
  appel2sp = 25;                                  // pas sprite nr aan in die van hele appel
 }
 if (arduboy.everyXFrames(18)){                   // verplaats appels om de 18 frames
  appel1Y += 6;                                   // verplaats appel 1 6px omlaag
  appel2Y += 6;                                   // verplaats appel 2 6px omlaag
  if (appel1Y > 45) {appel1sp = 26;}              // is appel 1 onderaan sprite kapotte appel
  if (appel2Y > 45) {appel2sp = 26;}              // is appel 2 onderaan sprite kapotte appel
 }
 Sprites::drawSelfMasked(appel1X, appel1Y, sprites, appel1sp); // teken appel 1
 Sprites::drawSelfMasked(appel2X, appel2Y, sprites, appel2sp); // teken appel 2
}
//-------------------------------------------------------------------------------------------

// teken een vliegende bij
void teken_bij() {
 static byte nr2 = 6;                          // nummer sprite (bij is nr 6 t/m 7)
 byte vertraging;                              // geheugen voor vertraging
 if (READ(spel,5) == 1) {vertraging = 8;}  // bij vijver en boot: vertraging = 8 frames
  else {vertraging = 5;}                       // andere scenes: vertraging = 5 frames
 if (arduboy.everyXFrames(vertraging)){        // vertraging. teken bij om de 8 of 5 frames
  bijX -=  3;                                  // verplaats positie bij
  if (bijX < 3){bijX = 116;}                   // als de bij links verdwijnt komt hij rechts
  nr2++;                                       // volgende sprite bij
  if (nr2 > 7){nr2 = 6;}                       // indien alle bij sprits gehad, begin opnieuw
 }
 Sprites::drawSelfMasked(bijX, 36, sprites, nr2); // teken de bij boven de groen
}
//-------------------------------------------------------------------------------------------

// teken een rollende bal op het pad
void teken_bal() {
 static byte nr = 2;                            // nummer sprite (bal is nr 2 t/m 5)
 if (arduboy.everyXFrames(5)){                  // vertraging. teken bal om de 5 frames.
  balX -=  2;                                   // verplaats positie bal
  if (balX < 3){balX = 116;}                    // als de bal links verdwijnt komt hij rechts
  nr++;                                         // volgende sprite bal
  if (nr > 5){nr = 2;}                          // indien alle bal sprits gehad, begin opnieuw
 }
 Sprites::drawSelfMasked(balX, 48, sprites, nr);// teken de bal precies in het looppad
}
//-------------------------------------------------------------------------------------------

// teken een rots op het pad
void teken_rots() {
Sprites::drawSelfMasked(77, 48, sprites, 1);    // teken een rots precies in het looppad
}
//-------------------------------------------------------------------------------------------

// teken 1 slinger op het scherm (slX1 = x-positie uiteinde slinger)
void teken_slinger() {
 Sprites::drawSelfMasked(48, 55, sprites, 8);            // teken water (links)
 Sprites::drawSelfMasked(56, 55, sprites, 9);            // teken water (midden)
 Sprites::drawSelfMasked(64, 55, sprites, 10);           // teken water (rechts)
 const byte slX2 = 60;                                   // x pos. slinger 1
 const byte slY = 14;                                    // y pos. slinger 1 en 2
 const byte vertraging1 = 3;                             // vertraging aantal frames links
 static int slRichting1 = -1;                            // richting slinger 1
 static int slUitslag1 = 0;                              // bepaalt hoever slinger 1 uitslaat
 for (byte a = 0; a < 25; a++) {                         // loopt alle 20 pixels slinger af
  slX1 = slX2 + (slUitslag1 * pgm_read_byte(&slinger[a]) >> 3); // uitslag slinger 1
  Arduboy2Base::drawPixel (slX1, slY + a, WHITE);        // tekent 1 pixel op lcd (slinger 1)
 }
 if (arduboy.everyXFrames(vertraging1)) {                // vertraging: wacht 3 frames
  slUitslag1 = slUitslag1 + slRichting1;                 // past de uitslag van slinger 1 aan
  if (slUitslag1 == slRichting1 * 8) {slRichting1*= -1;} // slinger op max stand staat keer om
 }
}
//-------------------------------------------------------------------------------------------

// teken 4 plassen met springende vissen
void teken_vissen() {
 for (byte tel = 20; tel < 110 ; tel+=27) {         // loep teken 4 losse plassen water
  Sprites::drawSelfMasked(tel, 55, sprites, 16);    // teken water2 (links)
  Sprites::drawSelfMasked(tel+8, 55, sprites,17);   // teken water2 (rechts)
 }
 if (arduboy.everyXFrames(5)){                      // pas animatie om de 5 frames aan
  visY = pgm_read_byte(&vis[visNr+12]);             // haal Y positie vis uit array
  visS = pgm_read_byte(&vis[visNr]);                // haal afbeeldingsnummer vis uit array
  visNr++;                                          // verhoog teller array met 1
  if (visNr > 11) {                                 // is volledige animatie afgespeeld?
    visNr = 0;                                      // ja: start animatie opnieuw
    visX = 22 + (random(0,4) * 27);                 // kies random plaats waar vis verschijnt
  }
 }
  if (visNr != 0) {Sprites::drawSelfMasked(visX, visY, sprites,visS);} // teken vis
}
//-------------------------------------------------------------------------------------------

// teken een vijver met daarin een heen en weer drijvende boot
void teken_vijver() {
 Sprites::drawSelfMasked(20, 55, sprites, 8);          // teken water (links)
 for (byte tel = 28; tel <93; tel += 8){               // loep om 8x water (midden) te teken
  Sprites::drawSelfMasked(tel, 55, sprites, 9);        // teken water (midden)
 }
 Sprites::drawSelfMasked(100, 55, sprites, 10);        // teken water (rechts)
 if (arduboy.everyXFrames(4)){                         // vertraging. teken bij om de 4 frames
   bootX += bootR ;                                    // verplaats boot 2 pixels
  if ((bootX < 28) || (bootX > 78)){bootR *= -1;};     // als boot op eind is keer richting om
 }
 Sprites::drawSelfMasked(bootX, 50, sprites, 13);      // teken boot (links)
 Sprites::drawSelfMasked(bootX + 8, 50, sprites, 14);  // teken boot (midden)
 Sprites::drawSelfMasked(bootX + 16, 50, sprites, 15); // teken boot (rechts)
}
//-------------------------------------------------------------------------------------------

// teken gras (als schermvulling) waar je bovenop loopt (scene voor opbouwen extra levens)
void teken_gras() {
 static byte grasX1;                     // X-positie gras (links)
 static byte grasX2;                     // X-positie gras (midden)
 static byte grasX3;                     // X-positie gras (rechts)
 static byte grasNr;                     // bepaald welk van de 2 gras afbeeldingen je ziet
 if (scene > scene_old) {                // controleer of deze scene voor 1e keer draait
  scene_old = scene;                     // pas scene_old aan aan nieuwe situatie
  grasX1 = random(17,38);                // kies de X positie voor gras links
  grasX2 = random(49,70);                // kies de X positie voor gras midden
  grasX3 = random(81,100);               // kies de X positie voor gras rechts
  grasNr = random (0,2);                 // grasNr bepaald welk van de 2 afbeeldingen je ziet
 }
 Sprites::drawSelfMasked(grasX1, 55, sprites, 11 + grasNr); // zet afbeelding op scherm
 Sprites::drawSelfMasked(grasX2, 55, sprites, 12 - grasNr); // zet afbeelding op scherm
 Sprites::drawSelfMasked(grasX3, 55, sprites, 11 + grasNr); // zet afbeelding op scherm
}
//-------------------------------------------------------------------------------------------

// teken links en rechts 4 sprites van de boomstam incl. mask zodat man erachter kan lopen
void tekenBoomstam() {
  Sprites::drawExternalMask(3, 24, stam, stam_mask, 0,0);   // teken sprite boom linksboven
  Sprites::drawExternalMask(3, 32, stam, stam_mask, 1,1);   // teken sprite boom
  Sprites::drawExternalMask(3, 40, stam, stam_mask, 2,2);   // teken sprite boom
  Sprites::drawExternalMask(3, 48, stam, stam_mask, 3,3);   // teken sprite boom linksonder
  Sprites::drawExternalMask(117, 24, stam, stam_mask, 4,4); // teken sprite boom rechtsboven
  Sprites::drawExternalMask(117, 32, stam, stam_mask, 5,5); // teken sprite boom
  Sprites::drawExternalMask(117, 40, stam, stam_mask, 6,6); // teken sprite boom
  Sprites::drawExternalMask(117, 48, stam, stam_mask, 7,7); // teken sprite boom rechtsonder
}
//-------------------------------------------------------------------------------------------

// zet aantal levens (max 5 gezichtjes) + scene + score + tijd op bovenste regel van het lcd
void updateTekst() {
 tijd2 = tijd - (millis() - speeltijd) / 1000;           // bereken overgebleven speeltijd
 if (tijd2 < 0) {tijd2 = 0;}                             // indien tijd om: tijd2 = 0
 for (byte a = 0; a < levens; a++) {                     // loop het aantal levens af
  Sprites::drawSelfMasked(32+a*6, 1, sprites, 0);        // teken een gezichtje op lcd
 }
 printGetal(scene,22);                                   // print level (scene) 2 cijfers max
 if (score > 990) {score = 990;}                         // controle score getal te groot

 printGetal(score,85);                                   // print score 3 cijfers max. 65535
 printGetal(tijd2,116);                                  // print tijd 3 cijfers max. 999
}
//-------------------------------------------------------------------------------------------

// zet het gevraagde getal in video ram op de x_as positie (1e regel bovenaan scherm)
void printGetal(unsigned int getal, byte x_as){
unsigned int a = 10000;            // begin variabele voor tellen aantal karakters
unsigned int b;                    // tijdelijke opslag van het te printen getal
 while (a > getal){a /= 10;}       // bepaalt uit hoeveel cijfers het getal bestaat (deeltal)
 if (a == 0) {a = 1;}              // indien a = 0 maak a 1(voorkomt deling door 0)
 do {
  b = getal / a;                   // in b staat nu enkel het 1e cijfer van het getal
  getal = getal - (b * a);         // haal het 1e cijfer van het getal
  a /= 10;                         // pas het deeltal aan (10 keer zo klein)
  Arduboy2Base::sBuffer[x_as]= pgm_read_byte (& cijfer[b*3]);     // zet in video ram
  Arduboy2Base::sBuffer[x_as+1]= pgm_read_byte (& cijfer[b*3+1]); // zet in video ram
  Arduboy2Base::sBuffer[x_as+2]= pgm_read_byte (& cijfer[b*3+2]); // zet in video ram
  x_as += 4;                       // bepaal positie volgende cijfer op display
 }
 while (a > 0);                    // ga door totdat alle cijfers van het getal geweest zijn
}
//-------------------------------------------------------------------------------------------

// EIGEN SPRITE ROUTINE teken man op positie manX, manY)
// Indien man iets raakt geeft de routine een 1 terug
bool tekenMan2(byte nr) {
byte video1;                                      // data uit video geheugen
byte video2;                                      // sprite data
byte video3;                                      // verschoven bitje
byte video4;                                      // verschoven sprite data
byte video5;                                      // verschoven sprite data
unsigned int adres;                               // adres in video geheugen
byte bitje;                                       // aantal te verplaatsen bitjes
bool bots = 0;                                    // reset collision detectie (botsing)
 adres = ((manY>>3) * 128) + manX;                // bepaal RAM adres d.m.v. positie X en Y
 bitje = manY % 8;                                // bepaal bit positie Y in RAM adres
 for (byte a=0; a<2; a++){                        // loep 2x
  for (byte b=0; b<16; b++){                      // loep loop alle pixels plaatje man af
   video1 = Arduboy2Base::sBuffer[adres+b];       // Haal video data vanaf RAM adres
   video2 = Arduboy2Base::sBuffer[adres+b+128];   // Haal video data RAM adres 1 regel lager
   video3 = pgm_read_byte (& man[nr*32+a*16+b]);  // Lees bit patroon man uit flash geheugen
   video4 = video3<<bitje;                        // verplaats patroon zodat het in RAM past
   video5 = video3>>(8-bitje);                    // verplaats patroon zodat het in RAM past
   if ((video1 & video4) > 0) {bots = 1;}         // bots als patroon en RAM op zelfde plaats
   if ((video2 & video5) > 0) {bots = 1;}         // bots als patroon en RAM op zelfde plaats
   Arduboy2Base::sBuffer[adres+b] = video1 | video4;      // sla patroon op in video RAM
   Arduboy2Base::sBuffer[adres+b+128] = video2 | video5;  // sla patroon op in video RAM
  }
  adres=adres+128;                                // Ga naar volgende regel en herhaal alles
 }
return (bots);                                    // is er een botsing geef een 1 terug
}
//-------------------------------------------------------------------------------------------

// Test of je scene hebt uitgespeeld (achter rechter boom bent) en ga naar volgende scene.
void testNextScene(){
 if (manX >= 119) {
  scene++;                                     // naar volgende scene
    arduboy.initRandomSeed();                  // initialiseer random getallen
  if (pgm_read_byte (& level[scene]) == 0xFF){ // controleer op laatste level gespeeld.
    scene--;                                   // zet scene op vorige level
    arduboy.setCursor(11, 35);                 // stel cursor positie in
    arduboy.setTextSize(2);                    // stel letter grote in
    arduboy.print("game ends");                // print tekst "Game over"
    tekenBoomstam();                           // teken de linker en rechter boomstam
    updateTekst();                             // print teksten
    arduboy.display();                         // update display
    while (arduboy.pressed(B_BUTTON) == 0){delay(1);}// wacht totdat er op B wordt gedrukt
    scene = 1;                                 // start nieuw spel bij scene 1
    score = 0;                                 // reset score
    levens = 5;                                // reset aantal levens
    speeltijd = millis();                      // sla huidige tijd op
  }
  else {
    arduboy.setCursor(17, 35);                 // stel cursor positie in
    arduboy.setTextSize(2);                    // stel letter grote in
    arduboy.print("Scene ");                   // print tekst "scene"
    arduboy.print (scene);                     // print het scene nummer
    tekenBoomstam();                           // teken de linker en rechter boomstam
    updateTekst();                             // print teksten
    arduboy.display();                         // update display
    delay(1500);
  }
  reset_man();                                 // zet parameters man en objecten goed
  if (levens < 5){levens++;}                   // Je krijgt 1 extra leven (max = 5)
 }
}
//-------------------------------------------------------------------------------------------
