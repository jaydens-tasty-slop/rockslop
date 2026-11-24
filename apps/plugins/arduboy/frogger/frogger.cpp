#include <Arduboy2.h>
#include "Images.h"
#include "Font3x5.h"

Arduboy2 arduboy;
Font3x5 font3x5 = Font3x5();

struct Object {

    int x;
    int y;
    int type;
    int delay;
    int turtleimage; // 1 full .. 4 underwater

    void decreaseDelay() {

        if (delay > 0) {
            delay = delay - 1;
        }

    }

};

int frameCount = 0;

int frogX = 62;
int frogY = 58;

Object car1;
Object car2;
Object car3;
Object car4;
Object car5;
Object car6;
Object log1;
Object log2;
Object log5;
Object log6;
Object log4;
Object log3; 

void launchCar(Object &car, Object otherCar); void launchLog(Object &log, Object otherLog); void launchLogRev(Object &log, Object otherLog); void drawLog(Object fred); void drawCar(Object car); bool carCrash(Object car);
void setup() {
  
    arduboy.begin();
    arduboy.setFrameRate(60);
    arduboy.initRandomSeed();
    
    car1.x = 130;
    car1.y = 32;
    car1.type =1;
    
    car2.x = 180;
    car2.y = 32;
    car2.type =2;

    car3.x = 64;
    car3.y = 41;
    car3.type =2;
    
    car4.x = 110;
    car4.y = 41;
    car4.type =3;

    car5.x = 45;
    car5.y = 50;
    car5.type = 1;

    car6.x = 70;
    car6.y = 50;
    car6.type = 3;

    log1.x = 75;
    log1.y = 5; 
    log1.type =0;

    log2.x = 25;
    log2.y = 5; 
    log2.type =0;

    log3.x = 31;
    log3.y = 12;
    log3.type =0;   

    log4.x = 5;
    log4.y = 12;
    log4.type =0;

    log5.x = 42;
    log5.y = 19;
    log5.type =0;

    log6.x = 70;
    log6.y = 19;
    log6.type =0;
}   


void loop() {

    if ( !arduboy.nextFrame() ) return;
    arduboy.clear();
    frameCount = frameCount + 1;
    

    // Update delays ..

    log1.decreaseDelay();
    log2.decreaseDelay();
    log3.decreaseDelay();
    log4.decreaseDelay();
    log5.decreaseDelay();
    log6.decreaseDelay();


    // Controlling ..

    if (arduboy.pressed(UP_BUTTON)) {
        frogY = frogY - 1;
    }
    if (arduboy.pressed(DOWN_BUTTON) && frogY<58) {
        frogY = frogY + 1;
    }
    if (arduboy.pressed(RIGHT_BUTTON) && frogX<123) {
        frogX = frogX + 1;
    }
    if (arduboy.pressed(LEFT_BUTTON)&& frogX>0) {
        frogX = frogX - 1;
    }
    

    // -------------------------------------------------
    //  Update the cars ..

    if (frameCount % 2 == 0) {

        car1.x = car1.x - 1;
        car2.x = car2.x - 1;

    }
    else {

        car1.x = car1.x - 2;
        car2.x = car2.x - 2;

    }

    if (car1.x< - 20) { launchCar(car1, car2); }
    if (car2.x< - 20) { launchCar(car2, car1); }

    car3.x = car3.x - 1;
    if (car3.x< - 20) { launchCar(car3, car4);;}

    car4.x = car4.x - 1;
    if (car4.x< - 20) { launchCar(car4, car3);;}

    if (frameCount % 2 == 0) {
        
        car5.x = car5.x - 1;
        if (car5.x< - 20) { launchCar(car5, car6); }

        car6.x = car6.x - 1;
        if (car6.x< - 20) { launchCar(car6, car5); }
    }


    log1.x=log1.x-1;
    if (log1.x<-25) { launchLog(log1,log2); }
    log2.x=log2.x-1;
    if (log2.x<-25) { launchLog(log2,log1);}

    log3.x=log3.x+1;
    if (log3.x >128) { launchLogRev(log3,log4); }
    log4.x=log4.x+1;
    if (log4.x >128) { launchLogRev(log4,log3);}

    if (frameCount % 2 == 0) {

        log5.x=log5.x-1;
        if (log5.x<-25) { launchLog(log5,log6); }
        log6.x=log6.x-1;
        if (log6.x<-25) { launchLog(log6,log5);}
    }
    

    // Look for collision ..

    if (carCrash(car1)) {

        arduboy.print("DEAD ! 1");
    }

    if (carCrash(car2)) {

        arduboy.print("DEAD ! 2");
    }
    if (carCrash(car3)) {

        arduboy.print("DEAD ! 3");
    }

    if (carCrash(car4)) {

        arduboy.print("DEAD ! 4");
    }
     if (carCrash(car5)) {

        arduboy.print("DEAD ! 5");
    }

    if (carCrash(car6)) {

        arduboy.print("DEAD ! 6");
    }


    // Drawing ..

    arduboy.drawLine(0,26,127,26);
    arduboy.drawLine(0,30,127,30);
    arduboy.drawLine(0,57,127,57);

    for (int x = 8; x < 128; x = x + 12) {

        arduboy.drawLine(x, 39, x + 4, 39);
        arduboy.drawLine(x, 48, x + 4, 48);

    }


    Sprites::drawErase(frogX - 1, frogY - 1, FrogMask, 0);
    Sprites::drawSelfMasked(frogX, frogY, Frogimage, 0);
    
    drawCar(car1);
    drawCar(car2);
    drawCar(car3);
    drawCar(car4);
    drawCar(car5);
    drawCar(car6);
    drawLog(log1);
    drawLog(log2);
    drawLog(log5);
    drawLog(log6);
    drawLog(log3);
    drawLog(log4);

    font3x5.setCursor(0, 58);
    font3x5.print("SCORE:0000");
    font3x5.setCursor(93, 58);
    font3x5.print("HIGH:0000");

    arduboy.display();
    
}

