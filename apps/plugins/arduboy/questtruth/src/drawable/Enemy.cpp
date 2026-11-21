/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include <stdlib.h>
#include <math.h>
#include "../../../lib/ab_math.h"
#include "Enemy.h"
#include "../Utils.h"
#include "Player.h"

namespace
{
    static const short ENEMY_1_WIDTH = 10;
    static const short ENEMY_1_HEIGHT = 18;
    static const short ENEMY_2_WIDTH = 12;
    static const short ENEMY_2_HEIGHT = 12;
    static const short PARALYSED_TICKS = 500;
    static const short FOLLOW_PLAYER_DISTANCE = TILE_LENGTH * 5;
    static const short ENEMY_DEATH_TICKS = 21;
    static const short ENEMY_GO_BACK_MOVE = 2;
    static const short HIT_IMMUNE_TIME = 50;

    void movePosition1TowardsPosition2(const Position & p1, const Position & p2, short & px, short & py, short move)
    {
        short dx = p2.x - p1.x;
        short dy = p2.y - p1.y;
        float norm = sqrt(dx * dx + dy * dy); //normalise vector
        if (norm)
        {
            dx *= move / norm;
            dy *= move / norm;
        }

        px = p1.x + dx;
        py = p1.y + dy;
    }

    bool withinNRange(const Position p1, const Position & p2, short range)
    {
        int dx = p1.x - p2.x;
        int dy = p1.y - p1.y;

        return abs( sqrt(dx * dx + dy * dy) ) < range;
    }
}

PROGMEM static const byte BITMAP_1[] = {0xfc, 0x06, 0xc3, 0x01, 0x01, 0x01, 0xc3, 0x06, 
  0x1c, 0xe0, 0xff, 0x05, 0x82, 0xa5, 0xd0, 0x55, 
  0xa2, 0x85, 0x00, 0xff, 0x03, 0x01, 0x01, 0x03, 
  0x01, 0x03, 0x01, 0x01, 0x03, 0x01};

PROGMEM static const byte BITMAP_2[] = {0xf0, 0x0c, 0x02, 0x1a, 0x99, 0x81, 0x81, 0x99, 
  0x1a, 0x02, 0x0c, 0xf0, 0x00, 0x03, 0x04, 0x05, 
  0x08, 0x08, 0x08, 0x08, 0x05, 0x04, 0x03, 0x00};

//==========================================================
Enemy::Enemy()
    : pos(0,0)
    , initPos(0,0)
    , life(0)
    , shake(0)
    , displaySpriteA( true )
    , facingRight( true )
    , goBackToInitPos( false )
    , animFrameCounter( 0 )
    , paralysedCounter( 0 )
    , hitCounter(0)
{

}

//==========================================================
 void Enemy::spawn( const Position & spawnPosition, int8 enemyType )
 {
     type = enemyType;
     pos = spawnPosition;
     initPos = spawnPosition;

     switch(type)
     {
        case 1:
            life = 1;
            break;
        case 2:
            life = 10;
            break;
     }
 }

