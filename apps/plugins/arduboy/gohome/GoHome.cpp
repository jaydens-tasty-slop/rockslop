// Game:      Go Home
// Copyright: 2026 Frank van de Ven
// Licence:   MIT
 
 #include <Arduboy2.h>
#include <EEPROM.h>                                // installeer de Arduboy2 bibliotheek
 Arduboy2 arduboy;                                    // maakt een Arduboy object aan
 Sprites sprites;                                     // Namespace voor sprites instellen
 #include "GoHome.h"                                  // grafische en level data

//------------------------------------------------------------------------------------------

// variabelen
 unsigned long tijd;                                  // opslaan systeemtijd
 unsigned long tijd2;                                 // opslaan tijd (voor print level)
 static unsigned long wacht = 5;                      // wacht lus verplaatsen speler
 static unsigned long wacht2 = 1500;                  // wacht lus nieuw level
 static unsigned long wacht3 = 2000;                  // wacht lus openingsscherm
 static unsigned long wacht4 = 2000;                  // tijd dat level nr zichtbaar is
 const int adresEEPROM = 0x1FE;                       // adres in EEPROM (t/m 0x200)
 byte introScherm = 0;                                // staat openingsscherm is op lcd?
 byte game;                                           // stage machine
 bool inv = 0;                                        // knipper bit. laat speler knipperen
 byte x,y,z;                                          // loep en tijdelijke variabelen
 byte level;                                          // level nummer
 byte veld[16][8];                                    // speelveld
 byte veld_old[16][8];                                // oude speelveld
 byte verplaats;                                      // speler verplaatst
 int manX[3];                                         // x positie 3 spelers                                      
 int manY[3];                                         // y positie 3 spelers
 int manX_old[3];                                     // vorige x positie 3 spelers                                      
 int manY_old[3];                                     // vorige y positie 3 spelers 
//------------------------------------------------------------------------------------------

void nieuw_continu();
void intro();
bool controle();
void undo();
void beweeg(int rX, int rY);
void invoer();
void print_speelveld();
void lees_speelveld(byte level2);
void laad_level_nr();
void EEPROM_schrijf (int adres_eeprom, byte data_eeprom);
byte EEPROM_lees (int adres_eeprom);

void setup() {
 arduboy.begin();                                     // initialiseert de Arduboy2
 arduboy.setFrameRate(60);                            // opgeven snelheid: 60 frames/sec.
 laad_level_nr();                                     // haal level nr uit EEPROM
 game = 0;                                            // ga naar stap 0                                  
}
//------------------------------------------------------------------------------------------

