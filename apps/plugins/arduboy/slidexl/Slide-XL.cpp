// Game:      Slide XL
// Copyright: 2025 Frank van de Ven
// Licence:   MIT


#include <Arduboy2.h>                                 // installeer de Arduboy2 bibliotheek
Arduboy2 arduboy;                                     // maakt een Arduboy object aan
#include "data.h"                                     // installeer bestand met afbeeldingen
// ------------------------------------------------------------------------------------------

// variabelen
 #define wachttijd 1500                               // tijd dat startscherm zichtbaar is
 enum stage {start,wis,links,rechts,schut,toetsen,    // alle stappen van stage machine
      voorbeeld,mogelijk,verplaats,controle,gewonnen,info};
 byte bord[6][6];                                     // array spelbord
 byte spel1;                                          // 0 = 3x3, 1 = 4x4, 2 = 4x4 (tekening)
 byte spel2;                                          // 3 = bord 3x3, 4 = bord 4x4 (loops)
 byte spelX;                                          // X positie spelbord
 byte spelY;                                          // Y positie spelbord
 byte spelX_old;                                      // vorige X positie spelbord
 byte spelY_old;                                      // vorige Y positie spelbord
 byte posC[3] = {14,51,91};                           // positie pijl in menu scherm
 unsigned long wacht;                                 // voor vertragingen
 int tijd, tijd1, tijd2;                              // speeltijd in seconden
 unsigned long tijd_new;                              // speeltijd new in milliseconden
 unsigned long tijd_old;                              // speeltijd old in milliseconden
 unsigned long tijd_correctie;                        // speeltijd correctie in milliseconden
 int moves;                                           // aantal gemaakte zetten
 int moves_old;                                       // vorige aantal zetten
 int game = start;                                    // stage machine, begint bij start
 byte aantal;                                         // aantal keer dat je blokjes schut
 byte knop, knop2, knop3, knop4;                      // invoer toetsenbord
 byte knop_old;                                       // vorige ingedrukte toetsenbord
 byte kijk;                                           // kijk of zet mogelijk is (1 = ja)
 byte richting;                                       // random richting bij schudden
 byte test;                                           // controle of spel opgelost is
// ------------------------------------------------------------------------------------------

void einde();
byte controleren();
void schudden();
void verplaatsen();
byte bekijk_zet(byte knop3);
byte knoppen();
void spelbord();
void setup_bord();
void menu();
void opgelost();
void infoveld();
void data();

void setup() {
 arduboy.begin();                                     // initialiseert de Arduboy2 bibliotheek
}
// ------------------------------------------------------------------------------------------

void loop() {
switch (game) {                                       // volgorde spel onderdelen
  case start:                                         // print startscherm en menu
   menu();                                            // start scherm + keuze spel
   arduboy.initRandomSeed();                          // setup random getallen
   game = wis;                                        // ga naar setup spelbord
  break;
  case wis:                                           // setup spelbord
   setup_bord();                                      // setup array met beginstatus spel
   game = links;                                      // ga naar print linker deel scherm
  break;
  case links:                                         // print linker deel scherm
   infoveld();                                        // zet informatie op linker scherm
   game = rechts;                                     // ga naar print rechter deel scherm
  break;
  case rechts:                                        // print rechter deel scherm
   spelbord();                                        // zet spelbord op scherm
   game = schut;                                      // ga naar schuif blokjes door elkaar
  break;
  case schut:                                         // schuif blokjes door elkaar
   aantal = 100;                                      // teller aantal keer schudden (100)
   moves = 0; moves_old = 0;                          // reset tellers aantal zetten
   tijd = -1;                                         // stel start speeltijd in 
   schudden();                                        // schuif de stenen door elkaar
   game = info;                                       // ga naar print data op scherm
  break;
  case toetsen:                                       // lees de knoppen
    knop = knoppen();                                 // lees invoer toetsenbord
    if (knop == 0) {game = info;}                     // er is nergens op gedrukt 
    if (knop > 0 && knop < 5) {game = mogelijk;}      // er is op een pijltje gedrukt
    if (knop == 5) {game = voorbeeld;}                // er is op A gedrukt
    if (knop == 6) {game = start;}                    // er is op B gedrukt
  break;
  case voorbeeld:                                     // laat oplossing zien
   opgelost();                                        // A ingedrukt = oplossing bekijken
  break;
  case mogelijk:                                      // is verplaatsen mogelijk?
   knop2 = knop;
   kijk = bekijk_zet(knop2);                          // is verplaatsen mogelijk?
    if (kijk == false) {game = info;}                 // nee: ga naar info
    if (kijk == true) {game = verplaats;}             // ja: ga naar verplaats blokje
  break;
  case verplaats:                                     // verplaats de blokjes op het scherm
   verplaatsen();                                     // verplaats de blokjes
   game = controle;                                   // ga naar controle
  break;
  case controle:                                      // controleer of de blokjes goed staan
   test = controleren();                              // controleer of blokjes goed staan
    if (test == false) {game = info;}                 // nee: speel door. ga naar info
    if (test == true) {game = gewonnen;}              // ja: ga naar gewonnen
  break;
  case gewonnen:                                      // einde spel
   einde();                                           // laat duimpje zien + wacht op B toets
   game = start;                                      // ga terug naar opstart scherm
  break;
  case info:                                          // werk tijd bij en zet data op scherm
   data();                                            // zet data op linker scherm
   game = toetsen;                                    // ga terug naar opvragen toetsen
  break; 
 }
}
// ------------------------------------------------------------------------------------------

