#include "cannon.h"

#define CANNONLENGTH 5

#define ANGLEMIN 0
#define ANGLEMAX 85
#define ANGLEDELTA 1

#define FORCEMIN 20
#define FORCEMAX 120
#define FORCEDELTA 2

static __attribute__((noinline)) float bb_sin(float a) { return sinf(a); }
static __attribute__((noinline)) float bb_cos(float a) { return cosf(a); }

float rad_of_deg(uint8_t a)
{
  return (PI * a) / 180;
}

Cannon::Cannon(unsigned int x, unsigned int y, uint8_t angle, float force, bool dir_right)
{
  _x = x;
  _y = y;
  _angle = angle;
  _force = force;
  _dir_right = dir_right;
}

void Cannon::draw(const Gamebuino &gb, bool aiming) const
{
  gb.display.setColor(WHITE);
  gb.display.drawCircle(_x, LCDHEIGHT - (_y + 2), 2);
  if (_dir_right)
  {
    gb.display.drawLine(_x - 2, LCDHEIGHT - _y, _x, LCDHEIGHT - (_y + 2));
    gb.display.drawLine(_x, LCDHEIGHT - (_y + 2), 
      _x + CANNONLENGTH * bb_cos(rad_of_deg(_angle)), 
      LCDHEIGHT - (_y + 2 + CANNONLENGTH * bb_sin(rad_of_deg(_angle))));
  }
  else
  {
    gb.display.drawLine(_x + 2, LCDHEIGHT - _y, _x, LCDHEIGHT - (_y + 2));
    gb.display.drawLine(_x, LCDHEIGHT - (_y + 2), 
      _x - CANNONLENGTH * bb_cos(rad_of_deg(_angle)), 
      LCDHEIGHT - (_y + 2 + CANNONLENGTH * bb_sin(rad_of_deg(_angle))));
  }
  if (aiming)
  {
    gb.display.setColor(GRAY);
    gb.display.fillTriangle(_x - 1, LCDHEIGHT - (_y - 3), _x, LCDHEIGHT - (_y - 2), _x + 1, LCDHEIGHT - (_y - 3));
    if (_dir_right)
    {
      gb.display.setColor(WHITE);
      gb.display.drawFastVLine(_x - 6, LCDHEIGHT - (_y + 6), 6);
      gb.display.setColor(BLACK);
      gb.display.fillRect(_x - 5, LCDHEIGHT - (_y + 6), 2, 6);
      gb.display.setColor(WHITE);
      uint8_t forcebar = 5 * (_force - FORCEMIN) / (FORCEMAX - FORCEMIN) + 1;
      gb.display.fillRect(_x - 5, LCDHEIGHT - (_y + forcebar), 2, forcebar);
    }
  }
}

void Cannon::move(uint8_t ypos)
{
  _y = ypos;
}

void Cannon::up()
{
  _angle = constrain(_angle + ANGLEDELTA, ANGLEMIN, ANGLEMAX);
}

void Cannon::down()
{
  _angle = constrain(_angle - ANGLEDELTA, ANGLEMIN, ANGLEMAX);
}

void Cannon::longer()
{
  _force = constrain(_force + FORCEDELTA, FORCEMIN, FORCEMAX);
}

void Cannon::shorter()
{
  _force = constrain(_force - FORCEDELTA, FORCEMIN, FORCEMAX);
}