void loop() {  
 if (!(arduboy.nextFrame())) {return;}                // wacht op volgende frame
 arduboy.pollButtons();                               // is er een knop ingedrukt is geweest?
 if (game != 1 ) {arduboy.clear();}                   // wis het display (wordt zwart)
 if (arduboy.frameCount % 15 == 0) {inv = inv ^ 1;}   // inverteer knipper bit
 switch (game) {                                      // besturing spel onderdelen
 case 0:                                              // intro scherm
  intro();                                            // begin scherm met keuze menu
  game = 1;                                           // ga naar stap 1  
 break;
 case 1:                                              // Keus nieuw game of continue
  nieuw_continu();                                    // lees knoppen: new game / continue
 break;
 case 2:                                              // laad level in geheugen
  lees_speelveld(level);                              // lees het speelveld
  tijd2 = millis();                                   // sla tijd op (voor level nr op lcd)
  game = 3;                                           // ga naar stap 3                                 
 break;
 case 3:                                              // lees knoppen
  invoer();                                           // leest status knoppen
 break;
 case 4:                                              // Verplaats spelers naar links
  beweeg(-1,0);                                       // beweeg naar links
  game = 9;                                           // ga naar stap 3  
 break;
 case 5:                                              // Verplaats spelers naar rechts
  beweeg(1,0);                                        // beweeg naar rechts
  game = 9;                                           // ga naar stap 3  
 break;
 case 6:                                              // Verplaats spelers omhoog
  beweeg(0,-1);                                       // beweeg omhoog
  game = 9;                                           // ga naar stap 3 
 break;
 case 7:                                              // Verplaats spelers omlaag
  beweeg(0,1);                                        // beweeg omlaag
  game = 9;                                           // ga naar stap 3 
 break;
 case 8:                                              // undo functie
  undo();                                             // ga terug naar vorige positie
  game = 3;                                           // ga naar stap 3 
 break;
 case 9:                                              // spelers thuis. nieuw level of game
 if (controle() == 1){                                // controleer of spelers thuis zijn 
  tijd = millis();                                    // sla systeemtijd op   
  game = 10;                                          // ga naar stap 10
  }
  else {game = 3;}                                    // zo niet, ga naar stap 3
 break;
 case 10:                                             // volgende level
   if (millis() > tijd + wacht2) {                    // wachttijd tot volgende level
    level++;                                          // volgende level nr
    if (level == 60) {                                // zijn alle levels gespeeld?
     level = 0;                                       // start weer met level 0
     EEPROM_schrijf(adresEEPROM+2,0x00);              // zet level "0" in EEPROM  
     memset(veld,0,sizeof(veld));                     // wis oude speelveld 
     game = 0;                                        // ga naar stap 0
     break;
    }
    EEPROM_schrijf(adresEEPROM+2,level);              // zet level nr in EEPROM
    tijd2 = millis();                                 // sla systeemtijd op
    game = 2;                                         // ga naar stap 2
  }     
 break;
 }
 if (introScherm == 1) {print_speelveld();}           // zet speelveld en level nr op lcd
 arduboy.display();                                   // zet video geheugen op lcd
}
// ------------------------------------------------------------------------------------------

// lees knoppen (keuze new game of continue vorige spel)
void nieuw_continu() { 
 sprites.drawOverwrite(72,  34, tek, 3 + inv);        // sprites op openingsscherm
 arduboy.display();                                   // zet video geheugen op lcd
 if (arduboy.justPressed(A_BUTTON) == true) {         // is er op A gedrukt?
  level = 0;                                          // maak level nr "0"
  EEPROM_schrijf(adresEEPROM+2,0x00);                 // zet level "0" in EEPROM
  introScherm = 1;                                    // schakel print speelveld in
  game = 2;                                           // ga naar stap 2
 }
 if (arduboy.justPressed(B_BUTTON) == true) {         // is er op B gedrukt
  introScherm = 1;                                    // schakel print speelveld in
  game = 2;                                           // ga naar stap 2
 } 
}
// ------------------------------------------------------------------------------------------

// Beginscherm met keuze menu (new spel of doorgaan)
void intro(){
 arduboy.drawCompressed(0,0, start, WHITE);           // zet intro tekening op scherm
 arduboy.setCursor(21,53);                            // zet cursor goed
 arduboy.print("Frank");                              // zet tekst op display
 arduboy.setCursor(53,53);                            // zet cursor goed
 arduboy.print("van");                                // zet tekst op display
 arduboy.setCursor(74,53);                            // zet cursor goed
 arduboy.print("de");                                 // zet tekst op display
 arduboy.setCursor(89,53);                            // zet cursor goed
 arduboy.print("Ven");                                // zet tekst op display
 arduboy.display();                                   // zet video geheugen op het display
 tijd = millis();                                     // sla huidige tijd op
 while (millis() < tijd + wacht3){delay(1);}                   // wacht tot wachttijd 3 om is
 arduboy.fillRect(15,53,96, 10, BLACK);               // zet een zwart blok op het scherm
 arduboy.setCursor(4,53);                             // zet cursor goed
 arduboy.print("A:");                                 // zet tekst op display
 arduboy.setCursor(15,53);                            // zet cursor goed
 arduboy.print("New");                                // zet tekst op display
 arduboy.setCursor(37,53);                            // zet cursor goed
 arduboy.print("game");                               // zet tekst op display
 if (level > 0){                                      // laat continue zien bij level > 0
  arduboy.setCursor(65,53);                           // zet cursor goed
  arduboy.print("B:");                                // zet tekst op display
  arduboy.setCursor(77,53);                           // zet cursor goed
  arduboy.print("Continue");                          // zet tekst op display
 }
 arduboy.fillRect(27,22,62,28, BLACK);                // zet een zwart blok op het scherm
 arduboy.drawRect(31,31,52,14, WHITE);                // zet hierin een rechthoek
 sprites.drawOverwrite(33,  34, tek, 2);              // sprite op openingsscherm
 sprites.drawOverwrite(42,  34, tek, 5);              // sprite op openingsscherm
 sprites.drawOverwrite(52,  34, tek, 1);              // sprite op openingsscherm
 sprites.drawOverwrite(62,  34, tek, 6);              // sprite op openingsscherm
 arduboy.display();                                   // zet video geheugen op lcd
}
// ------------------------------------------------------------------------------------------

