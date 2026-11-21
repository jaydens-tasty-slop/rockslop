// Game:      Tiles
// Copyright: 2026 Frank van de Ven
// Licence:   MIT
 
 #include <Arduboy2.h>                                // installeer de Arduboy2 bibliotheek
 Arduboy2 arduboy;                                    // maakt een Arduboy object aan
 Sprites sprites;                                     // Namespace voor sprites instellen
 #include "Tiles.h"                                   // grafische data
 #define wacht1 2000                                  // wachttijd 1: openingsscherm
 #define wacht2 2000                                  // wachttijd 2: laat naam zien
 #define wacht3 20                                    // wachttijd 3: tijd wissen tegels
 #define grens1 200                                   // boven deze grens extra tegels
 #define grens2 350                                   // boven deze grens extra tegels
 #define grens3 500                                   // boven deze grens extra tegels
 #define aantal_pogingen 3                            // aantal keer nieuwe tegels kiezen

// variabelen
 byte x, y, x1, y1;                                   // tellers
 byte Cx, Cy;                                         // cursor positie
 byte Cx_old, Cy_old;                                 // vorige cursor positie
 unsigned long wacht;                                 // delay
 unsigned long klok;                                  // opslag speeltijd
 int score;                                           // opslag score
 byte bord[8][8];                                     // spelbord
 byte tegel_max;                                      // tot welke tegel mag je random kiezen
 byte knop[3];                                        // inhoud 3 knoppen
 byte select;                                         // nr. van de geselecteerde knop 0-2
 byte select2;                                        // tijdelijke opslag select
 byte keus;                                           // tegel nr geselecteerde knop
 byte keus_old;                                       // vorige tegel nr geselecteerde knop
 bool knipper;                                        // laat knop knipperen
 byte toets1;                                         // 1=links 2=rechts 3=omhoog 4=omlaag
 byte toets2;                                         // leest knop A 0=knoppen 3=spelbord
 byte B;                                              // tijdelijke variabele invoer A knop
 byte aantal_rand;                                    // aantal keer nieuwe tegels kiezen (2)
 byte pastTegel;                                      // 0 = tegel past niet. 1 = tegel past
 byte rij;                                            // 1 = er zijn rijen te wissen
 byte control;                                        // fouten doorgeven aan loop()
 byte vol[16];                                        // x, y nummers rijen die vol zijn
 byte vol2;                                           // teller aantal volle rijen
// ------------------------------------------------------------------------------------------

void intro();
void einde_spel();
void aantal_tegels();
void wis_rijen();
byte contoleer_rijen();
byte past_tegel();
void herstel_tegelknop();
void wis_cursortegel();
void plaatstegel();
void  bedien_cursortegel();
void plaats_cursortegel();
void wis_tegelknop();
void bedien_knoppen();
void selecteer_tegelknop();
void invoer();
void display_knoppen();
void display_bord();
void tijd_score();
void display_opbouw();
void reset();

void setup() {
 arduboy.begin();                                     // initialiseert de Arduboy2 bibliotheek
 arduboy.setFrameRate(30);                            // opgeven snelheid: 30 frames/sec.
 arduboy.initRandomSeed();                            // init random nummers
 intro();                                             // laat intro zien
 reset();                                             // zet variablen goed
}
// ------------------------------------------------------------------------------------------

