#include "LeWord.h"
#include "Pokitto.h"
#include "images/Images.h"
#include "utils/Constants.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

void Game::title_Init() {

    this->gameState = GameState::Title;

    titleScreenVars.delay = random(16, 48);

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void Game::title() { 

    if (PC::buttons.pressed(BTN_A)) {

        switch (titleScreenVars.index) {

            case TitleScreenMode::English:
                this->gamePlayVars.mode = GameMode::English; 
                this->cookie->setMode(this->gamePlayVars.mode);
                this->gameState = GameState::Game_Init; 
                this->sounds.fadeMode = FadeMode::FadeOut;
                break;

            case TitleScreenMode::French:
                this->gamePlayVars.mode = GameMode::French; 
                this->cookie->setMode(this->gamePlayVars.mode);
                this->gameState = GameState::Game_Init; 
                this->sounds.fadeMode = FadeMode::FadeOut;
                break;

            case TitleScreenMode::EnglishStats:
                this->gamePlayVars.mode = GameMode::English; 
                this->statisticsScreenVars.numberOfAttempts = 7;
                this->gameState = GameState::Stats_Init; 
                break;

            case TitleScreenMode::FrenchStats:
                this->gamePlayVars.mode = GameMode::French; 
                this->statisticsScreenVars.numberOfAttempts = 7;
                this->gameState = GameState::Stats_Init; 
                break;

        }        

    }

    if (PC::buttons.pressed(BTN_LEFT)) {
        
        if (titleScreenVars.index != TitleScreenMode::English) {

            titleScreenVars.index--;
    
        }

    }

    if (PC::buttons.pressed(BTN_RIGHT)) {
        
        if (titleScreenVars.index != TitleScreenMode::SoundEffects) {
            
             titleScreenVars.index++;

        }

    }

    if (PC::buttons.pressed(BTN_UP)) {
    
        switch (titleScreenVars.index) {

            case TitleScreenMode::EnglishStats:
                titleScreenVars.index = TitleScreenMode::English;
                break;

            case TitleScreenMode::FrenchStats:
                titleScreenVars.index = TitleScreenMode::French;
                break;

            case TitleScreenMode::SoundEffects:

                this->cookie->sfx--;
                this->cookie->saveCookie();

                if (this->cookie->sfx != SoundSettings::Both && this->cookie->sfx != SoundSettings::Music) {

                    this->sounds.muteTheme();
                    
                }
                else {

                    this->sounds.playTheme(this->cookie->track, this->cookie->sfx);

                }

                break;

        }                

    }
    else if (PC::buttons.pressed(BTN_DOWN)) {

        switch (titleScreenVars.index) {

            case TitleScreenMode::English:
                titleScreenVars.index = TitleScreenMode::EnglishStats;
                break;

            case TitleScreenMode::French:
                titleScreenVars.index = TitleScreenMode::FrenchStats;
                break;

            case TitleScreenMode::FrenchStats:
                titleScreenVars.index = TitleScreenMode::SoundEffects;
                break;

            case TitleScreenMode::SoundEffects:

                this->cookie->sfx++;
                this->cookie->saveCookie();

                if (this->cookie->sfx != SoundSettings::Both && this->cookie->sfx != SoundSettings::Music) {

                    this->sounds.muteTheme();
                    
                }
                else {

                    this->sounds.playTheme(this->cookie->track, this->cookie->sfx);
                    
                }

                break;

        }        
        
    }

    uint8_t indexes[6] = { 0, 0, 0, 0, 0, 0, };

    if (titleScreenVars.delay == 0) {

        indexes[titleScreenVars.charIndex] = titleScreenVars.frameIndex - 1;

    }
    else {

        titleScreenVars.delay--;

        if (titleScreenVars.delay == 0) {

            titleScreenVars.charIndex = random(0, 6);
            titleScreenVars.frameIndex = 19;
            
        }

    }

    PD::drawBitmap(5, 26, Images::TitleScreen_L[indexes[0]]);
    PD::drawBitmap(22, 26, Images::TitleScreen_E[indexes[1]]);
    PD::drawBitmap(39, 26, Images::TitleScreen_W[indexes[2]]);
    PD::drawBitmap(56, 26, Images::TitleScreen_O[indexes[3]]);
    PD::drawBitmap(73, 26, Images::TitleScreen_R[indexes[4]]);
    PD::drawBitmap(90, 26, Images::TitleScreen_D[indexes[5]]);

    PD::drawBitmap(0, 57, titleScreenVars.index != TitleScreenMode::SoundEffects ? Images::TitleScreen_Lower: Images::TitleScreen_Lower_Inactive);

    switch (titleScreenVars.index) {

        case TitleScreenMode::English:
            PD::drawBitmap(7, 57, Images::Pointer);
            break;

        case TitleScreenMode::French:
            PD::drawBitmap(65, 57, Images::Pointer);
            break;

        case TitleScreenMode::EnglishStats:
            PD::drawBitmap(7, 67, Images::Pointer);
            break;

        case TitleScreenMode::FrenchStats:
            PD::drawBitmap(65, 67, Images::Pointer);
            break;


    }        

    switch (this->cookie->sfx) {

        case SoundSettings::Music:
            PD::drawBitmap(89, 79, titleScreenVars.index == TitleScreenMode::SoundEffects ? Images::Sound_Music_White: Images::Sound_Music_Inactive);
            break;

        case SoundSettings::SFX:
            PD::drawBitmap(89, 79, titleScreenVars.index == TitleScreenMode::SoundEffects ? Images::Sound_SFX_White: Images::Sound_SFX_Inactive);
            break;

        case SoundSettings::Both:
            PD::drawBitmap(89, 79, titleScreenVars.index == TitleScreenMode::SoundEffects? Images::Sound_Both_White: Images::Sound_Both_Inactive);
            break;

        default:
            PD::drawBitmap(89, 79, titleScreenVars.index == TitleScreenMode::SoundEffects ? Images::Sound_None_White: Images::Sound_None_Inactive);
            break;

    }

    if (titleScreenVars.frameIndex > 0) {
    
        titleScreenVars.frameIndex--;
    
        if (titleScreenVars.frameIndex == 0) {

            titleScreenVars.delay = random(32, 200);

        }

    }

}
