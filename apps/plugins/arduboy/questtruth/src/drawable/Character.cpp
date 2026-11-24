/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include "Character.h"
#include "../Utils.h"
#include "Player.h"

namespace
{
    static const short CHARACTER_1_WIDTH = 5;
    static const short CHARACTER_1_HEIGHT = 20;
    static const short CHARACTER_2_WIDTH = 9;
    static const short CHARACTER_2_HEIGHT = 15;
    static const short CHARACTER_3_WIDTH = 10;
    static const short CHARACTER_3_HEIGHT = 18;
}

//SKINNY GUY
PROGMEM static const byte BITMAP_1_A[] = {0x87, 0x8f, 0xff, 0x0f, 0x00, 0x13, 0x1e, 0xe1, 
0xff, 0x00, 0x08, 0x07, 0x00, 0x00, 0x0f};

PROGMEM static const byte BITMAP_1_B[] = {0x80, 0x87, 0xef, 0x1f, 0x0f, 0x0f, 0x00, 0xe9, 
0xff, 0x00, 0x08, 0x07, 0x00, 0x00, 0x0f};

//LITTLE GIRL
PROGMEM static const byte BITMAP_2_A[] = {0x38, 0x14, 0x06, 0x89, 0x73, 0xdb, 0x06, 0x14, 
0x38, 0x01, 0x03, 0x7d, 0x21, 0x10, 0x21, 0x7d, 0x03, 0x01};

PROGMEM static const byte BITMAP_2_B[] = {0x10, 0x3c, 0x06, 0x89, 0x73, 0xdb, 0x06, 0x3c, 
0x10, 0x02, 0x43, 0x3d, 0x21, 0x10, 0x21, 0x3d, 0x43, 0x02};

//OLD GUY
PROGMEM static const byte BITMAP_3_A[] = {0x1c, 0x22, 0x41, 0x41, 0x41, 0x22, 0x9c, 0x90, 
0xe0, 0x80, 0x00, 0xf8, 0x0a, 0x09, 0x05, 0x05, 0xc2, 0x7a, 0x3d, 0xc3, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03 };

PROGMEM static const byte BITMAP_3_B[] = {0x00, 0x1c, 0x22, 0x41, 0x41, 0x41, 0xa2, 0x9c, 
0xe0, 0x80, 0x00, 0xfc, 0x04, 0x05, 0x05, 0x05, 0xc2, 0x7a, 0x3d, 0xc3, 0x00, 0x01, 0x00, 0x00,  0x00, 0x02, 0x03, 0x00, 0x02, 0x03 };


//==========================================================
Character::Character()
    : Character(0)
{
}

//==========================================================
Character::Character(int8 characterId)
    : id(characterId)
    , pos(0,0)
    , displaySpriteA( true )
    , facingRight( true )
    , animFrameCounter( 0 )
    , visible( false )
    , distanceToMove(0)
{

}

//==========================================================
 void Character::spawn( const Position & spawnPosition )
 {
     pos = spawnPosition;
     pos.y += calculateTilePaddingForHeight(getHeight()); //if the character height doesn't fall right on tile end we shift it downwards

     visible = true;
 }

//==========================================================
TriggerEvent Character::move( Arduboy2 * arduboy )
{
    if(!visible) return NO_EVENT;

    //Frame animation
    if(animFrameCounter > PLAYER_ANIM_NB_FRAMES)
    {
        displaySpriteA = !displaySpriteA;
        animFrameCounter = 0;
    }
    animFrameCounter++;

    if( abs(distanceToMove) >= CHARACTER_MOVE)
    {
        if(distanceToMove > 0)
        {
            pos.x += CHARACTER_MOVE;
            distanceToMove -= CHARACTER_MOVE;
        }
        else //if distanceToMove < 0
        {
            pos.x -= CHARACTER_MOVE;
            distanceToMove += CHARACTER_MOVE;
        }
        if( abs(distanceToMove) < CHARACTER_MOVE)
        {
            distanceToMove = 0;
            return getTriggerEventForCharacterMovement();
        }
    }
    
    return NO_EVENT;
}

//==========================================================
void Character::draw( Arduboy2 * arduboy )
{
    if(!visible) return;

    short screenX = pos.x - Map::instance()->getScrollX();
    short screenY = pos.y - Map::instance()->getScrollY(); 
    
    switch(id)
    {
        case levels::Tile::_CHARACTER_1:
            if(displaySpriteA)
            {
                arduboy->drawBitmap(screenX, screenY, BITMAP_1_A, CHARACTER_1_WIDTH, CHARACTER_1_HEIGHT);
            }
            else
            {
                arduboy->drawBitmap(screenX, screenY, BITMAP_1_B, CHARACTER_1_WIDTH, CHARACTER_1_HEIGHT);
            }
        break;
        case levels::Tile::_CHARACTER_2:
            if(displaySpriteA)
            {
                arduboy->drawBitmap(screenX, screenY, BITMAP_2_A, CHARACTER_2_WIDTH, CHARACTER_2_HEIGHT);
            }
            else
            {
                arduboy->drawBitmap(screenX, screenY, BITMAP_2_B, CHARACTER_2_WIDTH, CHARACTER_2_HEIGHT);
            }
        break;
        case levels::Tile::_CHARACTER_3:
            if(displaySpriteA)
            {
                arduboy->drawBitmap(screenX, screenY, BITMAP_3_A, CHARACTER_3_WIDTH, CHARACTER_3_HEIGHT);
            }
            else
            {
                arduboy->drawBitmap(screenX, screenY, BITMAP_3_B, CHARACTER_3_WIDTH, CHARACTER_3_HEIGHT);
            }
        break;
    }
    
}

//==========================================================
const Position & Character::getPos() const
{
    return pos;
}

//==========================================================
const int8 Character::getId() const
{
    return id;
}

//==========================================================
bool Character::checkCharacterCollision(const Position & CharacterPosition, const Position & playerPosition)
{
    return rectangleCollision({playerPosition, PLAYER_WIDTH, PLAYER_HEIGHT}, {CharacterPosition, getWidth(), getHeight()});
}

//==========================================================
void Character::moveDistance(int8 distance)
{
    distanceToMove = distance;
}

//==========================================================
short Character::getWidth() const
{
    switch(id)
    {
        case levels::Tile::_CHARACTER_1:
            return CHARACTER_1_WIDTH;
        case levels::Tile::_CHARACTER_2:
            return CHARACTER_2_WIDTH;
        case levels::Tile::_CHARACTER_3:
            return CHARACTER_3_WIDTH;
        break;
    }
    return 0;
}

//==========================================================
short Character::getHeight() const
{
    switch(id)
    {
        case levels::Tile::_CHARACTER_1:
            return CHARACTER_1_HEIGHT;
        case levels::Tile::_CHARACTER_2:
            return CHARACTER_2_HEIGHT;
        case levels::Tile::_CHARACTER_3:
            return CHARACTER_3_HEIGHT;
        break;
    }
    return 0;
}

//==========================================================
TriggerEvent Character::getTriggerEventForCharacterMovement() const
{
    switch (id)
    {
        case levels::Tile::_CHARACTER_2:
            return TriggerEvent::START_DIALOG_2;
        case levels::Tile::_CHARACTER_3:
            return TriggerEvent::STOP_ANIM;
        default:
            return TriggerEvent::NO_EVENT;
    }
}
