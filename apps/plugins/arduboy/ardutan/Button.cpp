#include <stdlib.h>
#include <math.h>
#include <Arduboy.h>
#include "../lib/ab_math.h"
#include "Button.h"

Button::Button()
{
}
Button::Button(Arduboy *_arduboy, uint8_t _type)
{
  arduboy = _arduboy;
  type = _type;
  pre_press = false;
}

void Button::begin()
{
  press = arduboy->pressed(type);
}

void Button::commit()
{
  pre_press = press;
}

bool Button::isPress()
{
  return press;
}

bool Button::isPush()
{
  if (pre_press) return false;
  
  return isPress();
}

bool Button::isRelease()
{
  if (pre_press) {
    // 前回押されていた
  } else {
    // 前回押されていなければ、処理しない
    return false;
  }
  // 今回押されてたら、処理しない
  if (isPress()) {
    return false;
  }

  // 離された
  return true;
}

