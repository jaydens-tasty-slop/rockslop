/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _ITEM_MANAGER_H_
#define _ITEM_MANAGER_H_

#include <Arduboy2.h>
#include "abstract/Singleton.h"
#include "Constants.h"

class ItemsManager : public Singleton<ItemsManager>
{
    public:
        ItemsManager();
        void foundClue(int8 clueNb);
        byte getCluesFound() const;
        byte getItems() const;
        void foundItem(int8 itemId);
        bool hasItem(int8 itemId) const;
        bool hasClue(int8 clueId) const;
    private:
        byte cluesFound, items;
};

PROGMEM static const char CLUE_PRE_TEXT[] = "You found a clue:";
PROGMEM static const char CLUE_1[] = "Be wary";
PROGMEM static const char CLUE_2[] = "And attract";
PROGMEM static const char CLUE_3[] = "Your enemy";
PROGMEM static PGM_P const ALL_CLUES[3] = 
{
    CLUE_1,
    CLUE_2,
    CLUE_3
};

static const short NUMBER_OF_CLUES = 3;

#endif