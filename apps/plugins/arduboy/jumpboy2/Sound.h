#ifndef SOUND_H_
#define SOUND_H_

#define NOTE_C3  262
#define NOTE_C3S 277
#define NOTE_D3  294
#define NOTE_D3S 311
#define NOTE_E3  330
#define NOTE_F3  349
#define NOTE_G3  392
#define NOTE_G3S 415
#define NOTE_A3  440
#define NOTE_A3S 466
#define NOTE_B3  494

#define NOTE_C4  523



const uint16_t songMain[] PROGMEM = {
  NOTE_G3, 300,
  NOTE_F3, 300,
  NOTE_E3, 300,
  NOTE_C3, 150,
  NOTE_E3, 300,

  NOTE_G3, 300,
  NOTE_F3, 300,
  NOTE_E3, 300,
  NOTE_C3, 150,
  NOTE_E3, 300,

  NOTE_G3S, 300,
  NOTE_G3, 300,
  NOTE_F3, 300,
  NOTE_D3, 150,
  NOTE_F3, 300,

  NOTE_G3S, 300,
  NOTE_G3, 300,
  NOTE_F3, 300,
  NOTE_D3, 150,
  NOTE_F3, 300,
  TONES_REPEAT
};



const uint16_t songEnemyAppear[] PROGMEM = {
  NOTE_D3, 160,
  NOTE_F3, 160,
  NOTE_C3, 160,
  NOTE_D3, 160,
  TONES_END
};


const uint16_t songStageClear[] PROGMEM = {
  NOTE_A3, 160,
  NOTE_G3, 160,
  NOTE_A3, 160,
  NOTE_G3, 160,
  NOTE_C4, 160,
  TONES_END
};


const uint16_t songStageClear2[] PROGMEM = {
  NOTE_C4, 160,
  NOTE_B3, 160,
  NOTE_A3, 160,
  NOTE_B3, 160,
  NOTE_C4, 160,
  TONES_END
};


#endif
