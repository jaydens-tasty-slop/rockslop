#include <stdlib.h>
#include <math.h>
#include "Arduboy.h"
#include "bitmaps.h"
#include "../lib/ab_math.h"

// make an instance of arduboy used for many functions
Arduboy arduboy;

byte maxX = 127;
byte minX = 0;
byte maxY = 63;
byte minY = 10;
char gameState = 'T';
bool sound = false;

const byte maxProjs = 3;
const byte maxEnemies = 10;
byte fireDelay = 0;
byte movementCounter = 0;
byte movementSpeed = 10;

struct Enemy
{
  byte X = 0;
  byte Y = 0;
  byte W = 0;
  byte H = 0;
  byte moveDelay = 60;
  bool Active = false;
  char Dir = 'R';
  byte ExFrames = 0;
};

struct Proj
{
  byte X = 0;
  byte Y = 0;
  bool Active = false;
};

struct player
{
  byte X = 0;
  byte Y = 0;
  byte W = 7;
  byte H = 4;
};

typedef struct player Player;

Player p1;

Proj projs[maxProjs];
Enemy enemies[maxEnemies];

int score = 0;
int arrayPosition = 0;

void StartGame();
void EndGame();
void CollisionDetection();
void DrawFrame();
void addEnemy(byte x, byte y, byte w, byte h, char dir);

void StartGame()
{
  p1.X = (maxX / 2) - (p1.W / 2);
  p1.Y = maxY - p1.H;
  score = 0;
  for(int i = 5; i <= (maxX - 8);i+=10)
  {
      addEnemy(i, 5, 8, 6, 'R');
  }
  
  gameState = 'G';
}

void EndGame()
{
  if(sound == true)
  {
    arduboy.tunes.tone(200, 200);
    delay(400);
    arduboy.tunes.tone(100, 400);
    delay(600);
    arduboy.tunes.tone(50, 2000);
  }
    
  gameState = 'E';
}

void CollisionDetection()
{

}

void DrawFrame()
{
  //arduboy.drawRect(minX,minY, maxX +1, (maxY - minY) +1, WHITE);
  
  for (int i = 0; i < maxProjs; i++) 
  {
      if(projs[i].Active == true)
      {
        projs[i].Y -= 1;
        if(projs[i].Y == 0)
        {
          projs[i].Active = false;
        }
        else
        {
          arduboy.drawPixel(projs[i].X, projs[i].Y, WHITE);       
        }
      }
//      Serial.print("\nActive ");
//      Serial.print(projs[i].Active);
  }
      bool enemiesActive = false;
      for (int i = 0; i < maxEnemies; i++)
      {
          if(enemies[i].Active == true)
          {
            enemiesActive = true;
            for (int p = 0; p < maxProjs; p++) 
            {
              if(projs[p].Active == true)
              {
                if(projs[p].X >= enemies[i].X && (projs[p].X <= (enemies[i].X + enemies[i].W)) && projs[p].Y >= enemies[i].Y && (projs[p].Y <= (enemies[i].Y + enemies[i].H)))
                {
                  projs[i].Active = false;
                  enemies[i].Active = false;
                  enemies[i].ExFrames = 40;
                  continue;
                }
              }
            }
            
            if(movementCounter == movementSpeed)
            {
              switch(enemies[i].Dir)
              {
                case 'L':
                if(enemies[i].X > 0)
                {
                  enemies[i].X --;
                }
                else
                {
                  enemies[i].Y += 10;
                  enemies[i].Dir = 'R';
                }
                break;
                case 'R':
                if(enemies[i].X < maxX - enemies[i].W)
                {
                  enemies[i].X ++;
                }
                else
                {
                  enemies[i].Y += 10;
                  enemies[i].Dir = 'L';
                }
                break;      
              }
            }

            arduboy.drawSlowXYBitmap(enemies[i].X,enemies[i].Y, ship1, 8 , 6, WHITE);
            randomSeed(analogRead(5));
            int shouldFireResult = random(1, 10);
            if(shouldFireResult == 1)
            {
              //Fire if free projectiles
            }
                
              }
              if(enemies[i].ExFrames > 0)
              {
                enemiesActive = true;
                arduboy.drawSlowXYBitmap(enemies[i].X,enemies[i].Y, ex1, 8 , 6, WHITE);
                enemies[i].ExFrames --;
              }
          }

  if(enemiesActive == false)
  {
    gameState = 'T';
  }
  
  arduboy.drawSlowXYBitmap(p1.X,p1.Y, playerShip, p1.W, p1.H, WHITE);
  if(movementCounter < movementSpeed)
  {
      movementCounter ++;
      if(sound)
      {
        //arduboy.tunes.tone(800, 10);
      }
  }
  else
  {
    movementCounter = 0;
  }

  
}

