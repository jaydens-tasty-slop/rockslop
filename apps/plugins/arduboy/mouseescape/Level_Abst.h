#ifndef level_h
#define level_h

#include <Arduboy2.h>
#include "Bitmaps.h"
#include "Maus.h"
#include "Food.h"
#include "Snake.h"

enum class LevelState : unsigned char
{
  Won, Lost, Running
};

struct foodNode
{
  Food* food;
  foodNode* next;
};

struct enemyNode
{
  Snake* snake;
  enemyNode* next;
};

class Level_Abst
{
  public:
    Arduboy2 ab;
    byte levelNum = 0;
    byte goalX = WIDTH/2;
    byte goalY = 20;
    byte goalRadius = mouseSprite[0]/2;
    virtual void start(Maus* mouse) = 0;
    virtual void levelUpdate() = 0;
    virtual void levelDraw(Sprites* sprite) = 0;
    virtual void collisionCheck(Maus* mouse) = 0;

    LevelState levelState()
    {
      return state;
    }

    int getLevelNum()
    {
      return levelNum;
    }

    void playerDied()
    {
      if (lives == 0) state = LevelState::Lost;
      else lives--;
    }

    void lifeUp()
    {
      if (foodCollected == 10)
      {
        lives++;
        foodCollected = 0;
      }
    }
    unsigned int getLives()
    {
      return lives;
    }

    /* should only be called after game is lost and starting over */
    void setLives(unsigned int num)
    {
      lives = num;
      state = LevelState::Running;
      levelNum = 0;
    }
    
    void setNum(int num)
    {
      levelNum = num;
      if (levelNum > 20) state = LevelState::Won;
    }

    void addPoints(unsigned int amount)
    {
      points += amount;
    }

    unsigned int getPoints()
    {
      return points;
    }

    void collectedFood()
    {
      foodCollected++;
      lifeUp();
    }
    
  private:
    LevelState state = LevelState::Running;
    unsigned int lives = 3;
    byte foodCollected = 0;
    unsigned int points = 0;
};

#endif
