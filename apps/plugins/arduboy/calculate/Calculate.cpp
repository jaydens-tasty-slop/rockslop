#include <stdlib.h>
#include <math.h>
// Game:      Calculate
// Copyright: 2025 Frank van de Ven 
// Licence:   MIT
 
 #include <Arduboy2.h>
#include <EEPROM.h>                       // installeer de Arduboy2 bibliotheek
#include "../lib/ab_math.h"
 Arduboy2 arduboy;                           // maakt een Arduboy object aan

// variabelen
 const int adresEEPROM = 0x1F0;              // adres in EEPROM 0x1F0 t/m 0x1F5
 int eeprom1, eeprom2;                       // tijdelijke opslag EEPROM data
 float px, py, coss, sinn, hoek2;            // float variabelen voor cirkel berekeningen 
 bool richting;                              // de richting waarin de klok (cirkel) draait
 int hoek;                                   // de hoek van het cirkeldeel dat getekend wordt
 int hoek3;                                  // tijd die je nog hebt na goed antwoord
 byte nr;                                    // het nummer van het antwoord (0-4)
 byte nr_old;                                // het vorige nummer van het antwoord (0-4)
 byte game;                                  // game status. bepaalt verloop programma
 unsigned int frame_nr;                      // voor vertraging laten zien antwoord.
 long good1;                                 // aantal goede antwoorden
 long sums1;                                 // aantal gemaakte sommen
 long good2;                                 // aantal goede antwoorden uit EEPROM
 long sums2;                                 // aantal gemaakte sommen uit EEPROM
 long perc1;                                 // percentage goede antwoorden
 long perc2;                                 // percentage goede antwoorden uit EEPROM
 byte contr;                                 // controleert of bedachte antwoorden goed zijn
 int A;                                      // bovenste getal som
 int AA;                                     // tijdelijk opslag bovenste getal
 int B;                                      // onderste getal som
 int Q[5];                                   // 5 antwoorden
 byte Z;                                     // soort som (0= +, 1= -, 2= x, 3= /)
 byte Y;                                     // nr van het antwoord (0-4) dat goed is Q[Y]
 byte X;                                     // offset cursor positie bij 1 cijfer ipv 2
 char teken[5] = {'+','-','x','/',0};        // teken som + null karakter
 
// sprite duimpje
const unsigned char PROGMEM duim[] = {16,16,
 0xE0,0xE0,0xC0,0x80,0x40,0x70,0xC,0x2,0x62,0x5C,0x40,0x40,0x40,0x40,0x40,0x80,
 0x7F,0x7F,0x3F,0x10,0x20,0x20,0x40,0x40,0x40,0x40,0x55,0x55,0x55,0x55,0x35,0xA,
};

