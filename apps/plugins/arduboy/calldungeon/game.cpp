#include "game.h"

#include "bmp_bg.h"
#include "bmp_obj.h"
#include "bmp_enemy.h"

extern Arduboy mArduboy;

#define monsterParam(hp) ((uint16_t)hp << 12)

tObject garRoom1Obj[] = {
  { OBJ_BG, 0, 8 * 1, 8, 0 },	// Candle
  { OBJ_BG, 0, 8 * 5, 8, 0 },	// Candle
  { OBJ_GATE, 1, 8 * 9 - 6, 12, ((4 << 8) | 0) },	// Gate
  { OBJ_FG, 0, 8 * 3, 4, 0 },	// Column
  { OBJ_FG, 0, 8 * 7, 4, 0 },	// Column
  { OBJ_MAX, 0, 0, 0, 0 }
};

tObject garRoom2Obj[] = {
  { OBJ_BG, 0, 8 * 1, 8, 0 },	// Candle
  { OBJ_BG, 0, 8 * 5, 8, 0 },	// Candle
  { OBJ_ITEM, 0, 8 * 8 + 2, 12, 0 },	// Tresure box
  { OBJ_MONSTER, 0, 8 * 5, 12, monsterParam(2) },	// Monster
  { OBJ_FG, 0, 8 * 3, 4, 0 },	// Column
  { OBJ_FG, 0, 8 * 7, 4, 0 },	// Column
  { OBJ_FG, 2, 8 * 10, 0, 0 },	// Wall (Right side)
  { OBJ_MAX, 0, 0, 0, 0 }
};

tObject garRoom3Obj[] = {
  // { OBJ_BG, 0, 8 * 1 + 4, 8, 0 },	// Candle
  { OBJ_BG, 0, 8 * 5, 8, 0 },	// Candle
  { OBJ_GATE, 5, 8 * 9 - 6, 12, 0 },	// Gate
  { OBJ_GATE, 2, 6, 12, ((6 << 8) | 1) },
  { OBJ_MONSTER, 0, 8 * 5, 12, monsterParam(2) },	// Monster
  { OBJ_FG, 0, 8 * 3, 4, 0 },	// Column
  { OBJ_FG, 0, 8 * 7, 4, 0 },	// Column
  { OBJ_FG, 1, 0, 0, 0 },		// Wall (Left side)
  { OBJ_MAX, 0, 0, 0, 0 }
};

tObject garRoom4Obj[] = {
  { OBJ_BG, 0, 8 * 5, 8, 0 },	// Candle
  // { OBJ_BG, 0, 8 * 9, 8, 0 },	// Candle
  { OBJ_GATE, 6, 6, 12, 0 },
  { OBJ_ITEM, 0, 8 * 8 + 2, 12, 0 },	// Tresure box
  { OBJ_FG, 0, 8 * 3, 4, 0 },	// Column
  { OBJ_FG, 0, 8 * 7, 4, 0 },	// Column
  { OBJ_FG, 1, 0, 0, 0 },		// Wall (Left side)
  { OBJ_FG, 2, 8 * 10, 0, 0 },	// Wall (Right side)
  { OBJ_MAX, 0, 0, 0, 0 }
};

tObject garRoom5Obj[] = {
  { OBJ_BG, 0, 8 * 1, 8, 0 },	// Candle
  { OBJ_BG, 0, 8 * 3, 8, 0 },	// Candle
  { OBJ_BG, 0, 8 * 5, 8, 0 },	// Candle
  { OBJ_BG, 0, 8 * 7, 8, 0 },	// Candle
  { OBJ_BG, 0, 8 * 9, 8, 0 },	// Candle
  { OBJ_MAX, 0, 0, 0, 0 }
};

tObject garRoom6Obj[] = {
  //{ OBJ_BG, 0, 8 * 1, 8, 0 },	// Candle
  //{ OBJ_BG, 0, 8 * 5, 8, 0 },	// Candle
  //{ OBJ_ITEM, 0, 8 * 8 + 2, 12, 0 },	// Tresure box
  { OBJ_MONSTER, 0, 8 * 8, 12, monsterParam(5) },	// Monster
  { OBJ_FG, 4, 8 * 10, 0, 0 },	// Wall with Gate (Right side)
  { OBJ_MAX, 0, 0, 0, 0 }
};

