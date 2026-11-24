#ifndef SPINNER_H
#define SPINNER_H

#include <Arduboy2.h>
#include "assets.h"
#include "globals.h"
#include "sounds.h"

#define SPINNER_SPRITE_OFFSET_X 15 // The x offset (center) of the spinner sprite
#define SPINNER_SPRITE_OFFSET_Y 14 // The y offset (center) of the spinner sprite
#define SPINNER_SPRITE_VARIANTS 3 // The number of spinner designs (sprites)
#define SPINNER_DIRECTION_CLOCKWISE 0 // The spinner rotates clockwise
#define SPINNER_DIRECTION_COUNTER_CLOCKWISE 1 // The spinner rotates counter-clockwise
#define SPINNER_MAX_SPEED (FRAMERATE - 1) // The maximum speed of the spinner
#define SPINNER_SPEED_PERCENTAGE max(((spinner.speed - 50) * 100) / (SPINNER_MAX_SPEED - 50), 0)

struct Spinner
{
  public:
    int x; // The x position
    int y; // The y position
    byte sprite; // The index of the current sprite/design/variant
    byte frame; // The current sprite frame
    byte direction; // The current rotation direction
    byte speed; // The current speed
    byte friction; // The friction (larger value = less friction)
};

Spinner spinner;
const unsigned char *SPINNER_SPRITES[] {
  sprite_spinner1,
  sprite_spinner2,
  sprite_spinner3
};

// Spins the spinner in the given direction.
void spinner_spin(byte direction) {
  spinner.direction = direction;
  spinner.speed = SPINNER_MAX_SPEED;
}

// Changes the sprite/design/variant of the spinner to the previous one
void spinner_previousSprite() {
  if (spinner.sprite > 0) {
    spinner.sprite--;
  } else {
    spinner.sprite = SPINNER_SPRITE_VARIANTS - 1;
  }
}

// Changes the sprite/design/variant of the spinner to the next one
void spinner_nextSprite() {
  spinner.sprite++;
  if (spinner.sprite == SPINNER_SPRITE_VARIANTS) {
    spinner.sprite = 0;
  }
}

// Reset to the initial state
void spinner_reset() {
  spinner.x = (WIDTH / 2);
  spinner.y = (HEIGHT / 2);
  spinner.sprite = 0;
  spinner.frame = 0;
  spinner.direction = SPINNER_DIRECTION_CLOCKWISE;
  spinner.speed = 0;
  spinner.friction = 50;
}

// Input processing event
void spinner_processInput() {
  // Check if the A (left) button was pressed
  if (arduboy.justPressed(A_BUTTON)) {
    // Spin counter-clockwise
    spinner_spin(SPINNER_DIRECTION_COUNTER_CLOCKWISE);
  }
  // Check if the B (right) button was pressed
  if (arduboy.justPressed(B_BUTTON)) {
    // Spin clockwise
    spinner_spin(SPINNER_DIRECTION_CLOCKWISE);
  }
  // Check if the Left button was pressed
  if (arduboy.justPressed(LEFT_BUTTON)) {
    // Change the sprite of the spinner
    spinner_previousSprite();
  }
  // Check if the Right button was pressed
  if (arduboy.justPressed(RIGHT_BUTTON)) {
    // Change the sprite of the spinner
    spinner_nextSprite();
  }
}

// Loop event
void spinner_loop() {
  // Apply friction to the spinner (reducing its speed over time)
  if (spinner.speed > 0) {
    if (arduboy.everyXFrames(spinner.friction)) {
      spinner.speed--;
      // Stop the spinner after the speed is below 50 (looks better)
      if (spinner.speed == 50) {
        spinner.speed = 0;
      }
    }
  }

  // Animate the spinner
  if (spinner.speed > 0) {
    if (arduboy.everyXFrames(FRAMERATE - spinner.speed)) {
      spinner.frame =
        spinner.direction == SPINNER_DIRECTION_CLOCKWISE ?
          (++spinner.frame) % 4 :
          (--spinner.frame) % 4;
    }
  }

  // Play/update/stop the spinning sound
  sounds_playSpinSound(SPINNER_SPEED_PERCENTAGE);
}

// Drawing event
void spinner_draw() {
  // Draw the spinner
  sprites.drawOverwrite(spinner.x - SPINNER_SPRITE_OFFSET_X,
                        spinner.y - SPINNER_SPRITE_OFFSET_Y,
                        SPINNER_SPRITES[spinner.sprite],
                        spinner.frame);
}

#endif