// startscherm
const PROGMEM uint8_t opening[] = {
0x7f,0x37,0x10,0x97,0x2b,0xad,0x23,0xcc,0x75,0xb2,0xba,0xab,0xbe,0x7e,0x08,0x31,
0x9d,0x4e,0x87,0xad,0xed,0x76,0x74,0x66,0xb7,0xdb,0xd1,0x96,0x0e,0x2a,0x5c,0x56,
0xae,0x53,0x5a,0xc6,0x99,0x2b,0xad,0xa3,0x4c,0xe5,0x2a,0x9b,0xad,0x5c,0x2a,0xc4,
0x25,0x2b,0xd7,0xa9,0x54,0xe7,0x14,0x94,0xb4,0x54,0x52,0xd9,0x91,0xa4,0x94,0xdc,
0xa6,0x53,0x72,0xa9,0xe4,0x76,0xbb,0xdd,0x6e,0xb3,0xf1,0xeb,0xf8,0xec,0x76,0x9d,
0x4e,0xa7,0xb3,0xdb,0x25,0x73,0x0a,0x51,0x2e,0xd2,0x15,0x8e,0xd2,0xe9,0x94,0x2a,
0xb7,0xdd,0x2a,0xa9,0xac,0x92,0xca,0x6e,0xd7,0xa1,0x32,0x9d,0x4e,0xa7,0x23,0x6d,
0x55,0x77,0x75,0x57,0x77,0xaf,0xa2,0x72,0xaa,0xba,0xba,0xab,0xbb,0x52,0x33,0xa5,
0xd3,0xe9,0x74,0x3a,0xdd,0xab,0xea,0xae,0xee,0xea,0x7a,0x57,0x3a,0xdd,0xb9,0xd2,
0x3a,0x64,0x2d,0x95,0x54,0x4e,0x39,0x25,0x55,0xed,0x6a,0xab,0xa4,0x73,0x52,0xe9,
0x7c,0x7a,0x2b,0x95,0x5e,0x2b,0xe9,0x94,0x53,0x52,0xbd,0x55,0x52,0x49,0x65,0x95,
0x54,0x52,0x49,0xe5,0x94,0x54,0x52,0xc9,0x6d,0xe8,0x4a,0xa7,0xd3,0xe9,0x74,0x3a,
0x1d,0x6d,0x3a,0x74,0x29,0xa9,0xa4,0x93,0xca,0xce,0xe9,0x9c,0x54,0x76,0xd5,0xbf,
0xea,0xad,0xb2,0xea,0x56,0xb7,0x7a,0xbb,0x8e,0x93,0x7e,0xab,0xec,0x3a,0x9d,0x55,
0x52,0xd9,0x75,0x3a,0x9d,0x4e,0xa7,0x3b,0x57,0x5a,0xa7,0xd3,0xa5,0xbe,0x4a,0x55,
0xa5,0xaa,0x52,0xd5,0x0e,0x6d,0xe9,0x74,0xec,0x59,0xe5,0x9c,0x8e,0x7f,0xa9,0xe4,
0x4a,0x39,0xb7,0xcd,0x75,0x2b,0xe7,0x9c,0x72,0x4a,0x65,0x77,0x4e,0x49,0xa5,0x43,
0x6c,0xa7,0x3a,0xd7,0xb9,0x4e,0x4e,0x96,0x4e,0x77,0xae,0x76,0x1f,0xe4,0xa4,0x5c,
0xe5,0x3a,0xd7,0xb9,0x54,0x6c,0xcb,0xca,0x75,0x4a,0xea,0xf0,0xb9,0xdd,0x86,0x21,
0xa7,0x6c,0x5c,0xca,0x39,0x25,0x95,0x74,0xae,0x94,0x55,0x76,0x4c,0xa5,0xd3,0xd1,
0xa7,0xd3,0xe1,0x54,0x29,0xa7,0x94,0x72,0x6e,0x1b,0x96,0xb2,0x3b,0xa7,0x14,0x53,
0x7a,0xbb,0xdd,0xa6,0x43,0x6e,0x3a,0xe9,0x6c,0x3a,0xc2,0x75,0xe7,0x4a,0x96,0x8e,
0x2d,0xb5,0xaa,0xee,0xea,0xae,0xae,0x94,0x43,0x77,0x36,0x9b,0x8d,0x32,0xdf,0xb9,
0x92,0x65,0xe3,0x54,0xae,0x72,0x5d,0xd7,0xb9,0x54,0xf0,0xd1,0x56,0xd9,0xe9,0x6d,
0x64,0x4b,0xa5,0xf3,0xa9,0x4f,0x59,0x65,0x9d,0x55,0x3a,0x3c,0xdb,0xb5,0xb2,0x4a,
0x3a,0xe7,0x94,0x73,0x4e,0x39,0xe5,0x94,0x53,0x32,0x18,0x2a,0x9d,0xeb,0x74,0x3a,
0x9d,0x4e,0x27,0x2b,0xa5,0xea,0xae,0x52,0x2a,0x9d,0xac,0xee,0xaa,0x54,0x55,0x77,
0x25,0x7b,0x15,0x6e,0x3b,0x9d,0x4e,0xa7,0xd3,0xe9,0x74,0xb2,0x72,0x97,0xac,0x74,
0x3a,0xd7,0xc9,0x4a,0x55,0x75,0x57,0xb9,0xd3,0xc1,0x5d,0x69,0x65,0x9d,0x75,0x56,
0x59,0x67,0x9d,0x75,0x4e,0x66,0x97,0x50,0xf3,0x3a,0x9d,0x75,0x32,0x3a,0xe5,0x3a,
0xd7,0xb9,0x4e,0x07,0x97,0xad,0xd3,0x59,0x27,0x83,0x5d,0x01
};
// =========================================================================================

