/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include "EntitiesManager.h"
#include "DialogManager.h"
#include "ItemsManager.h"
#include "Utils.h"
#include "SoundManager.h"

//==========================================================
EntitiesManager::EntitiesManager(Arduboy2 * ardu)
    : arduboy(ardu)  
    , enemies_number(0)
    , character_number(0)
    , trigger_number(0)
    , triggerEvent(NO_EVENT)
{
}

//==========================================================
void EntitiesManager::drawEntities()
{
    for(int8 i=0; i < enemies_number; i++)
    {
        enemies[i].draw( arduboy );
    }

    for(int8 i=0; i < character_number; i++)
    {
        characters[i].draw( arduboy );
    }

    if(itemToBePickedUp.isPresent())
    {
        itemToBePickedUp.draw( arduboy );
    }
}

//==========================================================
void EntitiesManager::moveEntities()   //we assume there will only be one event triggered per call
{
    TriggerEvent generatedEvent = NO_EVENT;
    for(int8 i=0; i < enemies_number; i++)
    {
        generatedEvent = enemies[i].move( arduboy );
    }

    for(int8 i=0; i < character_number; i++)
    {
        generatedEvent = characters[i].move( arduboy );
    }

    if(generatedEvent != NO_EVENT)
    {
        triggerEvent = generatedEvent;
    }
}

//==========================================================
void EntitiesManager::spawnEntities(Map * map)
{
    short x = 0;
    short y = 0;

    for(short index = 0; index < map->getLevelLength(); index ++)
    {        
        levels::Cell c = levels::getCell(map->getCurrentLevel(), index);
        
        short count = (c.count == 0xFF) ? 1 : c.count;
        short stopX = x + count * TILE_LENGTH;
        for(; x < stopX; x += TILE_LENGTH)
        {
            if(levels::getTileEnemyType(c.tile) >= 0)
            {
                enemies[enemies_number].spawn( { x, y }, levels::getTileEnemyType(c.tile));
                enemies_number++;
            }
            else if(levels::isCharacterTile(c.tile))
            {
                characters[character_number] = Character(c.tile);
                characters[character_number].spawn( { x, y } );
                character_number++;
            }
            else if(levels::getTileTriggerId(c.tile) >= 0)
            {
                triggers[trigger_number].pos = { x, y };
                triggers[trigger_number].id = levels::getTileTriggerId(c.tile);
                triggers[trigger_number].triggered = false;
                trigger_number++;
            }
            else if(levels::getTileItemId(c.tile) >= 0)
            {
                int8 itemId = levels::getTileItemId(c.tile);
                itemToBePickedUp.spawn(itemId, { x, y } );
            }                
        }

        if(c.count == 0xFF)
        {                        
            y += TILE_LENGTH;
            x = 0;
        }
    }
}

//==========================================================
const CollisionCheckResult EntitiesManager::collisionCheck(const Position & ppos)
{
    for(short i=0; i < enemies_number; i++)
    {
        if(enemies[i].checkEnemyCollision(enemies[i].getPos(), ppos, false))
        {
            return enemies[i].isParalysed() ? HIT_ENEMY_INOFFENSIVE : HIT_ENEMY;
        }
    }

    for(short i=0; i < character_number; i++)
    {
        if(characters[i].checkCharacterCollision(characters[i].getPos(), ppos))
        {
            return HIT_CHARACTER;
        }
    }

    if(itemToBePickedUp.isPresent() && itemToBePickedUp.collidesWithPlayer(ppos))
    {
        ItemsManager::instance()->foundItem(itemToBePickedUp.getId());
        itemToBePickedUp = Item();

        DialogManager::instance()->printSingleSentence(F("You found\nan item"));
        SoundManager::instance()->playSound(HAPPY_SOUND);
    }

    return FREE;
}

//==========================================================
void EntitiesManager::fireCollisionCheck(const Position & pos, short rangeX, short rangeY, const HitType & hitType)
{
    for(short i=0; i < enemies_number; i++)
    {
        if(enemies[i].isAlive() && rectangleCollision({pos, rangeX, rangeY}, {enemies[i].getPos(), enemies[i].getWidth(), enemies[i].getHeight()}))
        {
            enemies[i].onHit(hitType);
        }
    }
}

//==========================================================
void EntitiesManager::triggerCheckAndExecute(const Position & ppos)
{
    for(int8 i=0; i < trigger_number; i++)
    {
        if(triggers[i].triggered) continue;

        if( rectangleCollision({ppos, PLAYER_WIDTH, PLAYER_HEIGHT}, {triggers[i].pos, TRIGGER_WIDTH, TRIGGER_HEIGHT}) )
        {
            int8 id = triggers[i].id;
            switch(id)
            {
                case 5:
                case 6:
                    triggerEvent = START_ANIM;
                    getCharacterWithId(63)->moveDistance(id == 5 ? -40 : 40);
                default:
                    triggers[i].triggered = true;
                    if(id != 6) DialogManager::instance()->printTextForTrigger(&triggers[i]);
                    break;
                case 2:
                    triggers[i].triggered = true;
                    getCharacterWithId(62)->moveDistance(-50);
        
                    triggerEvent = START_ANIM;
                    break;
                case 19:
                    triggerEvent = END_LEVEL;
                    SoundManager::instance()->playSound(HAPPY_SOUND);
                    break;
            }
        }
    }
}

//==========================================================
TriggerEvent EntitiesManager::popTriggerEvent()
{
    TriggerEvent e = triggerEvent;
    triggerEvent = NO_EVENT;
    return e;
}

//==========================================================
void EntitiesManager::startNewLevel()
{
    clearEntities();
    spawnEntities(Map::instance());
    triggerEvent = NO_EVENT;
}

//==========================================================
Trigger * EntitiesManager::getTriggerForEvent(const TriggerEvent & event)
{
    switch(event)
    {
        case START_DIALOG_2:
            return getTriggerWithId(2);
    }
    return nullptr;
}

//==========================================================
int8 EntitiesManager::countNumberOfLiveEnemiesOfType(int8 type) const
{
    int8 count = 0;
    for(short i=0; i < enemies_number; ++i)
    {
        if(enemies[i].getType() == type && enemies[i].isAlive())
        {
            count++;
        }
    }

    return count;
}

//==========================================================
void EntitiesManager::clearEntities()
{
    for(short i=0; i < enemies_number; i++)
    {
        enemies[i] = Enemy();
    }
    for(short i=0; i < character_number; i++)
    {
        characters[i] = Character();
    }
    for(short i=0; i < trigger_number; i++)
    {
        triggers[i] = Trigger();
    }

    enemies_number = 0;
    character_number = 0;
    trigger_number = 0;

    itemToBePickedUp = Item();
}

//==========================================================
Character* EntitiesManager::getCharacterWithId(int8 characterId)
{
    for(short i=0; i < character_number; i++)
    {
        if(characters[i].getId() == characterId)
        {
            return &characters[i];
        }
    }
    return nullptr;
}

//==========================================================
Trigger* EntitiesManager::getTriggerWithId(int8 triggerId)
{
    for(short i=0; i < trigger_number; i++)
    {
        if(triggers[i].id == triggerId)
        {
            return &triggers[i];
        }
    }
    return nullptr;
}