//=============================================================================
#define ROOM_MIN_X 0
#define ROOM_MIN_WALL_X (4 * 4)
#define ROOM_MAX_X (8 * 9 * 4)
#define ROOM_MAX_WALL_X (ROOM_MAX_X - 4 * 4)
tRoom garRoom[] = {
  {	// 0
	-1, 1, { 2, -1 },
	garRoom1Obj,
	ROOM_MIN_X, ROOM_MAX_X
  },
  {	// 1
	0, -1, { -1, -1 },
	garRoom2Obj,
	ROOM_MIN_X, ROOM_MAX_WALL_X
  },
  {	// 2
	-1, 4, { 0, 3 },
	garRoom3Obj,
	ROOM_MIN_WALL_X, ROOM_MAX_X
  },
  {	// 3
	-1, -1, { 2, -1 },
	garRoom4Obj,
	ROOM_MIN_WALL_X, ROOM_MAX_WALL_X
  },
  {	// 4
	2, 5, { -1, -1 },
	garRoom5Obj,
	ROOM_MIN_X, ROOM_MAX_X
  },
  {	// 5
	4, -1, { -1, -1 },
	garRoom6Obj,
	ROOM_MIN_X, ROOM_MAX_WALL_X
  },
};

//=============================================================================
#define NUM_TEMP_OBJ 4
tObject garTempObj[NUM_TEMP_OBJ];

//=============================================================================
static const int sawConvDec[5] = {
  1, 10, 100, 1000, 10000
};	// 16bit なので、高々 5 桁まで
void STR_ConvDec2Str(int wVal, int wKeta, char *pcStr)
{
  int wDiv, wChar;

  for (wKeta --; wKeta >= 0; wKeta --) {
	wDiv = sawConvDec[wKeta];
	wChar = wVal / wDiv;
	wVal -= wDiv * wChar;
	if (wChar > 9) wChar %= 10;
	*pcStr ++ = '0' + wChar;
  }
  *pcStr = '\0';
}

//=============================================================================
Game::Game()
{
  int cnt;

  mState = GAME_ST_TITLE;
  mCounter = 0;
  mInv = 0;
  mAnim = 0;

  prCur_room = NULL;
  prNext_room = &garRoom[0];
  wNext_x = 0;

  pcMsg = "Let's Go!";

  mPlayer = new Player(-1, 16 - 4);

  for (cnt = 0; cnt < NUM_TEMP_OBJ; cnt ++) {
    memset(&garTempObj[cnt], 0, sizeof(tObject));
  }
}

//=============================================================================
Game::~Game()
{
}

