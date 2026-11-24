#ifndef fightEngine_h
#define fightEngine_h
#include "actor.h"
#include "Globals.h"
#include "player.h"

enum moveType {
  attack,
  defend,
  special,
  magic,
  item,
  skip, 
};

void Engagement(playerCharacter& plyr, actor& cpu);
void engage(actor& agressor, actor& target, moveType agresMove, moveType trgtMove);  

#endif // fightEngine_h
