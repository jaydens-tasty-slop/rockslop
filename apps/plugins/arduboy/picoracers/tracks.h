#ifndef TRACK_H
#define TRACK_H

#include <Arduino.h>
#include "globals.h"
#include "road.h"
#include "cars.h"

#define TOTAL_AMOUNT_OF_LEVELS  2
#define TOTAL_AMOUNT_OF_ROADPARTS   1

void drawTheRoad()
{
  drawRoad();
}

void roadPart000()
{
  
}

typedef void (*FunctionPointer) ();
const FunctionPointer PROGMEM Levels[TOTAL_AMOUNT_OF_LEVELS][TOTAL_AMOUNT_OF_ROADPARTS] =
{
  { //TRACK 01
    roadPart000,
  },
  { //TRACK 02
    roadPart000,
  },
};

#endif
