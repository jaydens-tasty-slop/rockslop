#ifndef __LOGIC_H__
#define __LOGIC_H__

#include "data.h"

unsigned char place(Board* b, GridStates s, unsigned char x, unsigned char y, bool p);
bool placeable(Board* b, GridStates s);
bool turn(Board* b, Operation* o);
void shuffle(void);
void think(Board* b, Operation* o);

#endif /* __LOGIC_H__ */
