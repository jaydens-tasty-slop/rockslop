/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _CHARACTER_H_
#define _CHARACTER_H_

#include <Arduboy2.h>

#include "../abstract/Movable.h"
#include "../Constants.h"
#include "Map.h"

class Character : public Drawable, public Movable
{
    public:
        Character();
        Character(int8 characterId);

        void spawn( const Position & spawnPosition );
        TriggerEvent move( Arduboy2 * arduboy ) override;
        void draw( Arduboy2 * arduboy ) override;
        
        const Position & getPos() const;
        const int8 getId() const;

        bool checkCharacterCollision(const Position & playerPosition, const Position & enemyPosition);
        void moveDistance(int8 distance);
    private:

        short getWidth() const;
        short getHeight() const;
        TriggerEvent getTriggerEventForCharacterMovement() const;

        Position pos;
        
        int8 id;
        bool displaySpriteA;
        bool facingRight;
        bool visible;

        int8 animFrameCounter;
        int8 walkFrameSkipped;
        int8 distanceToMove;

};

#endif
