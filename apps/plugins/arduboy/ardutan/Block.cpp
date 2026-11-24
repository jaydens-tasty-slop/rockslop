#include <stdlib.h>
#include <math.h>
#include <Arduboy.h>
#include "../lib/ab_math.h"
#include <string.h>
#include "Block.h"
#include "GameManager.h"

Block::Block()
{
}

Block::Block(Arduboy *_arduboy, float _x, float _y, int _hp, bool _strong_flag)
{
  arduboy = _arduboy;
  x = _x;
  y = _y;
  next_y = y;
  active_flag = true;
  hp = _hp;
  strong_flag = _strong_flag;
}

float Block::getTop() { return y; }
float Block::getDown() { return y + GameManager::BLOCK_HEIGHT; }
float Block::getRight() { return x + GameManager::BLOCK_WIDTH; }
float Block::getLeft() { return x; }

void Block::draw()
{
  uint8_t num_color = WHITE;
  if (strong_flag) {
    arduboy->fillRect(x, y, GameManager::BLOCK_WIDTH, GameManager::BLOCK_HEIGHT, WHITE);
    num_color = BLACK;
  } else {
    arduboy->drawRect(x, y, GameManager::BLOCK_WIDTH, GameManager::BLOCK_HEIGHT, WHITE);
  }
  if (hp < 10) {
    // -1と、-3は調整池（凡そフォントサイズの半分）
    GameManager::DrawNumber(arduboy, hp, x + GameManager::BLOCK_HALF_WIDTH - 1, y + GameManager::BLOCK_HALF_HEIGHT - 3, num_color);
  } else if (hp < 100) {
    int tens_digit = floor(hp / 10);
    int digit = hp % 10;
    GameManager::DrawNumber(arduboy, tens_digit, x + (GameManager::BLOCK_WIDTH * 0.25) - 1, y + GameManager::BLOCK_HALF_HEIGHT - 3, num_color);
    GameManager::DrawNumber(arduboy, digit, x + (GameManager::BLOCK_WIDTH * 0.75) - 1, y + GameManager::BLOCK_HALF_HEIGHT - 3, num_color);
  } else {
    GameManager::DrawQuestionMark(arduboy, x + GameManager::BLOCK_HALF_WIDTH - 1, y + GameManager::BLOCK_HALF_HEIGHT - 3, num_color);
  }
}

int Block::getHp()
{
  return hp;
}

void Block::hit(int damage)
{
  hp -= damage;
  if (hp < 0) hp = 0;
}

void Block::setNextPos(float _next_y)
{
  next_y = _next_y;
}

void Block::moveNextPos()
{
  y += GameManager::BLOCK_MOVE_SPEED;
  if (y > next_y) y = next_y;
}

bool Block::isMove()
{
  return y != next_y;
}
