/* Rockbox host for ArduGolf: Arduboy sketch glue and game description. */
#include <Arduino.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>

void game_setup();
void game_loop();

static Arduboy2Base arduboy;
ArduboyTones sound(Arduboy2Audio::enabled);
uint8_t* buf;

void toggle_audio()
{
  arduboy.audio.toggle();
  if (!arduboy.audio.enabled())
    sound.noTone();
}

void save_audio_on_off() {}
bool audio_enabled() { return arduboy.audio.enabled(); }
uint16_t time_ms() { return (uint16_t)millis(); }
uint8_t poll_btns() { return arduboy.buttonsState(); }

void setup()
{
  arduboy.boot();
  arduboy.setFrameRate(30);
  buf = arduboy.getBuffer();
  game_setup();
}

void loop()
{
  if (!arduboy.nextFrame())
    return;
  game_loop();
  arduboy.display(false);
}

static const char *const help[] = {
    "3D minigolf: 18 holes, fewest strokes wins.", "",
    "Left/Right: aim. Up/Down: shot power.", "",
    "Select: hit the ball / confirm.", "",
    "Select + Down: back / menu (the B button).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "ArduGolf", "",
    "Original game by Tiberius Brown.", "",
    "Flipper Zero port: apfxtech,", "",
    "https://github.com/apfxtech/FlipperGolf", "",
    "License: MPL-2.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardugolf", "ArduGolf", help, credits, NULL,
    0, 0, 0, 0, 0, NULL, 0, "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
