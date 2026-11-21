#include "Renderable.h"

Renderable::~Renderable() {
    index = -1;
}

int8_t Renderable::getIndex() {
    return index;
}

void Renderable::setIndex(int8_t index) {
    this->index = index;
}