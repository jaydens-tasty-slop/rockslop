#include <stdlib.h>
#include <math.h>
#include <Arduboy.h>
#include "../lib/ab_math.h"
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "Ball.h"
#include "BallManager.h"
#include "Block.h"
#include "BlockManager.h"
#include "Button.h"
#include "GameManager.h"

Arduboy arduboy;

unsigned char phase;
BallManager* ball_mng;
Button* a_btn;
Button* b_btn;
Button* left_btn;
Button* right_btn;
BlockManager* block_mng;
int cursor_angle;
float cursor_x;
int cursor_next_turn;
float cursor_next_pos_x;
float character_pos_x;
int turn;
float cursor_move_factor;
bool first_ball_out_flag;

void before();
void after();
void viewCharacter(uint8_t c);

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);

  // ボタン制御インスタンス生成
  a_btn = new Button(&arduboy, A_BUTTON);
  b_btn = new Button(&arduboy, B_BUTTON);
  left_btn = new Button(&arduboy, LEFT_BUTTON);
  right_btn = new Button(&arduboy, RIGHT_BUTTON);

  ball_mng = new BallManager(&arduboy);
  block_mng = new BlockManager(&arduboy);

  // フェーズ初期化
  phase = GameManager::PHASE_INIT;
}

void loop() {
  if (!arduboy.nextFrame()) return;
  before();
  arduboy.clear();

  switch (phase) {
  case GameManager::PHASE_INIT:
    randomSeed(analogRead(0));
    cursor_x = GameManager::CURSOR_START_POS_X;
    cursor_angle = 30;
    turn = 0;
    cursor_move_factor = 1;
    cursor_next_turn = 0;
    cursor_next_pos_x = cursor_x;
    character_pos_x = cursor_x;
  
    block_mng->init();
    phase = GameManager::PHASE_TURN_START;
    break;
    
  case GameManager::PHASE_TURN_START:
    first_ball_out_flag = false;
    cursor_x = cursor_next_pos_x;
    turn++;
    ball_mng->init(turn);

    block_mng->createBlocks(turn);
    block_mng->setNextPosBlocks();
    if (block_mng->isScreenOut()) {
      phase = GameManager::PHASE_GAMEOVER;
    } else {
      phase = GameManager::PHASE_BLOCK_MOVE;
    }
    break;
    
  case GameManager::PHASE_BLOCK_MOVE:
    if (block_mng->moveBlocks()) {
      // 移動中
    } else {
      phase = GameManager::PHASE_READY;
    }
    break;
    
  case GameManager::PHASE_READY:
    if (a_btn->isPress()) {
      cursor_move_factor = 1;
    } else {
      cursor_move_factor = 3;
    }
    if (left_btn->isPress()) {
      cursor_angle += cursor_move_factor;
      if (cursor_angle > GameManager::CURSOR_ANGLE_MAX) cursor_angle = GameManager::CURSOR_ANGLE_MAX;
    }
    if (right_btn->isPress()) {
      cursor_angle -= cursor_move_factor;
      if (cursor_angle < GameManager::CURSOR_ANGLE_MIN) cursor_angle = GameManager::CURSOR_ANGLE_MIN;
    }
    // キャラクタの位置をカーソルに合わせる
    character_pos_x = cursor_x;
    
    {
      // カーソル描画
      double radian = cursor_angle * M_PI / 180.0;      
      float tmp_x = (float)(GameManager::CURSOR_SIZE * cos(radian));
      float tmp_y = (float)(GameManager::CURSOR_SIZE * sin(radian)) * -1;  // ゲームスクリーンでは左上が原点なので（計算上は左下が原点）、Y座標を反転する
      arduboy.drawLine(cursor_x, GameManager::CURSOR_START_POS_Y, cursor_x + tmp_x, GameManager::CURSOR_START_POS_Y + tmp_y, 1);

      if (b_btn->isRelease()) {
        // 移動量をセット
        ball_mng->setBallSpeed(tmp_x, tmp_y);
        phase = GameManager::PHASE_MAIN;
      }
    }
    break;
    
  case GameManager::PHASE_MAIN:
    ball_mng->addBall(cursor_x, GameManager::CURSOR_START_POS_Y);
    // ボールの移動＆描画
    {
      bool all_out_flag = ball_mng->moveBalls();
      if (first_ball_out_flag) {
      } else {
        if (ball_mng->getFirstOutBall() != NULL) {
          first_ball_out_flag = true;
          cursor_next_pos_x = ball_mng->getFirstOutBall()->getX();
          if (cursor_x > cursor_next_pos_x) {
            cursor_next_turn = -1;
          } else if (cursor_x < cursor_next_pos_x) {
            cursor_next_turn = 1;
          } else {
            cursor_next_turn = 0;
          }
        }
      }

      // ブロックとボールの当たり判定
      block_mng->hit(ball_mng);
      ball_mng->drawBalls();

      // MAINフェーズ終了判定
      // ボールを全て発射し、ボールが全て消えて、カーソルの移動も完了していたら終了
      if (ball_mng->finishAddBall() && all_out_flag && cursor_next_turn == 0) {
        phase = GameManager::PHASE_END;
      }
    }
    break;

  case GameManager::PHASE_END:
    // 一先ず開始用フェーズに移行するのみ
    phase = GameManager::PHASE_TURN_START;
    break;

  case GameManager::PHASE_GAMEOVER:
    arduboy.setTextSize(2);
    arduboy.setCursor((WIDTH / 2) - ((12 * 9) / 2), (HEIGHT / 2) - 4);
    arduboy.print("GAME OVER");
    arduboy.setTextSize(1);
    if (b_btn->isRelease()) {
      phase = GameManager::PHASE_INIT;
    }
    break;
    
  default:
    break;
  }

  // キャラクタのみ次の発射開始位置に移動させる
  if (ball_mng->getFirstOutBall() != NULL) {
    if (cursor_next_turn > 0) {
      character_pos_x += GameManager::CURSOR_NEXT_POS_SPEED;
      if (character_pos_x > cursor_next_pos_x) {
        character_pos_x = cursor_next_pos_x;
        cursor_next_turn = 0;
      }
    } else if (cursor_next_turn < 0) {
      character_pos_x -= GameManager::CURSOR_NEXT_POS_SPEED;
      if (character_pos_x < cursor_next_pos_x) {
        character_pos_x = cursor_next_pos_x;
        cursor_next_turn = 0;
      }
    } else {
    }
  }

  // 枠描画
  arduboy.drawLine(GameManager::FIELD_LEFT, GameManager::FIELD_TOP, GameManager::FIELD_LEFT, GameManager::FIELD_DOWN, WHITE);
  arduboy.drawLine(GameManager::FIELD_RIGHT, GameManager::FIELD_TOP, GameManager::FIELD_RIGHT, GameManager::FIELD_DOWN, WHITE);
  
  // ブロックの描画
  block_mng->drawBlocks();

  // 進行度表示
  arduboy.setCursor((WIDTH / 2) - (2 * (turn >= 100 ? 3 : turn >= 10 ? 2 : 1)), 2);
  arduboy.print(turn);
  // キャラクタ表示
  uint8_t character_no = 1;
  if (phase == GameManager::PHASE_MAIN) {
    character_no = 2;
  }
  viewCharacter(character_no);

  // 残りボール数表示
  if (phase != GameManager::PHASE_READY) {
    ball_mng->viewBallCount(cursor_x + 4, GameManager::CURSOR_START_POS_Y - 8);
  }

  after();
  arduboy.display();
}

void before()
{
  a_btn->begin();
  b_btn->begin();
  left_btn->begin();
  right_btn->begin();
}

void after()
{
  a_btn->commit();
  b_btn->commit();
  left_btn->commit();
  right_btn->commit();
}

void viewCharacter(uint8_t c)
{
  arduboy.drawChar(character_pos_x - 2, GameManager::CURSOR_START_POS_Y - 6, c, WHITE, BLACK, 1);
}

