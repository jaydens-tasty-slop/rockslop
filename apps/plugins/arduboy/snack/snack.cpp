#include <stdlib.h>
#include <math.h>
#include "Arduboy.h"
#include "../lib/ab_math.h"

// make an instance of arduboy used for many functions
Arduboy arduboy;

byte maxX = 127;
byte minX = 0;
byte maxY = 63;
byte minY = 10;
char gameState = 'T';
bool sound = false;

struct coords
{
  public:
  byte X = 0;
  byte Y = 0;
  bool Active = false;
};

typedef struct coords Coords;

struct player
{
  byte X = 0;
  byte Y = 0;
  char dir = 'R';
  int Len = 1;
  static const int MaxLength = 100;
  //Coords *Moves[MaxLength] = {NULL};
  Coords Moves[MaxLength];
};

typedef struct player Player;

Player p1;
Coords foodLocation;
Coords previousPosition;


int score = 0;
int arrayPosition = 0;

void GenerateFood()
{
  arduboy.initRandomSeed();
  foodLocation.X = random(minX + 1, maxX - 1);
  foodLocation.Y = random(minY + 1, maxY - 1);
}

void StartGame()
{
  p1.X = maxX / 2;
  p1.Y = (maxY / 2) + 10;
  GenerateFood();
  score = 0;
  arrayPosition = 0;
  p1.Len = 1;
  for (int i=0; i < p1.MaxLength; i++){
    p1.Moves[i].Active = false;
  }

  
  gameState = 'G';
}

void LogMove()
{
  p1.Moves[arrayPosition].Y = p1.Y;

  if(p1.Moves[arrayPosition].Active == false)
  {
      p1.Moves[arrayPosition].Active = true;    
  }
      p1.Moves[arrayPosition].X = p1.X;
      p1.Moves[arrayPosition].Y = p1.Y;

  if(arrayPosition < p1.Len -1)
  {
    arrayPosition += 1;
    
  }
  else
  {
    arrayPosition = 0;
  }
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

  //Check if hit wall

  if(p1.X >= maxX || p1.X <= minX || p1.Y >= maxY || p1.Y <= minY)
  {
    EndGame();
  }

  //Check if hit tail
  for (int i=0; i < p1.MaxLength; i++){

        if(p1.Moves[i].Active == true)
        {
        if(p1.Moves[i].X == p1.X && p1.Moves[i].Y == p1.Y)
        {
          EndGame();
        }
      }
  }

  // Check if on food
  if(p1.X == foodLocation.X && p1.Y == foodLocation.Y)
  {
    if(p1.Len < p1.MaxLength)
    {
      p1.Len += 1;
    }
    if(sound == true)
    {
      arduboy.tunes.tone(987, 160);
    }
    
    score += 1;
    LogMove();
    GenerateFood();
  }
}

void DrawTail()
{
  int tailPosition = arrayPosition;
  for (int i=0; i < p1.Len; i++){
      if(p1.Moves[tailPosition].Active == true)
      {
         arduboy.drawPixel(p1.Moves[tailPosition].X, p1.Moves[tailPosition].Y, 1);
      }
      else
      {
        return;
      }


      if(tailPosition > 0)
      {
        tailPosition -= 1;
      }
      else
      {
        tailPosition = p1.Len -1;
      }
   } 
}

void DrawFrame()
{
  arduboy.drawRect(minX,minY, maxX +1, (maxY - minY) +1, WHITE);
}

// This function runs once in your game.
// use it for anything that needs to be set only once in your game.
void GenerateFood();
void StartGame();
void LogMove();
void EndGame();
void CollisionDetection();
void DrawTail();
void DrawFrame();

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
    arduboy.setFrameRate(30);
    DrawFrame();
  
    if(arduboy.pressed(RIGHT_BUTTON)) {
      if(p1.dir != 'L')
      {
        p1.dir = 'R';
      }
    }
  
    if(arduboy.pressed(LEFT_BUTTON)) {
      if(p1.dir != 'R')
      {
        p1.dir = 'L';
      }
    }
  
    if(arduboy.pressed(UP_BUTTON)) {
      if(p1.dir != 'D')
      {
        p1.dir = 'U';
      }
    }
  
    if(arduboy.pressed(DOWN_BUTTON)) {
      if(p1.dir != 'U')
      {
        p1.dir = 'D'; 
      } 
    }
  
    switch(p1.dir)
    {
      case 'U':
        if(p1.Y > minY)
        {
          p1.Y -= 1;
        }
        break;
      case 'D':
        if(p1.Y < maxY)
        {
          p1.Y += 1;
        }
        break;
      case 'L':
        if(p1.X > minX)
        {
          p1.X-= 1;
        }
        break;
      case 'R':
        if(p1.X < maxX)
        {
          p1.X+= 1;
        }
      
      break;
    }

    CollisionDetection();
  
    arduboy.drawPixel(p1.X, p1.Y, 1);
    arduboy.drawPixel(foodLocation.X, foodLocation.Y, 1);
    
    if(p1.X != previousPosition.X || p1.Y != previousPosition.Y)
    {
      previousPosition.X = p1.X;
      previousPosition.Y = p1.Y;
      LogMove();
    }
    DrawTail();    
    
    arduboy.setCursor(63, 0);  
    arduboy.print("Score ");
  
    char scoreBuff[5];
    sprintf(scoreBuff, "%05d", score);
    
    arduboy.print(scoreBuff);
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
    arduboy.print("Snack");

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

  
  

  // then we finaly we tell the arduboy to display what we just wrote to the display
  arduboy.display();
}
