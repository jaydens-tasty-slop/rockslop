#include "menu.h"

#include "cglp.h"

#define MAX_GAME_COUNT 20
int gameCount = 0;
static Game games[MAX_GAME_COUNT];

void addGame(char *title, char *description,
             char (*characters)[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1],
             int charactersCount, Options options, void (*update)(void)) {
  if (gameCount >= MAX_GAME_COUNT) {
    return;
  }
  Game *g = &games[gameCount];
  g->title = title;
  g->description = description;
  g->characters = characters;
  g->charactersCount = charactersCount;
  g->options = options;
  g->update = update;
  gameCount++;
}

Game getGame(int index) { return games[index]; }

static void menuUpdate(void) {}

// Slot 0 is reserved (the upstream selection screen); the Rockbox menus in
// rockbox/cgl.c replace it, so games are numbered from 1.
void addMenu(void) {
  Options o = {
      .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};
  addGame("", "", NULL, 0, o, menuUpdate);
}
