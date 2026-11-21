/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _SOUND_MANAGER_H_
#define _SOUND_MANAGER_H_

#include <Arduboy2.h>
#include <ArduboyTones.h>
#include "abstract/Singleton.h"
#include "Constants.h"

typedef enum __attribute__ ((packed))
{
    PLAYER_HIT,
    PLAYER_FIRE,
    HAPPY_SOUND,
    UP,
    DOWN,
    OK,
    LETTER,
    DEATH
} Sound;

typedef enum __attribute__ ((packed))
{
    MINOR_THIRD     = 0b10000000, //if not set -> major
    MINOR_7TH       = 0b01000000,
    SUS_2           = 0b00100000,
    SUS_4           = 0b00010000,
    SIXTH           = 0b00001000,
    ADD_2           = 0b00000100, //if neither add_2, add_4 or add_5 -> octave
    ADD_4           = 0b00000010,
    ADD_5           = 0b00000001
} ChordModesMasks;

class SoundManager : public Singleton<SoundManager>
{
    public:
        SoundManager(Arduboy2 * arduboy);
        void init();
        void startMusic(short baseNote = 69, short speed = 10, short noteDuration = 24, short sequenceRepeat = 4);
        void resumeMusic();
        void playSound(Sound sound);
        void stopMusic();
        void playNextNote();
        void update();
        void startNoteBurst();
    private:
        void gotoNextSequence();
        int8 computePositionInChord();
        void playNote(short midiNote);

        void setSpeed(int8 speed);
        ArduboyTones tones;

        int8 nbSequences;
        int8 currentSeqTick;
        int8 currentSequence;
        int8 currentSoundPlaying;

        byte currentChordModes;
        int8 currentBaseMidiNote;
        int8 currentOrderInChord;
        int8 currentSpeed;

        bool musicOn = false;
        unsigned long nextNoteMs = 0;
        unsigned long noteIntervalMs = 100;

        int8 noteDuration;
        int8 sequenceRepeat;
};

#endif
