#include "Paddle.h"
#include "constants.h"
#include "GameEngine.h"

Paddle::Paddle(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy) : Sprite(w, h, x, y, vx, vy) {
    this->ax            = 0;
    this->ay            = 0;
    this->easing        = true;
    this->targetX       = .5*(SCREEN_WIDTH - this->w);
    this->easing_factor = .4;
    this->friction      = .98;
}

Paddle::~Paddle() = default;

const uint16_t Paddle::getTransparentColor() {
    return PADDLE_TRANSPARENT_COLOR;
}

const uint16_t* Paddle::getBitmap() {
    return PADDLE_BITMAP;
}

void Paddle::tick() {
    if (this->easing) {
        this->applyEasing();
    } else {
        this->vx += this->ax;
        this->x  += this->vx;
        this->ax  = 0;
        this->vx *= friction;
        if (this->x < 0 ) {
            this->x = 0;
        } else if (this->x + w > SCREEN_WIDTH-1) {
            this->x = SCREEN_WIDTH - 1 - w;
        }
    }
}

void Paddle::applyEasing() {
    int16_t dx = this->targetX - this->x;
    this->vx = dx * this->easing_factor;
    this->x  += this->vx;
    this->vx *= this->friction;

    if (this->vx == 0) {
        this->easing = false;
        GameEngine::paddleReady();
    }
}

void Paddle::left() {
    this->ax = -2;
}

void Paddle::right() {
    this->ax = 2;
}