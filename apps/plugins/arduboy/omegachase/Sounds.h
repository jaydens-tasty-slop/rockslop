#ifndef SOUNDS_H
#define SOUNDS_H

//  Omega Chase sounds
//  Version 1.0, May 22, 2023 
//  Copyright (c) 2023
//  By Karl P. Williams

const uint16_t wallBounce[] PROGMEM = {200, 40, 175, 40, TONES_END};

const uint16_t thrustNoise[] PROGMEM = { 30,20, 0,25, 60,30, 0,20, 35,20, 0,30, 70,30, 0,20, TONES_END};

const uint16_t fire[] PROGMEM = {900, 30, 700, 30, 500, 20, TONES_END};

const uint16_t song_1[] PROGMEM = {415.30,120, 369.99,120, 415.30,120, 440,120, 493.88,480, 329.63,480, 
                                   415.30,120, 369.99,120, 415.30,120, 440,120, 493.88,480, 329.63,480,
                                   415.30,120, 369.99,120, 415.30,120, 440,120, 493.88,480, 329.63,480,
                                   415.30,120, 440,120, 415.30,120, 440,120, 369.99,720, TONES_END};

const uint16_t song_2[] PROGMEM = {246.94,240, 277.18,720, 440,240, 415.30,240, 369.99,240, 329.63,240,
                                   329.63,720, 369.99,120, 415.70,120, 369.99,480, 329.63,480, 0,25, 329.63,720, TONES_END};

#endif