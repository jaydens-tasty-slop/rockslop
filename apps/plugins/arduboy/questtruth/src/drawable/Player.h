/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <Arduboy2.h>

#include "../Constants.h"
#include "../abstract/Drawable.h"
#include "../abstract/Movable.h"
#include "../abstract/Singleton.h"
#include "../EntitiesManager.h"
#include "Map.h"

class Player : public Drawable, public Movable, public Singleton<Player>
{
    public:
        Player();
        void levelStart();

        TriggerEvent move( Arduboy2 * arduboy ) override;
        void draw( Arduboy2 * arduboy ) override;
        void takeHit();

        inline Position & getPos() {return pos; }

        int8 getLife() const;
        void setLife(int8 life);
    private:

        bool isFalling();
        bool somethingIsAbove();
        bool checkCollisionWithMap(short playerX, short playerY) const;
        bool checkCollisionWithEntities(const Position & position); //return true if there is a collision
        void fire();
        void drawMuzzleSparkles(Arduboy2 * arduboy);

        int8 life;
        Position pos;

        float yVelocity;
        bool jumping;
        bool beingHit;
        bool firing;

        bool displaySpriteA;
        bool facingRight;
        int8 animFrameCounter;
        int8 hitFrameCounter;

};

#endif
