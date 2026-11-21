/* Rockbox game description for Rooftop Rescue. A works the rope (hold), B opens the in-game menu: SELECT = A, SELECT + DOWN = B. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Fly the rescue helicopter over the burning rooftops and winch "
    "survivors aboard.", "",
    "Left/Right: fly. Up/Down: wind the rope in/out.", "",
    "Select: lower the rope, or reel it in when it cannot go further "
    "(A, hold). Confirms in menus.", "",
    "Select + Down: in-game menu / toggle (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Rooftop Rescue",
    "",
    "Bert van't Veer (2019).",
    "",
    "https://github.com/BertVeer/Rooftop",
    "",
    "License: MIT. Music and effects are played through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "rooftop", "Rooftop Rescue", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=rope  Sel+Down=menu|Hold Sel+Up=menu"
};
