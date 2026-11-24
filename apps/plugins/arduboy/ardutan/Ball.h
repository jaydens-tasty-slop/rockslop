#ifndef _INC_BALL    //まだ読み込まれていなければ以下の処理をする
#define _INC_BALL    //すでに読み込まれているという目印をつける

#include <Arduboy.h>
#include "GameObject.h"

class Ball : public GameObject
{
 private:
  // 移動量
  float dx;
  float dy;
  float pre_dx;
  float pre_dy;
  int next_turn;
  float next_pos_x;
  float next_pos_move_speed_x;
  bool out_flag;
  bool hit_flag;
 public:
  Ball();
  Ball(Arduboy *_arduboy, float _x, float _y, float _dx, float _dy);
  float getTop();
  float getDown();
  float getRight();
  float getLeft();
  float getDX();
  float getDY();
  void setDX(float _dx);
  void setDY(float _dy);
  void riversDX();
  void riversDY();
  void move();
  void draw();
  void setNextPosX(float _next_pos_x);
  bool isOut();
  bool isHit();
  void hit();
};
#endif    //INC_BALL