void loop() {
 if (!(arduboy.nextFrame())) {return;}                // wacht op volgende frame
 arduboy.pollButtons();                               // is er een knop ingedrukt is geweest?
 if (arduboy.frameCount % 10 == 0) {knipper = knipper ^ 1;} // inverteer knipper bit
 arduboy.clear();                                     // wis het display (wordt zwart)
 display_opbouw();                                    // zet vaste objecten op display
 display_knoppen();                                   // laat knoppen zien en knipperen
 invoer();                                            // leest status knoppen
 switch (toets2) {                                    // besturing A knop
// omschakelen van spelbord naar tegelknoppen:
 case 0:
  pastTegel = past_tegel();                           // past tegel op spelbord?
  if (pastTegel == 1) {                               // tegel past niet op spelbord:
   wis_cursortegel();                                 // Wis de cursortegel
   herstel_tegelknop();                               // schakel de tegelknop weer in
  }
  else {                                              // tegel past op spelbord:
   plaatstegel();                                     // zet de tegel op het spelbord
   rij = contoleer_rijen();                           // controle gevulde rijen (nr opslaan)
   if (rij == 1) {toets2 = 3; break;}                 // zo ja, wis gevulde rijen + score
  }
  toets2 = 1;                                         // ga naar stap 1;
 break;
 case 1:          
  selecteer_tegelknop();                              // selecteer de meest linker tegelknop
  toets2 = 2;                                         // ga naar stap 2
 break;
 case 2:
  bedien_knoppen();                                   // bedien 3 tegelknoppen 
 break;
  case 3:
  wis_rijen();                                        // wis patronen van gevulde rijen
 break;
// omschakelen van tegelknoppen naar spelbord
 case 4:
  wis_tegelknop();                                    // wis de geselecteerde tegelknop
  plaats_cursortegel();                               // plaats cursortegel op spelbord
  toets2 = 5;                                         // ga naar stap 3
 break;
 case 5:
  bedien_cursortegel();                               // bedien cursortegel
 break;
 case 6:
  einde_spel();                                       // na 4 keer andere tegels: einde spel
 break;
 }
 aantal_tegels();                                     // afhankelijk van score meer tegels
 display_bord();                                      // zet speelbord op display
 tijd_score();                                        // zet tijd en score op display
 arduboy.display();                                   // zet video geheugen op het display
}
// ------------------------------------------------------------------------------------------

// zet intro plaatje en naam programmeur op display
void intro(){
 byte intro2;                                         // tijdelijke opslag
 arduboy.clear();                                     // wis het display (wordt zwart)
 for (x=0; x<16; x++){                                // loep horizontaal door het bord
  for (y=0; y<8; y++){                                // loep verticaal door het bord
  intro2 = pgm_read_byte (& plaatje[x][y]);           // haal patroon nr uit PROGMEM array
   sprites.drawOverwrite(x*8,y*8, patroon, intro2);   // zet tegel uit array op display
  }
 }
 for (x = 0; x < 5; x++) {
  sprites.drawSelfMasked(x * 16 + 23, 19, tekst, x + 10); // zet naam spel op display
  sprites.drawSelfMasked(x * 16 + 23, 35, tekst, x + 15); // zet naam spel op display
 }
 arduboy.display();                                   // zet video geheugen op het display
 wacht = millis();                                    // sla huidige tijd op
 while (millis() < wacht + wacht1){}                  // wacht tot wachttijd 1 om is
 arduboy.fillRect(16,49,96, 16, BLACK);               // zet een zwart blok op het scherm
 arduboy.drawRect(18,51,91,13, WHITE);                // zet hierin een rechthoek
 arduboy.setCursor(21,54);                            // zet cursor goed
 arduboy.print("Frank");                              // zet tekst op display
 arduboy.setCursor(53,54);                            // zet cursor goed
 arduboy.print("van");                                // zet tekst op display
 arduboy.setCursor(74,54);                            // zet cursor goed
 arduboy.print("de");                                 // zet tekst op display
 arduboy.setCursor(89,54);                            // zet cursor goed
 arduboy.print("Ven");                                // zet tekst op display
 arduboy.display();                                   // zet video geheugen op het display
 wacht = millis();                                    // sla huidige tijd op
 while (millis() < wacht + wacht2){}                  // wacht tot wachttijd 2 om is
}
// ------------------------------------------------------------------------------------------

// laat "game over" zien en stop programma
void einde_spel() {
 tijd_score();                                        // zet tijd en score op display
 for (x = 0; x < 4; x++){                             // doorloop letters 1e rij
  sprites.drawSelfMasked(x*16+ 7, 13, tekst, x+3);    // zet letters op display
 }
  for (x = 0; x < 3; x++){                            // doorloop letters 2e rij
  sprites.drawSelfMasked(x*16+ 7, 35, tekst, x+7);    // zet letters op display
 }
 arduboy.display();                                   // zet video geheugen op het display
 while (arduboy.pressed(A_BUTTON) == true || arduboy.pressed(B_BUTTON) == true) {delay(1);}
 while (arduboy.pressed(A_BUTTON) == false && arduboy.pressed(B_BUTTON) == false) {delay(1);}
 reset();                                             // reset instellingen voor nieuw spel
}
// ------------------------------------------------------------------------------------------

