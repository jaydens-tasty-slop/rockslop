/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include "Item.h"
#include "../Utils.h"
#include "Map.h"

namespace
{
    static const short ITEM_1_WIDTH = 4;
    static const short ITEM_1_HEIGHT = 7;
}

PROGMEM const byte ITEM_1_BITMAP[] = //PEPPER SPRAY
{
    0x7e, 0x43, 0x7f, 0x01
};

//=========================================
Item::Item()
    : pos(0,0)
    , id(0)
{
}

//=========================================
void Item::draw( Arduboy2 * arduboy )
{
    short screenX = pos.x - Map::instance()->getScrollX();
    short screenY = pos.y - Map::instance()->getScrollY(); 

    switch(id)
    {
        case 1:
            arduboy->drawBitmap(screenX, screenY, ITEM_1_BITMAP, ITEM_1_WIDTH, ITEM_1_HEIGHT);
        break;
    }
    
}

//=========================================
void Item::spawn(int8 newId, const Position & position )
{
    id = newId;
    pos = position;
    pos.y += calculateTilePaddingForHeight(getHeight());
}

//=========================================
bool Item::isPresent() const
{
    return id > 0;
}

//=========================================
bool Item::collidesWithPlayer(const Position & playerPos)
{
    short width, height = 0;
    if(id == 1) //PEPPER SPRAY
    {
        width = ITEM_1_WIDTH;
        height = ITEM_1_HEIGHT;
    }

    return rectangleCollision(
        {playerPos, PLAYER_WIDTH, PLAYER_HEIGHT}, 
        {pos, width, height});
}

//=========================================
int8 Item::getId() const
{
    return id;
}

//=========================================
short Item::getHeight() const
{
    if(id == 1) //PEPPER SPRAY
    {
        return ITEM_1_HEIGHT;
    }

    return 0;
}