//=============================================================================
// Check Input
//=============================================================================
void Game::check_input()
{
  static bool fReady = true;

  if (mState == GAME_ST_PLAY) {
    // ---------------------------------------------------------- Up Button ---
    if (mArduboy.pressed(UP_BUTTON)) {
      tObject *prObj;

      prObj = prCur_room->prObjs;

      while (prObj != NULL && prObj->nType != OBJ_MAX) {
	if (prObj->nType == OBJ_GATE) {
	  if ((mPlayer->x / 4) >= (prObj->wX - 4)
	      && (mPlayer->x / 4) < (prObj->wX + 4)) {
	    switch (prObj->ubNo) {
	    case 0:	// Door
	    case 3:
	      prObj->ubNo = prObj->uwParam >> 8;
	      set_message("Open the door");
	      break;
	    case 1:	// Door (Locked)
	      if ((mInv & 1) == 0) {
		set_message("Door is locked...");
	      } else {
		mInv &= ~1;

		prObj->ubNo = prObj->uwParam >> 8;
		set_message("Open the door");
	      }
	      break;
	    case 2:	// Door (Secret)
	      prObj->ubNo = 3;
	      set_message("Find secret door!");
	      break;

	    case 4:
	    case 5:
	    case 6:
	      set_next_scene(&garRoom[prCur_room->awOther_room[prObj->uwParam & 0xff]],
			     mPlayer->x);
	      break;
	    default:
	      break;
	    }
	  }
	}
	else if (prObj->nType == OBJ_ITEM) {
	  if ((mPlayer->x / 4) >= (prObj->wX - 4)
	      && (mPlayer->x / 4) < (prObj->wX + 4)) {
	    switch (prObj->ubNo) {
	    case 0:
	      set_message("Take a key!!");
	      prObj->ubNo = 1;

	      mInv |= 1;
	      break;

	    case 1:
	      set_message("Nothing to find...");
	      break;
	    }
	  }
	}

	prObj ++;
      }
    }

    // -------------------------------------------------------- Left Button ---
    else if (mArduboy.pressed(LEFT_BUTTON)) {
      if (mPlayer->is_movable()) {
	if (mPlayer->x > prCur_room->wMin_x) {
	  mPlayer->x --;
	} else {
	  if (prCur_room->wLeft_room >= 0) {
	    tRoom *prRoom;

	    prRoom = &garRoom[prCur_room->wLeft_room];
	    set_next_scene(prRoom, prRoom->wMax_x);
	  }
	}
	mPlayer->state = MAN_ST_MOVE;
	mPlayer->dir = DIR_LEFT;
      }
    }
    // ------------------------------------------------------- Right Button ---
    else if (mArduboy.pressed(RIGHT_BUTTON)) {
      if (mPlayer->is_movable()) {
	if (mPlayer->x <= prCur_room->wMax_x) {
	  mPlayer->x ++;
	} else {
	  if (prCur_room->wRight_room >= 0) {
	    tRoom *prRoom;

	    prRoom = &garRoom[prCur_room->wRight_room];
	    set_next_scene(prRoom, prRoom->wMin_x);
	  }
	}
	mPlayer->state = MAN_ST_MOVE;
	mPlayer->dir = DIR_RIGHT;
      }
    }
    else if (mPlayer->state == MAN_ST_MOVE) {
      mPlayer->state = MAN_ST_STAND;
    }
  }

  // ------------------------------------------------------------- A Button ---
  if (mArduboy.pressed(A_BUTTON)) {
    if (fReady) {
      switch (mState) {
      case GAME_ST_TITLE:
	set_next_scene(&garRoom[0], 0);
	break;
      case GAME_ST_PLAY:
	if (mPlayer->is_movable()) {
	  mPlayer->state = MAN_ST_ATTACK;
	  mPlayer->count = 0;
	}
	break;
      case GAME_ST_WAIT_MSG:
	next_message();
	break;
      default:
	break;
      }
    }
    fReady = false;
  } else {
    fReady = true;
  }
}