// afhankelijk van score bereid het aantal te kiezen tegels uit tegels
void aantal_tegels(){
 if (score > grens1) {tegel_max = 12;}
 if (score > grens2) {tegel_max = 16;}
 if (score > grens3) {tegel_max = 18;}
}
// ------------------------------------------------------------------------------------------

// wis gevulde rijen (nummers opgeslagen in array) patroon voor patroon + pas score aan
void wis_rijen() {
 for (x1 = 0; x1 < 8; x1++) {                         // doorloop alle rijen op x as
  if (vol[x1] == 1){                                  // moet deze rij gewist worden?
   score = score + 10;                                // verhoog score met 10 punten
   vol[x1] = 0;                                       // geef aan dat deze rij gewist wordt
   for (y1 = 0; y1 < 8; y1++) {                       // doorloop alle overige y posities
    wacht = millis();                                 // sla huidige tijd op
    while (millis() < wacht + wacht3) {}              // wacht wacht3 ms
    bord[x1][y1] = 0;                                 // wis het patroon van het bord
    display_bord();                                   // zet speelbord op display
    tijd_score();                                     // zet tijd en score op display
    arduboy.display();                                // zet video geheugen op het display
   }
  }
 }
 for (y1 = 0; y1 < 8; y1++) {                         // doorloop alle rijen op x as
  if (vol[y1 + 8] == 1){                              // moet deze rij gewist worden?
   score = score + 10;                                // verhoog score met 10 punten
   vol[y1 + 8] = 0;                                   // geef aan dat deze rij gewist wordt
   for (x1 = 0; x1 < 8; x1++) {                       // doorloop alle overige y posities
    wacht = millis();                                 // sla huidige tijd op
    while (millis() < wacht + wacht3) {}              // wacht wacht3 ms
    bord[x1][y1] = 0;                                 // wis het patroon van het bord
    display_bord();                                   // zet speelbord op display
    tijd_score();                                     // zet tijd en score op display
    arduboy.display();                                // zet video geheugen op het display
   }
  }
 }
 toets2 = 1;                                          // ga naar stap 1;
}
// ------------------------------------------------------------------------------------------

// kijk horizontaal / verticaal of er 8 tegeltjes achter elkaar liggen. Zo ja onthoud de rij
byte contoleer_rijen() {
 control = 0;                                         // bij 1 zijn er complete rijen gezien
 memset (vol,0,sizeof(vol));                          // reset
 for (x = 0; x < 8; x++) {                            // doorloop alle rijen op x as
  vol2 = 0;                                           // reset teller
  for (y = 0; y < 8; y++) {                           // doorloop alle overige y posities
  if (bord[x][y] != 0) {vol2++;}                      // als hier een tegel ligt: teller +1
  }
  if (vol2 == 8){vol[x] = 1; control = 1;}            // als hele rij vol zit: sla x nr op
 }
 for (y = 0; y < 8; y++) {                            // doorloop alle rijen op x as
  vol2 = 0;                                           // reset teller
  for (x = 0; x < 8; x++) {                           // doorloop alle overige y posities
  if (bord[x][y] != 0) {vol2++;}                      // als hier een tegel ligt, teller +1
  }
  if (vol2 == 8){vol[y+8] = 1; control = 1;}          // als hele rij vol zit: sla y nr op
 }
 return control;
}
// ------------------------------------------------------------------------------------------

// kijk of het de cursortegel wel op het bord geplaatst kan worden (ligt er niets onder)
byte past_tegel(){
 control = 0;                                        // bij 1 staat er iets onder de cursor
 for (x = 0; x < maat[keus][0]; x++){                // loep doorloopt alle x posities
  for (y = 0; y < maat[keus][1]; y++){               // loep doorloopt alle y posities
   if (vorm[keus*4+y][x] == 1){                      // ligt hier zichtbaar deel tegel?
    if (bord[x+Cx][y+Cy] != 8) {control++;}          // staat er iets onder verhoog control
   }
  }
 }
 if (control > 1) {control = 1;}                     // 1: er staat iets onder de cursortegel
 return control;
}
// ------------------------------------------------------------------------------------------

