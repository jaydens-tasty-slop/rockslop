#include <stdlib.h>
#include <math.h>
#include "data.h"
#include <EEPROM.h>

bool debug = false;

static void EEPROMWritelong(int address, unsigned long value) {
  unsigned char four = (value & 0xff);
  unsigned char three = ((value >> 8) & 0xff);
  unsigned char two = ((value >> 16) & 0xff);
  unsigned char one = ((value >> 24) & 0xff);
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

static unsigned long EEPROMReadlong(long address) {
  unsigned long four = EEPROM.read(address);
  unsigned long three = EEPROM.read(address + 1);
  unsigned long two = EEPROM.read(address + 2);
  unsigned long one = EEPROM.read(address + 3);
  return ((four << 0) & 0xff) + ((three << 8) & 0xffff) + ((two << 16) & 0xffffff) + ((one << 24) & 0xffffffff);
}

GridStates opponent(GridStates s) {
  if (s == GS_WHITE) return GS_BLACK;
  if (s == GS_BLACK) return GS_WHITE;
  return GS_EMPTY;
}

Board::Board() {
  clear();
}

Board &Board::init(void) {
  set(3, 3, GS_WHITE);
  set(4, 3, GS_BLACK);
  set(3, 4, GS_BLACK);
  set(4, 4, GS_WHITE);
  return *this;
}

Board &Board::clear(void) {
  for (unsigned char i = 0; i < 8; ++i) {
    fill[i] = 0x00;
    color[i] = 0x00;
  }
  return *this;
}

GridStates Board::get(unsigned char x, unsigned char y) {
  if ((fill[y] & (1 << x)) == 0)
    return GS_EMPTY;
  if ((color[y] & (1 << x)) == 0)
    return GS_WHITE;
  else
    return GS_BLACK;
}

Board &Board::set(unsigned char x, unsigned char y, GridStates s) {
  if (s == GS_EMPTY) {
    fill[y] &= ~(1 << x);
  } else {
    fill[y] |= (1 << x);
    if (s == GS_WHITE)
      color[y] &= ~(1 << x);
    else if (s == GS_BLACK)
      color[y] |= (1 << x);
  }
  return *this;
}

unsigned char Board::count(unsigned char &c0, unsigned char &c1) {
  unsigned char result = 0;
  c0 = c1 = 0;
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      if (get(i, j) == GS_WHITE) {
        ++c0;
        ++result;
      } else if (get(i, j) == GS_BLACK) {
        ++c1;
        ++result;
      }
    }
  }
  return result;
}

Vec2i::Vec2i() {
  x = y = 0;
}

Vec2i::Vec2i(unsigned char _x, unsigned char _y) {
  x = _x; y = _y;
}

Operation::Operation() {
  clear();
}

Operation &Operation::clear(void) {
  x = 3;
  y = 3;
  px = false;
  py = false;
  pa = false;
  pb = false;
  c0 = 2;
  c1 = 2;
  won = 0;
  total = 0;
  chosen = 1;
  playing = 1;
  state = GS_MENU;
  return *this;
}

Operation &Operation::reset(void) {
  x = 3;
  y = 3;
  px = false;
  py = false;
  pa = false;
  pb = false;
  c0 = 2;
  c1 = 2;
  playing = 1;
  return *this;
}

GridStates Operation::currentSide(void) {
  return (playing == 0) ? GS_WHITE : GS_BLACK;
}

GridStates Operation::opponentSide(void) {
  return (playing != 0) ? GS_WHITE : GS_BLACK;
}

void load(Operation* o) {
  o->won = EEPROMReadlong(700);
  o->total = EEPROMReadlong(708);
  if (o->won == 0xffffffff || o->total == 0xffffffff) {
    o->won = 0;
    o->total = 0;
    save(o);
  }
}

void save(Operation* o) {
  EEPROMWritelong(700, o->won);
  EEPROMWritelong(708, o->total);
}
