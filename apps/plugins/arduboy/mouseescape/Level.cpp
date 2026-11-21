#include "Level.h"

void Level::start(Maus* mouse)
{
  setNum(getLevelNum() + 1);

  if (levelState() == LevelState::Running)
  {
    cleanUp();
    drawLevelCard();
    makeFoodList();
    makeEnemyList();
    mouse->updateMaus(WIDTH / 2 - mouseSprite[0]/2, HEIGHT - mouseSprite[1]);
  }
}

void Level::levelUpdate()
{
  enemyNode* current = enemyRoot;
  byte x = 0;
  byte y = 0;
  while (current != NULL)
  {
    if (current->snake != NULL)
    {
      y = current->snake->getY();
      x = current->snake->getX();
      switch(current->snake->getFacing())
      {
        case 'u':
          y -= current->snake->getSpeed();
          break;
        case 'd':
          y += current->snake->getSpeed();
          break;
        case 'r':
          x += current->snake->getSpeed();
          break;
        case 'l':
          x -= current->snake->getSpeed();
          break;
      }
      current->snake->updateSnake(x,y);
    }
    current = current->next;
  }  
}

void Level::levelDraw(Sprites* sprite)
{
  drawStatusBar();
  drawGoal();
  foodNode* foodNode = foodRoot;
  while (foodNode != NULL)
  {
    if (foodNode->food != NULL) sprite->drawSelfMasked(foodNode->food->getX(), foodNode->food->getY(), foodSprites, foodNode->food->getFrame());
    foodNode = foodNode->next;
  }
  drawSnakes(sprite);
}

void Level::collisionCheck(Maus* mouse)
{
  byte x1 = mouse->getX();
  byte y1 = mouse->getY();
  byte x2 = x1 + mouseSprite[0];
  byte y2 = y1 + mouseSprite[1];
  Rect mouseRect = Rect(mouse->getX(), mouse->getY(), mouseSprite[0], mouseSprite[1]);
  Rect goalRect(goalX - 2, goalY, goalRadius/2, goalRadius/2);

  foodNode* foodNode = foodRoot;
  /* check food collision */
  while (foodNode != NULL)
  {
    if (foodNode->food != NULL)
    {
      if(ab.collide(Rect(foodNode->food->getX(), foodNode->food->getY(), foodSprites[0], foodSprites[1]), mouseRect))
      {
        collectedFood();
        addPoints(foodNode->food->getValue());
        delete foodNode->food;
        foodNode->food = NULL;
      }
    }
    foodNode = foodNode->next;
  }
  /* check goal collision */
  if(ab.collide(goalRect, mouseRect))
  {
    start(mouse);
  }

  /* snake collision */
  enemyNode* enemyNode = enemyRoot;
  while(enemyNode != NULL)
  {
    if(!mouse->getInvincible() && ab.collide(mouseRect, enemyNode->snake->getBody())) 
    {
      playerDied();
      mouse->updateMaus(WIDTH / 2 - mouseSprite[0]/2, HEIGHT - mouseSprite[1]);
      mouse->setInvincible(true);
      break;
    }
    enemyNode = enemyNode->next;
  }
}

/* Private */
bool Level::spotIsTaken(byte x, byte y)
{
  foodNode* current = foodRoot;
  if (x > goalX - (2 * goalRadius) && x < goalX + (2 * goalRadius))
  {
    if (y > goalY - (2 * goalRadius) && y < goalY + (2 * goalRadius))
    {
      return true;
    }
  }

  while (current != NULL)
  {
    if (current->food->getX() == x && current->food->getY() == y)
      return true;
    else current = current->next;
  }
  return false;
}

byte Level::getFoodValue()
{
  byte value1 = 0;
  byte value2 = 0;
  byte value3 = 0;
  byte value = 0;

  foodNode* current = foodRoot;
  while (current != NULL)
  {
    value = current->food->getValue();
    if (value == 1) value1++;
    else if (value == 5) value2++;
    else if (value == 10) value3++;

    current = current->next;
  }

  if (value1 < 2) return 1;
  if (value2 < 2) return 5;
  else return 10;
}

void Level::makeFoodList()
{
  foodRoot = new foodNode;
  foodRoot->food = new Food(WIDTH / 2, HEIGHT / 2, 1, 0);
  foodRoot->next = NULL;

  foodNode* current = foodRoot;
  byte x = 0;
  byte y = 0;
  byte value = 0;
  byte frame = 0;

  for (int i = 0; i < AMOUNT_FOOD - 1; i++)
  {
    foodNode* food = new foodNode;
    x = random(WIDTH - foodSprites[0]);
    y = random(11, HEIGHT - foodSprites[1]);

    while (spotIsTaken(x, y))
    {
      x = random(WIDTH - foodSprites[0]);
      y = random(11, HEIGHT - foodSprites[1]);
    }

    value = getFoodValue();
    switch (value)
    {
      case 1:
        frame = 0;
        break;
      case 5:
        frame = 1;
        break;
      case 10:
        frame = 2;
        break;
      default:
        frame = 0;
        break;
    }

    food->food = new Food(x, y, value, frame);
    food->next = NULL;
    current->next = food;
    current = food;
  }
}

void Level::makeEnemyList()
{
  enemyRoot = new enemyNode;
  enemyRoot->snake = new Snake(0, HEIGHT / 2 - 2, 'r', 3);
  enemyRoot->next = NULL;
  enemyNode* current = enemyRoot;

  int maxEnemies = (getLevelNum() / 5);
  for (int i = 1; i <= maxEnemies; i++)
  {
    enemyNode* enemy = new enemyNode;
    enemy->next = NULL;
    current->next = enemy;
    switch (i)
    {
      case 1:
        enemy->snake = new Snake(WIDTH, 12, 'l', 3);
        break;
      case 2:
        enemy->snake = new Snake(WIDTH / 8, 0, 'd', 3);
        break;
      case 3:
        enemy->snake = new Snake((7 * WIDTH) / 8, HEIGHT, 'u', 3);
        break;
    }
    current = enemy;
  }
}

void Level::drawStatusBar()
{
  ab.setCursor(0, 0);
  ab.print("Lives ");
  ab.print(getLives());
  ab.setCursor(WIDTH / 2, 0);
  ab.print("Points ");
  ab.print(getPoints());
  ab.drawLine(0, 10, WIDTH, 10, WHITE);
}

void Level::drawGoal()
{
  ab.drawCircle(goalX, goalY, goalRadius);
}

void Level::drawLevelCard()
{
  ab.clear();
  ab.setCursor(WIDTH / 2 - 16, HEIGHT / 2);
  ab.print("Level ");
  ab.print(getLevelNum());
  ab.display();
  ab.delayShort(2000);
}

void Level::drawSnakes(Sprites* sprite)
{
  enemyNode* current = enemyRoot;
  while (current != NULL)
  {
    if (current->snake != NULL) current->snake->drawSnake(sprite);
    current = current->next;
  }
}

void Level::cleanUp()
{
  foodNode* current = foodRoot;
  foodNode* next = NULL;
  while (current != NULL)
  {
    delete current->food;
    next = current->next;
    if (next != NULL) delete current;
    current = next;
  }

  enemyNode* enCurrent = enemyRoot;
  // upstream dereferenced a null enemyRoot (harmless on AVR, a fault here)
  enemyNode* enNext = NULL;
  while (enCurrent != NULL)
  {
    delete enCurrent->snake;
    enNext = enCurrent->next;
    if (enNext != NULL) delete enCurrent;
    enCurrent = enNext;
  }
}
