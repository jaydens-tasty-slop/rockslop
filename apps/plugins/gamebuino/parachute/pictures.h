/* The four 160x128 RGB565 pictures (background, title screen, two
 * spritesheets) are in parachute.dat on the card, made from the upstream
 * Pictures.h by tools/mkpics16.py in this order. */
#ifndef PARACHUTE_PICTURES
#define PARACHUTE_PICTURES
#include "gb_pics.h"

enum { PIC_BACKGROUND, PIC_SPLATCH, PIC_SHEET_A, PIC_SHEET_B };

static GbPics pics = {"parachute.dat", 160, 128, -2};
#define picRead(dst, pic, x, y, n) pics.read(dst, pic, x, y, n)

#endif