// controleer of alle spelers thuis zijn
bool controle() { 
 byte contr = 0;                                      // tijdelijke controle variabele
 for (z = 0; z < 3; z++) {                            // doorloop alle spelers
 if (veld[manX[z]][manY[z]] == 4) {                   // staat speler op doel?
  contr ++;}                                          // zo ja verhoog teller met 1
 }
 if (contr == 3) {contr = 1;}                         // alle spelers binnen? maak return 1
 else {contr = 0;}                                    // zo niet maak return 0
 return(contr);                                       // stuur 1 terug als je binnen bent
}
// ------------------------------------------------------------------------------------------

// Undo functie
void undo(){
 memcpy(veld,veld_old,sizeof(veld));                  // zet oude speelveld terug
 byte man = 0;                                        // maak teller 0 (1e speler)
 for (x = 0; x < 16; x++){                            // doorloop het speelveld horizontaal
  for (y = 0; y < 8; y++){                            // doorloop het speelveld verticaal
   if (veld[x][y] == 3 || veld[x][y] == 4) {          // kijk of hier een speler staat
    manX[man] = x;                                    // ja: sla x waarde speler op
    manY[man] = y;                                    // ja: sla y waarde speler op
    man++;                                            // volgende speler
   }
  }
 }
}
// ------------------------------------------------------------------------------------------