//=============================================================================
// Change State
//=============================================================================
void Game::do_state()
{
  tObject *prObj;
  tMonsterParam *prParam;

  switch (mState) {
  case GAME_ST_PLAY:
    // --------------------------------------------------------- Play State ---
    mPlayer->do_state();
    switch (mPlayer->state) {
    case MAN_ST_STAND:
    case MAN_ST_MOVE:
      for (prObj = prCur_room->prObjs;
	   prObj != NULL && prObj->nType != OBJ_MAX; prObj ++) {
	if (prObj->nType == OBJ_MONSTER) {
	  prParam = (tMonsterParam *)&prObj->uwParam;
	  if (prParam->ubHP > 0
	      && prObj->wX >= ((mPlayer->x / 4) - 8)
	      && prObj->wX < ((mPlayer->x / 4) + 8)) {
	    // Attack from Monster to Player
	    if (prObj->wX < (mPlayer->x / 4)) {
	      mPlayer->x += 8 * 4;
	    } else {
	      mPlayer->x -= 8 * 4;
	    }
	    mPlayer->hp --;

	    mPlayer->state = MAN_ST_DAMAGED;
	    mPlayer->count = 0;

	    garTempObj[1].nType = OBJ_TEMP_FG;
	    garTempObj[1].ubNo = 1;
	    garTempObj[1].wX = mPlayer->x / 4;
	    garTempObj[1].wY = mPlayer->y - 4;
	    garTempObj[1].uwParam = 30;

	    mArduboy.tunes.tone(400, 40);
	    mArduboy.setRGBled(0, 0, 128);

	    break;
	  }
	}
      }
      break;

    case MAN_ST_ATTACK:
      for (prObj = prCur_room->prObjs;
	   prObj != NULL && prObj->nType != OBJ_MAX; prObj ++) {
	if (prObj->nType == OBJ_MONSTER) {
	  prParam = (tMonsterParam *)&prObj->uwParam;
	  if (prParam->ubHP > 0 && prParam->ubCnt == 0
	      && ((mPlayer->dir == DIR_RIGHT
		   && (mPlayer->x / 4) >= (prObj->wX - 16 - 4)
		   && (mPlayer->x / 4) < (prObj->wX - 16 + 8))
		  || (mPlayer->dir == DIR_LEFT
		      && (mPlayer->x / 4) >= (prObj->wX + 16 - 8)
		      && (mPlayer->x / 4) < (prObj->wX + 16 + 4)))) {
	    prParam->ubCnt = 30;
	    if (random(0, 2) == 0) {
	      // HIT
	      if (mPlayer->dir == DIR_RIGHT) {
		prObj->wX += 6;
		if (prObj->wX >= (prCur_room->wMax_x / 4)) {
		  prObj->wX -= 12;
		}
	      } else {
		prObj->wX -= 6;
		if (prObj->wX < (prCur_room->wMin_x / 4)) {
		  prObj->wX += 12;
		}
	      }

	      garTempObj[0].nType = OBJ_TEMP_FG;
	      garTempObj[0].ubNo = 1;
	      garTempObj[0].wX = prObj->wX;
	      garTempObj[0].wY = prObj->wY - 2;
	      garTempObj[0].uwParam = 30;

	      prParam->ubHP --;
	      prParam->ubState = 1;
	      if (prParam->ubHP == 0) {
		prParam->ubCnt += 30;
	      }

	      mArduboy.tunes.tone(400, 40);
	      mArduboy.setRGBled(128, 0, 0);
	    } else {
	      // MISS
	      garTempObj[0].nType = OBJ_TEMP_FG;
	      garTempObj[0].ubNo = 0;
	      garTempObj[0].wX = prObj->wX;
	      garTempObj[0].wY = prObj->wY - 2;
	      garTempObj[0].uwParam = 30;

	      mArduboy.tunes.tone(1800, 20);
	    }
	    break;
	  }
	}
      }
      break;

    default:
      break;
    }
    break;

  case GAME_ST_CHANGE_SCENE:
    mCounter ++;
    if (mCounter == 32) {
      prCur_room = prNext_room;
      prNext_room = NULL;
      mPlayer->x = wNext_x;
    } else if (mCounter >= (64 + 16)) {
      if (pcMsg == NULL) {
	mState = GAME_ST_PLAY;
      } else {
	mState = GAME_ST_WAIT_MSG;
      }
      mCounter = 0;
    }
    break;

  default:
    break;
  }
}

//=============================================================================
void Game::draw_back(void)
{
  int cnt;

  for (cnt = 0; cnt < 11; cnt ++) {
    mArduboy.drawBitmap(cnt * 8, 0, ceil_bmp[0], 8, 16, WHITE);

    mArduboy.drawBitmap(cnt * 8, 24, ground_bmp, 8, 8, WHITE);
  }
}

//=============================================================================
tObject *Game::draw_bg_object(tObject *prObj)
{
  while (prObj->nType == OBJ_BG) {
    switch (prObj->ubNo) {
    case 0:
      // Candle
      if ((mAnim & 8) == 0) {
	mArduboy.drawBitmap(prObj->wX, prObj->wY, candle_bmp[0], 8, 8, WHITE);
	mArduboy.drawBitmap(prObj->wX, prObj->wY, candle_bmp[1], 8, 8, BLACK);
      } else {
	mArduboy.drawBitmap(prObj->wX, prObj->wY, candle_bmp[2], 8, 8, WHITE);
	mArduboy.drawBitmap(prObj->wX, prObj->wY, candle_bmp[3], 8, 8, BLACK);
      }
      break;

    default:
      break;
    }

    prObj ++;
  }

  return (prObj);
}

//=============================================================================
tObject *Game::draw_gate(tObject *prObj)
{
  while (prObj->nType == OBJ_GATE) {
    switch (prObj->ubNo) {
    case 0:
      // Door
      mArduboy.fillRect(prObj->wX, prObj->wY, 16, 16, BLACK);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, door_bmp[0], 16, 16, WHITE);
      break;

    case 1:
      // Door (Locked)
      mArduboy.fillRect(prObj->wX, prObj->wY, 16, 16, BLACK);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, door_bmp[1], 16, 16, WHITE);
      break;

    case 3:
      // Door (Secret)
      mArduboy.fillRect(prObj->wX, prObj->wY, 16, 16, BLACK);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, door_bmp[2], 16, 16, WHITE);
      break;

    case 4:
      // Door (Up Stair)
      mArduboy.fillRect(prObj->wX, prObj->wY, 16, 16, BLACK);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, door_bmp[4], 16, 16, WHITE);
      break;

    case 5:
      // Door (Down Stair)
      mArduboy.fillRect(prObj->wX, prObj->wY, 16, 16, BLACK);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, door_bmp[5], 16, 16, WHITE);
      break;

    case 6:
      // Door (Room)
      mArduboy.fillRect(prObj->wX, prObj->wY, 16, 16, BLACK);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, door_bmp[3], 16, 16, WHITE);
      break;

    default:
      break;
    }

    prObj ++;
  }

  return (prObj);
}