void bereken_antwoorden();
void controleer_antwoord();
void bereken_som();
void controle();
void game_over();
void new_game();
void print_cursor();
void wis();
void bereken_speeltijd();
void teken_klok();
void cirkel(int pos, int kleur);
void print_som();
void EEPROM_schrijf (int adres_eeprom, byte data_eeprom);
byte EEPROM_lees (int adres_eeprom);

void setup() {
 arduboy.begin();                            // initialiseert de Arduboy2 bibliotheek
 arduboy.setFrameRate(60);                   // framerate 60 beelden per sec.
 arduboy.setTextSize(1);                     // stel font grote in
 arduboy.clear();                            // wis het display
 arduboy.drawCompressed(0,0, opening, WHITE);// teken een gecomprimeerd plaatje
 arduboy.setCursor(5,56);                    // stel cursor positie in
 arduboy.print("A:");                        // zet tekst op display
 arduboy.setCursor(17,56);                   // stel cursor positie in
 arduboy.print("Play");                      // zet tekst op display
 arduboy.setCursor(46,56);                   // stel cursor positie in
 arduboy.print("game");                      // zet tekst op display
 arduboy.setCursor(88,56);                   // stel cursor positie in
 arduboy.print("B:");                        // zet tekst op display
 arduboy.setCursor(100,56);                  // stel cursor positie in
 arduboy.print("Code");                      // zet tekst op display
 arduboy.display();                          // zet inhoud video ram op display
 while (arduboy.pressed(A_BUTTON) == false){ delay(1); // Verlaat lus als er op A gedrukt wordt
  if (arduboy.pressed(B_BUTTON) == true) {   // Wordt er op B gedrukt?
   arduboy.fillRect (5,55,120,8,BLACK);      // Ja: teken een zwarte rechthoek (wis tekst)
   arduboy.setCursor(0,56);                  // stel cursor positie in
   arduboy.print("Code:");                   // zet tekst op display
   arduboy.setCursor(32,56);                 // stel cursor positie in
   arduboy.print("Frank van de Ven");        // zet tekst op display
   arduboy.display();                        // zet inhoud video ram op display
  }
  arduboy.initRandomSeed();                  // init random getallen
 }
 game = 0;                                   // start nieuw spel
}
// =========================================================================================

void loop() {
 if (!(arduboy.nextFrame())) {return;}       // wacht tot het tijd is voor volgende frame
 arduboy.pollButtons();                      // kijk naar knoppen
 switch (game) {
 case 0:                                     // stel alles in voor nieuwe game
  new_game();                                // reset variabelen + opbouw scherm
  break;
 case 1:                                     // bereken som
  bereken_som();
  break;
 case 2:                                     // bereken antwoorden
  bereken_antwoorden();
  break;
 case 3:                                     // print som en antwoorden
  print_som();
  break;
 case 4:                                     // speel spel 
  teken_klok();                              // teken de klok (cirkel)
  if (arduboy.justPressed(LEFT_BUTTON)) {    // wordt er op de linker knop gedrukt
   nr = nr - 1; if (nr == 255) {nr = 4;}}    // verplaats cursor naar links
  if (arduboy.justPressed(RIGHT_BUTTON)) {   // wordt er op de rechter knop gedrukt
   nr = nr + 1; if (nr == 5) {nr = 0;}}      // verplaats cursor naar rechts
  if (arduboy.justPressed(B_BUTTON) || arduboy.justPressed(UP_BUTTON)) {//op B of up gedrukt?
   frame_nr = arduboy.frameCount;
   game = 5;}
  print_cursor();                            // PRINT DE CURSOR POSITIE
  break;
 case 5:                                     // controleer antwoord
  controleer_antwoord();
  break;
 case 6:                                     // tijd is om. einde spel
  game_over();
  break;
 }
 arduboy.display();                          // zet inhoud video ram op display
}                                            // einde loop
// =========================================================================================

