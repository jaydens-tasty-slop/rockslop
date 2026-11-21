#include <Arduino.h>
#include <Arduboy2.h>
#include "game.h"
#include "input.h"
#include "renderer.h"
#include "sound.h"

Arduboy2 arduboy;

void setup()
{
    arduboy.begin();
    arduboy.setFrameRate(60);
    arduboy.clear();
    soundInit(arduboy);
    gameInit();
}

void loop()
{
    if (!arduboy.nextFrame())
        return;

    arduboy.pollButtons();
    inputUpdate(arduboy);
    gameUpdate();
    renderFrame(arduboy);
}
