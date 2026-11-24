#include <Gamebuino-Meta.h>
#include <stdint.h>


#include "commonvars.h"
#include "helperfuncs.h"

#include "../res/blocktiles.h"

Image *currentTiles = nullptr;

uint32_t getRandomSeed()
{
  /* YP3: no micros()/analogRead(); millis() hashed */
  return millis() * 2654435761u ^ (millis() >> 3);
}

void set_bkg_tile_xy(uint8_t x, uint8_t y, uint8_t tile)
{
  currentTiles->setFrame(tile);
  gb.display.drawImage(x * 8, y * 8, *currentTiles);
}

void set_bkg_data(Image *tiles)
{
  currentTiles = tiles;
}

Image* get_bkg_data()
{
  return currentTiles;
}

void set_bkg_tiles(uint8_t x, uint8_t y, Image *map)
{
  gb.display.drawImage(x, y,  *map);
}

void setBlockTilesAsBackground()
{
  set_bkg_data(&blockTiles);
}