void bereken_antwoorden() {
 for (byte tel3 = 0; tel3 < 5; tel3++) {     // loep loopt door alle antwoorden
  if (tel3 != Y) {                           // staat op deze pos. het antwoord? Nee:
   do {                                      // blijf antwoorden bedenken als contr 1 is
   contr = 0;                                // maak de controle bit 0
// 9 verschillende berekeningen om een antwoord te bedenken
   switch(random(0,9)) {                     // kies een willekeurige berekening
   case 0: // Vermenigvuldig de uitkomst met 1,2 (rond het getal af naar een geheel getal)
   Q[tel3] = Q[Y] * 1.2;
   break;
   case 1: // Deel de uitkomst door 1,2 (rond het getal af naar een geheel getal)
    Q[tel3] = Q[Y] / 1.2;  
   break;
   case 2: // Vermenigvuldig de uitkomst met 1,5 (rond het getal af naar een geheel getal)
    Q[tel3] = Q[Y] * 1.5;
   break;
   case 3: // Deel de uitkomst door 1,5 (rond het getal af naar een geheel getal)
    Q[tel3] = Q[Y] / 1.5;
   break;
   case 4: // Tel bij de uitkomst een willekeurig getal (tussen 1 en 10) bij op
    Q[tel3] = Q[Y] + random(1,11);
   break;
   case 5: // Trek van de uitkomst een willekeurig getal (tussen 1 en 10) van af
    Q[tel3] = Q[Y] - random(1,11);
   break;
   case 6: // Tel bij de uitkomst 10, 20 of 30 (random gekozen) bij op
    Q[tel3] = Q[Y] + (random(1,4) * 10);
   break;
   case 7: // Trek van de uitkomst 10,20 of 30 (random gekozen) van af
    Q[tel3] = Q[Y] - (random(1,4) * 10);
   break;
   case 8: // Kies een willekeurig getal tussen 1 en 99
    Q[tel3] = random(1,100);
   break;
   }
// controleer het antwoord of dat kan (b.v. <100. bij fout: maak "contr = 1")
   if (Q[tel3] > 99) {contr = 1;}                 // getal > 99 is fout
   if (Q[tel3] < 0)  {contr = 1;}                 // getal onder 0 is fout
   switch(Z) {                                    // controleer de soort berekening
   case 0:                                        // controleer optelling
    if (Q[tel3] < A || Q[tel3] < B) {contr = 1;}  // getal mag niet kleiner dan A of B zijn
   break;
   case 1:                                        // controleer aftreksom
    if (Q[tel3] > A) {contr = 1;}                 // getal mag niet groter dan A zijn
   break;
    case 2:                                       // controleer vermenigvuldiging
    if(Q[tel3] < A || Q[tel3] < B) {contr = 1;}   // getal mag niet kleiner dan A of B zijn 
   break;
   case 3:                                        // controleer deling
    if (Q[tel3] > A) {contr = 1;}                 // getal mag niet groter dan A zijn
   break;
   }
    if (Q[0] == Q[1] || Q[0] == Q[2]) {contr = 1;} // controle dubbele getallen
    if (Q[0] == Q[3] || Q[0] == Q[4]) {contr = 1;} // controle dubbele getallen
    if (Q[1] == Q[2] || Q[1] == Q[3]) {contr = 1;} // controle dubbele getallen
    if (Q[1] == Q[4] || Q[2] == Q[3]) {contr = 1;} // controle dubbele getallen
    if (Q[2] == Q[4] || Q[3] == Q[4]) {contr = 1;} // controle dubbele getallen
   }
   while (contr == 1);                       // blijf antwoorden bedenken als contr 1 is
  }
 } 
 game = 3;                                   // zet som en antwoorden op display
}
// -----------------------------------------------------------------------------------------

