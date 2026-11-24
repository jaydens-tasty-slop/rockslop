#include <stdlib.h>
#include <math.h>
#include <Arduboy.h>
#include "../lib/ab_math.h"
#include <string.h>
#include "Ball.h"
#include "GameManager.h"

Ball::Ball()
{
}

Ball::Ball(Arduboy *_arduboy, float _x, float _y, float _dx, float _dy)
{
  arduboy = _arduboy;
  x = _x;
  y = _y;
  dx = _dx;
  dy = _dy;
  pre_dx = dx;
  pre_dy = dy;
  next_turn = 0;
  next_pos_x = x;
  out_flag = false;
  hit_flag = false;
}
float Ball::getTop() { return y - GameManager::BALL_HALF_HEIGHT; }
float Ball::getDown() { return y + GameManager::BALL_HALF_HEIGHT; }
float Ball::getRight() { return x + GameManager::BALL_HALF_WIDTH; }
float Ball::getLeft() { return x - GameManager::BALL_HALF_WIDTH; }
float Ball::getDX() { return dx; }
float Ball::getDY() { return dy; }
void Ball::setDX(float _dx) { dx = _dx; }
void Ball::setDY(float _dy) { dy = _dy; }
void Ball::riversDX() { dx = pre_dx * -1; }
void Ball::riversDY() { dy = pre_dy * -1; }

void Ball::move()
{
  hit_flag = false;
  if (out_flag) {
    // 画面外に出てからの処理
    if (next_turn > 0) {
      x += next_pos_move_speed_x;
      if (x > next_pos_x) {
        x = next_pos_x;
        next_turn = 0;
        setActive(false);
      }
    } else if (next_turn < 0) {
      x -= next_pos_move_speed_x;
      if (x < next_pos_x) {
        x = next_pos_x;
        next_turn = 0;
        setActive(false);
      }
    } else {
      setActive(false);
    }
  } else {
    // 画面内の処理
    pre_dx = dx;
    pre_dy = dy;
    x += dx;
    y += dy;

    // 画面はみ出し処理
    if (x < GameManager::FIELD_LEFT + GameManager::BALL_HALF_WIDTH) {
      x = GameManager::FIELD_LEFT + GameManager::BALL_HALF_WIDTH;
      riversDX();
    }
    if (x > GameManager::FIELD_RIGHT - GameManager::BALL_HALF_WIDTH) {
      x = GameManager::FIELD_RIGHT - GameManager::BALL_HALF_WIDTH;
      riversDX();
    }

    if (y < GameManager::BALL_HALF_HEIGHT) {
      y = GameManager::BALL_HALF_HEIGHT;
      riversDY();
    }
    if (y > GameManager::FIELD_DOWN - GameManager::BALL_HEIGHT && dy > 0) {
      // 移動先をセットする
      y = GameManager::FIELD_DOWN - GameManager::BALL_HEIGHT;
      out_flag = true;
    }
  }
}

void Ball::draw()
{
  arduboy->drawCircle(x, y, GameManager::BALL_HALF_WIDTH, 1);
}

void Ball::setNextPosX(float _next_pos_x)
{
  next_pos_x = _next_pos_x;
  if (x > next_pos_x) {
    next_turn = -1;
  } else if (x < next_pos_x) {
    next_turn = 1;
  } else {
    next_turn = 0;
  }
  next_pos_move_speed_x = abs(x - next_pos_x) / 5;
}
bool Ball::isOut()
{
  return out_flag;
}
bool Ball::isHit()
{
  return hit_flag;
}
void Ball::hit()
{
  hit_flag = true;
}

