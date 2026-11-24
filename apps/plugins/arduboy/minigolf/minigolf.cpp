#include <stdlib.h>
#include "../lib/ab_math.h"
#ifndef TWO_PI
#define TWO_PI 6.283185307179586
#endif
#include <ArduboyFX.h>
#include "src/FX/fxdata.h"
#include "src/Game.h"
#include "src/CreditsSprite.h"
#include <Arduboy2.h>

Arduboy2Base arduboy;
Game game(arduboy);
unsigned long previousTime = 0;

void setup()
{
    arduboy.begin();
    arduboy.setFrameRate(60);

    FX::begin(FX_DATA_PAGE);

    previousTime = millis();

    game.Init();

    PlayCreditsAnimation(arduboy);
}

void loop()
{
    while (!arduboy.nextFrame())
        return;

    arduboy.clear();
    arduboy.pollButtons();

    unsigned long currentTime = millis();
    unsigned long timeDelta = currentTime - previousTime;
    previousTime = currentTime;
    float secondsDelta = timeDelta * (1.0f / 1000.0f);

    game.Tick(secondsDelta);
    game.Display();

    FX::display();
}