// einde spel. print duimpje en wacht op B toets
void einde(){
 data();                                              // zet laatste data op scherm
 Sprites::drawOverwrite(40, 47, sprite, 34);          // print duimpje op scherm
 arduboy.display();                                   // zet video geheugen op het display
 while (arduboy.pressed(B_BUTTON) != true) {delay(1);}// wacht tot B toets is ingedrukt
 while (arduboy.pressed(B_BUTTON) == true) {delay(1);}// wacht tot B toets wordt losgeleaten
}
// ------------------------------------------------------------------------------------------

// controleer of het spel is opgelost (0 is nee, 1 is ja)
byte controleren(){
 byte x; byte y; byte nr = 1, test = true;            // declareer variabelen
 for (y = 1; y < spel2+1; y++) {                      // loep doorloopt alle y waardes
  for (x = 1; x < spel2+1; x++) {                     // loep doorloopt alle x waardes
   if (spel1 == 0 && nr == 9) {nr = 16;}              // bij bord 3x3 vervang 9 door 16
   if (bord[x][y] != nr) {test = false;}              // controleer of blokje goed staat
   nr++;                                              // volgende nummer
  }
 }
 return test;                                         // return true als spel is opgelost
}
// ------------------------------------------------------------------------------------------

// schud de stenen door elkaar
void schudden(){
 while (aantal > 0) {                                 // loep tot aantal keer schudden 0 is
  richting = random (1,5);                            // kies een willekeurige richting
  while (bekijk_zet(richting) == 0) {                 // controller of deze zet mogelijk is
   richting = random (1,5);                           // nee: kies een andere richting
 }
  bord[spelX_old][spelY_old] = bord[spelX][spelY];    // verplaats blokje
  bord[spelX][spelY] = 16;                            // wis de oude positie
  spelX_old = spelX;                                  // backup de X positie
  spelY_old = spelY;                                  // backup de Y positie
  spelbord();                                         // update het spelbord op het scherm
  aantal--;                                           // verlaag schudden teller met 1
  wacht = millis();                                   // wachtlus
  while (millis() < (wacht + 30)) {delay(1);}// wacht 30ms zodat animatie te zien is
 }
}
// ------------------------------------------------------------------------------------------

// verplaats de blokjes
void verplaatsen() {
 bord[spelX_old][spelY_old] = bord[spelX][spelY];     // verplaats blokje
 bord[spelX][spelY] = 16;                             // wis de oude positie
 spelX_old = spelX;                                   // backup de X positie
 spelY_old = spelY;                                   // backup de Y positie
 spelbord();                                          // update het spelbord op het scherm
 moves++;                                             // verhoog aantal zetten met 1
}
// ------------------------------------------------------------------------------------------

