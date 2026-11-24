#include <Arduboy2.h>
Arduboy2 ab;
const unsigned char player[] PROGMEM  = {
    0xff, 0xfe, 0xfc, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0x70, 0x70, 0x20, 0x20, 
};
const unsigned char enemy[] PROGMEM  = {
0x20, 0x20, 0x70, 0x70, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xfc, 0xfe, 0xff, 
};
int easy = 0;
int medium = 1;
int hard = 2;
int diff = 0;
int gamestate  = 0;
int tkillz = 0;
int locx = 0; //player location x
int locy = 0; //player location y
int locyt = 0; //
int bulx = 0; //player bullet location x
int buly = 0; // player bullet location y
int buls = 0; // bullet starting x
int hp = 1; //current planes health
int life = 3; // extra lives (planes)
int killz = 0; // player kill count
int elocx = 100; //enemy location x
int elocy = 30; // enemy location y
int elocyt = 0; //8 - black space in bullet
int enemyex = 1; //enemy exists (0 no 1 yes)
int ehp = 0; //enemy hit points 
int ebulx = 0; // enemy bullet x coord
int ebuly = 0; //enemy bulley y coord
//
int elocx2 = 100; //enemy location x
int elocy2 = 30; // enemy location y
int elocyt2 = 0; //8 - black space in bullet
int enemyex2 = 1; //enemy exists (0 no 1 yes)
int ehp2 = 0; //enemy hit points 
int ebulx2 = 0; // enemy bullet x coord
int ebuly2 = 0; //enemy bulley y coord
int high = 0;
unsigned long ebulm = 0; // when bulx changed last
unsigned long pbulm = 0; // when bulx changed last
unsigned long emove = 0; // when bulx changed last
unsigned long cMillis = 0; // currant miliseconds
const long interval = 10; // time between bullet movments

void setup() {
  // put your setup code here, to run once:
ab.begin();
  // massivley lowered framerate to test one of my theories
ab.setFrameRate(60);
ab.clear();
}