void controleer_antwoord(){
 if (nr == Y){                               // kijk of cursor op positie juiste antwoord is
  Sprites::drawOverwrite(10, 27, duim, 0);   // print sprite duimpje links
  Sprites::drawOverwrite(102, 27, duim, 0);  // print sprite duimpje rechts
  bereken_speeltijd();                       // bereken tijd die je nog hebt+ zet klok terug
 }

 if (arduboy.frameCount < frame_nr + 20){    // laat het antwoord 15 frames zien
  if (Q[Y] > 9) {X = 0;} else {X = 6;}       // bereken offset X pos. cursor bij 1 cijfer
  arduboy.setCursor(60 + X,32);              // stel cursor positie in
  arduboy.print(Q[Y]);                       // print het antwoord onder de som
  arduboy.display();                         // zet inhoud video ram op display
 }
 else {                                      // na 15 frames bereken een nieuwe som
  sums1++;                                   // verhoog teller aantal sommen met 1
  if (nr == Y) {good1++;}                    // verhoog teller goed gemaakte sommen met 1
  arduboy.setCursor(5,10);                   // stel cursor positie in
  arduboy.print(sums1);                      // print totaal gemaakte sommen
  arduboy.setCursor(110,10);                 // stel cursor positie in
  arduboy.print(good1);                      // print totaal goed gemaakte sommen
  arduboy.display();                         // zet inhoud video ram op display
  game = 1;                                  // bereken nieuwe som en antwoorden
  if (sums1 > 998) {game = 6;}               // bij meer dan 999 sommen: game over
 }
}
// -----------------------------------------------------------------------------------------


void bereken_som() {
 for (byte tel4 = 0; tel4 <5; tel4++){       // loep doorloopt alle antwoorden
  Q[tel4] = tel4 + 100;                      // laad de antwoorden met een verschillend getal
 }
 Z = random(0,4);                            // kies een willekeurige som (+-*/)
 Y = random(0,5);                            // kies welk antwoord (0-4) goed is
 switch (Z) {
 case 0:                                     // bedenk een optelsom
  A = random(2,100);                         // kies getal A (tussen 2 en 99)
  B = random(2,100);                         // kies getal B (tussen 2 en 99)
  do {
   Q[Y] = A + B;                             // bereken de uitkomst van de optelling.
   controle();                               // controleer of uitkomst groter dan 99 is,
  }
  while (Q[Y] > 99);                         // blijf proberen tot antwoord < 100 is
 break;
 case 1:                                     // bedenk een aftreksom
  A = random(2,100);                         // kies getal A (tussen 2 en 99)
  B = random(2,100);                         // kies getal B (tussen 2 en 99)
   if (A < B) {AA = B; B = A; A = AA;}       // keer A en B om
   Q[Y] = A - B;                             // bereken de uitkomst van de aftreksom 
 break;
 case 2:                                     // bedenk een vermenigvuldiging
  A = random(2,100);                         // kies getal A (tussen 2 en 99)
  B = random(2,100);                         // kies getal B (tussen 2 en 99)
  do {
   Q[Y] = A * B;                             // bereken de uitkomst van de aftreksom
   controle();                               // controleer of uitkomst groter dan 99 is,
  }
  while (Q[Y] > 99);                         // blijf proberen tot antwoord < 100 is
 break;
 case 3:                                     // gedenk een deling
  A = random(2,100);                         // kies getal A (tussen 2 en 99)
  B = random(2,100);                         // kies getal B (tussen 2 en 99)
  do {
   Q[Y] = A * B;                             // bereken de uitkomst van de aftreksom
   controle();                               // controleer of uitkomst groter dan 99 is,
  }
  while (Q[Y] > 99);                         // blijf proberen tot antwoord < 100 is
  AA = Q[Y]; Q[Y] = A; A = AA;               // maak van een vermenigvuldiging een deling
 break;
 }
 game = 2;                                   // som is klaar. ga nu de antwoorden bedenken
}
// -----------------------------------------------------------------------------------------

// controleer of uitkomst groter dan 100 is. zo ja deel het grootste getal door 2.
void controle() {
 if (Q[Y] > 99) {                            // is het antwoord > 99?
 if (A - B == 0) {A = A / 2;}                // ja, als A gelijk is aan B: deel A door 2
 else                                        // anders
  {if (A - B > 0) {A = A / 2;}               // is A groter dan B: deel A door 2
   else {B = B / 2;}                         // is A kleiner dan B: deel B door 2
   }
 }
}
// -----------------------------------------------------------------------------------------

