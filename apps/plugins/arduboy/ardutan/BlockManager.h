#ifndef _INC_BLOCK_MANAGER    //まだ読み込まれていなければ以下の処理をする
#define _INC_BLOCK_MANAGER    //すでに読み込まれているという目印をつける

#include <Arduboy.h>
#include "GameManager.h"
#include "Block.h"
#include "BallManager.h"

class BlockManager
{
 private:
  Arduboy *arduboy;
  Block* blocks[GameManager::MAX_BLOCK_COUNT];
  unsigned int block_count;
  bool screen_out_flag;
 public:
  BlockManager(Arduboy *_arduboy);
  void init();
  void createBlocks(int turn);
  void setNextPosBlocks();
  bool isScreenOut();
  bool moveBlocks();
  void hit(BallManager *ball_mng);
  void drawBlocks();
};
#endif    //INC_BLOCK_MANAGER