// verplaats de spelers in de opgegeven richting (x,y)
void beweeg(int rX, int rY) {
 int aX, bX, aY, bY;                                  // x en y waardes (a = old b = nieuw)
 for (z = 0; z < 3; z++) {                            // doorloop alle spelers
  if ((manX[z] + rX) > -1 && (manX[z] + rX) < 16) {   // valt nieuwe x positie in speelveld
   if ((manY[z] + rY) > -1 && (manY[z] + rY) < 8) {   // valt nieuwe y positie in speelveld
    aX = manX[z]; aY = manY[z];                       // sla huidige x y positie speler op
    if(veld[aX+rX][aY+rY] != 1 && veld[aX+rX][aY+rY] != 3 && veld[aX+rX][aY+rY] != 4) { 
    manX_old[z] = manX[z];                            // backup oude x positie speler
    manY_old[z] = manY[z];                            // backup oude y positie speler 
    memcpy(veld_old,veld,sizeof(veld));               // backup speelveld        
    }
   }
  }
 }
 for (byte tel = 0; tel < 3; tel++) {                 // doorloop de lus 3 keer
  for (z = 0; z < 3; z++) {                           // doorloop alle spelers
   if ((manX[z] + rX) > -1 && (manX[z] + rX) < 16) {  // valt nieuwe x positie in speelveld
    if ((manY[z] + rY) > -1 && (manY[z] + rY) < 8) {  // valt nieuwe y positie in speelveld
     verplaats = 1;                                   // reset "is een speler verplaatst?"
      while (verplaats == 1) {                        // doorloop tot spelers stil staan 
        verplaats = 0;                                // reset "is een speler verplaatst?"
      aX = manX[z]; aY = manY[z];                     // sla huidige x y positie speler op
      bX = manX[z] + rX; bY = manY[z] + rY;           // sla nieuwe x y positie speler op
      if (veld[bX][bY] == 0 || veld [bX][bY] == 2) {  // is het nieuwe veld leeg of doel?
       if (veld[aX][aY] == 4) {veld[aX][aY] = 2;}     // oude veld doel met speler -> doel
       else {veld[aX][aY] = 0;}                       // anders veld wordt leeg
       if (veld[bX][bY] == 2) {veld[bX][bY] = 4;}     // is nieuwe veld doel -> doel+ speler
       else {veld[bX][bY] = 3;}                       // anders veld wordt speler
       manX[z] = bX;                                  // sla nieuwe x positie speler op
       manY[z] = bY;                                  // sla nieuwe y positie speler op       
       verplaats = 1;                                 // geef aan dat speler verplaatst is
      }
     print_speelveld();                               // zet het speelveld op het display 
     arduboy.display();                               // zet video geheugen op lcd
     tijd = millis();                                 // haal systeemtijd op
     while (millis() < tijd + wacht) {delay(1);}               // wacht tot tijd (wacht) om is
     if (rX == -1 && manX[z] == 0) {verplaats = 0;}   // staat speler links: einde loep
     if (rX == 1 && manX[z] == 15) {verplaats = 0;}   // staat speler rechts: einde loep    
     if (rY == -1 && manY[z] == 0) {verplaats = 0;}   // staat speler bovenaan: einde loep
     if (rY == 1 && manY[z] == 7) {verplaats = 0;}    // staat speler onderaan: einde loep
     }   
    }
   } 
  }
 }
}
// ------------------------------------------------------------------------------------------

// Invoer knoppen (game: 0=niets 1=links 2=rechts 3=omhoog 4=omlaag 5=A 6=B)
void invoer(){
 if (arduboy.justPressed(LEFT_BUTTON) == true)  {game = 4;}  // links ingedrukt
 if (arduboy.justPressed(RIGHT_BUTTON) == true) {game = 5;}  // rechts ingedrukt
 if (arduboy.justPressed(UP_BUTTON) == true)    {game = 6;}  // omhoog ingedrukt
 if (arduboy.justPressed(DOWN_BUTTON) == true)  {game = 7;}  // omlaag ingedrukt
 if (arduboy.justPressed(A_BUTTON) == true)     {game = 8;}  // A ingedrukt (undo)
 if (arduboy.justPressed(B_BUTTON) == true)     {game = 2;}  // B ingedrukt (opnieuw)
}
// ------------------------------------------------------------------------------------------

// print speelveld en level nummer
void print_speelveld(){
 byte lees,zet;                                       // tijdelijke variabelen
 for (byte x = 0; x<16; x++){                         // doorloop het speelveld horizontaal
  for (byte y = 0; y<8; y++){                         // doorloop het speelveld verticaal
   lees = veld[x][y];                                 // lees de waarde uit het speelveld
   if (lees != 0) {                                   // is dit veld niet leeg?
    zet = lees -1;                                    // bereken het sprite nr
   if (lees == 4) {zet = lees -1 + inv;}              // speler op doel? laat hem knipperen
   sprites.drawOverwrite(x*8,  y*8, tek, zet);        // sprites uit speelveld naar display
   }
  }
 }
 arduboy.drawRect (0,0,128,64, WHITE);                // zet de buitenrand op het scherm
  if (millis() < tijd2 + wacht4) {                    // print level nr voor wacht4 ms
   if (level < 9) {                                   // is het level nr < 9? 
    arduboy.fillRect(108,4,9,11, BLACK);              // zet een zwart blok op het scherm
    arduboy.drawRect (108,4,9,11, WHITE);             // zet er een witte rand opheen   
   }
   else {                                             // level nr groter
    arduboy.fillRect(108,4,15,11, BLACK);             // zet een zwart blok op het scherm
    arduboy.drawRect (108,4,15,11, WHITE);            // zet er een witte rand opheen
  }
   arduboy.setCursor(110,6);                          // zet cursor goed
   arduboy.print(level + 1);                          // zet level nr op display
  }
}
//------------------------------------------------------------------------------------------