//=============================================================================
tObject *Game::draw_item(tObject *prObj)
{
  while (prObj->nType == OBJ_ITEM) {
    switch (prObj->ubNo) {
    case 0:
      mArduboy.drawBitmap(prObj->wX, prObj->wY, tbox_bmp[0], 16, 16, WHITE);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, tbox_bmp[1], 16, 16, BLACK);
      break;

    case 1:
      mArduboy.drawBitmap(prObj->wX, prObj->wY, tbox_bmp[2], 16, 16, WHITE);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, tbox_bmp[3], 16, 16, BLACK);
      break;

    default:
      break;
    }

    prObj ++;
  }

  return (prObj);
}

//=============================================================================
tObject *Game::draw_monster(tObject *prObj)
{
  tMonsterParam *prParam;

  while (prObj->nType == OBJ_MONSTER) {
    prParam = (tMonsterParam *)&prObj->uwParam;

    switch (prObj->ubNo) {
    case 0:
      if ((prParam->ubState & 2) == 0
	  && ((prParam->ubState & 1) == 0 || (mAnim & 8) == 0)) {
	if ((mAnim & 0x38) != 0) {
	  mArduboy.drawBitmap(prObj->wX, prObj->wY, slime_bmp[0], 16, 16, WHITE);
	  mArduboy.drawBitmap(prObj->wX, prObj->wY, slime_bmp[1], 16, 16, BLACK);
	} else {
	  mArduboy.drawBitmap(prObj->wX, prObj->wY, slime_bmp[2], 16, 16, WHITE);
	  mArduboy.drawBitmap(prObj->wX, prObj->wY, slime_bmp[3], 16, 16, BLACK);
	}
      }
      break;

    default:
      break;
    }

    /***/
    if (prParam->ubCnt > 0) {
      prParam->ubCnt --;
      if (prParam->ubCnt == 0) {
	if (prParam->ubHP == 0) {
	  // Defeated
	  prParam->ubState |= 2;
	} else {
	  prParam->ubState = 0;
	}
      }
    }

#if 0
    {
      char acWork[8];

      STR_ConvDec2Str(prParam->ubState, 2, acWork);
      mArduboy.setCursor(0, 32);
      mArduboy.print(acWork);
      STR_ConvDec2Str(prParam->ubCnt, 2, acWork);
      mArduboy.setCursor(0, 40);
      mArduboy.print(acWork);
      STR_ConvDec2Str(prParam->ubHP, 2, acWork);
      mArduboy.setCursor(0, 48);
      mArduboy.print(acWork);
    }
#endif

    prObj ++;
  }

  return (prObj);
}

//=============================================================================
tObject *Game::draw_fg_object(tObject *prObj)
{
  while (prObj->nType == OBJ_FG) {
    switch (prObj->ubNo) {
    case 0: // Column
      mArduboy.drawBitmap(prObj->wX, prObj->wY, column_bmp[0], 8, 24, WHITE);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, column_bmp[1], 8, 24, BLACK);
      break;

    case 1: // Wall (Left side)
      mArduboy.drawBitmap(prObj->wX, prObj->wY, wall_bmp[0], 8, 32, WHITE);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, wall_bmp[1], 8, 32, BLACK);
      break;

    case 2: // Wall (Right side)
      mArduboy.drawBitmap(prObj->wX, prObj->wY, wall_bmp[2], 8, 32, WHITE);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, wall_bmp[3], 8, 32, BLACK);
      break;

    case 3: // Wall with Gate (Left side)
      mArduboy.drawBitmap(prObj->wX, prObj->wY, wall_bmp[4], 8, 32, WHITE);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, wall_bmp[5], 8, 32, BLACK);
      break;

    case 4: // Wall with Gate (Right side)
      mArduboy.drawBitmap(prObj->wX, prObj->wY, wall_bmp[6], 8, 32, WHITE);
      mArduboy.drawBitmap(prObj->wX, prObj->wY, wall_bmp[7], 8, 32, BLACK);
      break;

    default:
      break;
    }

    prObj ++;
  }

  return (prObj);
}

