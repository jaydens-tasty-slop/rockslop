// Game:      Ardu Skiing
// Copyright: 2026 Frank van de Ven
// Licence:   MIT
 
 #include <Arduboy2.h>
#include <EEPROM.h>                               // installeer de Arduboy2 bibliotheek
 Arduboy2 arduboy;                                   // maakt een Arduboy object aan
 Sprites sprites;                                    // Namespace voor sprites instellen
 #include "ArduSkiing.h"                             // grafische en level data
//------------------------------------------------------------------------------------------

// variabelen
 const int adresEEPROM = 0x201;                      // adres in EEPROM (t/m 0x204)
 int c = 56;                                         // regel nr voor controle botsing
 int x, x_old;                                       // positie speler in wereld (0 - 300)
 int y;                                              // teller y positie op baan
 const int veld = 300;                               // max grote skipiste
 int z;                                              // startwaarde vlaggen verticaal
 int z1;                                             // startwaarde linker boom verticaal
 int z2;                                             // startwaarde rechter boom verticaal
 int xP;                                             // startwaarde vlaggen horizontaal
 int xP1;                                            // startwaarde linker boom horizontaal
 int xP2;                                            // startwaarde rechter boom horizontaal
 unsigned long tijd;                                 // delay start klok
 byte klok;                                          // speeltijd klok
 float xb;                                           // x positie bovenaan piste
 float xo;                                           // x positie onderaan piste
 float m;                                            // factor x afneemt per regel
 float xm;                                           // x positie op die regel
 int verschil;                                       // x afstand tussen boven en onder
 byte startklok;                                     // bij 1 laat aftellen zien
 byte einde;                                         // bij 1 is einde spel
 int score;                                          // aantal punten
 int score2;                                         // tijdelijke teller aantal punten
 int scoreS;                                         // opgeslagen aantal punten
 int val;                                            // nr paal die valt 1= links 2= rechts

 
//------------------------------------------------------------------------------------------

void bereken_score();
void display_score();
void knoppen();
void save();
void start();
byte print_tijd();
void print_score(int PUNT);
void aftellen();
void plaats_vlag_boom();
void invoer();
void teken_ski_s();
void teken_boom(int X, int Y, int Z);
void teken_vlaggen(int X, int Y, int Z, int R);
void teken_bergen();
void teken_hek();
void opstartscherm();
void laad_score();
void EEPROM_schrijf (int adres_eeprom, byte data_eeprom);
byte EEPROM_lees (int adres_eeprom);

void setup() {
 arduboy.begin();                                    // initialiseert de Arduboy2
 arduboy.setFrameRate(40);                           // opgeven snelheid: 40 frames/sec.
 opstartscherm();                                    // teken het opstartscherm
 laad_score();                                       // haal opgeslagen score uit EEPROM
 arduboy.initRandomSeed();                           // init. random getallen
 start();                                            // zet variabelen goed voor start spel

}
//------------------------------------------------------------------------------------------

void loop() {
 if (!(arduboy.nextFrame())) {return;}               // wacht op volgende frame
 arduboy.pollButtons();                              // is er een knop ingedrukt is geweest?
 if (startklok == 1){aftellen();}                    // tel af van 5 naar 0 en start game
 arduboy.clear();                                    // wis display geheugen
 memset(arduboy.sBuffer, 0xFF, 0x400);               // maak alle pixels display wit
 teken_bergen();                                     // teken achtergrond met bergen
 teken_hek();                                        // teken hekwerk rond piste
 invoer();                                           // invoer knoppen en verplaats speler 
 einde = print_tijd();                               // zet speeltijd op scherm
 if (einde == 1) {                                   // bij een 1 is spel afgelopen (new game)
  display_score();                                   // zet score op display
  knoppen();                                         // handel knoppen af
 }
 bereken_score();                                    // bereken score op regel 63 
 print_score(score);                                 // zet de score op het scherm
 plaats_vlag_boom();                                 // plaats vlaggen, bomen, bereken score
 teken_ski_s();                                      // teken ski's boven andere tekeningen
 arduboy.display();                                  // zet display geheugen op scherm
}
//------------------------------------------------------------------------------------------

// controleer of je door de vlaggen bent gegaan zonder een poortje te raken
void bereken_score() {
 if (z == 63) {                                      // onderste regel?
  if (score2 == 7) 
   {score++;}
 else {score--;}
  if (score > 99) {score = 99;}                      // is score > 99 maak score 99
  if (score < -99) {score = -99;}                    // is score < -99 maak score -99
 }
}
//------------------------------------------------------------------------------------------