// bekijk of het verschuiven van het blokje mogelijk is (1 = ja, 0 = nee + pas X en Y aan
byte bekijk_zet(byte knop3) {
 byte OK = false;                                     // zet verplaatsen op niet mogelijk
 if (knop3 == 1) {                                    // knopje omhoog
  if (bord[spelX][spelY+1] != 0) {                    // test of omhoog mogelijk is
   spelY++;                                           // pas Y positie aan
   OK = true;                                         // geef aan dat zet mogelijk is
  }
 }
 if (knop3 == 2) {                                    // knopje rechts
  if (bord[spelX-1][spelY] != 0) {                    // test of rechts mogelijk is
   spelX--;                                           // pas X positie aan
   OK = true;                                         // geef aan dat zet mogelijk is
  }
 }
 if (knop3 == 3) {                                    // knopje omlaag
  if (bord[spelX][spelY-1] != 0) {                    // test of omlaag mogelijk is
   spelY--;                                           // pas Y positie aan
   OK = true;                                         // geef aan dat zet mogelijk is
  }
 }
 if (knop3 == 4) {                                    // knopje links
  if (bord[spelX+1][spelY] != 0) {                    // test of links mogelijk is
   spelX++;                                           // pas X positie aan
   OK = true;                                         // geef aan dat zet mogelijk is
  }
 }
 return OK;                                           // geef terug of zet mogelijk is
}
// ------------------------------------------------------------------------------------------

// lees de knoppen van de Arduboy
byte knoppen() {
 byte knopje = 0;                                     // reset knopje
 if (arduboy.pressed(UP_BUTTON)) {knopje = 1;}        // is er op omhoog gedrukt? knopje = 1
 if (arduboy.pressed(RIGHT_BUTTON)) {knopje = 2;}     // is er op rechts gedrukt? knopje = 2
 if (arduboy.pressed(DOWN_BUTTON)) {knopje = 3;}      // is er op omlaag gedrukt? knopje = 3
 if (arduboy.pressed(LEFT_BUTTON)) {knopje = 4;}      // is er op links gedrukt? knopje = 4
 if (arduboy.pressed(A_BUTTON)) {knopje = 5;}         // is er op A gedrukt? knopje = 5
 if (arduboy.pressed(B_BUTTON)) {knopje = 6;}         // is er op B gedrukt? knopje = 6
 if (knopje == knop_old) return 0;                    // is knopje al losgelaten? nee: return
 knop_old = knopje;                                   // bewaar ingedrukte knop
 return knopje;                                       // geef nieuwe waarde knop terug
}
// ------------------------------------------------------------------------------------------

// zet spelbord op het scherm
void spelbord(){
 byte offsetX, offsetY, nr;                           // declareer variabelen
 if (spel1 == 0) {offsetX = 73; offsetY = 8;}         // stel offset linker bovenhoek bord in
  else {offsetX = 64; offsetY = 0;}                   // stel offset linker bovenhoek bord in
 for (byte y=0; y<spel2; y++) {                       // loep y as door spelbord
  for (byte x=0; x<spel2; x++) {                      // loep x as door spelbord
   nr = bord[x+1][y+1];                               // haal nr sprite uit array   
    if (spel1 == 2) {nr = nr + 16;}                   // sprite offset bij tekening spel
   Sprites::drawOverwrite(x*16+offsetX, y*16+offsetY, sprite, nr); // zet sprites op scherm
   nr++;                                              // volgende sprite
   if (spel1 == 0 && nr == 9) {nr = 16;}              // bij bord 3x3 vervang 9 door 16
  }
 }
 arduboy.display();                                   // zet video geheugen op het display
}
// ------------------------------------------------------------------------------------------

// setup spelbord (zet hierin de cijfers van een opgelost spel)
void setup_bord() {
 byte x; byte y; byte nr = 1;                         // declareer variabelen
  for (y = 0; y < 6; y++) {                           // loep doorloopt alle y waardes
   for (x = 0; x < 6; x++) {                          // loep doorloopt alle x waardes
    bord [x][y] = 0;                                  // zet geheugenlocatie op 0
  }
 }
 for (y = 1; y < spel2+1; y++) {                      // loep doorloopt alle y waardes
  for (x = 1; x < spel2+1; x++) {                     // loep doorloopt alle x waardes
    bord[x][y] = nr;                                  // ze nummer in geheugen locatie
    nr++;
    if (spel1 == 0 && nr == 9) {nr = 16;}             // bij bord 3x3 vervang 9 door 16
  }
 }
 if (spel1 == 0) {
  spelX= 3; spelX_old= 3; spelY= 3; spelY_old= 3;}    // beginwaarde open blokje (spel 3x3)
  else {spelX= 4; spelX_old= 4; spelY= 4; spelY_old= 4;}// beginwaarde open blokje (spel 4x4)
}
// ------------------------------------------------------------------------------------------

