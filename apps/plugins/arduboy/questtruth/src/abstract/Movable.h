/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _MOVABLE_H_
#define _MOVABLE_H_

#include "../Constants.h"

/** Interface for all movable objects */
class Movable
{
    public:
        virtual TriggerEvent move( Arduboy2 * arduboy ) = 0;
};

#endif