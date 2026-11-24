#include <Arduino.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <ArduboyTonesPitches.h>
#include "globals.h"
#include "assets.h"
#include "spinner.h"
#include "ui.h"

void setup() {
  // Setup Arduboy-related stuff
  arduboy.begin();
  arduboy.setFrameRate(FRAMERATE);

  // Reset to the initial state
  spinner_reset();
  ui_reset();
}

void loop() {
  // Pause the loop until its time for the next frame
  if (!(arduboy.nextFrame())) {
    return;
  }

  // Poll the Arduboy buttons
  arduboy.pollButtons();

  // Input processing event
  spinner_processInput();
  ui_processInput();

  // Loop event
  spinner_loop();

  // Drawing event
  arduboy.clear();
  spinner_draw();
  ui_draw();
  arduboy.display();
}
