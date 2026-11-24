#include <Arduino.h>
#include <Arduino.h>

#include "game.h"
#include <Arduboy2.h>
#include <ArduboyPlaytune.h>

Arduboy2 arduboy;
Sprites sprites;

ArduboyPlaytune sound(arduboy.audio.enabled);

void setup()
{
//    Serial.begin(9600);
    arduboy.begin();
    arduboy.setFrameRate(60);
    arduboy.setTextSize(1);
    arduboy.clear();

    sound.initChannel(PIN_SPEAKER_1);
    sound.initChannel(PIN_SPEAKER_2);
}

void loop()
{
    if (!arduboy.nextFrame()) {
        return;
    }

    arduboy.pollButtons();

    tick();


#ifdef DEBUG
    {
        char line[32];
        sprintf(line, "%d%%", arduboy.cpuLoad());

        arduboy.setCursor(104, 2);
        arduboy.print(line);
    }
#endif


//    Serial.write(arduboy.getBuffer(), 128 * 64 / 8);
    arduboy.display();


} // loop

