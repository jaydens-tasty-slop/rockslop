/* Meta Warfare for Rockbox: the Arduino sketch (*.ino, concatenated in the
 * order the Arduino IDE uses) built against the Gamebuino META layer. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Turn-based tactics for two players or against the computer.", "",
    "Move the cursor with the arrows. SELECT (A) picks a unit or a tile "
    "and confirms; SELECT+DOWN (B) cancels; SELECT+LEFT (MENU) opens the "
    "game menu (resume, end turn, save, quit).", "",
    "Capture buildings for funds, build units in your bases and destroy the "
    "enemy. Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Meta Warfare", "",
    "Game by sentinel87.",
    "github.com/sentinel87/MetaWarfare",
    "License: MIT (c) 2019 sentinel87; full text in LICENSE.txt in "
    "rocks/games/gamebuino/metawarfare/.",
    NULL
};

static uint16_t framebuffer[80 * 64] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "metawarfare", "Meta Warfare", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 80, 64, (uint8_t *)framebuffer,
    "Sel=A  Sel+Down=B|Sel+Left=game menu|Hold Sel+Up=menu"
};

#include "MetaWarfare.ino"
#include "BaseLogic.ino"
#include "BattleMapLogic.ino"
#include "BattleSceneLogic.ino"
#include "ConsolePlayerLogic.ino"
#include "DataLogic.ino"
#include "Display.ino"
#include "MenuScreensLogic.ino"
