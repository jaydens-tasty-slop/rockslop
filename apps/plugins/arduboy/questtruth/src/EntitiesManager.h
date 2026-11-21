/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _ENTITIES_MANAGER_H_
#define _ENTITIES_MANAGER_H_

#include <Arduboy2.h>
#include "abstract/Singleton.h"
#include "drawable/Map.h"
#include "drawable/Enemy.h"
#include "drawable/Character.h"
#include "drawable/Item.h"
#include "Trigger.h"

class EntitiesManager : public Singleton<EntitiesManager>
{
    public: 
        EntitiesManager(Arduboy2 * arduboy);

        void drawEntities();
        void spawnEntities(Map * map);
        void moveEntities();

        const CollisionCheckResult collisionCheck(const Position & pos);
        void fireCollisionCheck(const Position & pos, short rangeX, short rangeY, const HitType & hitType);
        void triggerCheckAndExecute(const Position & pos);

        TriggerEvent popTriggerEvent();
        void startNewLevel();

        Trigger * getTriggerForEvent(const TriggerEvent & event);
        int8 countNumberOfLiveEnemiesOfType(int8 type) const;

    private:
        void clearEntities();
        Character* getCharacterWithId(int8 characterId);
        Trigger* getTriggerWithId(int8 triggerId);

        Arduboy2 * arduboy;
        
        Enemy enemies[MAX_ENEMIES_PER_LEVEL];
        Character characters[MAX_CHARACTERS_PER_LEVEL];
        Trigger triggers[MAX_TRIGGERS_PER_LEVEL];
        Item itemToBePickedUp;

        int8 enemies_number, character_number, trigger_number;
        TriggerEvent triggerEvent;
};

//===============================================================================
static bool isStartDialogEvent(const TriggerEvent& e)
{
    return e == START_DIALOG_2;
}


#endif
