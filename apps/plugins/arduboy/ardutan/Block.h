#ifndef _INC_BLOCK    //まだ読み込まれていなければ以下の処理をする
#define _INC_BLOCK    //すでに読み込まれているという目印をつける

#include <Arduboy.h>
#include "GameObject.h"

class Block : public GameObject
{
 private:
  int hp;
  float next_y;
  bool strong_flag;
 public:
  /* static const unsigned int BLOCK_WIDTH = 9; */
  /* static const unsigned int BLOCK_HEIGHT = 10; */
  /* static const unsigned int MOVE_SPEED = 1; */
  Block();
  Block(Arduboy *_arduboy, float _x, float _y, int _hp, bool _strong_flag);
  float getTop();
  float getDown();
  float getRight();
  float getLeft();
  void draw();
  int getHp();
  void hit(int damage);
  void setNextPos(float _next_y);
  void moveNextPos();
  bool isMove();
  void drawNumber(int number, int x, int y, uint8_t color);
  void drawQuestionMark(int x, int y, uint8_t color);
};
#endif    //INC_BLOCK