void addEnemy(byte x, byte y, byte w, byte h, char dir)
{
  for (int i = 0; i < maxEnemies; i++)
      {
          if(enemies[i].Active == false)
          {
            enemies[i].X = x;
            enemies[i].Y = y;
            enemies[i].H = h;
            enemies[i].W = w;
            enemies[i].Dir = dir;
            enemies[i].Active = true;
            return;
          }
      }
}

// This function runs once in your game.
// use it for anything that needs to be set only once in your game.
void setup() {
  // initiate arduboy instance
  arduboy.begin();

  // here we set the framerate to 15, we do not need to run at
  // default 60 and it saves us battery life
  arduboy.setFrameRate(30);
}


// our main game loop, this runs once every cycle/frame.
// this is where our game logic goes.
void loop() {
  // pause render until it's time for the next frame
  if (!(arduboy.nextFrame()))
    return;

  // first we clear our screen to black
  arduboy.clear();

  if(gameState == 'G')
  {
    arduboy.setFrameRate(60);
    DrawFrame();
  
    if(arduboy.pressed(RIGHT_BUTTON)) {
      if((p1.X) < (maxX - p1.W))
      {
        p1.X += 1;
      }
    }
  
    if(arduboy.pressed(LEFT_BUTTON)) {
      if(p1.X > 0)
      {
        p1.X -= 1;
      }
    }

    if(arduboy.pressed(B_BUTTON))
    {
      if(fireDelay == 0)
      {
        for (int i = 0; i < maxProjs; i++) 
        {
          if(projs[i].Active == false)
          {
            projs[i].X = p1.X + (p1.W /2);
            projs[i].Y = p1.Y - 1;
            projs[i].Active = true;
            fireDelay = 20;
            if(sound)
            {
              arduboy.tunes.tone(500, 100);
            }
            break;
          }
        }
      }
    }
  
  


    CollisionDetection();
      
//    arduboy.setCursor(63, 0);  
//    arduboy.print("Score ");
//  
//    char scoreBuff[5];
//    sprintf(scoreBuff, "%05d", score);
//    
//    arduboy.print(scoreBuff);

    if(fireDelay > 0)
    {
      fireDelay --;
    }
  }
  else if(gameState == 'E')
  {
    arduboy.setFrameRate(10);
    arduboy.setCursor(20, 20);  
    arduboy.print("Game Over");
    
    arduboy.setCursor(20, 40);  
    arduboy.print("Score ");
  
    char scoreBuff[5];
    sprintf(scoreBuff, "%05d", score);
    
    arduboy.print(scoreBuff);

    if(arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)) {
      gameState = 'T';
    }

  }
  else if(gameState == 'T')
  {
    arduboy.setFrameRate(10);
    arduboy.setCursor(50, 1);  
    arduboy.print("Space");

    arduboy.setCursor(20, 15);  
    arduboy.print("By Shane Powell");
    
    arduboy.setCursor(50, 55);  
    arduboy.print("Sound");

    arduboy.setCursor(90, 55);  
    arduboy.print("On");

    arduboy.setCursor(109, 55);  
    arduboy.print("Off");
   
    if(arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)) {
      StartGame();
    }
    else if(arduboy.pressed(LEFT_BUTTON))
    {
      sound = true;
    }
    else if(arduboy.pressed(RIGHT_BUTTON))
    {
      sound = false;
    }

    if(sound == true)
    {
          arduboy.drawRect(88,53, 18, 11, WHITE);
    }
    else
    {
          arduboy.drawRect(107,53, 21, 11, WHITE);
    }
  }

  
  

  arduboy.display();
}
