// This contains the regular and 90-degree-clockwise rotated versions
// of the 'block' graphics. They are all 8x10 pixels.
// (The arduboy.drawBitmap function requires rotated graphics as unput)

PROGMEM const unsigned char mask[10]={
0b00001100,
0b00111100,
0b11111110,
0b11111111,
0b11111111,
0b11111111,
0b11111111,
0b01111111,
0b00111100,
0b00110000
};

PROGMEM const unsigned char mask90c[16]={
124,252,254,254,255,255,252,248,0,0,3,3,1,1,0,0
};

PROGMEM const unsigned char block[10]={
0b00000000,
0b00001000,
0b00111100,
0b00111100,
0b00010010,
0b00000100,
0b00101010,
0b00010100,
0b00000000,
0b00000000
};

PROGMEM const unsigned char block90c[16]={
0,0,76,156,78,172,80,0,0,0,0,0,0,0,0,0
};

PROGMEM const unsigned char black[10]={
0b00000000,
0b01000000,
0b01000000,
0b01000011,
0b00101100,
0b00010000,
0b00010000,
0b00010000,
0b00000000,
0b00000000
};

PROGMEM const unsigned char black90c[16]={
0,14,16,224,16,16,8,8,0,0,0,0,0,0,0,0
};



