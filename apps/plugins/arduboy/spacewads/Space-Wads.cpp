#include <Arduboy2.h>
#include "Integers.h"
#include "Bitmaps.h"

Arduboy2 arduboy;

uint8_t blinking = 64;

void setup() {
  //Start The Arduboy
  arduboy.begin();
  //Make Random
  arduboy.initRandomSeed();
  //SetFrames
  arduboy.setFrameRate(30);
  //Fix Player Position


}

void loop() {

  //Frame Fix
  if(!arduboy.nextFrame()) return;
  arduboy.clear(); 
  arduboy.pollButtons();

  switch (gamestate) {
      
    case 0:    
    
      // Start

      player.x = 60;
      player.y = 50;
      player.lives = 3;
      player.mode = MODE_ACTIVE;

      for (uint8_t i = 0; i < NUMBER_OF_BULLETS; ++i) {
        bullet[i].y = -2;
      }

      for (uint8_t i = 0; i < NUMBER_OF_STARS; ++i) {
        star[i].active = false;
      }

      for (uint8_t i = 0; i < NUMBER_OF_ASTEROIDS; ++i) {
        asteroid[i].mode = MODE_INACTIVE;
      }

      arduboy.setTextSize(2);
      arduboy.setCursor(5,20);
      arduboy.print("Space Wads");
      arduboy.setTextSize(0);
      arduboy.setCursor(45,55);
      arduboy.print("Press A");
      if (arduboy.justPressed(A_BUTTON)) { gamestate = 1; blinking = 64;}
      break;

    case 1:  
    
      // DrawPlayer

      if (blinking > 0) blinking--;
      if (blinking == 0 || ((blinking / 8) % 2 == 0)) {

        switch (player.mode) {

          case MODE_ACTIVE:
            arduboy.drawBitmap(player.x, player.y, Player,8,7);
            break;
          
          case MODE_EXPLOSION_3:
            Sprites::drawSelfMasked(player.x - 2, player.y - 2, PlayerExplosion1, 0);
            break;
          
          case MODE_EXPLOSION_2:
            Sprites::drawSelfMasked(player.x - 2, player.y - 2, PlayerExplosion2, 0);
            break;
          
          case MODE_EXPLOSION_1:
            Sprites::drawSelfMasked(player.x - 2, player.y - 2, PlayerExplosion3, 0);
            break;

        }
 
      }

      if (player.mode < MODE_ACTIVE) {

        player.mode = player.mode - 1;
          
        if (player.mode == MODE_INACTIVE) {

          if (player.lives == 0) {
            player.lives = 3;
            gamestate = 3;
          }
          else {
            player.mode = MODE_ACTIVE;
          }

        }
        
      }

      arduboy.setCursor(0,0);
      arduboy.print(player.lives);

      
      // Movement      
      if (arduboy.pressed(RIGHT_BUTTON))     { player.x+=1; }
      if (arduboy.pressed(LEFT_BUTTON))      { player.x-=1; }
      if (arduboy.pressed(UP_BUTTON))        { player.y-=1; }
      if (arduboy.pressed(DOWN_BUTTON))      { player.y+=1; }
      if (arduboy.pressed(B_BUTTON))         { gamestate = 1; }


      //Shoot?
      if (arduboy.justPressed(A_BUTTON)) { 

        for (uint8_t i = 0; i < NUMBER_OF_BULLETS; ++i) {
        
          if (bullet[i].y <= -2) {

            bullet[i].x = player.x + 4;
            bullet[i].y = player.y;
            break;
          
          }

        }

      }
      

      // Check limits      
      if (player.x > 120)   player.x = 120;
      if (player.x < 1)     player.x = 1;
      if (player.y > 57)    player.y = 57;
      if (player.y < 1)     player.y = 1;

      
      // Draw Stars      
      for (uint8_t i = 0; i < NUMBER_OF_STARS; ++i) {
        
        if (!star[i].active) {
          
          star[i].active = true;
          star[i].x = random(0,128);
          star[i].y = 0;
          star[i].inc = random(1, 4);
          
        }
        
        if (star[i].randomize == true) {
          star[i].y = random(0,64);
          star[i].randomize = false;
        }
        
        if (star[i].active) {
          
          arduboy.drawRect(star[i].x,star[i].y,1,1);
          star[i].y = star[i].y + star[i].inc;
          
          if(star[i].y >= 64) {
            star[i].active = false;
          }
          
        }

      }


      //Draw Asteroids
      for (uint8_t i = 0; i < NUMBER_OF_ASTEROIDS; ++i) {
        
        if (asteroid[i].mode == MODE_INACTIVE) {
          asteroid[i].mode = MODE_ACTIVE;
          asteroid[i].x = random(0, 124);
          asteroid[i].y = 0;
          asteroid[i].inc = random(1, 3);
        }
        
        if (asteroid[i].randomize == true) {
          asteroid[i].y = random(0, 32);
          asteroid[i].randomize = false;
        }
        
        if (blinking == 0 && player.mode == MODE_ACTIVE && asteroid[i].mode == MODE_ACTIVE) {
          
          //Set Up Collison Boxes
          Rect rectA = { player.x, player.y, 8, 7};
          Rect rectB = { asteroid[i].x, asteroid[i].y, 6, 6};
           
          //Collision Checking
           
          if (arduboy.collide(rectA, rectB)) {
            player.lives-=1;
            player.mode = MODE_EXPLOSION_3;
            asteroid[i].mode = MODE_EXPLOSION_3;
            blinking = 64;
          }
          
        }
        
        if (asteroid[i].mode > MODE_INACTIVE) {
          
          switch (asteroid[i].mode) {
            
            case MODE_ACTIVE:
              arduboy.drawBitmap(asteroid[i].x,asteroid[i].y,Asteroid,6,6);
              break;
            
            case MODE_EXPLOSION_3:
              Sprites::drawSelfMasked(asteroid[i].x - 2, asteroid[i].y - 2, Explosion1, 0);
              break;
            
            case MODE_EXPLOSION_2:
              Sprites::drawSelfMasked(asteroid[i].x - 2, asteroid[i].y - 2, Explosion2, 0);
              break;
            
            case MODE_EXPLOSION_1:
              Sprites::drawSelfMasked(asteroid[i].x - 2, asteroid[i].y - 2, Explosion3, 0);
              break;

          }

          asteroid[i].y = asteroid[i].y + asteroid[i].inc;
          
          if (asteroid[i].y >= 64) {
            asteroid[i].mode = MODE_INACTIVE;
          }

          if (asteroid[i].mode > MODE_INACTIVE && asteroid[i].mode < MODE_ACTIVE) {
            asteroid[i].mode = asteroid[i].mode - 1;
          }

        }

      }


      // Draw bullets

      for (uint8_t i = 0; i < NUMBER_OF_BULLETS; ++i) {

        if (bullet[i].y > -2) {

          bullet[i].y = bullet[i].y - 3;
          arduboy.drawLine(bullet[i].x, bullet[i].y, bullet[i].x, bullet[i].y + 2, WHITE);

          Rect rectA = { bullet[i].x, bullet[i].y, 2, 2};

          for (uint8_t j = 0; j <NUMBER_OF_ASTEROIDS; j++) {

            Rect rectB = { asteroid[j].x, asteroid[j].y, 6, 6};

            if (arduboy.collide(rectA, rectB)) {

              asteroid[j].mode = MODE_EXPLOSION_3;
              bullet[i].y = -2;

            }

          }

        }

      }

      break;

    case 2:
      arduboy.setCursor(48,30);
      arduboy.print("Paused");
      arduboy.setCursor(45,40);
      arduboy.print("Press A");
      if(arduboy.justPressed(A_BUTTON)) { gamestate = 1; }
      break;
      
    case 3:
      arduboy.setCursor(38,30);
      arduboy.print("Game Over");
      if(arduboy.justPressed(B_BUTTON)) { gamestate = 0; }
      
  }

  arduboy.display();

}


