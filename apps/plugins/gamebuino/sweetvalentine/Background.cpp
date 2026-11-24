#include <Gamebuino-Meta.h>
#include "Background.h"
#include "constants.h"
#include "gb_pics.h"

static GbPics background = {"sweetvalentine.dat", SCREEN_WIDTH, SCREEN_HEIGHT, -2};

Background::Background() { }

Background::~Background() = default;

void Background::draw(uint8_t sliceY, uint8_t sliceHeight, uint16_t* buffer) {
    background.read(buffer, 0, 0, sliceY, SCREEN_WIDTH * RENDERING_SLICE_HEIGHT);
}