// game over
void game_over() {
 if (sums1 < 1) {perc1 = 0;}                 // bij geen goede sommen percentage = 0%
  else {perc1 = (good1 * 100) / sums1;}      // bereken percentage
 if ((EEPROM_lees(adresEEPROM) == 0x46) && (EEPROM_lees(adresEEPROM+1) == 0x56)) {
                                             // kijk of de letters "F" "V" in EEPROM staan.
 eeprom1 = EEPROM_lees(adresEEPROM+2);       // lees 1e byte uit EEPROM
 eeprom2 = EEPROM_lees(adresEEPROM+3);       // lees 2e byte uit EEPROM
 sums2 = eeprom1 << 8;                       // zet 1e byte in sums2 en verplaats 8 bits
 sums2 = sums2 + eeprom2;                    // tel 2e byte bij sums2 op
 eeprom1 = EEPROM_lees(adresEEPROM+4);       // lees 3e byte uit EEPROM
 eeprom2 = EEPROM_lees(adresEEPROM+5);       // lees 4e byte uit EEPROM
 good2 = eeprom1 << 8;                       // zet 1e byte in good2 en verplaats 8 bits
 good2 = good2 + eeprom2;                    // tel 2e byte bij good2 op
 if (sums2 < 1) {perc2 = 0;}                 // bij geen goede sommen percentage = 0%
 else {perc2 = (good2 * 100) / sums2;}       // bereken percentage
 }
 else {                                       // Nee, staat nog geen game data
  EEPROM_schrijf(adresEEPROM,0x46);           // zet "F" in EEPROM
  EEPROM_schrijf(adresEEPROM+1,0x56);         // zet "V" in EEPROM
  EEPROM_schrijf(adresEEPROM+2,0x00);         // zet "00" in EEPROM
  EEPROM_schrijf(adresEEPROM+3,0x00);         // zet "00" in EEPROM
  EEPROM_schrijf(adresEEPROM+4,0x00);         // zet "00" in EEPROM
  EEPROM_schrijf(adresEEPROM+5,0x00);         // zet "00" in EEPROM
  sums2 = 0;                                  // maak aantal sommen uit EEPROM 00
  good2 = 0;                                  // maak aantal goed uit EEPROM 00
  perc2 = 0;                                  // maak percentage uit EEPROM 00
 }
 if ((sums1 == sums2 && good1 > good2) || (sums1 > sums2)) {// nieuwe high score wegschrijven
 eeprom1 = sums1 >> 8;
 eeprom2 = sums1 & 0x00FF;
 EEPROM_schrijf(adresEEPROM+2,eeprom1);       // zet LSB deel sums1 in EEPROM
 EEPROM_schrijf(adresEEPROM+3,eeprom2);       // zet MSB deel sums1 in EEPROM
 eeprom1 = good1 >> 8;
 eeprom2 = good1 & 0x00FF;
 EEPROM_schrijf(adresEEPROM+4,eeprom1);       // zet LSB deel good1 in EEPROM
 EEPROM_schrijf(adresEEPROM+5,eeprom2);       // zet MSB deel good1 in EEPROM
 }
 arduboy.clear();                            // wis het display
 arduboy.setTextSize(2);                     // stel font grote in
 arduboy.setCursor(10,28);                   // stel cursor positie in
 arduboy.print("Game over");                 // zet tekst op display
 arduboy.setTextSize(1);                     // stel font grote in
 arduboy.setCursor(0,0);                     // stel cursor positie in
 arduboy.print("Sums:");                     // print "Sums"
 arduboy.print(sums1);                       // print totaal gemaakte sommen
 arduboy.setCursor(53,0);                    // stel cursor positie in
 arduboy.print("Good:");                     // print "Good"
 arduboy.print(good1);                       // print totaal goed gemaakte sommen
 arduboy.setCursor(105,0);                   // stel cursor positie in
 arduboy.print(perc1);                       // print percentage gemaakte sommen
 arduboy.print("%");                         // print "%"
 if (sums2 != 0) {                           // Print Hi score als EEPROM sums2 > 0 is
  arduboy.setCursor(12,10);                  // stel cursor positie in
  arduboy.print("Hi:");                      // print "Hi"
  arduboy.print(sums2);                      // print totaal gemaakte sommen uit EEPROM
  arduboy.setCursor(53,10);                  // stel cursor positie in
  arduboy.print("Good:");                    // print "Good"
  arduboy.print(good2);                      // print totaal goed gemaakte sommen uit EEPROM
  arduboy.setCursor(105,10);                 // stel cursor positie in
  arduboy.print(perc2);                      // print percentage gemaakte sommen uit EEPROM
  arduboy.print("%");                        // print "%"
 }
 arduboy.setCursor(4,55);                    // stel cursor positie in
 arduboy.print("Press A for new game");      // zet tekst op display
 if (arduboy.justPressed(A_BUTTON)) {game = 0;} // wordt er op A gedrukt start nieuw spel
}
// -----------------------------------------------------------------------------------------