// zet start scherm en daarna het menu op het scherm + bediening menu
void menu() {
 spel1 = 0;                                           // maak spel 1 (3x3) als default
 arduboy.clear();                                     // wis het display (wordt zwart)
 arduboy.drawCompressed(0,0, startscherm, WHITE);     // zet start tekening op scherm
 arduboy.display();                                   // zet video geheugen op het display
 wacht = millis();                                    // laad wachttijd met huidige tijd
 while (millis() < wacht + wachttijd) {delay(1);}// wacht tot opgegeven tijd om is
 arduboy.fillRect (0,36,128,28, BLACK);               // maak onderste deel van scherm zwart
 Sprites::drawOverwrite(0, 56, sprite, 33);           // zet 2 pijltjes links op scherm
 arduboy.setCursor(13, 55); arduboy.println("A:Select");// tekst op vaste plaats op scherm
 arduboy.setCursor(64, 55); arduboy.println("game");  // zet tekst op vaste plaats op scherm
 arduboy.setCursor(93, 55); arduboy.println("B:Code");// zet tekst op vaste plaats op scherm
 Sprites::drawOverwrite(posC[spel1], 36, sprite, 0);  // zet pijl op het scherm
 arduboy.display();                                   // zet video geheugen op het display
 while (arduboy.pressed(A_BUTTON) == false) { delay(1);         // A knop: doorloop loep tot spel start
  if (arduboy.pressed(B_BUTTON) == true) {            // B knop: wordt er op B gedrukt?
   arduboy.fillRect (0,55,128,8, BLACK);              // maak onderste deel scherm zwart
   arduboy.setCursor(5, 55); arduboy.println("Code:");// zet tekst op vaste plaats op scherm
   arduboy.setCursor(36, 55); arduboy.println("Frank");// zet tekst op vaste plaats op scherm
   arduboy.setCursor(68, 55); arduboy.println("van"); // zet tekst op vaste plaats op scherm
   arduboy.setCursor(89, 55); arduboy.println("de");  // zet tekst op vaste plaats op scherm
   arduboy.setCursor(104, 55); arduboy.println("Ven");// zet tekst op vaste plaats op scherm
   arduboy.display();                                 // zet video geheugen op het display
  }
  if (arduboy.pressed(LEFT_BUTTON) == true) {         // wordt er op links gedrukt?
  Sprites::drawOverwrite(posC[spel1], 36, sprite, 16);// wis pijl op het scherm
  spel1 = spel1 - 1;                                  // verschuif pijl naar links
   if (spel1 == 255) {spel1 = 2;}                     // indien < 0 spel1 wordt 2
   Sprites::drawOverwrite(posC[spel1], 36, sprite, 0);// zet pijl op het scherm
   arduboy.display();                                 // zet video geheugen op het display
  while (arduboy.pressed(LEFT_BUTTON) == true) {delay(1);}// wacht tot knop los gelaten wordt
  }
  if (arduboy.pressed(RIGHT_BUTTON) == true) {        // wordt er op rechts gedrukt?
  Sprites::drawOverwrite(posC[spel1], 36, sprite, 16);// wis pijl op het scherm
  spel1 = spel1 + 1;                                  // verschuif pijl naar rechts
   if (spel1 == 3) {spel1 = 0;}                       // indien > 2 spel1 wordt 0
   Sprites::drawOverwrite(posC[spel1], 36, sprite, 0);// zet pijl op het scherm
   arduboy.display();                                 // zet video geheugen op het display
  while (arduboy.pressed(RIGHT_BUTTON) == true) {delay(1);}// wacht tot knop los gelaten wordt
  }
 }
 if (spel1 == 0) {spel2 = 3;} else {spel2 = 4;}       // stel grote spelbord in
 knop_old = 5;                                        // zorg dat 1e toets genegeerd wordt
}
// ------------------------------------------------------------------------------------------

