#include <Gamebuino-Meta.h>
#include "palettedefines.h"

#define MAKECOLOR(r,g,b) (((b >> 3) & 0x1f) |  (((g >> 2) & 0x3f) << 5) | (((r >> 3) & 0x1f) << 11))

const Color BLACK_WHITE_PALETTE_GAME[] = {

    (Color) MAKECOLOR(0,0,0), // 0x0
    (Color) MAKECOLOR(130,130,130), // 0x2
    (Color) MAKECOLOR(255,255,255), // 0x1
    (Color) MAKECOLOR(181,181,181), // 0x3
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};

const Color BLACK_WHITE_PALETTE_TITLE[] = {

    (Color) MAKECOLOR(0,0,0), // 0x0
    (Color) MAKECOLOR(10,10,10), // 0x1
    (Color) MAKECOLOR(111,111,111), // 0x2
    (Color) MAKECOLOR(181,181,181), // 0x3
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};

const Color WHITE_BLUE_PALETTE_TITLE[] = {

    (Color) WHITE, // 0x0
    (Color) LIGHTBLUE, // 0x3
    (Color) BLUE, // 0x2
    (Color) DARKBLUE, // 0x1
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};

const Color WHITE_BLUE_PALETTE_GAME[] = {

    (Color) WHITE, // 0x0
    (Color) BLUE, // 0x1
    (Color) LIGHTBLUE, // 0x3
    (Color) DARKBLUE, // 0x2
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};


const Color TROPICAL_PALETTE_TITLE[] = {

    (Color) BEIGE, // 0x0
    (Color) YELLOW, // 0x3
    (Color) ORANGE, // 0x2
    (Color) RED, // 0x1
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};

const Color TROPICAL_PALETTE_GAME[] = {

    (Color) BEIGE, // 0x0
    (Color) ORANGE, // 0x1
    (Color) YELLOW, // 0x3
    (Color) RED, // 0x2
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};


const Color GBC_PALETTE_GAME[] = {

    (Color) MAKECOLOR(128,128,128), // 0x0
    (Color) MAKECOLOR(0,0,128), // 0x1
    (Color) MAKECOLOR(0,0,255), // 0x2
    (Color) MAKECOLOR(0,0,64), // 0x3
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};

const Color GBC_PALETTE_TITLE[] = {

    (Color) MAKECOLOR(128,128,128), // 0x0
    (Color) MAKECOLOR(0,0,255), // 0x1
    (Color) MAKECOLOR(0,0,128), // 0x2
    (Color) MAKECOLOR(0,0,64), // 0x3
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};

const Color RED_PALETTE_GAME[] = {

    (Color) MAKECOLOR(255,128,128), // 0x0
    (Color) MAKECOLOR(128,0,0), // 0x1
    (Color) MAKECOLOR(255,0,0), // 0x2
    (Color) MAKECOLOR(64,0,0), // 0x3
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};

const Color RED_PALETTE_TITLE[] = {

    (Color) MAKECOLOR(255,128,128), // 0x0
    (Color) MAKECOLOR(255,0,0), // 0x1
    (Color) MAKECOLOR(128,0,0), // 0x2
    (Color) MAKECOLOR(64,0,0), // 0x3
    (Color) MAKECOLOR(0,0,0), // 0x4
    (Color) MAKECOLOR(255,0,255), // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};


const Color GB_PALETTE[] = {

    (Color) 0xe765, // 0x0
    (Color) 0x3dc8, // 0x1
    (Color) 0x23a6, // 0x2
    (Color) 0x01c5, // 0x3
    (Color) 0x0000, // 0x4
    (Color) 0xf81f, // 0x5
    (Color) 0x0000, // 0x6
    (Color) 0x0000, // 0x7
    (Color) 0x0000, // 0x8
    (Color) 0x0000, // 0x9
    (Color) 0x0000, // 0xa
    (Color) 0x0000, // 0xb
    (Color) 0x0000, // 0xc
    (Color) 0x0000, // 0xd
    (Color) 0x0000, // 0xe
    (Color) 0x0000  // 0xf

};
