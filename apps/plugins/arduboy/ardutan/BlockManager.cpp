#include <stdlib.h>
#include <math.h>
#include <Arduboy.h>
#include "../lib/ab_math.h"
#include <string.h>
#include "BlockManager.h"
#include "GameManager.h"
#include "Block.h"
#include "Block.h"
#include "BallManager.h"

BlockManager::BlockManager(Arduboy *_arduboy)
{
  arduboy = _arduboy;
  screen_out_flag = false;
  for (int i = 0; i < GameManager::MAX_BLOCK_COUNT; i++) {
    if (blocks[i] == NULL) continue;
    blocks[i] = NULL;
  }
}

void BlockManager::init()
{
  block_count = 0;
  screen_out_flag = false;

  for (int i = 0; i < GameManager::MAX_BLOCK_COUNT; i++) {
    if (blocks[i] == NULL) continue;
    delete(blocks[i]);
    blocks[i] = NULL;
  }
}

void BlockManager::createBlocks(int turn)
{
  // 必ず開けるようにするため、開けるindexをランダムで決定
  char empty_index = random(GameManager::BLOCK_CREATE_NUM);
  bool duble_flag = false;
  for (int i = 0; i < GameManager::BLOCK_CREATE_NUM; i++) {
    if (empty_index == i) continue;

    // 50% + 経過ターン数 - 10%の確率で作成
    char tmp_r = random(100 + 1);
    if (tmp_r >= (50 + turn) - 10) continue;

    char tmp_x = GameManager::BLOCK_CREATE_START_POS_X + (i * (GameManager::BLOCK_WIDTH + GameManager::BLOCK_CREATE_INTERVAL_POS_W));

    int hp = turn;
    bool strong_flag = false;
    // nで割り切れる数の時は、HPを2倍するブロックを1つは出す
    if (turn % GameManager::BLOCK_STRONG_TURN == 0 && !duble_flag) {
      char double_r = random(2);
      // 抽選に当たるか、最後のブロック生成ならHPを2倍に
      if (double_r == 1 || i == GameManager::BLOCK_CREATE_NUM - 1) {
        hp *= 2;
        duble_flag = true;
        strong_flag = true;
      }
    }

    // 空いてるバッファを使う
    for (int j = 0; j < GameManager::MAX_BLOCK_COUNT; j++) {
      if (blocks[j] != NULL) continue;
      blocks[j] = new Block(arduboy, tmp_x, GameManager::BLOCK_CREATE_START_POS_Y, hp, strong_flag);
      blocks[j]->setNextPos(GameManager::BLOCK_HEIGHT);
      break;
    }
  }
}

void BlockManager::setNextPosBlocks()
{
  // ブロックの移動先セット
  for (int i = 0; i < GameManager::MAX_BLOCK_COUNT; i++) {
    if (blocks[i] == NULL) continue;
    Block* block = blocks[i];
    if (block->isActive()) {
      if (block->isMove()) {
        // 移動先セット済みは処理しない
      } else {
        // 移動先が、画面外に出てしまうなら、ゲームオーバー
        if (block->getY() + (GameManager::BLOCK_HEIGHT * 2) > HEIGHT) {
          screen_out_flag = true;
        }
        block->setNextPos(block->getY() + GameManager::BLOCK_HEIGHT + GameManager::BLOCK_CREATE_INTERVAL_POS_H);
      }
    }
  }
}

bool BlockManager::isScreenOut()
{
  return screen_out_flag;
}

// ブロックの移動（１つでも移動中のブロックがあればtrueを返す）
bool BlockManager::moveBlocks()
{
  bool is_move_flag = false;
  for (int i = 0; i < GameManager::MAX_BLOCK_COUNT; i++) {
    if (blocks[i] == NULL) continue;
    Block* block = blocks[i];
    if (block->isActive()) {
      if (block->isMove()) {
        block->moveNextPos();
        is_move_flag = true;
      }
    }
  }
  
  return is_move_flag;
}

void BlockManager::hit(BallManager *ball_mng)
{
  // ブロックとボールの当たり判定
  for (int i = 0; i < GameManager::MAX_BLOCK_COUNT; i++) {
    if (blocks[i] == NULL) continue;
    Block* block = blocks[i];
    if (!block->isActive()) continue;
    ball_mng->isHit(block);
  }
}

void BlockManager::drawBlocks()
{
  // ブロックの描画
  for (int i = 0; i < GameManager::MAX_BLOCK_COUNT; i++) {
    if (blocks[i] == NULL) continue;
    Block* block = blocks[i];
    if (block->isActive()) {
      block->draw();
    } else {
      delete(blocks[i]);
      blocks[i] = NULL;
    }
  }
}

