#include "Arduboy.h"

typedef enum {
    MAN_ST_NONE,
    MAN_ST_STAND,
    MAN_ST_MOVE,
    MAN_ST_ATTACK,
    MAN_ST_DAMAGED,
    MAN_ST_MAX
} eManState;

typedef enum {
  DIR_LEFT,
  DIR_RIGHT
} eDir;

class Player {
 public:
    eManState state;
    eDir dir;

    int x;
    int y;
    int count;

    int hp;	// hit point
    int ap;	// attack point
    int dp;	// defend point

    Player(int _x, int _y) {
        state = MAN_ST_STAND;
        dir = DIR_RIGHT;

        x = _x;
        y = _y;
        count = 0;

        hp = 5;
        ap = 4;
        dp = 2;
    };
    ~Player() {};

    void do_state();

    bool is_movable() {
        if (state == MAN_ST_STAND || state == MAN_ST_MOVE) {
            return (true);
        } else {
            return (false);
        }
    };

    void move(int dx);
    void draw(uint16_t anim);
};
