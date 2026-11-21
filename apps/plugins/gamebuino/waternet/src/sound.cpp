#include <stdint.h>
#include <string.h>
#include <Gamebuino-Meta.h>

#include "commonvars.h"
#include "sound.h"

//I (joyrider3774) created the music in this tool : https://onlinesequencer.net

uint8_t prev_music, music_on, sound_on;
int8_t track;

constexpr uint8_t sfxSustain = 100 * 15 / frameRate;

constexpr uint8_t pause1 = 15 * frameRate / 60;
constexpr uint8_t pause2 = 6  * frameRate / 60;
constexpr uint8_t pause3 = 14 * frameRate / 60;
constexpr uint8_t pause4 = 13 * frameRate / 60;
constexpr uint16_t trackEnd = 0;

constexpr uint8_t NOTE_REST = 63;
constexpr uint8_t NOTE_D6 = 4 + 24;
constexpr uint8_t NOTE_DS6 = 5 + 24;
constexpr uint8_t NOTE_E6 = 6 + 24;
constexpr uint8_t NOTE_F6 = 7 + 24;
constexpr uint8_t NOTE_C4 = 2;
constexpr uint8_t NOTE_CS4 = 3;
constexpr uint8_t NOTE_D4 = 4;
constexpr uint8_t NOTE_DS4 = 5;
constexpr uint8_t NOTE_E4 = 6;
constexpr uint8_t NOTE_F4 = 7;



#define setNote(note, dur) ((uint16_t)note<<2U) + (((uint16_t)dur)<<8U)

//https://onlinesequencer.net/2498607
const uint16_t music_levelsCleared[] ={
    setNote(NOTE_D6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_D6,pause4),
    setNote(NOTE_REST,pause4),
    setNote(NOTE_D6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_F6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_D6,pause4),
    setNote(NOTE_REST,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_REST,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_F6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_REST,pause4),
    setNote(NOTE_D6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_D6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_D6,pause4),
    setNote(NOTE_D6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_REST,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_REST,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_F6,pause4),
    setNote(NOTE_DS6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_REST,pause4),
    setNote(NOTE_F6,pause4),
    setNote(NOTE_E6,pause4),
    setNote(NOTE_F6,pause4),
    trackEnd
};

//https://onlinesequencer.net/2484974
const uint16_t music_won[] ={
  setNote(NOTE_DS6,pause2),
  setNote(NOTE_D6,pause2),
  setNote(NOTE_DS6,pause2),
  setNote(NOTE_E6,pause2),
  setNote(NOTE_DS6,pause2),
  setNote(NOTE_E6,pause2),
  setNote(NOTE_F6,pause2 << 2),
  setNote(NOTE_REST,pause2),
  trackEnd
};

//https://onlinesequencer.net/2485064
const uint16_t music_game[] ={ 
    setNote(NOTE_REST,pause3),
    setNote(NOTE_C4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_C4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_F4,pause3),
    setNote(NOTE_REST,pause3),

    setNote(NOTE_C4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_C4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_F4,pause3),
    setNote(NOTE_REST,pause3),

    setNote(NOTE_D4,pause3),
    setNote(NOTE_C4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_C4,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_C4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_C4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_C4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_F4,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_E4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_D4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_DS4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_CS4,pause3),
    setNote(NOTE_REST,pause3),
    setNote(NOTE_D4,pause3),
    trackEnd
};

//https://onlinesequencer.net/2484977
const uint16_t music_intro[] = {
    setNote(NOTE_C4,pause1),
    setNote(NOTE_C4,pause1 * 2),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_DS4,pause1),
    setNote(NOTE_C4,pause1),
    setNote(NOTE_REST,pause1 * 8),
    
    setNote(NOTE_C4,pause1),
    setNote(NOTE_C4,pause1 * 2),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_DS4,pause1),
    setNote(NOTE_C4,pause1),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_C4,pause1),
    setNote(NOTE_CS4,pause1),
    setNote(NOTE_REST,pause1 * 4),

    setNote(NOTE_C4,pause1),
    setNote(NOTE_C4,pause1 * 2),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_DS4,pause1),
    setNote(NOTE_C4,pause1),
    setNote(NOTE_REST,pause1 * 8),
    
    setNote(NOTE_C4,pause1),
    setNote(NOTE_C4,pause1 * 2),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_DS4,pause1),
    setNote(NOTE_C4,pause1),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_C4,pause1),
    setNote(NOTE_CS4,pause1),
    setNote(NOTE_REST,pause1 * 4),

    setNote(NOTE_D4,pause1),
    setNote(NOTE_D4,pause1), 
    setNote(NOTE_CS4,pause1),
    setNote(NOTE_C4,pause1),
    setNote(NOTE_DS4,pause1),
    setNote(NOTE_CS4,pause1),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_DS4,pause1),
    setNote(NOTE_CS4,pause1),
    setNote(NOTE_DS4,pause1),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_D4,pause1),
    setNote(NOTE_E4,pause1),
    setNote(NOTE_C4,pause1),
    setNote(NOTE_CS4,pause1),
    setNote(NOTE_E4,pause1),
    setNote(NOTE_D4,pause1 * 3),
    setNote(NOTE_REST,pause1 * 4),
    trackEnd
};

void setMusicOn(uint8_t value)
{
    music_on = value;
    if(music_on)
    {
        if (prev_music != 0)
            SelectMusic(prev_music, 1);
        else
        {
            //this can only happen on title screen
            //normally this does not belong here
            //but if music was off in eeprom no music was playing
            //before and you can only enable it in the main - options - menu
            //where title music plays
            if (gameState == gsTitle)
                SelectMusic(musTitle, 1);
        }
    }
    else
    {
        gb.sound.stop(track);
    }
}

void setSoundOn(uint8_t value)
{
    sound_on = value;
}

uint8_t isMusicOn()
{
    return music_on;
}

uint8_t isSoundOn()
{
    return sound_on;
}

void initSound()
{
    sound_on = 0;
}

void SelectMusic(uint8_t musicFile, uint8_t force)
{
    if (((prev_music != musicFile) || force) && music_on)
    {
        prev_music = musicFile;
        gb.sound.stop(track);

        switch (musicFile) 
        {
            case musTitle:
                track = gb.sound.play(music_intro, true);
                break;
            case musLevelClear:
                track = gb.sound.play(music_won);
                break;
            case musAllLevelsClear:
                track = gb.sound.play(music_levelsCleared, true);
                break;
            case musGame:
                track = gb.sound.play(music_game, true);
                break;
        }
    }
}

void playSound(uint16_t tone)
{
    if(!sound_on)
        return;
    gb.sound.tone(tone, sfxSustain);
}

void initMusic()
{
    music_on = 0;
    prev_music = 0;
    track = -1;
}

void playGameMoveSound()
{
    playSound(750);
}

void playErrorSound()
{
    playSound(210);
}

void playMenuSelectSound()
{
    playSound(1250);
}

void playMenuBackSound()
{
   playSound(1000);
}

void playMenuAcknowlege()
{
    playSound(900);
}

void playGameAction()
{
    playSound(600);
}