// laat de score en opgeslagen score zien.
void display_score() {
 byte offset;                                        // tijdelijke variabele
 arduboy.clear();                                    // wis display geheugen
 memset(arduboy.sBuffer, 0xFF, 0x400);               // maak alle pixels display wit
 teken_bergen();                                     // teken achtergrond met bergen
 x = 150;                                            // zet positie hekwerk in het midden
 teken_hek();                                        // teken hekwerk rond piste
 arduboy.setTextColor(BLACK);                        // zet font op zwart
 arduboy.fillRect(10,21,110,36,WHITE);               // teken een witte rechthoek
 arduboy.drawRect(11,22,108,34,BLACK);               // teken daarbinnen een zwart kader
 offset = 23;                                        // stel de offset van de x-as in
 if (score >= 0) {offset = offset + 2;}              // corrigeer offset voor neg. getallen
 if ((score > -10) && (score < 10)) {offset=offset+3;} // corrigeer offset grote getallen
 arduboy.setCursor(offset, 25);                      // zet cursor goed
 arduboy.print("Your score:");                       // print tekst
 arduboy.print(score);                               // print score
 offset = 20;                                        // stel de offset van de x-as in
 if (scoreS >= 0) {offset = offset + 2;}              // corrigeer offset voor neg. getallen
 if ((scoreS > -10) && (scoreS < 10)) {offset=offset+3;} // corrigeer offset grote getallen
 arduboy.setCursor(offset, 35);                      // zet cursor goed
 arduboy.print("Saved score:");                      // print tekst
 arduboy.print(scoreS);                              // print opgeslagen score
 arduboy.setCursor(14, 45);                          // zet cursor goed
 arduboy.print("A:Save");                            // print tekst
 arduboy.setCursor(57, 45);                          // zet cursor goed
 arduboy.print("B:New game");                        // print tekst
 arduboy.display();                                  // zet display geheugen op scherm
}
//------------------------------------------------------------------------------------------

// handel knoppen af bij einde spel (nieuw spel + sla score op in EEPROM)
void knoppen() {
 while(arduboy.pressed(B_BUTTON) == false){ delay(1);          // start nieuw spel als B wordt gedrukt
  if (arduboy.pressed(A_BUTTON) == true) {save();}   // save score als A wordt ingedrukt
 };
 start();                                            // start nieuw spel
}
//------------------------------------------------------------------------------------------

// sla score op in EEPROM
void save() {
 while (arduboy.pressed(A_BUTTON) == true) {delay(1);}// wacht tot toets wordt losgelaten
 scoreS = score;                                     // pas opgeslagen score aan met huidige
 display_score();                                    // update saved score op display
 EEPROM_schrijf(adresEEPROM+2,highByte(score));      // zet MSB deel score in EEPROM
 EEPROM_schrijf(adresEEPROM+3,lowByte(score));       // zet LSB deel score in EEPROM
}
//------------------------------------------------------------------------------------------

// zet de variabelen goed voor het starten van het spel
void start(){
 val = 0;                                            // er vallen geen palen om
 z = 18;                                             // startwaarde vlaggen verticaal
 z1 = 40;                                            // startwaarde linker boom verticaal
 z2 = 50;                                            // startwaarde rechter boom verticaal
 xP = 150;                                           // startwaarde vlaggen horizontaal
 xP1 = 95;                                           // startwaarde linker boom horizontaal
 xP2 = 310;                                          // startwaarde rechter boom horizontaal
 x = veld / 2;                                       // start positie speler op piste
 x_old = x;                                          // start positie speler op piste
 startklok = 1;                                      // bij 1 laat aftellen zien
 einde = 0;                                          // bij 1 is einde spel
 score = 0;                                          // reset score teller
 klok = 120;                                         // stel speeltijdklok in op huidge tijd
}
//------------------------------------------------------------------------------------------

