/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _BOX_VIEW_H_
#define _BOX_VIEW_H_

#include <Arduboy2.h>

#include "Constants.h"

class BoxView
{
public:
    BoxView();
    void reset();
    bool update(Arduboy2 * arduboy);
    bool updateCinematic(Arduboy2 * arduboy);

private:
    void drawLines(Arduboy2 * arduboy);
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();


    short aX = 20;
    short aY = 14;

    short bX = 110;
    short bY = 14;

    short cX = 20;
    short cY = 50;

    short dX = 110;
    short dY = 50;

    short aaX = 0;
    short aaY = 0;

    short bbX = 128;
    short bbY = 0;

    short ccX = 0;
    short ccY = 64;

    short ddX = 128;
    short ddY = 64;

    short frameCount;
    short inscructionNb;
};

#endif