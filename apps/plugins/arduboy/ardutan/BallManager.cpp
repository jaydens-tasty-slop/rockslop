#include <stdlib.h>
#include <math.h>
#include <Arduboy.h>
#include "../lib/ab_math.h"
#include <string.h>
#include "BallManager.h"
#include "GameManager.h"
#include "Ball.h"
#include "Block.h"

BallManager::BallManager(Arduboy *_arduboy)
{
  arduboy = _arduboy;
  for (int i = 0; i < GameManager::MAX_BALL_BUFFER; i++) {
    if (balls[i] == NULL) continue;
    balls[i] = NULL;
  }
}

void BallManager::init(int _ball_limit)
{
  first_out_ball = NULL;
  ball_count = 0;
  ball_make_wait_frame_counter = 0;
  ball_dx = 0;
  ball_dy = 0;
  ball_limit = _ball_limit;
  if (ball_limit > GameManager::MAX_BALL_COUNT) ball_limit = GameManager::MAX_BALL_COUNT;

  for (int i = 0; i < GameManager::MAX_BALL_BUFFER; i++) {
    if (balls[i] == NULL) continue;
    delete(balls[i]);
    balls[i] = NULL;
  }
}

void BallManager::addBall(float _x, float _y)
{
  if (GameManager::MAX_BALL_COUNT <= ball_count) return;
  if (ball_count >= ball_limit) return;

  if (ball_make_wait_frame_counter > 0) {
    ball_make_wait_frame_counter--;
    return;
  }

  // 空いてるバッファを使う
  for (int i = 0; i < GameManager::MAX_BALL_BUFFER; i++) {
    if (balls[i] != NULL) continue;
    balls[i] = new Ball(arduboy, _x, _y, ball_dx, ball_dy);
    ball_count++;
    break;
  }
  ball_make_wait_frame_counter = GameManager::BALL_MAKE_WAIT_FRAME;
}

void BallManager::setBallSpeed(float facter_x, float facter_y)
{
  ball_dx = (facter_x / GameManager::CURSOR_SIZE) * GameManager::BALL_SPEED;
  ball_dy = (facter_y / GameManager::CURSOR_SIZE) * GameManager::BALL_SPEED;
}  

bool BallManager::moveBalls()
{
  bool all_out_flag = true;
  bool before_out_flag = false;
  for (int i = 0; i < GameManager::MAX_BALL_BUFFER; i++) {
    if (balls[i] == NULL) continue;
    Ball* ball = balls[i];
    if (ball->isActive()) {
      before_out_flag = ball->isOut();
      ball->move();
      // 今回外に出たボールのみ処理対象
      if (!before_out_flag && ball->isOut()) {
        // 最初のボール着地点を、次のスタート地点にする
        if (first_out_ball == NULL) {
          next_pos_x = ball->getX();
          first_out_ball = ball;
        }
        // ボールも着地点に向かわせる
        ball->setNextPosX(next_pos_x);
      }
      // ボールが１つでもあれば、終了しない
      all_out_flag = false;
    } else {
      delete(balls[i]);
      balls[i] = NULL;
    }
  }

  return all_out_flag;
}  

void BallManager::drawBalls()
{
  for (int i = 0; i < GameManager::MAX_BALL_BUFFER; i++) {
    if (balls[i] == NULL) continue;
    Ball* ball = balls[i];
    if (ball->isActive()) {
      ball->draw();
    }
  }
}

Ball* BallManager::getFirstOutBall()
{
  return first_out_ball;
}

bool BallManager::finishAddBall()
{
  return ball_count == ball_limit;
}

void BallManager::viewBallCount(int x, int y)
{
  if (finishAddBall()) return;

  // 残り弾数表示
  arduboy->setCursor(x, y);
  arduboy->print("x");
  arduboy->print(ball_limit - ball_count);
}

