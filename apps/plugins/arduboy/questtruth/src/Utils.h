/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _UTILS_H_
#define _UTILS_H_

#include <Arduboy2.h>

#include "Constants.h"

//==========================================================
bool static rectangleCollision(const Rectangle & rectA, const Rectangle & rectB)
{
    return rectA.pos.x                < rectB.pos.x + rectB.width 
        && rectA.pos.x + rectA.width  > rectB.pos.x 
        && rectA.pos.y                < rectB.pos.y + rectB.height
        && rectA.pos.y + rectA.height > rectB.pos.y;
}

//==========================================================
short static calculateTilePaddingForHeight(const short & height)
{
    return (TILE_LENGTH - (height % TILE_LENGTH)) % TILE_LENGTH;
}

#endif