// print de tijd op het scherm
byte print_tijd(){
 if (tijd + 1000 < millis()) {                             // is er een seconden verstreken?
  tijd = millis();                                         // sla huidige systeemtijd op
  klok--;                                                  // verlaag klok met 1 seconden
 }
 int minuut = klok / 60;                                   // bereken het aantal minuten 
 int sec_10 = (klok % 60) / 10;                            // bereken het aantal seconden (L)
 int sec_1 = (klok % 60) % 10;                             // bereken het aantal seconden (R)
 sprites.drawExternalMask(2,17,getal,getal_mask,minuut,0); // zet minuten op het scherm
 sprites.drawExternalMask(6,17,getal,getal_mask,10,0);     // zet een punt op het scherm
 sprites.drawExternalMask(8,17,getal,getal_mask,sec_10,0); // zet seconden op het scherm (L)
 sprites.drawExternalMask(12,17,getal,getal_mask,sec_1,0); // zet seconden op het scherm (R)
 if (klok == 0) {return 1;} else {return 0;}               // return 1 bij einde speeltijd
}
//------------------------------------------------------------------------------------------

// print de score op het scherm (+/- 99 is max.)
void print_score(int PUNT){
 int PUNT_10, PUNT_1;                                        // tijdelijke variabelen
 if (PUNT < 0) {                                             // is score negatief?
  PUNT_10 = ((PUNT ^ 0xFFFF) + 1) / 10;                      // maak score pos. geef L getal
  PUNT_1 = ((PUNT ^ 0xFFFF) + 1) % 10;                       // maak score pos. geef R getal
  sprites.drawExternalMask(115,17,getal,getal_mask,11,0);    // zet een "-" op display
 }
 else {
  PUNT_10 = PUNT / 10;                                       // score positief: geef L getal
  PUNT_1 = PUNT % 10;                                        // score positief: geef R getal
 }
 sprites.drawExternalMask(119,17,getal,getal_mask,PUNT_10,0);// zet L getal op display
 sprites.drawExternalMask(123,17,getal,getal_mask,PUNT_1,0); // zet R getal op display
}
//------------------------------------------------------------------------------------------

// tel af van 5 naar 1 en start het spel
void aftellen() {
 arduboy.clear();                                    // wis display geheugen
 memset(arduboy.sBuffer, 0xFF, 0x400);               // maak alle pixels display wit
 teken_bergen();                                     // teken achtergrond met bergen
 teken_hek();                                        // teken hekwerk rond piste
 teken_ski_s();                                      // teken ski's boven andere tekeningen
 arduboy.display();                                  // zet display geheugen op scherm
 for (byte tel = 0; tel < 5; tel++){                 // tel af van 6 naar 1 (5 -> 0)
  sprites.drawOverwrite(57, 30, cijfers, tel);       // sprite cijfer op display
  arduboy.display();                                 // zet display geheugen op scherm
  tijd = millis();                                   // sla huidige tijd op
  while (tijd + 1000 > millis()){delay(1);}// wacht 1 seconden
 }
 startklok = 0;                                      // zet aftelklok uit
 tijd = millis();                                    // sla huidige tijd op voor speeltijd
}
//------------------------------------------------------------------------------------------

// verplaats positie vlaggen en bomen verticaal en werk score bij
void plaats_vlag_boom() {
  z++; z1++; z2++;                                   // verplaats vlaggen en boom
// controller of vlaggen / bomen onderaan staan. Kies dan nieuwe plaats vlag en boom
 if (z > 63) {val = 0; z=18; xP = random (50,219);}             // kies plaats vlaggen
 if (z1 > 63) {z1 = 18 - random(0,30); xP1 = random (70,95);}   // kies plaats linker boom
 if (z2 > 63) {z2 = 18 - random(0,30); xP2 = random (295,340);} // kies plaats rechter boom
// bereken positie vlaggen horizontaal en plaats vlaggen
 verschil = xP - x;                                  // bereken afstand tussen speler en vlag
 xb = 64 + verschil / 37;                            // bereken offset bovenaan piste
 xo = 64 - verschil;                                 // bereken offset onderaan piste
 m = (xb - xo) / 46;                                 // bereken factor voor hoek beweging
 xm = xb + (z - 18) * m;                             // bereken horizontale positie vlag
 if (z == 18){score2 = 0;}                           // reset score2 teller
 if (z > c) {                                         // contr. of je tussen de vlaggen gaat
  if (x == x_old && verschil > -10 && verschil < 10) {val = 1;}  // L ski recht. raakt vlag
  if (x == x_old && verschil > -58 && verschil < -38) {val = 2;} // R ski recht. raakt vlag
  if (x < x_old && verschil > -14 && verschil < 12) {val = 1;}   // L ski links. raakt vlag
  if (x < x_old && verschil > -63 && verschil < -37) {val = 2;}  // R ski links. raakt vlag
  if (x > x_old && verschil > -11 && verschil < 15) {val = 1;}   // L ski rechts. raakt vlag
  if (x > x_old && verschil > -60 && verschil < -34) {val = 2;}  // R ski rechts. raakt vlag
  if (verschil < -12 && verschil > -36) {score2++;}  // ski's tussen vlagen? ja: 1 punt erbij
 }
 teken_vlaggen (xm,z,(z - 16) / 1.5, val);           // plaats vlaggen op scherm
 
// bereken positie linker boom horizontaal en plaats linker boom
 if (z1 > 17) {                                      // kijk of boom op piste staat
  verschil = xP1 - x;                                // bereken afstand tussen speler en boom
  xb = xP1 / 2.5;                                    // bereken offset bovenaan piste
  xo = 134 - verschil;                               // bereken offset onderaan piste
  m = (xb - xo) / 46;                                // bereken factor voor hoek beweging
  xm = xb + (z1 - 18) * m;                           // bereken horizontale positie boom
  teken_boom (xm,z1,(z1 - 16) / 1.5);                // plaats linker boom op scherm
 }
// bereken positie rechter boom horizontaal en plaats rechter boom
 if (z2 > 17) {                                      // kijk of boom op piste staat
  verschil = xP2 - x;                                // bereken afstand tussen speler en boom
  xb = xP2 / 4;                                      // bereken offset bovenaan piste
  xo = 64 - verschil;                                // bereken offset onderaan piste
  m = (xb - xo) / 46;                                // bereken factor voor hoek beweging
  xm = xb + (z2 - 18) * m;                           // bereken horizontale positie boom
  teken_boom (xm,z2,(z2 - 16) / 1.5);                // plaats rechter boom op scherm
 }
}
//------------------------------------------------------------------------------------------