//=============================================================================
void Game::draw_temp_bg(void)
{
  tObject *prObj = garTempObj;
  int wCnt;

  for (wCnt = 0; wCnt < NUM_TEMP_OBJ; wCnt ++) {
    if (prObj->nType == OBJ_TEMP_BG) {
      switch (prObj->ubNo) {
      default:
	break;
      }
    }
    prObj ++;
  }
}

//=============================================================================
void Game::draw_temp_fg(void)
{
  tObject *prObj = garTempObj;
  int wCnt;

  for (wCnt = 0; wCnt < NUM_TEMP_OBJ; wCnt ++) {
    if (prObj->nType == OBJ_TEMP_FG) {
      switch (prObj->ubNo) {
      case 0:
	mArduboy.drawBitmap(prObj->wX, prObj->wY, mini_msg_bmp[2], 16, 8, WHITE);
	mArduboy.drawBitmap(prObj->wX, prObj->wY, mini_msg_bmp[3], 16, 8, BLACK);

	prObj->uwParam --;
	if (prObj->uwParam == 0) {
	  // Delete
	  prObj->nType = OBJ_NONE;
	}
	break;

      case 1:
	mArduboy.drawBitmap(prObj->wX, prObj->wY, mini_msg_bmp[0], 16, 8, WHITE);
	mArduboy.drawBitmap(prObj->wX, prObj->wY, mini_msg_bmp[1], 16, 8, BLACK);

	prObj->uwParam --;
	if (prObj->uwParam == 0) {
	  // Delete
	  prObj->nType = OBJ_NONE;

	  mArduboy.setRGBled(0, 0, 0);
	}
	break;

      default:
	break;
      }
    }

    prObj ++;
  }
}

//=============================================================================
#define PARAM_WIN_X (122 - 33)
#define PARAM_WIN_W (32 + 6)
void Game::draw_param_win(void)
{
  char acWork[6];

  mArduboy.drawLine(PARAM_WIN_X - 1, 0, PARAM_WIN_X - 1, 32, BLACK);
  mArduboy.drawRect(PARAM_WIN_X, 0, PARAM_WIN_W, 32, WHITE);

#if 0
  // Draw Possesed Items
  if ((mInv & 1) != 0) {
    GLCD_DrawBitmap(PARAM_WIN_X + 1, 24, &grItemKey);
  }
  if ((mInv & 2) != 0) {
    GLCD_DrawBitmap(PARAM_WIN_X + 1 + 8, 24, &grItemPotion);
  }
#endif

#if 0
  GLCD_DrawBitmap(PARAM_WIN_X + 1, 1, &grItemHeart);
  GLCD_DrawBitmap(PARAM_WIN_X + 1, 1 + 6, &grItemSword);
  GLCD_DrawBitmap(PARAM_WIN_X + 1 + 15, 1 + 6, &grItemShield);

  GLCD_SetCustomFont((tFont *)&grCustomFont);
  STR_ConvDec2Str(grGame.wHP, 2, acWork);
  GLCD_DrawString(PARAM_WIN_X + 1 + 7, 2, acWork);
  STR_ConvDec2Str(grGame.wAP, 2, acWork);
  GLCD_DrawString(PARAM_WIN_X + 1 + 7, 2 + 6, acWork);
  STR_ConvDec2Str(grGame.wDP, 2, acWork);
  GLCD_DrawString(PARAM_WIN_X + 1 + 15 + 7, 2 + 6, acWork);
#else
  mArduboy.setCursor(PARAM_WIN_X + 2, 2);
  mArduboy.print("HP:");
  mArduboy.print(itoa(mPlayer->hp, acWork, 10));
  mArduboy.setCursor(PARAM_WIN_X + 2, 2 + 8);
  mArduboy.print("AP:");
  mArduboy.print(itoa(mPlayer->ap, acWork, 10));
  mArduboy.setCursor(PARAM_WIN_X + 2, 2 + 16);
  mArduboy.print("DP:");
  mArduboy.print(itoa(mPlayer->dp, acWork, 10));
#endif
}