// schakel de tegelknop weer in
void herstel_tegelknop() {
 knop[select] = keus;                                 // zet tegelknop weer aan
}
// ------------------------------------------------------------------------------------------

// wis de cursortegel (de oude situatie op het bord wordt hersteld)
void wis_cursortegel() {
 for (x = 0; x < maat[keus_old][0]; x++){             // loep doorloopt alle x posities
  for (y = 0; y < maat[keus_old][1]; y++){            // loep doorloopt alle y posities
   if (vorm[keus_old*4+y][x] == 1){                   // ligt hier zichtbaar deel tegel?
    bord[x+Cx_old][y+Cy_old] = bord[x+Cx_old][y+Cy_old]-8;// ja:zet geinverteerd patroon uit
   }
  }
 }
}
// ------------------------------------------------------------------------------------------

// verander de cursortegel op het spelbord de tegel met het patroon dat bij de vorm hoort
void plaatstegel() {
 for (x = 0; x < maat[keus][0]; x++){                // loep doorloopt alle x posities
  for (y = 0; y < maat[keus][1]; y++){               // loep doorloopt alle y posities
   if (vorm[keus*4+y][x] == 1){                      // ligt hier zichtbaar deel tegel?
    bord[x+Cx][y+Cy] =  maat[keus][2];               // ja: plaats tegel patroon
   }
  }
 }
}
// ------------------------------------------------------------------------------------------

// met de 4 pijltjes kan je de cursortegel verplaatsen over de bestaande tegels heen
void  bedien_cursortegel() {
 if (toets1 != 0) {
  for (x = 0; x < maat[keus_old][0]; x++){            // loep doorloopt alle x posities
   for (y = 0; y < maat[keus_old][1]; y++){           // loep doorloopt alle y posities
    if (vorm[keus_old*4+y][x] == 1){                  // ligt hier zichtbaar deel tegel?
     bord[x+Cx_old][y+Cy_old] = bord[x+Cx_old][y+Cy_old]-8;// ja:zet geinverteerd patroon uit
    }
   }
  }
 if (toets1 == 1) {                                   // is knop links ingedrukt?
  Cx--;                                               // pas cursor positie x as aan
  if (Cx == 255) {                                    // x positie < 0?
    Cx = 0;}}                                         // zo ja: x=0
 if (toets1 == 2) {                                   // is knop rechts ingedrukt?
  Cx++;                                               // pas cursor positie x as aan
  if (Cx > 8-maat[keus][0]) {                         // x positie groter dan bord?
    Cx = 8-maat[keus][0];}}                           // zo ja: x krijgt max. waarde
 if (toets1 == 3) {                                   // is knop omhoog ingedrukt?
  Cy--;                                               // pas cursor positie y as aan
  if (Cy == 255) {                                    // y positie < 0?
    Cy = 0;}}                                         // zo ja: y=0
 if (toets1 == 4) {                                   // is knop omlaag ingedrukt?
  Cy++;                                               // pas cursor positie y as aan
  if (Cy > 8-maat[keus][1]) {                         // y positie groter dan bord?
    Cy = 8-maat[keus][1];}}                           // zo ja: y krijgt max. waarde
  for (x = 0; x < maat[keus][0]; x++){                // loep doorloopt alle x posities
   for (y = 0; y < maat[keus][1]; y++){               // loep doorloopt alle y posities
    if (vorm[keus*4+y][x] == 1){                      // ligt hier zichtbaar deel tegel?
     bord[x+Cx][y+Cy] =  bord[x+Cx][y+Cy] + 8;        // ja: inverteer het patroon op bord
    }
   }
  }
 }
 Cx_old = Cx; Cy_old = Cy; keus_old = keus;           // backup x,y positie en tegelnummer
}
// ------------------------------------------------------------------------------------------