// lees speelveld (0=leeg, 1=muur, 2=doel, 3=speler, 4=speler op doel)
void lees_speelveld(byte level2) {
 memset(veld,0,sizeof(veld));                         // wis oude speelveld
 // lees muren
 for (x = 0; x < 16; x++){                            // doorloop het speelveld horizontaal
 z = pgm_read_byte (& leveldata[x + level2*22]);      // lees muren uit speelveld data
  for (y = 0; y < 8; y++){                            // doorloop het speelveld verticaal
   if (bitRead(z,y) == 1){                            // staat hier een muur
    veld[x][y] = 1;                                   // ja, zet deze muur in spel array
   }
  }
 }
 // lees doelen
 byte doelX,doelY;                                    // tijdelijke variabelen
 for (x = 16; x < 19; x++){                           // doorloop alle doelen
  z = pgm_read_byte (& leveldata[x + level2*22]);     // lees doelen uit speelveld data
  doelX = (z & 0xF0) >> 4;                            // bereken de x positie van het doel
  doelY = z & 0x0F;                                   // bereken de y positie van het doel
  veld[doelX][doelY] = 2;                             // zet doel in spel array
 }
 // lees speler
 byte spelerX,spelerY;                                // tijdelijke variabelen
 for (x = 19; x < 22; x++){                           // doorloop alle spelers
  z = pgm_read_byte (& leveldata[x + level2*22]);     // lees spelers uit speelveld data
  spelerX = (z & 0xF0) >> 4;                          // bereken de x positie van de speler
  spelerY = z & 0x0F;                                 // bereken de y positie van de speler
  if (veld[spelerX][spelerY] == 2) {                  // staat er een doel op deze plaats
   veld[spelerX][spelerY] = 4;                        // ja zet hier een knipperende speler
  }
  else {                                              // nee
   veld[spelerX][spelerY] = 3;                        // zet hier een speler
  }
 manX[x-19] = spelerX;                                // sla x positie speler op
 manY[x-19] = spelerY;                                // sla y positie speler op
 manX_old[x-19] = spelerX;                            // sla x positie speler op
 manY_old[x-19] = spelerY;                            // sla y positie speler op 
 memcpy(veld_old,veld,sizeof(veld));                  // backup speelveld        
 }
}
//------------------------------------------------------------------------------------------

// Haal het level nr uit EEPROM. Staat er nog geen nummer is sla nr 0 op in EEPROM
void laad_level_nr(){
 level = 13;                                   // start met level 0
 if ((EEPROM_lees(adresEEPROM) == 0x46) && (EEPROM_lees(adresEEPROM+1) == 0x56)) {
                                              // kijk of de letters "F" "V" in EEPROM staan.
  level = EEPROM_lees(adresEEPROM+2);}        // ja: haal laatste level uit EEPROM
 else {                                       // nee:
  EEPROM_schrijf(adresEEPROM,0x46);           // zet "F" in EEPROM
  EEPROM_schrijf(adresEEPROM+1,0x56);         // zet "V" in EEPROM
  EEPROM_schrijf(adresEEPROM+2,0x00);         // zet level "0" in EEPROM
 }
}
//-------------------------------------------------------------------------------------------

// Routine schrijft alleen naar EEPROM als data is aangepast
void EEPROM_schrijf (int adres_eeprom, byte data_eeprom) {
 EEPROM.update(adres_eeprom, data_eeprom);   // Rockbox: EEPROM is a per-game file
}
//----------------------------------------------------------------------------------------

// Routine leest data uit EEPROM
byte EEPROM_lees (int adres_eeprom) {
 return EEPROM.read(adres_eeprom);
}
//-------------------------------------------------------------------------------------------
