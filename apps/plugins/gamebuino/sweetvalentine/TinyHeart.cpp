#include <Gamebuino-Meta.h>
#include "TinyHeart.h"
#include "constants.h"
#include "Application.h"

TinyHeart::TinyHeart(uint8_t rank, uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy) : Sprite(w, h, x, y, vx, vy) {
    this->rank  = rank;
    this->dying = false;
    this->dead  = false;
}

TinyHeart::~TinyHeart() = default;

const uint16_t TinyHeart::getTransparentColor() {
    return TINYHEART_TRANSPARENT_COLOR;
}

const uint16_t* TinyHeart::getBitmap() {
    return TINYHEART_BITMAP;
}

void TinyHeart::tick() {
    if (!this->dead) {
        if (random(0, 10)) {
            vx = random(0,3) - 1;
        }

        if (this->dying) vy++;

        x += vx;
        y += vy;

        if (x < 1 || x + w > SCREEN_WIDTH - 1) x -= 2*vx;

        if (y > SCREEN_HEIGHT) {
            if (Application::mode == Application::MODE_SPLASH_ENDING) {
                this->dead = true;
                Application::tinyHeartDied(this->rank);
            } else {
                x = random(0, SCREEN_WIDTH - 1 - w);
                y = -h;
                vy = random(1, 4);
            }
        }
    }
}

void TinyHeart::die() {
    this->dying = true;
}