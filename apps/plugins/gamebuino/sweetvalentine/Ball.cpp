#include "Ball.h"
#include "constants.h"

Ball::Ball(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy) : Sprite(w, h, x, y, vx, vy) { }

Ball::~Ball() = default;

const uint16_t Ball::getTransparentColor() {
    return BALL_TRANSPARENT_COLOR;
}

const uint16_t* Ball::getBitmap() {
    return BALL_BITMAP;
}

void Ball::tick() {
    this->x += this->vx;
    this->y += this->vy;
}

void Ball::start() {
    vx = (!random(0,2) ? -1 : 1) * random(1,3);
    vy = 3;
}