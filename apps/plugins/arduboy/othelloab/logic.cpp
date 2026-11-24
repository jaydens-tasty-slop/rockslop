#include <stdlib.h>
#include <math.h>
#include "logic.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <limits.h>
#include <stdlib.h>

PROGMEM const static Vec2i LEVEL0[] = {
  Vec2i(3, 3), Vec2i(4, 3), Vec2i(3, 4), Vec2i(4, 4)
};

static Vec2i LEVEL1[] = {
  Vec2i(0, 0), Vec2i(7, 0), Vec2i(7, 7), Vec2i(0, 7)
};

static Vec2i LEVEL2[] = {
  Vec2i(2, 2), Vec2i(3, 2), Vec2i(4, 2),
  Vec2i(5, 2), Vec2i(5, 3), Vec2i(5, 4),
  Vec2i(5, 5), Vec2i(4, 5), Vec2i(3, 5),
  Vec2i(2, 5), Vec2i(2, 4), Vec2i(2, 3)
};

static Vec2i LEVEL3[] = {
  Vec2i(1, 0), Vec2i(2, 0), Vec2i(3, 0), Vec2i(4, 0), Vec2i(5, 0), Vec2i(6, 0),
  Vec2i(7, 1), Vec2i(7, 2), Vec2i(7, 3), Vec2i(7, 4), Vec2i(7, 5), Vec2i(7, 6),
  Vec2i(6, 7), Vec2i(5, 7), Vec2i(4, 7), Vec2i(3, 7), Vec2i(2, 7), Vec2i(1, 7),
  Vec2i(0, 6), Vec2i(0, 5), Vec2i(0, 4), Vec2i(0, 3), Vec2i(0, 2), Vec2i(0, 1)
};

static Vec2i LEVEL4[] = {
  Vec2i(3, 1), Vec2i(4, 1),
  Vec2i(6, 3), Vec2i(6, 4),
  Vec2i(4, 6), Vec2i(3, 6),
  Vec2i(1, 4), Vec2i(1, 3)
};

static Vec2i LEVEL5[] = {
  Vec2i(2, 1), Vec2i(5, 1),
  Vec2i(6, 2), Vec2i(6, 5),
  Vec2i(5, 6), Vec2i(2, 6),
  Vec2i(1, 5), Vec2i(1, 2)
};

static Vec2i LEVEL6[] = {
  Vec2i(1, 1), Vec2i(6, 1), Vec2i(6, 6), Vec2i(1, 6)
};

static void swap(Vec2i &l, Vec2i &r) {
  Vec2i t = l;
  l = r;
  r = t;
}

static bool valid(unsigned char x, unsigned char y) {
  return x <= 7 && y <= 7;
}

static bool valid(char x, char y) {
  return x >= 0 && y >= 0 && x <= 7 && y <= 7;
}

