/* 
 *  Arduboy - Magic 8 Ball
 *  
 */


/* Sentences:
 01: As I see it, yes.
 02: Ask again later.
 03: Better not tell you now.
 04: Cannot predict now.
 05: Concentrate and ask again.
 06: Don’t count on it.
 07: It is certain.
 08: It is decidedly so.
 09: Most likely.
 10: My reply is no.
 11: My sources say no.
 12: Outlook not so good.
 13: Outlook good.
 14: Reply hazy, try again.
 15: Signs point to yes.
 16: Very doubtful.
 17: Without a doubt.
 18: Yes.
 19: Yes – definitely.
 20: You may rely on it.
 */

#include <Arduboy2.h>
#include "bitmaps.h" //load bitmaps
 
  int logo = 0;
  int debug = 0;

Arduboy2 arduboy;
void mainScreen();
void askMeSomething();
void thinking();
void prediction ();

void setup() {

  arduboy.begin();
  arduboy.setFrameRate(15);
  arduboy.initRandomSeed();
 
  arduboy.display();
 }

 void loop() {
  if (!(arduboy.nextFrame()))
  return;
    
arduboy.pollButtons();

  if (logo == 0)
    {
      arduboy.clear();
      arduboy.drawBitmap(0, 8, mainLogo, 128, 64, WHITE);
      arduboy.display();
      delay(3000);
      logo = 1;
  }
  mainScreen();
        if (arduboy.pressed(A_BUTTON)) {
             askMeSomething();
        } 
}

void mainScreen() {
      arduboy.clear();
      arduboy.drawBitmap(0, 0, ball, 128, 64, 1);
      arduboy.drawFastHLine(0, 55, 128, WHITE);
      arduboy.setCursor(0, 56);
      arduboy.print(F("Press A and ask me..."));
      arduboy.display();
}

void askMeSomething(){
      arduboy.clear();
      arduboy.drawBitmap(45, 5, five, 43, 43, 1);
      arduboy.drawFastHLine(0, 50, 128, WHITE);
      arduboy.setCursor(0, 54);
      arduboy.print(F("Listening..."));
      arduboy.display();
      delay(1000);
      arduboy.clear();
      arduboy.drawBitmap(45, 5, four, 43, 43, 1);
      arduboy.drawFastHLine(0, 50, 128, WHITE);
      arduboy.setCursor(0, 54);
      arduboy.print(F("Listening..."));
      arduboy.display();
      delay(1000);
      arduboy.clear();
      arduboy.drawBitmap(45, 5, three, 43, 43, 1);
      arduboy.drawFastHLine(0, 50, 128, WHITE);
      arduboy.setCursor(0, 54);
      arduboy.print(F("Listening..."));
      arduboy.display();
      delay(1000);
      arduboy.clear();
      arduboy.drawBitmap(45, 5, two, 43, 43, 1);
      arduboy.drawFastHLine(0, 50, 128, WHITE);
      arduboy.setCursor(0, 54);
      arduboy.print(F("Listening..."));
      arduboy.display();
      delay(1000);
      arduboy.clear();
      arduboy.drawBitmap(45, 5, one, 28, 43, 1);
      arduboy.drawFastHLine(0, 50, 128, WHITE);
      arduboy.setCursor(0, 54);
      arduboy.print(F("Listening..."));
      arduboy.display();
      delay(1000);
      arduboy.clear();
      arduboy.drawBitmap(45, 5, zero, 43, 43, 1);
      arduboy.drawFastHLine(0, 50, 128, WHITE);
      arduboy.setCursor(0, 54);
      arduboy.print(F("Listening..."));
      arduboy.display();
      delay(1000);

      
      thinking();
}

void thinking(){
      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe0, 128, 64, 1);
      arduboy.display();
      delay(300);
      
      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe1, 128, 64, 1);
      arduboy.display();
      delay(300);
      
      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe2, 128, 64, 1);
      arduboy.display();
      delay(300);
      
      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe3, 128, 64, 1);
      arduboy.display();
      delay(300);
      
      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe4, 128, 64, 1);
      arduboy.display();
      delay(300);
      
      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe5, 128, 64, 1);
      arduboy.display();
      delay(300);

      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe6, 128, 64, 1);
      arduboy.display();
      delay(300);

      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe7, 128, 64, 1);
      arduboy.display();
      delay(300);
      
      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe8, 128, 64, 1);
      arduboy.display();
      delay(300);
      
      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe9, 128, 64, 1);
      arduboy.display();
      delay(300);

      arduboy.clear();
      arduboy.drawBitmap(0, 0, pixilframe10, 128, 64, 1);
      arduboy.display();
      delay(300);
            
      //Show rotate ball
      prediction();
}

void prediction (){
    //debug = debug + 1;
    arduboy.clear();
    byte random8BallPrediction = random(1, 21);

switch (random8BallPrediction) {
    case 1:
      arduboy.clear();
      arduboy.setCursor(10, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("As I see it... yes."));
      arduboy.display();
      delay(3000);
      break;
    case 2:
      arduboy.clear();
      arduboy.setCursor(15, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Ask again later."));
      arduboy.display();
      delay(3000);
      break;
    case 3:
      arduboy.clear();
      arduboy.setCursor(15, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Better not tell\n      you now."));
      arduboy.display();
      delay(3000);
      break;
    case 4:
      arduboy.clear();
      arduboy.setCursor(15, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Cannot predict \n             now."));
      arduboy.display();
      delay(3000);
      break;
    case 5:
      arduboy.clear();
      arduboy.setCursor(25, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Concentrate \n       and ask again."));
      arduboy.display();
      delay(3000);
      break;
    case 6:
      arduboy.clear();
      arduboy.setCursor(15, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Dont count on it."));
      arduboy.display();
      delay(3000);
      break;
    case 7:
      arduboy.clear();
      arduboy.setCursor(25, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("It is certain."));
      arduboy.display();
      delay(3000);
      break;
    case 8:
      arduboy.setCursor(10, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("It is decidedly so."));
      arduboy.display();
      delay(3000);
      break;
    case 9:
      arduboy.setCursor(30, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Most likely."));
      arduboy.display();
      delay(3000);
      break;
    case 10:
      arduboy.setCursor(20, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("My reply is no."));
      arduboy.display();
      delay(3000);
      break;
    case 11:
      arduboy.setCursor(10, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("My sources say no."));
      arduboy.display();
      delay(3000);
      break;
    case 12:
      arduboy.setCursor(5, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Outlook not so good."));
      arduboy.display();
      delay(3000);
      break;
    case 13:
      arduboy.setCursor(25, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Outlook good."));
      arduboy.display();
      delay(3000);
      break;
    case 14:
      arduboy.setCursor(1, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Reply hazy, try again."));
      arduboy.display();
      delay(3000);
      break;
    case 15:
      arduboy.setCursor(10, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Signs point to yes."));
      arduboy.display();
      delay(3000);
      break;
    case 16:
      arduboy.setCursor(20, 25);
      arduboy.print(F("Very doubtful."));
      arduboy.display();
      delay(3000);
      break;
    case 17:
      arduboy.setCursor(20, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Without a doubt."));
      arduboy.display();
      delay(3000);
      break;
    case 18:
      arduboy.setCursor(60, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Yes."));
      arduboy.display();
      delay(3000);
      break;
    case 19:
      arduboy.setCursor(15, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("Yes, definitely."));
      arduboy.display();
      delay(3000);
      break;
    case 20:
      arduboy.setCursor(10, 25);
      arduboy.setTextSize(1);
      arduboy.print(F("You may rely on it."));
      arduboy.display();
      delay(3000);
      break;
  }
  }