// laat de opgeloste versie van het spelbord zien
void opgelost(){
 byte offsetX, offsetY, nr;                           // declareer variabelen
 if (spel1 == 0) {offsetX = 73; offsetY = 8;}         // stel offset linker bovenhoek bord in
  else {offsetX = 64; offsetY = 0;}                   // stel offset linker bovenhoek bord in
 if (spel1 == 2) {nr = 17;}                           // sprite offset bij tekening spel
  else {nr = 1;}                                      // sptite offset bij cijfer spel
 for (byte y=0; y<spel2; y++) {                       // loep y as door spelbord
  for (byte x=0; x<spel2; x++) {                      // loep x as door spelbord
   Sprites::drawOverwrite(x*16+offsetX, y*16+offsetY, sprite, nr); // zet sprites op scherm 
   nr++;                                              // volgende sprite
   if (spel1 == 0 && nr == 9) {nr = 16;}              // bij bord 3x3 vervang 9 door 16
  }
 }
 arduboy.display();                                   // zet video geheugen op het display
 while (arduboy.pressed(A_BUTTON) == true) {          // is de A knop nog ingedrukt?
  data();}                                            // update tijd op scherm
  spelbord();                                         // losgelaten? zet spel terug op scherm
  game = toetsen;                                     // ga naar lezen knoppen
}
// ------------------------------------------------------------------------------------------

// zet alle informatie op het linker deel van het scherm
void infoveld(){
 arduboy.clear();                                     // wis het display (wordt zwart)
 arduboy.drawCompressed(5,2, slidexl, WHITE);         // zet tekening SlideXL op scherm
 arduboy.drawFastVLine (61,0,64,WHITE);               // zet verticale lijn op scherm
 arduboy.setCursor(0, 24); arduboy.println("Time:");  // zet tekst op vaste plaats op scherm
 arduboy.setCursor(0, 34); arduboy.println("Moves:"); // zet tekst op vaste plaats op scherm
 arduboy.setCursor(0, 47); arduboy.println("A:");     // zet tekst op vaste plaats op scherm
 arduboy.setCursor(12, 47); arduboy.println("view");  // zet tekst op vaste plaats op scherm
 arduboy.setCursor(0, 57); arduboy.println("B:");     // zet tekst op vaste plaats op scherm
 arduboy.setCursor(12, 57); arduboy.println("menu");  // zet tekst op vaste plaats op scherm
}
// ------------------------------------------------------------------------------------------

// zet tijd en aantal moves op het scherm
void data() {
 if (tijd == -1) {                                    // bij -1 setup tijd variabelen
  tijd_correctie = millis();                          // haal huidige correctie tijd op
  tijd_old = tijd_correctie - 1000;                   // stel oude tijd in
  tijd = 0;                                           // zet setup tijdvariabelen uit
 }
 tijd_new = millis();                                 // haal huidige tijd op
 if (tijd_new > tijd_old + 1000) {                    // is er 1 seconden verstreken
  tijd_old = tijd_new;                                // ja. stel nieuwe oude tijd in
  tijd = (tijd_new - tijd_correctie) / 1000;          // bereken verstreken tijd in sec.
  tijd1 = tijd / 60;                                  // bereken minuten
  tijd2 = tijd % 60;                                  // bereken enkel seconden
  arduboy.fillRect (30, 24,31,8, BLACK);              // zet zwart blok over oude tekst
  arduboy.setCursor(30, 24);                          // cursor positie
  arduboy.print(tijd1);                               // zet minuten op het scherm
  if (tijd2 < 10) {arduboy.print(".0");}              // zet .0 op het scherm
   else {arduboy.print(".");}                         // zet . op het scherm
  arduboy.print(tijd2);                               // zet seconden op scherm
  arduboy.display();                                  // zet video geheugen op het display
 }
 if (moves != moves_old) {                            // is er een blokje verschoven?
  arduboy.fillRect (36, 34,24,8, BLACK);              // ja: zet zwart blok over oude tekst
  arduboy.setCursor(36, 34); arduboy.print(moves);    // zet nieuwe zetten op het scherm
  arduboy.display();                                  // zet video geheugen op het display 
  moves_old = moves;                                  // update moves_old
 }
}
// ------------------------------------------------------------------------------------------
