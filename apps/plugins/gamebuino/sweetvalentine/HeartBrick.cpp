#include "HeartBrick.h"
#include "constants.h"

HeartBrick::HeartBrick(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy) : Sprite(w, h, x, y, vx, vy) { }

HeartBrick::~HeartBrick() = default;

const uint16_t HeartBrick::getTransparentColor() {
    return HEART_BRICK_TRANSPARENT_COLOR;
}

const uint16_t* HeartBrick::getBitmap() {
    return HEART_BRICK_BITMAP;
}

void HeartBrick::tick() { }