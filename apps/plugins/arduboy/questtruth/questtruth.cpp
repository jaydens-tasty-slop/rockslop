/**
* The Question For Truth is a game for Arduboy
*/

#include <Arduboy2.h>
#include "src/Game.h"

Game game;

//==========================================================
void setup()
{
    game.init();
    randomSeed(millis());

}

//==========================================================
void loop()
{
    game.update();
}
