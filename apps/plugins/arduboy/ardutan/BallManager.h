#ifndef _INC_BALL_MANAGER    //まだ読み込まれていなければ以下の処理をする
#define _INC_BALL_MANAGER    //すでに読み込まれているという目印をつける

#include <Arduboy.h>
#include "GameObject.h"
#include "GameManager.h"
#include "Ball.h"
#include "Block.h"

class BallManager
{
 private:
  Arduboy *arduboy;
  Ball* balls[GameManager::MAX_BALL_BUFFER];
  Ball* first_out_ball;
  unsigned int ball_count;
  int ball_limit;
  int ball_make_wait_frame_counter;
  float ball_dx;
  float ball_dy;
  float next_pos_x;
 public:
  BallManager(Arduboy *_arduboy);
  void init(int _ball_limit);
  void addBall(float _x, float _y);
  void setBallSpeed(float facter_x, float facter_y);
  bool moveBalls();
  void drawBalls();
  Ball* getFirstOutBall();
  bool finishAddBall();
  void viewBallCount(int x, int y);
  void isHit(Block* block);
};
#endif    //INC_BALL_MANAGER
