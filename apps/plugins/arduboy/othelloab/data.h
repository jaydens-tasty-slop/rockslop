#ifndef __DATA_H__
#define __DATA_H__

#ifndef countof
# define countof(A) (sizeof(A) / sizeof(*(A)))
#endif /* countof */

extern bool debug;

enum GameStates {
  GS_MENU,
  GS_MAIN,
  GS_PAUSED,
  GS_WIN,
  GS_LOSE,
  GS_DRAW
};

enum GridStates {
  GS_EMPTY,
  GS_WHITE,
  GS_BLACK
};

GridStates opponent(GridStates s);

struct Board {
  unsigned char fill[8];
  unsigned char color[8];

  Board();

  Board &init(void);
  Board &clear(void);
  GridStates get(unsigned char x, unsigned char y);
  Board &set(unsigned char x, unsigned char y, GridStates s);
  unsigned char count(unsigned char &c0, unsigned char &c1);
};

struct Vec2i {
  unsigned char x : 3;
  unsigned char y : 3;

  Vec2i();
  Vec2i(unsigned char _x, unsigned char _y);
};

struct Operation {
  /**< Cursor. */
  unsigned char x : 3;
  unsigned char y : 3;
  /**< Arrow key states. */
  bool px : 1;
  bool py : 1;
  /**< A/B key states. */
  bool pa : 1;
  bool pb : 1;
  /**< Counts */
  unsigned char c0 : 6;
  unsigned char c1 : 6;
  unsigned long won;
  unsigned long total;
  /**< Side state. */
  unsigned char chosen : 1;
  unsigned char playing : 1;
  /**< Game state */
  GameStates state : 3;

  Operation();

  Operation &clear(void);
  Operation &reset(void);
  GridStates currentSide(void);
  GridStates opponentSide(void);
};

void load(Operation* o);
void save(Operation* o);

#endif /* __DATA_H__ */
