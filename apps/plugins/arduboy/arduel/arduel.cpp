// Curly
// work started 15th April 2017
// version 1 done ______ ? <--- add date there :) woooo 
// Arduel
// yes just for the prize :D
// my arduboy has been acting funny latley and i desperatly need a new one
//128 64
#include "Arduboy2.h" 
Arduboy2 ab; 
int gamestate = 1;
//platforms
int p1x = 0; //platform 1 x loc
int p1y = 0; //platform 1 y loc
int p1w = 0; //width of platform 1
int p2x = 0; //platform 2 x loc
int p2y = 0; //platform 2 y lox
int p2w = 0; //platform 2 width
int p3x = 0; //platform 3 x loc
int p3y = 0; //platform 3 y loc
int p3w = 0; //platform 3 width
int p4x = 0; //platform 4 x loc
int p4y = 0; //platform 4 y loc
int p4w = 0; //platform 4 width

//enemies
int ex1 = 0;
int ey1 = 0;
int eh1 = 0;
int ex2 = 0;
int ey2 = 0;
int eh2 = 0;
int ex3 = 0;
int ey3 = 0;
int eh3 = 0;

//player
int hp = 1; //player hp
int locx = 40;
int locy = 0;

// can go (udlr)
int cgu = 0;
int cgd = 0;
int cgl = 0;
int cgr = 0;

int voodoo = 0;
int VooDoo = 0;
int WhooKNoo = 0;

//random
int flamez = 0;
char flames = 0;
//sprites
const unsigned char flame1[] PROGMEM  = {
  0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xf8, 0xff, 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xff, 0xff, 0xff, 0xfc, 0xfc, 0xf8, 0xf9, 0xff, 
};
const unsigned char flame2[] PROGMEM  = {
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xf9, 0xf8, 0xfc, 0xfc, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xf8, 0xfe, 
};
const unsigned char rider[] PROGMEM  = {
  0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0xee, 0xfe, 0xee, 0x00, 0x00, 0x00, 0x00, 0x4, 0xc, 0xc, 0xe, 0xf, 0x4, 0x4, 0xe, 0x1e, 0x3e, 0x3e, 0x36, 0x76, 0x76, 0x76, 0x76, 
};
void drawplats() {
  //initialize
  
  if (VooDoo == 0) { VooDoo = random(1,65); }       // x
  if (WhooKNoo == 0) { WhooKNoo = random(20, 50); } //width

// set platform 1, initialize rands for platform 2
if (p1y == 0) {
  p1y = 16;
  //voodoo = random(18,36);
}
if (p1x == 0) {
  p1x = VooDoo;
  //VooDoo = random(0,20);
}
if (p1w == 0) {
p1w = WhooKNoo;
//WhooKNoo = random(20, 40);
}

// set platform 2, initialize rands for platform 3
if (p2y == 0) {
  p2y = random(33,37);
 // voodoo = random(18,36);
}
if (p2x == 0) {
  p2x = random(1,20);
  //VooDoo = random(70,100);
}
if (p2w == 0) {
p2w = random(20, 40);
//WhooKNoo = random(20, 40);
}
// set platform 3, initialize rands for platform 4
if (p3y == 0) {
  p3y = p2y;
 // voodoo = 40;
}
if (p3x == 0) {
  p3x = random(70,100);
 // VooDoo = random(1, 65);
}
if (p3w == 0) {
p3w = random(20, 40);
//WhooKNoo = random(20, 50);
}
// set platform 4, 
if (p4y == 0) {
  p4y = 54;
}
if (p4x == 0) {
  p4x = random(1, 65);
}
if (p4w == 0) {
p4w = random(20, 50);
}
//draw platform 1
        ab.drawFastHLine(p1x,p1y,p1w,WHITE);
//draw platform 2
        ab.drawFastHLine(p2x,p2y,p2w,WHITE);
//draw platform 3
        ab.drawFastHLine(p3x,p3y,p3w,WHITE);
//draw platform 4
        ab.drawFastHLine(p4x,p4y,p4w,WHITE);
};

void enemies() {
  if ((p1x > 0) && (p1y > 0) && (p1y > 0) && (eh1 == 0)) { 
   ex1 = random((p1x), (p1x + p1w));
   ey1 = (p1y - 1);
   eh1 = random(1, 5);

      if (eh1 > 0) {
              ab.setCursor (ex1, ey1); 
              ab.print("!");
      }
  }
};

void setup() { //initial set up of game
  ab.begin(); //starts up the arduboy 
  ab.setFrameRate(30); //speed controll
  ab.clear(); //clears the screen
  ab.initRandomSeed();
  Serial.begin(9600); //opens a conection through the microusb port
}