// plaats de geselecteerde cursortegel linksboven (0,0) op het scherm
void plaats_cursortegel() {
 for (x = 0; x < maat[keus][0]; x++){                 // loep doorloopt alle x posities
  for (y = 0; y < maat[keus][1]; y++){                // loep doorloopt alle y posities
   if (vorm[keus*4+y][x] == 1){                       // ligt hier zichtbaar deel tegel?
    bord[x][y] = bord[x][y] + 8;                      // ja: inverteer het patroon op bord
   }
  }
 }
 keus_old = keus;                                     // sla geselecteerde nummer tegel op
 Cx_old = 0; Cy_old = 0; Cx = 0; Cy = 0;              // zet cursor positie linksboven
}
// ------------------------------------------------------------------------------------------

// wis de geselecteerde tegelknop
void wis_tegelknop() {
 keus = knop[select];                                 // sla nr tegel geselecteerde knop op
 knop[select] = 19;                                   // wis de knop
}
// ------------------------------------------------------------------------------------------

// Met de horizontale pijltjes kan je door de 3 tegelknoppen lopen (laten knipperen)
void bedien_knoppen() {
 select2 = select;                                    // bewaar vorige selectie knop
 if (toets1 == 1) {                                   // wordt er op links gedrukt?
  do {select--;                                       // kies de linker knop
   if (select == 255) {select = select2;}             // helemaal links? zet oude knop terug
  }
  while (knop[select] == 19);                         // knop niet aanwezig? tel door
 }
 if (toets1 == 2) {                                   // wordt er op rechts gedrukt?
  do {select++;                                       // kies de rechter knop
   if (select == 3) {select = select2;}               // helemaal rechts? zet oude knop terug
  }
  while (knop[select] == 19);                         // knop niet aanwezig? tel door
 }
}
// ------------------------------------------------------------------------------------------

// selecteer de meest linker tegelknop. staan er geen meer kies random 3 nieuwe knoppen
void selecteer_tegelknop() {
 if (knop[0] == 19 && knop[1] == 19 && knop[2] == 19){// zijn alle knoppen leeg:
  knop[0] = random(0,tegel_max+1);                    // kies een willekeurige knop
  do {knop[1] = random(0,tegel_max+1);}               // kies een willekeurige knop
  while (knop[1] == knop[0]);                         // knoppen mogen niet gelijk zijn
  do {knop[2] = random(0,tegel_max+1);}               // kies een willekeurige knop
  while (knop[2] == knop[0] || knop[2] == knop[1]);   // knoppen mogen niet gelijk zijn
 }
 if (knop[2] != 19) {select = 2;}                     // rechter knop niet leeg. kies die
 if (knop[1] != 19) {select = 1;}                     // middelste knop niet leeg. kies die
 if (knop[0] != 19) {select = 0;}                     // linker knop niet leeg. kies die
}
// ------------------------------------------------------------------------------------------

// Invoer knoppen (toets1: 1=links 2=rechts 3=omhoog 4=omlaag. toets2: 0=knoppen 1=spelbord)
void invoer(){
 toets1 = 0;
 if (B == 0 && arduboy.justPressed(A_BUTTON) == true && toets2 == 2) // 1e keer op A gedrukt 
  {toets2 = 4;                                                       // ga naar stap 3
   B = 1;                                                            // knop is ingedrukt
   }
 if (B == 0 && arduboy.justPressed(A_BUTTON) == true && toets2 == 5) // 2e keer op A gedrukt
  {toets2 = 0;                                                       // ga naar stap 0
  B = 1;                                                             // knop is ingedrukt
  }
  if (B == 0 && arduboy.justPressed(B_BUTTON) == true && toets2 == 2) { // B ingedrukt?
  B = 1;                                                             // knop is ingedrukt
  if (aantal_rand == 0) {toets2 = 6;}                                // bij 0: einde spel
  if (aantal_rand > 0) {                                             // kan je tegels kiezen?
    memset (knop,19,sizeof(knop));                                   // reset de 3 tegels
    aantal_rand--;                                                   // aantal keer -1
    toets2 = 1;                                                      // ga naar select knop
  }
 }
 if (arduboy.justPressed(LEFT_BUTTON) == true) {toets1 = 1;}         // links ingedrukt nr 1
 if (arduboy.justPressed(RIGHT_BUTTON) == true) {toets1 = 2;}        // rechts ingedrukt nr 2
 if (arduboy.justPressed(UP_BUTTON) == true) {toets1 = 3;}           // omhoog ingedrukt nr 3
 if (arduboy.justPressed(DOWN_BUTTON) == true) {toets1 = 4;}         // omlaag ingedrukt nr 4
 if (B == 1 && arduboy.justPressed(A_BUTTON) == false &&             // zijn knoppen weer los?
     arduboy.justPressed(B_BUTTON) == false) {B = 0;}                // geef dan invoer vrij
}
// ------------------------------------------------------------------------------------------

