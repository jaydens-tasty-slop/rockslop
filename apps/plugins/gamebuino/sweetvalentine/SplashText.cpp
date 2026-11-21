#include <Gamebuino-Meta.h>
#include "SplashText.h"
#include "constants.h"
#include "Application.h"

SplashText::SplashText(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy) : Sprite(w, h, x, y, vx, vy) {
    this->easing_factor = .4;
    this->friction      = .98;
}

SplashText::~SplashText() = default;

const uint16_t SplashText::getTransparentColor() {
    return SPLASH_TEXT_TRANSPARENT_COLOR;
}

const uint16_t* SplashText::getBitmap() {
    return SPLASH_TEXT_BITMAP;
}

void SplashText::moveToY(int16_t y) {
    this->targetY = y;
    this->easing  = true;
}

void SplashText::tick() {
    if (this->easing) {
        this->applyEasing();
    }
}

void SplashText::applyEasing() {
    int16_t dy = this->targetY - this->y;
    this->vy = dy * this->easing_factor;
    this->y  += this->vy;
    this->vy *= this->friction;

    if (this->vy == 0) {
        this->easing = false;
    }
}