//=============================================================================
void Game::draw_message(void)
{
  mArduboy.setCursor(0, 32 + 6);
  mArduboy.print(pcMsg);

  if ((mAnim & 32) != 0) {
    mArduboy.fillTriangle(0 + 60, 46 + 4,
			  8 + 60, 46 + 4,
			  4 + 60, 50 + 4, WHITE);
  }

#if 0 // DEBUG
  char work[16];
  mArduboy.setCursor(0, 50);
  mArduboy.print(itoa(sizeof(tObject), work, 10));
  mArduboy.setCursor(64, 50);
  mArduboy.print(itoa(sizeof(tMonsterParam), work, 10));
#endif
}

//=============================================================================
// Draw Frame
//=============================================================================
void Game::draw()
{
  tObject *prObj = NULL;

  draw_back();

  if (prCur_room != NULL) {
    prObj = prCur_room->prObjs;
  }

  if (mState == GAME_ST_PLAY
      || mState == GAME_ST_WAIT_MSG
      || mState == GAME_ST_CHANGE_SCENE) {
    if (prObj != NULL) {
      prObj = draw_bg_object(prObj);
      prObj = draw_gate(prObj);
      prObj = draw_item(prObj);
      prObj = draw_monster(prObj);
      draw_temp_bg();
      mPlayer->draw(mAnim);
      prObj = draw_fg_object(prObj);
      draw_temp_fg();
    }
  }

  draw_param_win();

  switch (mState) {
  case GAME_ST_TITLE:
    // ------------------------------------------------------- Title Screen ---
    mArduboy.drawBitmap(0, 4, column_bmp[0], 8, 24, WHITE);
    mArduboy.drawBitmap(0, 4, column_bmp[1], 8, 24, BLACK);
    mArduboy.drawBitmap(8 * 10, 4, column_bmp[0], 8, 24, WHITE);
    mArduboy.drawBitmap(8 * 10, 4, column_bmp[1], 8, 24, BLACK);

#if 0
    GLCD_SetFont(3);
    GLCD_EraseRect(0, 14 - 1, 8 * 11, 7);
    GLCD_DrawString(1, 14, "-CALL OF DUNGEON-");

    GLCD_SetCustomFont((tFont *)&grCustomFont);
    // GLCD_EraseRect(0, 22 - 1, 8 * 11, 7);
    GLCD_DrawString(14, 22, "PUSH \"A\" BUTTON");
#else
    mArduboy.setCursor(12, 32 + 4);
    mArduboy.print(F("-CALL OF DUNGEON-"));

    mArduboy.setCursor(18, 32 + 10 + 4);
    mArduboy.print(F("PUSH \"A\" BUTTON"));
#endif
    break;

  case GAME_ST_PLAY:
    break;

  case GAME_ST_WAIT_MSG:
    draw_message();
    break;

  case GAME_ST_CHANGE_SCENE:
    if (mCounter < 32) {
      mArduboy.fillRect(0, 0, 122 - 32 - 2, mCounter, WHITE);
    } else if (mCounter >= (32 + 16)) {
      mArduboy.fillRect(0, 0, 122 - 32 - 2, (64 - (mCounter - 16)), WHITE);
    } else {
      mArduboy.fillRect(0, 0, 122 - 32 - 2, 32, WHITE);
    }
    break;

  default:
    break;
  }

  mAnim ++;
}



//=============================================================================
void Game::set_message(char *msg)
{
  mState = GAME_ST_WAIT_MSG;
  pcMsg = msg;

  mArduboy.tunes.tone(600, 40);
}

//=============================================================================
void Game::set_next_scene(tRoom *_prNext_room, int16_t _wNext_x)
{
  prNext_room = _prNext_room;
  wNext_x = _wNext_x;

  if (mState != GAME_ST_WAIT_MSG) {
    mState = GAME_ST_CHANGE_SCENE;
  }
  mCounter = 0;
}

//=============================================================================
void Game::next_message(void)
{
  if (prNext_room == NULL) {
    mState = GAME_ST_PLAY;
  } else {
    mState = GAME_ST_CHANGE_SCENE;
  }
  pcMsg = NULL;
}