void drawLog(Object fred) {

    if (fred.type == 0) {

        Sprites::drawSelfMasked(fred.x, fred.y, Log, 0);

    }
    else {

        switch (fred.delay) {

            case 40 ... 200:
                Sprites::drawSelfMasked(fred.x, fred.y, Turtle1, 0);
                break;
            
            case 35 ... 39:
                Sprites::drawSelfMasked(fred.x, fred.y, Turtle2, 0);
                break;
            
            case 30 ... 34:
                Sprites::drawSelfMasked(fred.x, fred.y, Turtle3, 0);
                break;
            
            case 15 ... 29:
                break;
            
            case 10 ... 14:
                Sprites::drawSelfMasked(fred.x, fred.y, Turtle3, 0);
                break;
            
            case 5 ... 9:
                Sprites::drawSelfMasked(fred.x, fred.y, Turtle2, 0);
                break;

            case 1 ... 4:
                Sprites::drawSelfMasked(fred.x, fred.y, Turtle1, 0);
                break;

            case 0:
                Sprites::drawSelfMasked(fred.x, fred.y, Turtle1, 0);
                fred.delay= random(50,100);
                break;

        }

    }

}

void drawCar(Object car) {

    if (car.type == 1) {

        Sprites::drawSelfMasked(car.x, car.y, Carimage1, 0);

    }

    if (car.type == 2) {

        Sprites::drawSelfMasked(car.x, car.y, Carimage2, 0);

    }

    if (car.type == 3) {

        Sprites::drawSelfMasked(car.x, car.y, Carimage3, 0);

    }


}

bool carCrash(Object car) {

    Rect frogRect = { frogX, frogY, 4, 5 };
    Rect carRect = { car.x, car.y, 17, 6 };

    if (car.type = 1) {
        carRect.width = 10;
    }

    if (arduboy.collide(frogRect, carRect)) {

        return true;

    }
    else {
    
        return false;
    
    }

}

void launchCar(Object &car, Object otherCar) {

    if (otherCar.x > 110) {

        car.x = random(otherCar.x + 20, otherCar.x + 70);

    }
    else {

        car.x = random(130, 180);

    }

}


void launchLog(Object &log, Object otherLog) {

    if (otherLog.x > 110) {

        log.x = random(otherLog.x + 35, otherLog.x + 70);

    }
    else {

        log.x = random(130, 180);

    }
    log.type = random(0,2);
    log.turtleimage=1;
    log.delay= random(50,100);
}
void launchLogRev(Object &log, Object otherLog) {

    if (otherLog.x <0) {

        log.x = random(otherLog.x - 70, otherLog.x -35);

    }
    else {

        log.x = random(-70, -25);

    }
    log.type = random(0,2);
    log.turtleimage=1;
    log.delay= random(50,100);

}