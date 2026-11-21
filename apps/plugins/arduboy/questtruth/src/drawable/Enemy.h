/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _ENEMY_H_
#define _ENEMY_H_

#include <Arduboy2.h>

#include "../abstract/Movable.h"
#include "../Constants.h"
#include "Map.h"

class Enemy : public Drawable, public Movable
{
    public:
        Enemy();

        void spawn( const Position & spawnPosition, int8 enemyType);
        TriggerEvent move( Arduboy2 * arduboy ) override;
        void draw( Arduboy2 * arduboy ) override;
        
        const Position & getPos() const;
        int8 getType() const;

        bool checkEnemyCollision(const Position & playerPosition, const Position & enemyPosition, bool damageOn);
        void onHit( const HitType & hitType );

        short getWidth() const;
        short getHeight() const;
        bool isAlive() const;
        bool isParalysed() const;
    private:
        void shakeEnemyForParalysis();
        bool checkMapCollision(short x, short y, short width, short height) const;

        int8 type;
        int8 life;
        int8 shake;
        Position pos;
        Position initPos;
        
        bool displaySpriteA;
        bool facingRight;
        bool goBackToInitPos;

        int8 animFrameCounter;
        int8 walkFrameSkipped;
        short paralysedCounter;
        int8 hitCounter;

};

#endif
