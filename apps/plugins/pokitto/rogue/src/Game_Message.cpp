#include "Game.h"
#include "Pokitto.h"
#include "Strings.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

void Game::setActiveMessage(uint8_t id)
{
  activeMessage = messages[id];
}

void Game::itmToGitm(uint8_t type, uint8_t r, uint8_t k)
{
  activeMessage = getItemName(type, r, k);
}