static int score(Board* b, unsigned char x, unsigned char y, GridStates s) {
  int result = 0;
  for (unsigned char i = 0; i < countof(LEVEL0); ++i) {
    if (b->get(LEVEL0[i].x, LEVEL0[i].y) == s)
      result += 5;
  }
  for (unsigned char i = 0; i < countof(LEVEL6); ++i) {
    if (LEVEL6[i].x == x && LEVEL6[i].y == y) {
      result -= 10;
      break;
    }
  }
  for (unsigned char i = 0; i < countof(LEVEL1); ++i) {
    if (LEVEL1[i].x == x && LEVEL1[i].y == y) {
      result += 100;
      break;
    }
  }
  if (x == 0 || x == 7 || y == 0 || y == 7) {
    unsigned char c0 = 0, c1 = 0, c2 = 0, c3 = 0;
    unsigned char o0 = 0, o1 = 0, o2 = 0, o3 = 0;
    for (unsigned char i = 0; i <= 7; ++i) {
      if (y == 0) {
        if (b->get(i, 0) == s) ++c0;
        else if (b->get(i, 0) == opponent(s)) ++o0;
      }
      if (y == 7) {
        if (b->get(i, 7) == s) ++c1;
        else if (b->get(i, 7) == opponent(s)) ++o1;
      }
      if (x == 0) {
        if (b->get(0, i) == s) ++c2;
        else if (b->get(0, i) == opponent(s)) ++o2;
      }
      if (x == 7) {
        if (b->get(7, i) == s) ++c3;
        else if (b->get(7, i) == opponent(s)) ++o3;
      }
    }
    if (c0 == 1 && o0 == 0) result += 12;
    if (c1 == 1 && o1 == 0) result += 12;
    if (c2 == 1 && o2 == 0) result += 12;
    if (c3 == 1 && o3 == 0) result += 12;
    do {
      GridStates cs = s;
      GridStates os = opponent(s);
      char dx = (x == 0 || x == 7) ? 0 : 1;
      char dy = (y == 0 || y == 7) ? 0 : 1;
      unsigned char t = 0;
      char tx = x, ty = y;
      do {
        tx += dx; ty += dy;
        if (b->get(tx, ty) == os) {
          t |= 0x01;
          break;
        } else if (b->get(tx, ty) == cs) {
          t |= 0x02;
        } else if (b->get(tx, ty) == GS_EMPTY) {
          t |= 0x04;
          break;
        }
      } while (valid(tx, ty));
      dx = -dx; dy = -dy;
      tx = x, ty = y;
      do {
        tx += dx; ty += dy;
        if (b->get(tx, ty) == os) {
          t |= 0x10;
          break;
        } else if (b->get(tx, ty) == cs) {
          t |= 0x20;
        } else if (b->get(tx, ty) == GS_EMPTY) {
          t |= 0x40;
          break;
        }
      } while (valid(tx, ty));
      if ((t & 0x01) != 0 && (t & 0x40) != 0) {
        result -= 12;
      } else if ((t & 0x10) != 0 && (t & 0x04) != 0) {
        result -= 12;
      }
    } while (false);
  }
  return result;
}

static void think(Board* b, Board* r, Operation* o, unsigned char x, unsigned char y, int &sco) {
  Board t = *b;
  if (place(&t, o->currentSide(), x, y, true)) {
    int ret = score(&t, x, y, o->currentSide());
    if (ret > sco) {
      if (debug) {
        Serial.print(x);
        Serial.print(", ");
        Serial.print(y);
        Serial.print(" > ");
        Serial.println(ret);
      }
      *r = t;
      sco = ret;
    } else {
      if (debug) {
        Serial.print(x);
        Serial.print(", ");
        Serial.print(y);
        Serial.print(" < ");
        Serial.println(ret);
      }
    }
  }
}

unsigned char place(Board* b, GridStates s, unsigned char x, unsigned char y, bool p) {
  char tx, ty;
  char result = 0;
  if (b->get(x, y) != GS_EMPTY) return result;
  for (char j = -1; j <= 1; ++j) {
    for (char i = -1; i <= 1; ++i) {
      if (i == 0 && j == 0) continue;
      char t = 0;
      tx = x; ty = y;
      do {
        tx += i; ty += j;
        if (!valid(tx, ty))
          break;
        if ((t == 0 || t == 1) && b->get(tx, ty) == GS_EMPTY) {
          break;
        } else if (t == 0 && b->get(tx, ty) == s) {
          break;
        } else if (t == 0 && b->get(tx, ty) == opponent(s)) {
          ++t;
        } else if (t == 1 && b->get(tx, ty) == s) {
          ++t;
          break;
        }
      } while (true);
      if (t == 2) {
        ++result;
        if (p) {
          while (tx != x || ty != y) {
            tx -= i; ty -= j;
            b->set(tx, ty, s);
          }
        }
      }
    }
  }
  return result;
}

bool placeable(Board* b, GridStates s) {
  for (unsigned char i = 0; i < 8; ++i) {
    for (unsigned char j = 0; j < 8; ++j) {
      if (place(b, s, i, j, false) != 0)
        return true;
    }
  }
  return false;
}