void loop() { //everything in here is your game
  if (!(ab.nextFrame())) //checks to see if it is time for the next frame
    return; //if its not check again
    ab.clear(); //clear screen
if (gamestate == 0) { //title
  //yep
}
if (gamestate == 1) { //play
  //ab.clear();
  drawplats();
  enemies();
  //get movment restrictions
//row 1 at or above platform 1 
    //down
   if (locy + 15 == p1y) { //if 1px above plat 1
    if (locx + 16 < p1x) { cgd = 0;  } //changed back to 16
    if (locx > (p1x + p1w)) { cgd = 0; } //reset cgd if right of plat 1
    if (locx <= (p1x + p1w) && (locx + 16 >= p1x)) { cgd = 1; } //stop from going down
  }
   //if ((locy + 15) < (p1y) or locy > p1y && locy <= p1y +16 ) {
   if ((locy == p1y + 2) or  locy + 16 == p1y) { //reset cgd if below pl1 or above pl1
    cgd = 0;
   }
  //cgu
  if (locy >= p1y && locy < (p1y +1)) {
    if (locx + 16 < p1x) { cgu = 0;  } //changed back to 16
    if (locx > (p1x + p1w)) { cgu = 0; } //reset cgu if right of plat 1
    if (locx <= (p1x + p1w) && (locx + 16 >= p1x)) { cgu = 1; } //stop from going down
  }
   //if ((locy + 15) < (p1y) or locy > p1y && locy <= p1y +16 ) {
   if ((locy == p1y + 2) or  locy + 16 == p1y) { //reset cgu if more than 1 below pl1 and above pl2
    cgu = 0;
   }
  
 //cgl or cgr
 
 if (locy >= (p1y - 15) && locy < (p1y)) { // removed +1 from the latter
    //cgl
    if (locx > (p1x + p1w)) { cgl = 0;  } //reset cgl if to right of plat 1
    if ((locx) == (p1x + p1w)) { cgl = 1; } //stop from going left if at plat 1
    //cgr
    if ((locx + 16) < p1x) { cgr = 0;  } //reset cgr if to left of plat 1
    if ((locx + 16) == (p1x)) { cgr = 1; } //stop from going left if at plat 1
 }

 if (((locy + 16) == (p1y - 1)) or locy == (p1y + 1)) { 
 cgl = 0;
 cgr = 0;
}

  //  }
//row 2 at or above platforms 2 and 3
 //NOTE this one will have to account for plats 2 and 3 for l r cg
 //down
   if (locy + 15 == p2y) { //if 1px above plat 1
    if (locx + 16 < p2x && locx < p3x) { cgd = 0;  } //changed back to 16
    if (locx > (p2x + p2w)&& locx < p3x) { cgd = 0; } //reset cgd if right of plat 1
    if (locx <= (p2x + p2w) && (locx + 16 >= p2x)) { cgd = 1; } //stop from going down
  }
   //if ((locy + 15) < (p1y) or locy > p1y && locy <= p1y +16 ) {
   if ((locy == p2y + 2) or  locy + 16 == p2y) { //reset cgd if below pl1 or above pl1
    if (locx < p3x && locx > (p2x + p2w)) {
      cgd = 0;
    }
   }
  //cgu
  if (locy >= p2y && locy < (p2y +1)) {
    if (locx + 16 < p2x && locx < p3x) { cgu = 0;  } //changed back to 16
    if (locx > (p2x + p2w)&& locx < p3x) { cgu = 0; } //reset cgu if right of plat 1
    if (locx <= (p2x + p2w) && (locx + 16 >= p2x)) { cgu = 1; } //stop from going down
  }
   //if ((locy + 15) < (p1y) or locy > p1y && locy <= p1y +16 ) {
   if ((locy == p2y + 2) or  locy + 16 == p2y) { //reset cgu if more than 1 below pl1 and above pl2
    cgu = 0;
   }
  
 //cgl or cgr
 
 if (locy >= (p2y - 15) && locy < (p2y) && locx < p3x) { // removed +1 from the latter
    //cgl
    //if (locx > (p2x + p2w)) { cgl = 0;  } //reset cgl if to right of plat 1
    if ((locx) == (p2x + p2w)) { cgl = 1; } //stop from going left if at plat 1
    //cgr
    //if ((locx + 16) < p2x) { cgr = 0;  } //reset cgr if to left of plat 1
    if ((locx + 16) == (p2x)) { cgr = 1; } //stop from going left if at plat 1
 }

 if (((locy + 16) == (p2y - 1)) or locy == (p2y + 1)) { 
 cgl = 0;
 cgr = 0;
 }
 //plat 3
 //down
   if (locy + 15 == p3y) { //if 1px above plat 1
    if (locx + 16 < p3x && locx > (p2x + p2w)) { cgd = 0;  } //changed back to 16
    if (locx > (p3x + p3w) && locx > (p2x + p2w)) { cgd = 0; } //reset cgd if right of plat 1
    if (locx <= (p3x + p3w) && (locx + 16 >= p3x)) { cgd = 1; } //stop from going down
  }
   //if ((locy + 15) < (p1y) or locy > p1y && locy <= p1y +16 ) {
   if ((locy == p3y + 2) or  locy + 16 == p3y) { //reset cgd if below pl1 or above pl1
    if (locx > (p2x + p2w)) {
      cgd = 0;
    }
   }

   
  //cgu
  if (locy >= p3y && locy < (p3y +1)) {
    if (locx + 16 < p2x && locx > p2x) { cgu = 0;  } //changed back to 16
    if (locx > (p3x + p3w) && locx > p2x) { cgu = 0; } //reset cgu if right of plat 1
    if (locx <= (p3x + p3w) && (locx + 16 >= p3x)) { cgu = 1; } //stop from going down
  }
   //if ((locy + 15) < (p1y) or locy > p1y && locy <= p1y +16 ) {
   if ((locy == p3y + 2) or  locy + 16 == p3y) { //reset cgu if more than 1 below pl1 and above pl2
    cgu = 0;
   }
    if (locy >= (p3y - 15) && locy < (p2y) && locx > p2x) { // removed +1 from the latter
    //cgl
    //if (locx > (p3x + p3w)) { cgl = 0;  } //reset cgl if to right of plat 3
    if ((locx) == (p3x + p3w)) { cgl = 1; } //stop from going left if at plat 1
    //cgr
    //if ((locx + 16) < p3x) { cgr = 0;  } //reset cgr if to left of plat 1
    if ((locx + 16) == (p3x)) { cgr = 1; } //stop from going left if at plat 1
 }

 if (locy >= (p3y - 15) && locy < (p2y)) {
     if ((locx + 16) < p2x) { cgr = 0;  } //reset cgr if to left of plat 2
     if (locx > (p3x + p3w)) { cgl = 0;  } //reset cgl if to right of plat 3
//critical
     if (locx > (p2x + p2w) && locx < p3x) { cgl = 0;  } //reset cgl if to right of plat 1
     if ((locx + 16) < p3x && locx > (p2x + p2w)) { cgr = 0;  } //reset cgr if to left of plat 1
 }

 
//row 3 at or above platform 4
   //down
   if (locy + 15 == p4y) { //if 1px above plat 4
    if (locx + 16 < p1x) { cgd = 0;  } 
    if (locx > (p4x + p4w)) { cgd = 0; } //reset cgd if right of plat 4
    if (locx <= (p4x + p4w) && (locx + 16 >= p4x)) { cgd = 1; } //stop from going down
  }
   //if ((locy + 15) < (p1y) or locy > p1y && locy <= p1y +16 ) {
   if ((locy == p4y + 2) or  locy + 16 == p4y) { //reset cgd if below pl4 or above pl4
    cgd = 0;
   }
  //cgu
  if (locy >= p4y && locy < (p4y +1)) {
    if (locx + 16 < p4x) { cgu = 0;  } //changed back to 16
    if (locx > (p4x + p4w)) { cgu = 0; } //reset cgu if right of plat 1
    if (locx <= (p4x + p4w) && (locx + 16 >= p4x)) { cgu = 1; } //stop from going down
  }
   //if ((locy + 15) < (p1y) or locy > p1y && locy <= p1y +16 ) {
   if ((locy == p4y + 2) or  locy + 16 == p4y) { //reset cgu if more than 1 below pl1 and above pl2
    cgu = 0;
   }
  
 //cgl or cgr
 
 if (locy >= (p4y - 15) && locy < (p4y)) { // removed +1 from the latter
    //cgl
    if (locx > (p4x + p4w)) { cgl = 0;  } //reset cgl if to right of plat 1
    if ((locx) == (p1x + p1w)) { cgl = 1; } //stop from going left if at plat 1
    //cgr
    if ((locx + 16) < p4x) { cgr = 0;  } //reset cgr if to left of plat 1
    if ((locx + 16) == (p4x)) { cgr = 1; } //stop from going left if at plat 1
 }

 if (((locy + 16) == (p4y - 1)) or locy == (p4y + 1)) { 
 cgl = 0;
 cgr = 0;
}

    


 
 // end movment restrictions
 //flame select

 // ab.drawBitmap(120, 56, flame2, 8, 8, WHITE);
 //}
 
 
 
 //ab.drawBitmap(0, 56, flame1, 8, 8, WHITE);
  
  ab.drawBitmap(locx, locy, rider, 16, 16, WHITE);
  if (ab.pressed(LEFT_BUTTON) && (cgl == 0)) {
          
          locx -=1;
        }
  if (ab.pressed(RIGHT_BUTTON)&& (cgr == 0)) {
          locx +=1;
        }
  if (ab.pressed(UP_BUTTON)&& (cgu == 0)) {
          locy -=1;
        }
 if (ab.pressed(DOWN_BUTTON)&& (cgd == 0)) {
          locy +=1;
        }
 if (locx < 0) { //if player goes off screen to the left, bring him in to the right
  locx = 120;
 }
 if (locx > 120) { //if player goes off screen to the right bring him in to the left
  locx = 0;
 }
}
if (gamestate == 2) { //lose
  //lose screen
}

    
Serial.write(ab.getBuffer(), 128 * 64 / 8); //ssend the screen over the microusb (i use it for craits screen mirror)
ab.display();
}
