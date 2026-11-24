#include "Level_Abst.h"

class Level : public Level_Abst
{
  public:
    Level() {}
    void start(Maus* mouse);
    void levelUpdate();
    void levelDraw(Sprites* sprite);
    void collisionCheck(Maus* mouse);

  private:
    byte AMOUNT_FOOD = 5;
    foodNode* foodRoot;
    enemyNode* enemyRoot;
    bool spotIsTaken(byte x, byte y);
    byte getFoodValue();
    void makeFoodList();
    void makeEnemyList();
    void drawStatusBar();
    void drawGoal();
    void drawLevelCard();
    void drawSnakes(Sprites* sprite);
    void cleanUp();
};
