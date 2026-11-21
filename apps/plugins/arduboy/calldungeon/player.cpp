#include "player.h"
#include "bmp_man.h"

extern Arduboy mArduboy;

void Player::move(int dx)
{
  if (dx < 0) {
    x += dx;
    if (x <= -16) x = 127;
  } else {
    x += dx;
    if (x >= 128) x = -15;
  }
}

void Player::do_state()
{
  switch (state) {
  case MAN_ST_ATTACK:
    count ++;
    if (count >= 20) state = MAN_ST_STAND;
    break;

  case MAN_ST_DAMAGED:
    count ++;
    if (count >= 40) state = MAN_ST_STAND;
    break;

  default:
    break;
  }
}

void Player::draw(uint16_t anim)
{
  int draw_x, draw_y;

  draw_y = y;
  draw_x = x / 4;

  if (dir == DIR_RIGHT) {
    switch (state) {
    case MAN_ST_STAND:
      mArduboy.drawBitmap(draw_x, draw_y, man_bmp[0], 16, 16, WHITE);
      mArduboy.drawBitmap(draw_x, draw_y, man_bmp[1], 16, 16, BLACK);
      break;
    case MAN_ST_MOVE:
      if ((draw_x & 2) == 0) {
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[0], 16, 16, WHITE);
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[1], 16, 16, BLACK);
      } else {
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[2], 16, 16, WHITE);
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[3], 16, 16, BLACK);
      }
      break;
    case MAN_ST_ATTACK:
      mArduboy.drawBitmap(draw_x + 8, draw_y, man_bmp[4], 16, 16, WHITE);
      mArduboy.drawBitmap(draw_x + 8, draw_y, man_bmp[5], 16, 16, BLACK);
      break;
    case MAN_ST_DAMAGED:
      if ((anim & 8) == 0) {
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[0], 16, 16, WHITE);
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[1], 16, 16, BLACK);
      }
      break;
    default:
      break;
    }
  } else {
    switch (state) {
    case MAN_ST_STAND:
      mArduboy.drawBitmap(draw_x, draw_y, man_bmp[6], 16, 16, WHITE);
      mArduboy.drawBitmap(draw_x, draw_y, man_bmp[7], 16, 16, BLACK);
      break;
    case MAN_ST_MOVE:
      if ((draw_x & 2) == 0) {
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[6], 16, 16, WHITE);
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[7], 16, 16, BLACK);
      } else {
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[8], 16, 16, WHITE);
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[9], 16, 16, BLACK);
      }
      break;
    case MAN_ST_ATTACK:
      mArduboy.drawBitmap(draw_x - 8, draw_y, man_bmp[10], 16, 16, WHITE);
      mArduboy.drawBitmap(draw_x - 8, draw_y, man_bmp[11], 16, 16, BLACK);
      break;
    case MAN_ST_DAMAGED:
      if ((anim & 8) == 0) {
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[6], 16, 16, WHITE);
	mArduboy.drawBitmap(draw_x, draw_y, man_bmp[7], 16, 16, BLACK);
      }
      break;
    default:
      break;
    }
  }
}
