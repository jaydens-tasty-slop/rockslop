#ifndef SOUNDS_H
#define SOUNDS_H

const uint16_t soundCorrect[] PROGMEM = {NOTE_F4,100,NOTE_A4,150,TONES_END};
const uint16_t soundWin[] PROGMEM = {NOTE_E4,100,NOTE_F4,100,NOTE_C5,250,TONES_END};
const uint16_t soundIncorrect[] PROGMEM = {NOTE_A4,100,NOTE_F3,150,TONES_END};
const uint16_t soundDead[] PROGMEM = {NOTE_D4,100,NOTE_B3,100,NOTE_E3,100,NOTE_F2,150,TONES_END};

#endif

