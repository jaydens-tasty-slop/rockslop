#include <Gamebuino-Meta.h>
#include "GameController.h"

static const char *const help[] = {
    "A mad scientist's teleporter has burst: its stack of four chemical cells "
    "is scattered through space and time. Find the cells and put them back "
    "in the container next to the teleporter, in the right order "
    "(yellow, red, green, violet). Apples are a bonus.", "",
    "Left and right walk, UP walks through a door. SELECT (A) jumps; press "
    "it again while jumping to cling on and climb, with left or right. "
    "SELECT+DOWN (B) picks up or drops an object, or puts a cell into the "
    "container.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "gLiTcH", "",
    "Game by Pierre Jarriges.",
    "github.com/Codnpix/Glitch_META",
    "License: MIT (c) 2019 Pierre Jarriges; full text in LICENSE.txt in "
    "rocks/games/gamebuino/glitch/.",
    NULL
};

/* A is jump (SELECT, with a direction held), B is SELECT+DOWN. */
static const struct gb_rule keys[] = {
    {GB_P_SELECT | GB_P_DOWN, GB_B_B},
    {GB_P_SELECT,             GB_B_A},
    {GB_P_UP,                 GB_B_UP},
    {GB_P_DOWN,               GB_B_DOWN},
    {GB_P_LEFT,               GB_B_LEFT},
    {GB_P_RIGHT,              GB_B_RIGHT},
    {0, 0}
};

static uint16_t framebuffer[80 * 64] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "glitch", "gLiTcH", help, credits,
    keys, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 80, 64, (uint8_t *)framebuffer,
    "Sel=A (jump)  Sel+Down=B|Up=door|Hold Sel+Up=menu"
};

GameController * gameController;

void setup()
{
    gb.begin();

    gameController = new GameController();
    gameController->initGame();
}

void loop()
{
    gb.waitForUpdate();

    gameController->updateGame();
    gameController->draw();

    /*
    gb.display.setColor(RED);
    gb.display.printf("CPU : %d \n", gb.getCpuLoad());
    gb.display.printf("RAM : %d \n", gb.getFreeRam());
    */
}
