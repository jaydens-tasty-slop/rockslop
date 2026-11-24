#include <Gamebuino-Meta.h>
#include "Sprite.h"
#include "constants.h"

Sprite::Sprite(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy) : w(w), h(h), x(x), y(y), vx(vx), vy(vy) { }

Sprite::~Sprite() = default;

int16_t Sprite::getX() {
    return x;
}

void Sprite::setX(int16_t x) {
    this->x = x;
}

int16_t Sprite::getVx() {
    return vx;
}

void Sprite::setVx(int16_t vx) {
    this->vx = vx;
}

int16_t Sprite::getY() {
    return y;
}

void Sprite::setY(int16_t y) {
    this->y = y;
}

int16_t Sprite::getVy() {
    return vy;
}

void Sprite::setVy(int16_t vy) {
    this->vy = vy;
}

void Sprite::draw(uint8_t sliceY, uint8_t sliceHeight, uint16_t* buffer) {
    // this accessor must be defined in the subclasses
    const uint16_t transparentColor = this->getTransparentColor();
    // this accessor must also be defined in the subclasses
    const uint16_t* bitmap = this->getBitmap();
    // the sprite edges along the Y axis are determined, which are
    // located inside the current tranche
    int8_t startIndex = y <= sliceY ? 0 : y - sliceY;
    int8_t endIndex = y + h >= sliceY + sliceHeight ? sliceHeight - 1 : y + h - sliceY - 1;
    // we prepare a variable that will receive in turn the color codes
    // of each of the pixels that make up the sprite (in the current slice)
    uint16_t value;
    // coordinates of the processed pixel
    uint8_t px,py;
    // and we scan the sprite between the Y edges defined above
    for (py = startIndex; py <= endIndex; py++) {
        // as well as between the edges along the X axis
        for (px = x; px < x + w; px++) {
            // we pick the color of the corresponding pixel
            value = bitmap[(sliceY - y + py) * w - x + px];
            // and if it is not the transparency color
            if (value != transparentColor) {
                // we copy it into the buffer
                buffer[px + py * SCREEN_WIDTH] = value;
            }
        }
    }
}