// opbouw scherm voor nieuw spel + reset variabelen:
void new_game() {
 arduboy.clear();                            // wis het display
 for (int i=1; i< 178; i++) {cirkel (i,1);}  // teken een volledige cirkel
 richting = 0;                               // richting klok = 0: (aftellen)
 arduboy.setCursor(0,0);                     // stel cursor positie in
 arduboy.print("Sums");                      // print Sums
 arduboy.setCursor(105,0);                   // stel cursor positie in
 arduboy.print("Good");                      // print Good
 good1 = 0;                                  // reset teller aantal goede antwoorden
 sums1 = 0;                                  // reset aantal gemaakte sommen
 hoek = 1;                                   // de hoek van het cirkeldeel dat getekend wordt
 hoek3 = 1;                                  // tijd die je nog hebt na goed antwoord
 nr = 0;                                     // selecteer antwoord 1
 nr_old = 0;                                 // selecteer antwoord 1
 game = 1;                                   // start nieuwe game
}

// -----------------------------------------------------------------------------------------

// print cursor
void print_cursor(){
 arduboy.drawRect (nr_old*20+18,53,15,11,BLACK);// teken een zwarte rechthoek op oude plaats
 arduboy.drawRect (nr*20+18,53,15,11,WHITE); // teken een nieuwe rechthoek op cursor plaats
 nr_old = nr;                                // bewaar cursor positie als vorige positie
}
// -----------------------------------------------------------------------------------------

// wis alle cijfers van display
void wis() {
 arduboy.fillRect (60,10,12,8,BLACK);        // wis bovenste getal
 arduboy.fillRect (52,20,20,8,BLACK);        // wis middelste getal + teken voor het getal
 arduboy.fillRect (60,32,12,8,BLACK);        // wis onderste getal
 arduboy.fillRect (16,53,96,11,BLACK);       // wis 5 antwoorden + cursor
 Sprites::drawErase(10, 27, duim, 0);        // wis sprite duimpje links
 Sprites::drawErase(102, 27, duim, 0);       // wis sprite duimpje rechts
}
// -----------------------------------------------------------------------------------------

// bereken de speeltijd die je nog hebt (maximaal een halve cirkel)
void bereken_speeltijd(){
 richting = 1;                               // draai richting klok om
 hoek3 = hoek - 89;                          // bereken het eindpunt, een halve cirkel terug
 if (hoek3 < 1) {hoek3 = 1;}                 // bij meer dan de max. speeltijd: cirkel max.
}
// -----------------------------------------------------------------------------------------