// lees invoer knoppen en verplaats speler
void invoer() {
 if (arduboy.pressed(LEFT_BUTTON)) {                 // wordt er op de linker knop gedrukt?
   x = x - 4;                                        // verplaats speler 2 px naar links
   if (x < 14) {x = 14;}                             // zorg dat speler binnen piste blijft
 }
 if (arduboy.pressed(RIGHT_BUTTON)){                 // wordt er op de rechter knop gedrukt?
  x = x + 4;                                         // verplaats speler 2 px naar rechts
  if (x > veld - 14) {x = veld - 14;}                // zorg dat speler binnen piste blijft
 }
}
//------------------------------------------------------------------------------------------

// teken 2 ski's
void teken_ski_s() {
 if (x > x_old){                                     // nieuwe positie > oude positie
  sprites.drawExternalMask(50,48,ski,ski_mask,3,3);  // teken ski naar rechts
  sprites.drawExternalMask(63,48,ski,ski_mask,3,3);  // teken ski naar rechts
  }
  if (x < x_old) {                                   // nieuwe positie < oude positie
  sprites.drawExternalMask(50,48,ski,ski_mask,2,2);  // teken ski naar links
  sprites.drawExternalMask(63,48,ski,ski_mask,2,2);  // teken ski naar links
  }
 if ( x == x_old){                                   // nieuwe positie > oude positie
  sprites.drawExternalMask(51,48,ski,ski_mask,0,0);  // teken ski naar voren
  sprites.drawExternalMask(62,48,ski,ski_mask,1,1);  // teken ski naar voren
 }
 x_old = x;                                          // update oude positie
}
//------------------------------------------------------------------------------------------

// teken een boom op positie x,y met de vergrotingsfactor z
void teken_boom(int X, int Y, int Z) {               // pos X,Y en vergrotingsfactor Z
 Z=Z*1.5;                                            // vergrotingsfactor boom verhogen
 arduboy.fillRect (X, Y-Z/5, Z/8, Z/5, BLACK);       // teken stam
 arduboy.fillTriangle(X-Z/2.8,Y-Z/5, X+Z/2.0,Y-Z/5, X+Z/15,Y-Z, BLACK); // teken boom
}
//------------------------------------------------------------------------------------------