void loop() {

 if (!(ab.nextFrame()))
    return;
    ab.clear();
    locyt = (locy +7);
    elocyt = (elocy + 7);
    elocyt2 = (elocy2 +7);
    // gs 1
    // move the player back one space
    if (ab.pressed(LEFT_BUTTON) && (gamestate == 1) && (locx > 1)){
    locx -= 1; //edited this to make the player move faster
  }

  // move player forward one space
  if (ab.pressed(RIGHT_BUTTON) && (gamestate == 1) && (locx < 100)){
    locx += 1; //dito edit
  }

  // move player up one space
    if (ab.pressed(UP_BUTTON) && (gamestate == 1) && (locy > 1)){
    locy -= 1;
  }

  //move player down one space
  if (ab.pressed(DOWN_BUTTON) && (gamestate == 1) && (locy < 44)){
    locy += 1;
  }
if ((gamestate == 1) && (tkillz >= 100)) {
  tkillz -=100;
  life += 1;
}
if (gamestate == 0) {
  hp = 1;
  killz = 0;
  life = 3;
 // print  TITLE 
  ab.setCursor((13), (0)); 
  ab.print("Military Aviator");  
// print VERSION
  ab.setCursor((30), (40)); 
  ab.print("VERSION 0.1 "); 
// print "HIGH SCORE" , WICH IS REALY JUST KILL COUNT untill i figure out eeprom
  ab.setCursor(14, 50); 
  ab.print("HIGH SCORE "); 
  // print kill count
  ab.setCursor(100, 50); 
  ab.print(high); 
  }
if ((gamestate == 0) && (diff == 0)) {
  ab.setCursor((30), (25)); 
      ab.print("easy");
      enemyex2 = 0;
}
if ((gamestate == 0) && (diff == 1)) {
  ab.setCursor((30), (25)); 
      ab.print("medium");
      enemyex2 = 1;
      
}
if ((gamestate == 0) && (diff == 2)) {
  ab.setCursor((30), (25)); 
      ab.print("hard");
}
  if (diff >= 3) {
    diff = 0;
  }
  if (diff < 0) {
    diff = 3;
  }
  // if on title page and a button is pressed, play
  if (ab.pressed(RIGHT_BUTTON) && (gamestate == 0) && ab.everyXFrames(30)){
   diff += 1;
  }
    // if on title page and a button is pressed, play
  if (ab.pressed(LEFT_BUTTON) && (gamestate == 0) && ab.everyXFrames(30)){
   diff -= 1;
  }
 // if on title page and a button is pressed, play
  if (ab.pressed(DOWN_BUTTON) && (gamestate == 0)){
    gamestate += 1;
  }
// if on PLAY page and B button is pressed, PAUSE
  if (ab.pressed(B_BUTTON) && (gamestate == 1)){
    gamestate = 2;
  }
  // if on PAUSE page and B button is pressed, play
  if (ab.pressed(A_BUTTON) && (gamestate == 2)){
    gamestate = 1;
  }
   // if on PAUSE page and B button is pressed, play
  if (ab.pressed(B_BUTTON) && (gamestate == 3)){
    gamestate = 0;
  }
  
  if (gamestate == 1) {
 // print  hp 
  ab.setCursor((0), (55)); 
      ab.print("HP");
  // print the players hp
  ab.setCursor((14), (55)); 
      ab.print(hp);

      ab.setCursor((25), (55)); 
      ab.print("kills");
  // print the ehp
  ab.setCursor((60), (55)); 
      ab.print(killz);
   ab.setCursor((80), (55)); 
      ab.print("life");
  // print the ehp
  ab.setCursor((110), (55)); 
      ab.print(life);


ab.setCursor((15), (45)); 
      ab.print("y2");
ab.setCursor((35), (45)); 
      ab.print(elocy2);
   ab.setCursor((60), (45)); 
      ab.print("x2");
  // print the ehp
  ab.setCursor((100), (45)); 
      ab.print(elocx2);
      ab.drawBitmap(locx, locy, player, 16, 8, WHITE);
      ab.drawBitmap(elocx, elocy, enemy, 16, 8, WHITE);
  }
  if ((gamestate == 1) && (enemyex2 == 1)) {
    ab.drawBitmap(elocx2, elocy2, enemy, 16, 8, WHITE);
  }
if (gamestate == 2) {
 // print  pause 
      ab.setCursor((30), (25)); 
      ab.print("PAUSE");
      ab.setCursor((17), (55)); 
      ab.print("A to UNPAUSE");
}
if (killz >= high){
  high = killz;
}
if (gamestate == 3) {
  elocx = 0;
  ebuly = 0;
  ebulx = 0;
  bulx = 0;
  buly = 0;
 // print  game over 
      ab.setCursor((30), (25)); 
      ab.print("GAME OVER");

           ab.setCursor((0), (50)); 
      ab.print("KILLS");
      ab.setCursor((40), (50)); 
      ab.print(killz);
}   
  if (hp <= 0) {
    life -= 1;
    hp = 1;
  }

  if ((life < 0) && gamestate == 1) {
    gamestate = 3;
  }
//about bullet
    // bullet
       //if a bullet is not on the screen and player presses a shoot
  if (ab.pressed(A_BUTTON)&& ab.everyXFrames(5) && (gamestate == 1) && (bulx == 0)){
      bulx = (locx + 16);
      buly = (locy);
      buls = bulx;
  }
   //if bullet is within a given range of enemy enemy hp = ehp-1
  if ((bulx >=1) && (bulx >= elocx) && (buly >= (elocy))&& (buly <= elocyt) && (gamestate == 1)) {
    ehp -= 1;
    killz += 1;
    tkillz +=1;
    bulx = 0;
    buly = 0;
  }

 //if bullet is within a given range of enemy2 enemy hp = ehp2-1
  if ((bulx >=1) && (bulx >= elocx2) && (buly >= (elocy2))&& (buly <= elocyt2) && (gamestate == 1)) {
    ehp2 -= 1;
    killz += 1;
    tkillz +=1;
    bulx = 0;
    buly = 0;
  }
 // move the players bullet forward 2 px untill it falls off the screen if with a delay without a delay
    if ((gamestate == 1) && (bulx <= 128) && (bulx > 0) && ab.everyXFrames(2)){
      bulx += 2;
    
  }

 if (bulx > 0) {
    ab.setCursor((bulx), (buly)); 
    ab.print("-");
 }
 //if bullet is behind the enemy get rid of it
 if (bulx >= (elocx +17)){
  bulx = 0;
 }
 // about ebullet
    //if bullet is within a given range of enemy enemy hp = ehp-1
  if ((ebulx >=1) && (ebulx <= locx) && (ebuly >= (locy))&& (ebuly <= locyt) && (gamestate == 1)) {
    hp -= 1;
    ebulx = 0;
    ebuly = 0;
  }
 
   // move the enemy bullet forward 2 px untill it falls off the screen if with a delay without a delay
    if ((gamestate == 1) && (ebulx > 0) && ab.everyXFrames(2)){
      ebulx -= 2;
    
  }

 if (ebulx > 0) {
    ab.setCursor((ebulx), (ebuly)); 
    ab.print("-");
 }
 //if ebullet is behind the enemy get rid of it
 if (ebulx <= (locx - 2)){
  ebulx = 0;
 }
 //ebullet2
 //ebullet2
 // about ebullet
    //if bullet is within a given range of enemy enemy hp = ehp-1
  if ((ebulx2 >=1) && (ebulx2 <= locx) && (ebuly2 >= (locy))&& (ebuly2 <= locyt) && (gamestate == 1)) {
    hp -= 1;
    ebulx2 = 0;
    ebuly2 = 0;
  }
 
   // move the enemy bullet forward 2 px untill it falls off the screen if with a delay without a delay
    if ((gamestate == 1) && (ebulx2 > 0) && ab.everyXFrames(2)){
      ebulx2 -= 2;
    
  }

 if (ebulx2 > 0) {
    ab.setCursor((ebulx2), (ebuly2)); 
    ab.print("-");
 }
 //if ebullet is behind the enemy get rid of it
 if (ebulx2 <= (locx - 2)){
  ebulx2 = 0;
 }
     //enemy 1
     // if enemy1 is higher than player go down
  if ((gamestate == 1) && (enemyex == 1) && (elocy < locy) && ab.everyXFrames(5)&& (elocx >= (locx + 8))) {
     elocx -= 1;
     elocy += 1;
    
  }  
       // if enemy1 is lower than player go forward and up
  if ((gamestate == 1) && (enemyex == 1) && (elocy > locy) && ab.everyXFrames(3) && (elocx >= (locx + 8))) {
     elocx -= 1;
     elocy -= 1;
    
  } 
    // if enemy1 is behind player walk him off screen 
   if ((gamestate == 1) && (enemyex == 1) && ab.everyXFrames(3) && (elocx <= (locx))) {
      elocx -= 1;
      
   }
  // if enemy1 is at same height as player go forward shoot
   if ((gamestate == 1) && (enemyex == 1) && (elocy == locy) && (elocx >= locx + 10) && (ebulx == 0) && ab.everyXFrames(3)) {
      //go forward
      elocx -= 1;
      ebulx = (elocx - 10);
      ebuly = elocy;
    
      //shoot
      
      
      
  }
  // if enemy 1is at same height as player and more than 8 px from him
   if ((gamestate == 1) && (enemyex == 1) && (elocy == locy) && (elocx >= locx + 10 ) && ab.everyXFrames(3)) {
      //go forward
      elocx -= 1;

  }
    
   // if enemy is  within 18 px  x of player go up & locy = elocy
   if ((gamestate == 1) && (enemyex == 1) && (elocx <= (locx + 8)) && (elocy +20 <= locy) && (elocy - 20 >= locy) && ab.everyXFrames(3)) {
     
     elocy -= 10;
     elocx +=10;
  }
    
    if (ehp <= 0) {
    
    ebulx = 0;
    ebulx = 0;
    elocx = (random(128, 138));
    elocy = (random(0, 30));
    ehp = 1;
  }

  

    // if enemy1 falls off the screen set it back  
  if (elocx < 0) {
    
    ebulx = 0;
    ebulx = 0;
    elocx = (random(128, 138));
    elocy = (random(0, 30));
    
  }

    if (elocy < 0) {
    elocx = (random(128, 138));
    ebulx = 0;
    ebulx = 0;
    elocy = (random(0, 30));
    
  }
    if (elocy > 55) {
    elocx = (random(128, 138));
    ebulx = 0;
    ebulx = 0;
    elocy = (random(0, 30));
    
  }
      
   // enemy 2
   // enemy 2
        // if enemy is higher than player go down
  if ((gamestate == 1) && (enemyex2 == 1) && (elocy2 < locy) && ab.everyXFrames(5)&& (elocx2 >= (locx + 8))) {
     elocx2 -= 1;
     elocy2 += 1;
    
  }  
       // if enemy is lower than player go forward and up
  if ((gamestate == 1) && (enemyex2 == 1) && (elocy2 > locy) && ab.everyXFrames(3) && (elocx2 >= (locx + 8))) {
     elocx2 -= 1;
     elocy2 -= 1;
    
  } 
    // if enemy is behind player walk him off screen 
   if ((gamestate == 1) && (enemyex2 == 1) && ab.everyXFrames(3) && (elocx2 <= (locx))) {
      elocx2 -= 1;
      
   }
  // if enemy is at same height as player go forward shoot
   if ((gamestate == 1) && (enemyex2 == 1) && (elocy2 == locy) && (elocx2 >= locx + 10) && (ebulx2 == 0) && ab.everyXFrames(3)) {
      //go forward
      elocx2 -= 1;
      ebulx2 = (elocx2 - 10);
      ebuly2 = elocy2;
    
      //shoot
      
      
      
  }
  // if enemy is at same height as player and more than 8 px from him
   if ((gamestate == 1) && (enemyex2 == 1) && (elocy2 == locy) && (elocx2 >= locx + 10 ) && ab.everyXFrames(3)) {
      //go forward
      elocx2 -= 1;

  }
    
   // if enemy is  within 18 px  x of player go up & locy = elocy
   if ((gamestate == 1) && (enemyex2 == 1) && (elocx2 <= (locx + 8)) && (elocy2 +20 <= locy) && (elocy2 - 20 >= locy) && ab.everyXFrames(3)) {
     
     elocy2 -= 10;
     elocx2 +=10;
  }
    
    if ((ehp2 <= 0) && (enemyex2 == 1)) {
    
    ebulx2 = 0;
    ebulx2 = 0;
    elocx2 = (random(128, 138));
    elocy2 = (random(0, 30));
    ehp2 = 1;
  }

  

    // if enemy falls off the screen set it back  
  if ((elocx2 < 0) && (enemyex2 == 1)) {
    
    ebulx2 = 0;
    ebulx2 = 0;
    elocx2 = (random(128, 138));
    elocy2 = (random(0, 30));
    
  }

    if ((elocy2 < 0) && (enemyex2 == 1)) {
    elocx2 = (random(128, 138));
    ebulx2 = 0;
    ebulx2 = 0;
    elocy2 = (random(0, 30));
    
  }
    if ((elocy2 > 55) && (enemyex2 == 1)) {
    elocx2 = (random(128, 138));
    ebulx2 = 0;
    ebulx2 = 0;
    elocy2 = (random(0, 30));
    
  }
 
ab.display();
}
