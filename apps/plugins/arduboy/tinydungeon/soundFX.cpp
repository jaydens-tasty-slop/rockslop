#include "soundFX.h"
#include "settings.h"

#if defined(__AVR_ATtiny85__)
  // required for _delay_us()
  #include <util/delay.h>
#elif defined( _USE_ARDUBOY2_ )
  #include <Arduboy2.h>
  #include <ArduboyTones.h>
  extern Arduboy2 arduboy;
  //extern BeepPin1 beep;
  extern ArduboyTones sound;
#endif

#ifdef _USE_ARDUBOY2_
  // yeah, figure that out later...
const uint16_t stepSoundNotes[] PROGMEM = {
  NOTE_C1H, 16,  NOTE_REST, 200,  NOTE_F1H, 16,  TONES_END
};

const uint16_t wallSoundNotes[] PROGMEM = {
  NOTE_C1H, 50,  NOTE_REST, 200,  TONES_END
};

const uint16_t swordSoundNotes[] PROGMEM = {
  NOTE_CS8H, 32,  NOTE_AS1H, 32,  TONES_END
};

const uint16_t switchSoundNotes[] PROGMEM = {
  NOTE_D3H, 16,  TONES_END
};

const uint16_t spaceySoundNotes[] PROGMEM = {
  NOTE_D7H, 32,  NOTE_E7H, 32,  NOTE_GS7H, 32,
  NOTE_DS7H, 32,  NOTE_G7H, 32,  NOTE_CS7H, 32,  NOTE_FS7H, 32,
  NOTE_B6H, 32,  NOTE_DS7H, 32,  NOTE_C7H, 32,  NOTE_G7H, 32,
  NOTE_GS6H, 32,  NOTE_B7H, 32,  NOTE_F6H, 32,  NOTE_DS8H, 32,
  NOTE_CS6H, 32,
  TONES_END
};

const uint16_t potionSoundNotes[] PROGMEM = {
  NOTE_AS4H, 16,  NOTE_B4H, 16,  NOTE_C5H, 16,  NOTE_D5H, 16,
  NOTE_DS5H, 16,  NOTE_C5H, 16,  NOTE_C4H, 16,  NOTE_D4H, 16,
  NOTE_F4H, 16,  NOTE_GS4H, 16,  NOTE_FS4H, 16,  NOTE_D4H, 16,
  TONES_END
};
#endif

/*--------------------------------------------------------*/
void stepSound()
{
#ifndef NO_SOUND
  #ifdef _USE_ARDUBOY2_
    sound.tones(stepSoundNotes);
  #else
    Sound( 100, 1 );
    Sound( 200, 1 );
    _delay_ms( 100 );
  #endif
#endif
}

/*--------------------------------------------------------*/
void wallSound()
{
#ifndef NO_SOUND
  #ifdef _USE_ARDUBOY2_
    sound.tones(wallSoundNotes);
  #else
    Sound( 50, 1 );
    _delay_ms( 100 );
  #endif
#endif
}

/*--------------------------------------------------------*/
void swordSound()
{
#ifndef NO_SOUND
  #ifdef _USE_ARDUBOY2_
    sound.tones(swordSoundNotes);
  #else
    Sound( 50, 10 );
  #endif
#endif
}

/*--------------------------------------------------------*/
void potionSound()
{
#ifndef NO_SOUND
  #ifdef _USE_ARDUBOY2_
    sound.tones(potionSoundNotes);
  #else
    Sound( 100, 30 );
    _delay_ms( 40 );
    Sound( 100, 30 );
    _delay_ms( 20 );
    Sound( 150, 30 );
  #endif
#endif
}

/*--------------------------------------------------------*/
void switchSound()
{
#ifndef NO_SOUND
  #ifdef _USE_ARDUBOY2_
    sound.tones(switchSoundNotes);
  #else
    Sound( 50, 10 );
  #endif
#endif
}