// teken 2 vlaggen op positie x,y, vergrotingsfactor z, R = nr. valg die moet vallen 
void teken_vlaggen(int X, int Y, int Z, int R) {
 int W;                                              // tijdelijke variable
  if (R == 1 && z > c) {                             // R = 1: laat linker vlag vallen
  W = 38 - Z;                                        // bereken grote vallende paal
  arduboy.fillRect (X, Y - W, Z/16+1, W, BLACK);     // rechthoek als paal
  arduboy.fillTriangle(X,(Y-W), X,(Y-W)+W/4, X-Z/3-1,(Y-W)+W/8, BLACK); // driehoek als vlag
 }
 else {
  arduboy.fillRect (X, Y-Z, Z/16+1, Z, BLACK);       // rechthoek als paal
  arduboy.fillTriangle(X,Y-Z, X,Y-Z+Z/4+1, X-Z/3-1,Y-Z+Z/8, BLACK); // driehoek als vlag
 }
 X = X + (Z * 1.6);                                  // bereken X positie 2e vlag
 if (R == 2 && z > c) {                              // R = 2: laat rechter vlag vallen
  W = 38 - Z;                                        // bereken grote vallende paal
  arduboy.fillRect (X, Y - W, Z/16+1, W, BLACK);     // rechthoek als paal
  arduboy.fillTriangle(X,(Y-W), X,(Y-W)+W/4, X-Z/3-1,(Y-W)+W/8, BLACK); // driehoek als vlag
 }
 else {
  arduboy.fillRect (X, Y-Z, Z/16+1, Z, BLACK);       // rechthoek als paal
  arduboy.fillTriangle(X,Y-Z, X,Y-Z+Z/4+1, X-Z/3-1,Y-Z+Z/8, BLACK); // driehoek als vlag
 }
}
//------------------------------------------------------------------------------------------

// teken achtergrond met bergen
void teken_bergen() {
 for (byte tel = 0; tel < 8; tel++){                 // doorloop alle sprites met bergen (8)
  sprites.drawOverwrite(tel*16, 0, berg, tel+0);     // teken de berg sprite
 }
}
//------------------------------------------------------------------------------------------

// Tekenen hekwerk
void teken_hek(){
 int X1 = veld - x - veld + 64;                      // bereken pos waar linker hek begint
 int X2 = veld - x + 64;                             // bereken pos waar rechter hek begint
 arduboy.fillTriangle(60,18 ,X1,76 ,X1,64 , BLACK);  // teken een driehoek als linker hek
 arduboy.drawLine(60,18,X1,64,BLACK);                // teken een lijn onder linker hek
 arduboy.fillTriangle(68,18 ,X2,76 ,X2,64 , BLACK);  // teken een driehoek als rechter hek
 arduboy.drawLine(68,18,X2,64,BLACK);                // teken een lijn onder rechter hek
}
//------------------------------------------------------------------------------------------

// teken het opstartscherm en wacht tot er op A of B gedrukt wordt
void opstartscherm() {
 arduboy.clear();                                    // wis display geheugen
 arduboy.drawCompressed(0,0, FX, WHITE);             // teken gecomprimeerde tekening
 arduboy.display();                                  // zet display geheugen op scherm
 while(arduboy.pressed(A_BUTTON) == false && arduboy.pressed(B_BUTTON) == false ){delay(1);}
}
//------------------------------------------------------------------------------------------

// haal de opgeslagen score uit EEPROM. is er nog geen score opgeslagen, zet een 0 in EEPROM
void laad_score(){
 if ((EEPROM_lees(adresEEPROM) == 0x46) && (EEPROM_lees(adresEEPROM+1) == 0x56)) { //F of V?
  scoreS = EEPROM_lees(adresEEPROM+2);               // lees MSB deel (int) score
  scoreS = scoreS << 8;                              // schuif bits 8 plaatsen op
  scoreS = scoreS + EEPROM_lees(adresEEPROM+3);      // lees LSB deel en tel het erbij op
 }
 else {                                              // nee. nog geen score opgeslagen
  EEPROM_schrijf(adresEEPROM,0x46);                  // zet "F" in EEPROM
  EEPROM_schrijf(adresEEPROM+1,0x56);                // zet "V" in EEPROM
  EEPROM_schrijf(adresEEPROM+2,0x00);                // zet "0" als score in EEPROM (MSB)
  EEPROM_schrijf(adresEEPROM+3,0x00);                // zet "0" als score in EEPROM (LSB)
 }
}
//-------------------------------------------------------------------------------------------

// Routine schrijft alleen naar EEPROM als data is aangepast
void EEPROM_schrijf (int adres_eeprom, byte data_eeprom) {
 EEPROM.update(adres_eeprom, data_eeprom);   // Rockbox: EEPROM is a per-game file
}
byte EEPROM_lees (int adres_eeprom) {
 return EEPROM.read(adres_eeprom);
}
//-------------------------------------------------------------------------------------------
