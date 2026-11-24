/* Rockbox game description for Under the Tower. A (confirm, attack, talk)
 * is the main button; B opens the pause/inventory menu outside battles. */
#include <Arduino.h>

static const char *const help[] = {
    "A role-playing game in a world with a tower at its centre: explore the "
    "overworld and dungeons, talk to people, and fight turn-based battles "
    "with your party.", "",
    "Directions: move, choose.", "",
    "Select: the A button (confirm, talk, attack).", "",
    "Select + Down: the B button (menu / inventory, back in battles).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Under the Tower", "",
    "wuuff, ported from the Gamebuino original.", "",
    "https://github.com/wuuff/under-the-tower-arduboy", "",
    "License: GPL-3.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "undertower", "Under the Tower", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
