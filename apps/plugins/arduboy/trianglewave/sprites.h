// Triangle Wave: Arcade Game for the Arduboy
// 2020 (C) SergioFLS
// This game is licensed under the MIT License.
// For more info read the LICENSE file.

#ifndef SPRITES_H
#define SPRITES_H

// images converted with
// https://teamarg.github.io/arduboy-image-converter/

const unsigned char PROGMEM spr_shipdown[] =
{
// width, height,
8, 8,
0x10, 0x18, 0x24, 0x22, 0x43, 0x4c, 0xb0, 0xc0, 
};

const unsigned char PROGMEM spr_shipup[] =
{
// width, height,
8, 8,
0x08, 0x18, 0x24, 0x44, 0xc2, 0x32, 0x0d, 0x03, 
};

const unsigned char PROGMEM spr_shipmiddle[] =
{
// width, height,
8, 8,
0x7f, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 
};

const unsigned char PROGMEM spr_groundup[] =
{
// width, height,
8, 8,
0x02, 0x04, 0x0a, 0x02, 0x02, 0x04, 0x02, 0x02, 
};

const unsigned char PROGMEM spr_grounddown[] =
{
// width, height,
8, 8,
0x40, 0x20, 0x50, 0x40, 0x40, 0x20, 0x40, 0x40, 
};

const unsigned char PROGMEM spr_sawblade[] =
{
// width, height,
8, 8,
0x2a, 0xbc, 0x43, 0xc2, 0x43, 0xc2, 0x3d, 0x54, 
};

#endif