// zet de 3 knoppen op het scherm en laat geselecteerde knop knipperen
void display_knoppen(){
 for (x=0; x<3 ;x++) {                                // loep door alle knoppen
  if (knop[x] != 19) {                                // kijk of knop leeg is
   if (!(x == select && knipper == 0)) {              // kijk of geselecteerde knop knippert
    Arduboy2Base::drawRoundRect(x*20+68, 44, 20, 20, 5, WHITE); // teken de omtrek vd knop
    sprites.drawOverwrite(x*20+70,46, tegel, knop[x]);// teken de inhoud van de knop
   }
  }
 }
}
// ------------------------------------------------------------------------------------------

// zet speelbord op display
void display_bord() {
 byte tek;                                            // tijdelijke opslag patroon
 for (x=0; x<8; x++){                                 // loep horizontaal door het bord
  for (y=0; y<8; y++){                                // loep verticaal door het bord
   tek = bord[x][y];                                  // lees patroon nr van bord array
   if (tek > 8) {tek = 9;}                            // bij geinverteer patroon neem kruis
   sprites.drawOverwrite(x*8+1,y*8, patroon, tek);    // zet tegel uit array op display
  }
 }
}
// ------------------------------------------------------------------------------------------

// zet de tijd en de score op het display
void tijd_score() {
 unsigned int klok2;                                  // declareer variabele voor klok
 arduboy.setCursor(106,20);                           // zet cursor goed
 arduboy.print(score);                                // zet score op display
 klok2 = (millis() - klok) / 1000;                    // bereken verstreken tijd in seconden
 arduboy.setCursor(100,31);                           // zet cursor goed
 if (klok2 / 60 < 10) {arduboy.print("0");}           // indien aantal min. < 10 print een 0
 arduboy.print(klok2 / 60);                           // zet aantal min. op display
 arduboy.setCursor(111,31);                           // zet cursor goed
 arduboy.print(".");                                  // zet een punt op display
 arduboy.setCursor(117,31);                           // zet cursor goed
 if (klok2 % 60 < 10) {arduboy.print("0");}           // indien aantal sec. < 10 print een 0
 arduboy.print(klok2 % 60);                           // zet aantal sec. op display
}
// ------------------------------------------------------------------------------------------

// zet vaste objecten op display
void display_opbouw(){
 Arduboy2Base::drawFastVLine (66,0,64,WHITE);         // verticale lijn op display
 sprites.drawOverwrite(74,0, tekst, 0);               // zet naam spel op display
 sprites.drawOverwrite(90,0, tekst, 1);               // zet naam spel op display
 sprites.drawOverwrite(106,0, tekst, 2);              // zet naam spel op display
 arduboy.setCursor(70,20);                            // zet cursor goed
 arduboy.print("Score:");                             // zet tekst op display
 arduboy.setCursor(70,31);                            // zet cursor goed
 arduboy.print("Time:");                              // zet tekst op display
}
// ------------------------------------------------------------------------------------------

// zet alle variabelen goed voor een nieuw spel
void reset(){
 klok = millis();                                     // sla starttijd begin spel op
 score = 0;                                           // reset score
 memset (bord,0,sizeof(bord));                        // maak spelbord leeg
 memset (knop,19,sizeof(knop));                       // reset de 3 tegels
 select = 0;                                          // schakel linker knop in
 tegel_max = 8;                                       // kies een tegel t/m tegel nummer 8
 toets2 = 1; B = 0;                                   // schakel naar knoppen invoer
 aantal_rand = aantal_pogingen;                       // aantal keer nieuwe tegels kiezen
}
// ------------------------------------------------------------------------------------------
