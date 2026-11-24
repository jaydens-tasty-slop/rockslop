/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _ITEM_H_
#define _ITEM_H_

#include <Arduboy2.h>

#include "../Constants.h"
#include "../abstract/Drawable.h"

class Item : public Drawable
{
    public:
        Item();
        void draw( Arduboy2 * arduboy ) override;
        void spawn(int8 newId, const Position & position);
        bool isPresent() const;
        bool collidesWithPlayer(const Position & playerPos);

        int8 getId() const;
    private:
        short getHeight() const;
        
        int8 id;
        Position pos;
};

#endif