// teken de klok
void teken_klok(){
 if (richting == 0) {                        // richting klok = 0: (aftellen)
  if (arduboy.everyXFrames(4)){              // voer code om de 4 frames uit (klok)
   cirkel (hoek, richting);                  // wis een deel van de rand van de cirkel
   hoek++;                                   // verhoog positie cirkelboog
    if (hoek >= 178) {game = 6; return;}     // is volledige cirkel gewist verander richting
  }
 }
 else {                                      // richting klok = 1: (optellen) geen vertraging
  for (byte tel1 = 0; tel1 < 4; tel1++) {    // loep. teken 4 cirkel delen in 1 keer
   hoek--;                                   // verlaag positie cirkelboog
   cirkel (hoek, richting);                  // teken een deel van de rand van de cirkel
   if (hoek <= hoek3) {richting = 0; return;}// halve cirkel getekend? verander richting
  }
 }
}
// -----------------------------------------------------------------------------------------

// teken een gedeelte van de cirkel in wit of zwart (helemaal rond is van 1 t/m 177 partjes)
void cirkel(int pos, int kleur){
 hoek2 = pos * 0.0353;                       // 0.0353 = PI/90 + aangepast voor beste cirkel
 sinn = sin(hoek2);                          // sinus berekenen
 coss = cos(hoek2);                          // cosinus berekenen
 for (byte tel2 = 20; tel2 < 24; tel2 ++){   // loep (straal cirkel 20px t/m 23px)
  px = 64 + tel2 * sinn;                     // bereken x pos d.m.v. sinus berekening
  py = 24 + tel2 * coss;                     // bereken y pos d.m.v. cosinus berekening
  arduboy.drawPixel(px, py, kleur);          // zet op de berekende plaats pixel aan / uit
 }
}
// -----------------------------------------------------------------------------------------

// Print de som en antwoorden op het scherm
void print_som() {
// print de som
 wis();                                      // wis cijfers van display
 if (A > 9) {X = 0;} else {X = 6;}           // bereken offset X pos. cursor bij 1 cijfer
 arduboy.setCursor(60 + X,10);               // stel cursor positie in
 arduboy.print(A);                           // print 1e getal
 if (B > 9) {X = 0;} else {X = 6;}           // bereken offset X pos. cursor bij 1 cijfer
 arduboy.setCursor(60 + X,20);               // stel cursor positie in
 arduboy.print(B);                           // print 2e getal
 arduboy.setCursor(52,20);                   // stel cursor positie in
 arduboy.print(teken[Z]);                    // print het teken +-*/ van de som
 arduboy.drawFastHLine (52,29,20);           // teken een streep onder de getallen
// print de 5 antwoorden
 if (Q[0] > 9) {X = 0;} else {X = 6;}        // bereken offset X pos. cursor bij 1 cijfer
 arduboy.setCursor(20 + X,55);               // stel cursor positie in
 arduboy.print(Q[0]);                        // print antwoord 1
 if (Q[1] > 9) {X = 0;} else {X = 6;}        // bereken offset X pos. cursor bij 1 cijfer
 arduboy.setCursor(40 + X,55);               // stel cursor positie in
 arduboy.print(Q[1]);                        // print antwoord 2
 if (Q[2] > 9) {X = 0;} else {X = 6;}        // bereken offset X pos. cursor bij 1 cijfer
 arduboy.setCursor(60 + X,55);               // stel cursor positie in
 arduboy.print(Q[2]);                        // print antwoord 3
 if (Q[3] > 9) {X = 0;} else {X = 6;}        // bereken offset X pos. cursor bij 1 cijfer
 arduboy.setCursor(80 + X,55);               // stel cursor positie in
 arduboy.print(Q[3]);                        // print antwoord 4
 if (Q[4] > 9) {X = 0;} else {X = 6;}        // bereken offset X pos. cursor bij 1 cijfer
 arduboy.setCursor(100 + X,55);              // stel cursor positie in
 arduboy.print(Q[4]);                        // print antwoord 5
 game = 4;                                   // ga naar spelen spel
}
// -----------------------------------------------------------------------------------------

// Routine schrijft alleen naar EEPROM als data is aangepast
void EEPROM_schrijf (int adres_eeprom, byte data_eeprom) {
 EEPROM.update(adres_eeprom, data_eeprom);   // Rockbox: EEPROM is a per-game file
}
byte EEPROM_lees (int adres_eeprom) {
 return EEPROM.read(adres_eeprom);
}
//-------------------------------------------------------------------------------------------
