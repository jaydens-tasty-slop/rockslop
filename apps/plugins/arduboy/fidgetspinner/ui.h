#ifndef UI_H
#define UI_H

#include <Arduboy2.h>
#include "assets.h"
#include "globals.h"

struct Ui
{
  public:
    bool invertedColors; // Indicates if the screen colors are inverted
};

Ui ui;

// Inverts the colors of the screen
void ui_invertColors() {
  ui.invertedColors = !ui.invertedColors;
  arduboy.invert(ui.invertedColors);
}

// Reset to the initial state
void ui_reset() {
  ui.invertedColors = false;
}

// Input processing event
void ui_processInput() {
  // Check if the Up/Down button was pressed
  if ((arduboy.justPressed(UP_BUTTON)) || (arduboy.justPressed(DOWN_BUTTON))) {
    // Invert the screen colors
    ui_invertColors();
  }
}

// Drawing event
void ui_draw() {
  // Draw the CCW text
  sprites.drawPlusMask(8, HEIGHT - 8, sprite_ui_buttons_plus_mask, 0);
  arduboy.setCursor(8+9, HEIGHT - 8);
  arduboy.print(F("CCW"));

  // Draw the CW text
  sprites.drawPlusMask(8+31, HEIGHT - 8, sprite_ui_buttons_plus_mask, 1);
  arduboy.setCursor(8+31+9, HEIGHT - 8);
  arduboy.print(F("CW"));

  // Draw the SPN text
  sprites.drawPlusMask(8+56, HEIGHT - 8, sprite_ui_buttons_plus_mask, 2);
  arduboy.setCursor(8+56+9, HEIGHT - 8);
  arduboy.print(F("SPN"));

  // Draw the BG text
  sprites.drawPlusMask(8+87, HEIGHT - 8, sprite_ui_buttons_plus_mask, 3);
  arduboy.setCursor(8+87+9, HEIGHT - 8);
  arduboy.print(F("BG"));
}

#endif
