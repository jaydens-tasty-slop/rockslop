#include <stdlib.h>
#include <math.h>
#include <Arduboy.h>
#include "../lib/ab_math.h"
#include <string.h>
#include "GameObject.h"

GameObject::GameObject()
{
  setActive(true);
}

float GameObject::getX() { return x; }
float GameObject::getY() { return y; }
void GameObject::setX(float _x) { x = _x; }
void GameObject::setY(float _y) { y = _y; }

bool GameObject::isActive()
{
  return active_flag;
}

void GameObject::setActive(bool flag)
{
  active_flag = flag;
}

// Defaults for the virtual members the upstream base class only declares
// (the AVR toolchain tolerated the missing vtable; the plugin linker does not).
void GameObject::draw() {}
float GameObject::getTop() { return y; }
float GameObject::getDown() { return y; }
float GameObject::getRight() { return x; }
float GameObject::getLeft() { return x; }