bool turn(Board* b, Operation* o) {
  unsigned char c0, c1;
  b->count(c0, c1);
  o->c0 = c0;
  o->c1 = c1;
  if (placeable(b, o->opponentSide())) {
    o->playing = (o->playing == 0) ? 1 : 0;
    return false;
  }
  if (placeable(b, o->currentSide())) {
    return false;
  }
  return true; /* Game over. */
}

void shuffle(void) {
  for (unsigned char i = 0; i < 24; ++i) {
    swap(LEVEL1[rand() % countof(LEVEL1)], LEVEL1[rand() % countof(LEVEL1)]);
    swap(LEVEL2[rand() % countof(LEVEL2)], LEVEL2[rand() % countof(LEVEL2)]);
    swap(LEVEL3[rand() % countof(LEVEL3)], LEVEL3[rand() % countof(LEVEL3)]);
    swap(LEVEL4[rand() % countof(LEVEL4)], LEVEL4[rand() % countof(LEVEL4)]);
    swap(LEVEL5[rand() % countof(LEVEL5)], LEVEL5[rand() % countof(LEVEL5)]);
    swap(LEVEL6[rand() % countof(LEVEL6)], LEVEL6[rand() % countof(LEVEL6)]);
  }
}

void think(Board* b, Operation* o) {
  if (debug)
    Serial.println("-----");
  Board r;
  int sco = INT_MIN;
  for (unsigned char i = 0; i < countof(LEVEL1); ++i) {
    think(b, &r, o, LEVEL1[i].x, LEVEL1[i].y, sco);
  }
  for (unsigned char i = 0; i < countof(LEVEL2); ++i) {
    think(b, &r, o, LEVEL2[i].x, LEVEL2[i].y, sco);
  }
  for (unsigned char i = 0; i < countof(LEVEL4); ++i) {
    think(b, &r, o, LEVEL4[i].x, LEVEL4[i].y, sco);
  }
  for (unsigned char i = 0; i < countof(LEVEL3); ++i) {
    GridStates cs = o->currentSide();
    GridStates os = o->opponentSide();
    if (place(b, cs, LEVEL3[i].x, LEVEL3[i].y, false)) {
      char dx = (LEVEL3[i].x == 0 || LEVEL3[i].x == 7) ? 0 : 1;
      char dy = (LEVEL3[i].y == 0 || LEVEL3[i].y == 7) ? 0 : 1;
      unsigned char t = 0;
      char tx = LEVEL3[i].x, ty = LEVEL3[i].y;
      do {
        tx += dx; ty += dy;
        if (b->get(tx, ty) == os) {
          t |= 0x01;
        } else if (b->get(tx, ty) == cs) {
          t |= 0x02;
        } else if (b->get(tx, ty) == GS_EMPTY) {
          t |= 0x04;
          break;
        }
      } while (valid(tx, ty));
      dx = -dx; dy = -dy;
      tx = LEVEL3[i].x, ty = LEVEL3[i].y;
      do {
        tx += dx; ty += dy;
        if (b->get(tx, ty) == os) {
          t |= 0x10;
        } else if (b->get(tx, ty) == cs) {
          t |= 0x20;
        } else if (b->get(tx, ty) == GS_EMPTY) {
          t |= 0x40;
          break;
        }
      } while (valid(tx, ty));
      if ((t & 0x01) != 0 && (t & 0x40) != 0) {
        think(b, &r, o, LEVEL3[i].x, LEVEL3[i].y, sco);
      } else if ((t & 0x10) != 0 && (t & 0x04) != 0) {
        think(b, &r, o, LEVEL3[i].x, LEVEL3[i].y, sco);
      }
    }
  }
  for (unsigned char i = 0; i < countof(LEVEL5); ++i) {
    think(b, &r, o, LEVEL5[i].x, LEVEL5[i].y, sco);
  }
  for (unsigned char i = 0; i < countof(LEVEL6); ++i) {
    think(b, &r, o, LEVEL6[i].x, LEVEL6[i].y, sco);
  }
  for (unsigned char i = 0; i < 8; ++i) {
    for (unsigned char j = 0; j < 8; ++j) {
      think(b, &r, o, i, j, sco);
    }
  }
  if (sco != INT_MIN) {
    *b = r;
  }
}