//==========================================================
TriggerEvent Enemy::move( Arduboy2 * arduboy )
{
    if(life <= 0) return NO_EVENT;

    short width = getWidth();
    short height = getHeight();

    if(paralysedCounter != 0)
    { 
        shakeEnemyForParalysis();
        return NO_EVENT;
    }

    if(walkFrameSkipped > ENEMY_WALK_FRAME_SKIP)
    {
        walkFrameSkipped = 0;
    }
    else
    {
        walkFrameSkipped++;
        return NO_EVENT; //only move every n frames
    }

    Position &playerPos = Player::instance()->getPos();
    short distancePlayerX = playerPos.x - pos.x;

    if(goBackToInitPos)
    {
        short newPosX = 0;
        short newPosY = 0;
        movePosition1TowardsPosition2(pos, initPos, newPosX, newPosY, ENEMY_GO_BACK_MOVE);

        if(checkMapCollision(newPosX, newPosY, width, height ) )
        {
            goBackToInitPos = false;
        }
        else
        {
            pos.x = newPosX;
            pos.y = newPosY;

            if(withinNRange(pos, initPos, ENEMY_FOLLOW_MOVE))
            {
                goBackToInitPos = false;
            }
        }

        if(checkEnemyCollision(pos, playerPos, true))
        {
            life = -ENEMY_DEATH_TICKS;
            return NO_EVENT;
        }
    }
    if(type == 2 && abs(distancePlayerX) < FOLLOW_PLAYER_DISTANCE) //PLAYER IS NEARBY
    {
        short newPosX = 0;
        short newPosY = 0;
        movePosition1TowardsPosition2(pos, playerPos, newPosX, newPosY, ENEMY_FOLLOW_MOVE);
        
        goBackToInitPos = true;

        if(!checkMapCollision(newPosX, pos.y, width, height ) )
        {
            goBackToInitPos = false;
            pos.x = newPosX;
        }
        if(!checkMapCollision(pos.x, newPosY, width, height ) )
        {
            goBackToInitPos = false;
            pos.y = newPosY;
        }

        if(checkEnemyCollision(pos, playerPos, true))
        {
            life = -ENEMY_DEATH_TICKS;
            return NO_EVENT;
        }
    }
    else //NORMAL PATROL MOVE
    {
        int8 diffX = pos.x - initPos.x;
        if(diffX > ENEMY_WALK_MAX 
            || checkEnemyCollision({pos.x + ENEMY_MOVE, pos.y}, playerPos, true) 
            || checkMapCollision(pos.x + ENEMY_MOVE, pos.y, width, height ) )
        {
            facingRight = false;
        }
        else if(diffX < -ENEMY_WALK_MAX 
            || checkEnemyCollision({pos.x - ENEMY_MOVE, pos.y}, playerPos, true)
            || checkMapCollision(pos.x - ENEMY_MOVE, pos.y, width, height ) )
        {
            facingRight = true;
        }

        if(facingRight)
        {
            pos.x += ENEMY_MOVE; 
        }
        else
        {
            pos.x -= ENEMY_MOVE;
        }
    }
    
    return NO_EVENT;
}

//==========================================================
void Enemy::draw( Arduboy2 * arduboy )
{
    if(life == 0) return;

    short screenX = pos.x - Map::instance()->getScrollX();
    short screenY = pos.y - Map::instance()->getScrollY(); 

    if(paralysedCounter > 0)
    {
        screenY += shake;
    }

    if(life < 0)
    {
        arduboy->drawCircle(screenX + getWidth() / 2, screenY + getHeight() / 2, abs(life / 3));
        life++;
        return;
    }

    switch(type)
    {
        case 1:
            arduboy->drawBitmap(screenX, screenY, BITMAP_1, ENEMY_1_WIDTH, ENEMY_1_HEIGHT);
            break;
        case 2:
            arduboy->drawBitmap(screenX, screenY, BITMAP_2, ENEMY_2_WIDTH, ENEMY_2_HEIGHT);
            break;
    }
    
}

//==========================================================
const Position & Enemy::getPos() const
{
    return pos;
}

//==========================================================
int8 Enemy::getType() const
{
    return type;
}

//==========================================================
bool Enemy::checkEnemyCollision(const Position & enemyPosition, const Position & playerPosition, bool damageOn)
{
    //TODO redo that more accurately
    if(life > 0 && rectangleCollision({playerPosition, PLAYER_WIDTH, PLAYER_HEIGHT}, {enemyPosition, getWidth(), getHeight()}))
    {
        if(damageOn)
        {
            Player::instance()->takeHit();
        }
        return true;
    }
    return false;
}

//==========================================================
bool Enemy::checkMapCollision(short x, short y, short width, short height) const
{
    return Map::instance()->checkCollision(x + ENEMY_MOVE, y, width, height );
}


//==========================================================
void Enemy::onHit( const HitType & type )
{
    if(hitCounter == 0 && type == HitType::PEPPER_SPRAY)
    {
        paralysedCounter = PARALYSED_TICKS;
        hitCounter = HIT_IMMUNE_TIME;
    }
}

//==========================================================
short Enemy::getWidth() const
{
    switch(type)
    {
        case 1:
            return ENEMY_1_WIDTH;
        case 2:
            return ENEMY_2_WIDTH;
    }
    return 0;
}

//==========================================================
short Enemy::getHeight() const
{
    switch(type)
    {
        case 1:
            return ENEMY_1_HEIGHT;
        case 2:
            return ENEMY_2_HEIGHT;
    }
    return 0;
}

//==========================================================
bool Enemy::isAlive() const
{
    return life > 0;
}

//==========================================================
bool Enemy::isParalysed() const
{
    return paralysedCounter > 0;
}

//==========================================================
void Enemy::shakeEnemyForParalysis()
{
    if(paralysedCounter > 0)
    {
        if(paralysedCounter % 2)
        {
            shake--;
        }
        else
        {
            shake++;
        }
    
        paralysedCounter--;
        if(hitCounter > 0) hitCounter--;
    }
}