#pragma once

#include <LibAudio>

/* Rockbox port: the sound effects are raw unsigned 8-bit 16 kHz files below
 * the game's data dir (.rockbox/rocks/games/pokitto_leword/), written by
 * tools/leword_data.py. The theme volume fades are not applied. The three
 * theme tunes are not shipped (no licence): the music settings play nothing. */
static constexpr Audio::Clip sfx_Success = { "sfx/Success.raw" };
static constexpr Audio::Clip sfx_Ding_00 = { "sfx/Ding_00.raw" };
static constexpr Audio::Clip sfx_Ding_01 = { "sfx/Ding_01.raw" };
static constexpr Audio::Clip sfx_Ding_02 = { "sfx/Ding_02.raw" };
static constexpr Audio::Clip sfx_Ding_03 = { "sfx/Ding_03.raw" };

struct Sounds {

    File soundEffectFile;

    uint8_t themeToPlay_Next = 255;
    SoundSettings soundSettings_Next;
    FadeMode fadeMode = FadeMode::None;

    static inline uint16_t volume = 255;

    void connectVolumeControl(uint32_t) {}

    enum class Effects : uint8_t {
        Success,
        Ding_00,
        Ding_01,
        Ding_02,
        Ding_03,
    };

    void updateFades() {

        switch (this->fadeMode) {

            case FadeMode::FadeIn:
                volume = volume + 8;
                if (volume >= 247) {
                    volume = 255;
                    this->fadeMode = FadeMode::None;
                }
                break;

            case FadeMode::FadeOut:

                volume = volume - 8;

                if (volume <= 8) {
                    
                    this->volume = 0;

                    if (this->themeToPlay_Next != 255) {
                        this->fadeMode = FadeMode::FadeIn;
                        this->playTheme(this->themeToPlay_Next, this->soundSettings_Next);
                    }
                    else {
                        this->fadeMode = FadeMode::None;
                    }

                }
                break;

            default: break;

        }

    }



    // Sound effects.

    void playTheme(uint8_t themeToPlay, SoundSettings soundSettings, bool ignoreFadeOut = false, bool ensureFullVolume = false) {


        if (volume > 0 && !ignoreFadeOut) {

            this->fadeMode = FadeMode::FadeOut;
            this->themeToPlay_Next = themeToPlay;
            this->soundSettings_Next = soundSettings;

        }
        else {

            this->themeToPlay_Next = 255;

            if (ensureFullVolume) volume = 255;

            switch (soundSettings) {

                case SoundSettings::Music:
                case SoundSettings::Both:

                    // The tunes were removed from the port (licence unclear): no music.
                    break;

                case SoundSettings::SFX:
                case SoundSettings::None:
        
                    Audio::stop<0>();
                    break;

            }

        }

    }

    void muteTheme() {

        Audio::stop<0>();        
        
    }

    void playSoundEffect(Sounds::Effects soundEffect, SoundSettings soundSettings) {

        switch (soundSettings) {

            case SoundSettings::SFX:
            case SoundSettings::Both:

                switch (soundEffect) {
                    
                    case Sounds::Effects::Success:
                        Audio::play<2>(sfx_Success, 255, 1);        
                        break;
                    
                    case Sounds::Effects::Ding_00:
                        Audio::play<2>(sfx_Ding_00, 255, 1);        
                        break;
                    
                    case Sounds::Effects::Ding_01:
                        Audio::play<2>(sfx_Ding_01, 255, 1);        
                        break;
                    
                    case Sounds::Effects::Ding_02:
                        Audio::play<2>(sfx_Ding_02, 255, 1);        
                        break;
                    
                    case Sounds::Effects::Ding_03:
                        Audio::play<2>(sfx_Ding_03, 255, 1);        
                        break;

                }

                break;

            default: break;

        }

    }    
    
};