void BallManager::isHit(Block* block)
{
  for (int i = 0; i < GameManager::MAX_BALL_BUFFER; i++) {
    if (balls[i] == NULL) continue;
    Ball* ball = balls[i];
    if (!ball->isActive()) continue;
    // 画面外の弾は処理しない
    if (ball->isOut()) continue;
    // 既にブロックに当たっている弾は処理しない
    // if (ball->isHit()) continue;

    bool hit_flag = false;
    bool top_hit_flag = false;
    bool down_hit_flag = false;
    bool left_hit_flag = false;
    bool right_hit_flag = false;
    // ブロック上との当たり判定
    if (ball->getDY() > 0 &&
        ball->getX() <= block->getRight() && ball->getX() >= block->getLeft() &&
        ball->getDown() >= block->getTop() && ball->getDown() <= block->getDown()) {
      top_hit_flag = true;
    }
    // ブロック下との当たり判定
    if (ball->getDY() < 0 &&
        ball->getX() <= block->getRight() && ball->getX() >= block->getLeft() &&
        ball->getTop() >= block->getTop() && ball->getTop() <= block->getDown()) {
      down_hit_flag = true;
    }
    // ブロックの左との当たり判定
    if (ball->getDX() > 0 &&
        ball->getRight() >= block->getLeft() && ball->getRight() <= block->getRight()  &&
        ball->getY() >= block->getTop() && ball->getY() <= block->getDown()) {
      left_hit_flag = true;
    }
    // ブロックの右との当たり判定
    if (ball->getDX() < 0 &&
        ball->getLeft() >= block->getLeft() && ball->getLeft() <= block->getRight() &&
        ball->getY() >= block->getTop() && ball->getY() <= block->getDown()) {
      right_hit_flag = true;
    }

    float abs_x = 0;
    float abs_y = 0;
    if (ball->getDY() > 0 && ball->getDX() > 0) {
      abs_x = abs(ball->getX() - block->getLeft());
      abs_y = abs(ball->getY() - block->getTop());
      // ➘右下
      // 左上にヒット
      if (top_hit_flag && left_hit_flag) {
        hit_flag = true;
        if (abs_x < abs_y) {
          // Xのが近い
          ball->setX(block->getLeft() - GameManager::BALL_HALF_WIDTH);
          ball->riversDX();
        } else if (abs_x > abs_y) {
          // Yのが近い
          ball->setY(block->getTop() - GameManager::BALL_HALF_HEIGHT);
          ball->riversDY();
        } else {
          // 一緒
          ball->setX(block->getLeft() - GameManager::BALL_HALF_WIDTH);
          ball->setY(block->getTop() - GameManager::BALL_HALF_HEIGHT);
          ball->riversDX();
          ball->riversDY();
        }
      }
    } else  if (ball->getDY() > 0 && ball->getDX() < 0) {
      abs_x = abs(ball->getX() - block->getRight());
      abs_y = abs(ball->getY() - block->getTop());
      // ↙左下
      // 右上にヒット
      if (top_hit_flag && right_hit_flag) {
        hit_flag = true;
        if (abs_x < abs_y) {
          // Xのが近い
          ball->setX(block->getRight() + GameManager::BALL_HALF_WIDTH);
          ball->riversDX();
        } else if (abs_x > abs_y) {
          // Yのが近い
          ball->setY(block->getTop() - GameManager::BALL_HALF_HEIGHT);
          ball->riversDY();
        } else {
          // 一緒
          ball->setY(block->getTop() - GameManager::BALL_HALF_HEIGHT);
          ball->setX(block->getRight() + GameManager::BALL_HALF_WIDTH);
          ball->riversDX();
          ball->riversDY();
        }
      }
    } else if (ball->getDY() < 0 && ball->getDX() > 0) {
      abs_x = abs(ball->getX() - block->getLeft());
      abs_y = abs(ball->getY() - block->getDown());
      // ➚右上
      // 左下にヒット
      if (down_hit_flag && left_hit_flag) {
        hit_flag = true;
        if (abs_x < abs_y) {
          // Xのが近い
          ball->setX(block->getLeft() - GameManager::BALL_HALF_WIDTH);
          ball->riversDX();
        } else if (abs_x > abs_y) {
          // Yのが近い
          ball->setY(block->getDown() + GameManager::BALL_HALF_HEIGHT);
          ball->riversDY();
        } else {
          // 一緒
          ball->setY(block->getDown() + GameManager::BALL_HALF_HEIGHT);
          ball->setX(block->getLeft() - GameManager::BALL_HALF_WIDTH);
          ball->riversDX();
          ball->riversDY();
        }
      }
    } else if (ball->getDY() < 0 && ball->getDX() < 0) {
      abs_x = abs(ball->getX() - block->getRight());
      abs_y = abs(ball->getY() - block->getDown());
      // ↖左上
      // 右下にヒット
      if (down_hit_flag && right_hit_flag) {
        hit_flag = true;
        if (abs_x < abs_y) {
          // Xのが近い
          ball->setX(block->getRight() + GameManager::BALL_HALF_WIDTH);
          ball->riversDX();
        } else if (abs_x > abs_y) {
          // Yのが近い
          ball->setY(block->getDown() + GameManager::BALL_HALF_HEIGHT);
          ball->riversDY();
        } else {
          // 一緒
          ball->setX(block->getRight() + GameManager::BALL_HALF_WIDTH);
          ball->setY(block->getDown() + GameManager::BALL_HALF_HEIGHT);
          ball->riversDX();
          ball->riversDY();
        }
      }
    }

    if (hit_flag) {
      // 処理済み
    } else {
      if (top_hit_flag) {
        ball->setY(block->getTop() - GameManager::BALL_HALF_HEIGHT);
        ball->riversDY();
        hit_flag = true;
      }
      if (down_hit_flag) {
        ball->setY(block->getDown() + GameManager::BALL_HALF_HEIGHT);
        ball->riversDY();
        hit_flag = true;
      }
      if (left_hit_flag) {
        ball->setX(block->getLeft() - GameManager::BALL_HALF_WIDTH);
        ball->riversDX();
        hit_flag = true;
      }
      if (right_hit_flag) {
        ball->setX(block->getRight() + GameManager::BALL_HALF_WIDTH);
        ball->riversDX();
        hit_flag = true;
      }
    }
          
    if (hit_flag) {
      block->hit(1);
      ball->hit();
      if (block->getHp() == 0) {
        block->setActive(false);
        break;
      }
    }
  }
}

