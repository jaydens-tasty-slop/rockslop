#include "LeWord.h"
#include "Pokitto.h"
#include "images/Images.h"
#include "utils/Constants.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

using PC = Pokitto::Core;
using PD = Pokitto::Display;

void Game::setup(GameCookie *cookie) {

    this->cookie = cookie;
//    this->sounds.connectVolumeControl(1);

    for (uint8_t i = 0; i < Constants::NumberOfLetters; i++) {
        this->letters[i].init();
    }

}

void Game::loop() {

    PC::buttons.pollButtons();
    PD::clear();

    switch (this->gameState) {

        case GameState::SplashScreen_Init:

            this->splashScreen_Init();
            [[fallthrough]]

        case GameState::SplashScreen:

            this->splashScreen();
            break;

        case GameState::Title_Init:

            this->sounds.playTheme(this->cookie->track, this->cookie->sfx, true, true);
            this->title_Init();
            this-> gameState = GameState::Title;
            [[fallthrough]]

        case GameState::Title:

            this->renderBackground();
            this->title();
            break;

        case GameState::Game_Init:

            this->game_Init();
            this-> gameState = GameState::Game_Play;
            [[fallthrough]]

        case GameState::Game_Play:

            this->renderBackground();
            this->game();
            break;

        case GameState::Pause_Init:

            this->pause_Init();
            this-> gameState = GameState::Pause;
            [[fallthrough]]

        case GameState::Pause:

            this->renderBackground();
            this->pause();
            break;

        case GameState::Stats_Init:

            this->statistics_Init();
            [[fallthrough]]

        case GameState::Stats:

            this->renderBackground();
            this->statistics();
            break;

    }

    this->sounds.updateFades();


}

void Game::renderBackground() {

    for (uint8_t i = 0; i < Constants::NumberOfLetters; i++) {

        Letter &letter = this->letters[i];

        switch (letter.direction) {

            case Direction::Left:

                switch (letter.speed) {

                    case 0:
                        letter.x--;
                        break;

                    case 1:
                        if (PC::frameCount % 3 <= 1) {
                            letter.x--;
                        }
                        break;

                    case 2:
                        if (PC::frameCount % 2 == 0) {
                            letter.x--;
                        }
                        break;
                }

                if (letter.x < -32) {
                    letter.size = random(0, 2);
                    letter.y = random(-32, 88);
                    letter.x =  random(112, 512);
                    letter.speed = random(0, 3);    
                    letter.image = random(0, 32);
                }
                break;

            case Direction::Right:

                switch (letter.speed) {

                    case 0:
                        letter.x++;
                        break;

                    case 1:
                        if (PC::frameCount % 3 <= 1) {
                            letter.x++;
                        }
                        break;

                    case 2:
                        if (PC::frameCount % 2 == 0) {
                            letter.x++;
                        }
                        break;

                }

                if (letter.x >110) {
                    letter.size = random(0, 2);
                    letter.y = random(-32, 88);
                    letter.x =  random(-512, 0);
                    letter.speed = random(0, 3);                
                    letter.image = random(0, 32);
                }

                break;

        }

        PD::drawBitmap(letter.x, letter.y, Images::Letters[letter.image]);

    }

}