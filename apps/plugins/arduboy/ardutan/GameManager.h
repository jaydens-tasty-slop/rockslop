#ifndef _INC_GAMEMANAGER    //まだ読み込まれていなければ以下の処理をする
#define _INC_GAMEMANAGER    //すでに読み込まれているという目印をつける

#include <Arduboy.h>
#include "GameObject.h"

class GameManager
{
 private:
 public:
  static const unsigned char MAX_BALL_COUNT = 2000;
  static const unsigned char MAX_BALL_BUFFER = 10;
  static const unsigned char BALL_MAKE_WAIT_FRAME = 10;
  static const unsigned char BALL_WIDTH = 4;
  static const unsigned char BALL_HEIGHT = 4;
  static const unsigned char BALL_HALF_WIDTH = BALL_WIDTH / 2;
  static const unsigned char BALL_HALF_HEIGHT = BALL_HEIGHT / 2;
  static const unsigned char BALL_SPEED = 2;
  static const unsigned char MAX_BLOCK_COUNT = 35;
  static const unsigned char BLOCK_CREATE_NUM = 7;
  static const unsigned char BLOCK_CREATE_INTERVAL_POS_W = 1;
  static const unsigned char BLOCK_CREATE_INTERVAL_POS_H = 1;
  static const unsigned char BLOCK_WIDTH = 9;
  static const unsigned char BLOCK_HEIGHT = 9;
  static const unsigned char BLOCK_HALF_WIDTH = BLOCK_WIDTH / 2;
  static const unsigned char BLOCK_HALF_HEIGHT = BLOCK_HEIGHT / 2;
  static const char BLOCK_CREATE_START_POS_Y = -BLOCK_HEIGHT;
  static const unsigned char BLOCK_MOVE_SPEED = 1;
  static const unsigned char BLOCK_STRONG_TURN = 10;
  static const unsigned char BLOCK_ALL_LENGTH = BLOCK_CREATE_NUM * (BLOCK_WIDTH + BLOCK_CREATE_INTERVAL_POS_W);
  static const unsigned char BLOCK_CREATE_START_POS_X = (WIDTH / 2) - (BLOCK_ALL_LENGTH / 2);

  static const unsigned char PHASE_INIT = 0;
  static const unsigned char PHASE_TURN_START = 1;
  static const unsigned char PHASE_BLOCK_MOVE = 2;
  static const unsigned char PHASE_READY = 3;
  static const unsigned char PHASE_MAIN = 4;
  static const unsigned char PHASE_END = 5;
  static const unsigned char PHASE_GAMEOVER = 6;

  static const unsigned char CURSOR_SIZE = 10;
  static const unsigned char CURSOR_START_POS_X = WIDTH / 2;
  static const unsigned char CURSOR_START_POS_Y = HEIGHT - 2;
  static const unsigned char CURSOR_ANGLE_MIN = 20;
  static const unsigned char CURSOR_ANGLE_MAX = 180 - CURSOR_ANGLE_MIN;
  static const unsigned char CURSOR_NEXT_POS_SPEED = 3;

  static const unsigned char FIELD_WIDTH = (BLOCK_WIDTH + BLOCK_CREATE_INTERVAL_POS_W) * BLOCK_CREATE_NUM;
  static const unsigned char FIELD_HEIGHT = HEIGHT;
  static const unsigned char FIELD_START_X = (WIDTH / 2) - (FIELD_WIDTH / 2);
  static const unsigned char FIELD_START_Y = (WIDTH / 2) - (FIELD_WIDTH / 2);
  static const unsigned char FIELD_TOP = 0;
  static const unsigned char FIELD_DOWN = FIELD_HEIGHT;
  static const unsigned char FIELD_RIGHT = FIELD_START_X + FIELD_WIDTH;
  static const unsigned char FIELD_LEFT = FIELD_START_X;

  static void DrawNumber(Arduboy *arduboy, int number, int x, int y, uint8_t color)
  {
    switch (number) {
    case 0:
      arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
      arduboy->drawLine(x + 0, y + 6, x + 2, y + 6, color);
      arduboy->drawLine(x + 0, y + 0, x + 0, y + 6, color);
      arduboy->drawLine(x + 2, y + 0, x + 2, y + 6, color);
      break;
    case 1:
      arduboy->drawLine(x + 1, y + 0, x + 1, y + 6, color);
      break;
    case 2:
      arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
      arduboy->drawLine(x + 0, y + 3, x + 2, y + 3, color);
      arduboy->drawLine(x + 0, y + 6, x + 2, y + 6, color);
      arduboy->drawLine(x + 0, y + 4, x + 0, y + 5, color);
      arduboy->drawLine(x + 2, y + 1, x + 2, y + 2, color);
      break;

    case 3:
      arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
      arduboy->drawLine(x + 0, y + 3, x + 2, y + 3, color);
      arduboy->drawLine(x + 0, y + 6, x + 2, y + 6, color);
      arduboy->drawLine(x + 2, y + 0, x + 2, y + 6, color);
      break;

    case 4:
      arduboy->drawLine(x + 0, y + 3, x + 2, y + 3, color);
      arduboy->drawLine(x + 0, y + 0, x + 0, y + 3, color);
      arduboy->drawLine(x + 2, y + 0, x + 2, y + 6, color);
      break;
    
    case 5:
      arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
      arduboy->drawLine(x + 0, y + 3, x + 2, y + 3, color);
      arduboy->drawLine(x + 0, y + 6, x + 2, y + 6, color);
      arduboy->drawLine(x + 0, y + 1, x + 0, y + 2, color);
      arduboy->drawLine(x + 2, y + 4, x + 2, y + 5, color);
      break;

    case 6:
      arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
      arduboy->drawLine(x + 0, y + 3, x + 2, y + 3, color);
      arduboy->drawLine(x + 0, y + 6, x + 2, y + 6, color);
      arduboy->drawLine(x + 0, y + 0, x + 0, y + 6, color);
      arduboy->drawLine(x + 2, y + 4, x + 2, y + 5, color);
      break;

    case 7:
      arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
      arduboy->drawLine(x + 0, y + 1, x + 0, y + 2, color);
      arduboy->drawLine(x + 2, y + 0, x + 2, y + 6, color);

      break;
    
    case 8:
      arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
      arduboy->drawLine(x + 0, y + 3, x + 2, y + 3, color);
      arduboy->drawLine(x + 0, y + 6, x + 2, y + 6, color);
      arduboy->drawLine(x + 0, y + 0, x + 0, y + 6, color);
      arduboy->drawLine(x + 2, y + 0, x + 2, y + 6, color);
      break;

    case 9:
      arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
      arduboy->drawLine(x + 0, y + 3, x + 2, y + 3, color);
      arduboy->drawLine(x + 0, y + 6, x + 2, y + 6, color);
      arduboy->drawLine(x + 0, y + 1, x + 0, y + 2, color);
      arduboy->drawLine(x + 2, y + 0, x + 2, y + 6, color);
    
      break;
    default:
      break;
    }
  }

  static void DrawQuestionMark(Arduboy *arduboy, int x, int y, uint8_t color)
  {
    arduboy->drawLine(x + 0, y + 0, x + 2, y + 0, color);
    arduboy->drawLine(x + 0, y + 1, x + 0, y + 1, color);
    arduboy->drawLine(x + 1, y + 3, x + 1, y + 4, color);
    arduboy->drawLine(x + 1, y + 6, x + 1, y + 6, color);
    arduboy->drawLine(x + 2, y + 1, x + 2, y + 2, color);
  }

  GameManager();
};
#endif    //INC_GAMEMANAGER
