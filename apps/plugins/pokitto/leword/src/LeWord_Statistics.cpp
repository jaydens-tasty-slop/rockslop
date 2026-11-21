#include "LeWord.h"
#include "Pokitto.h"
#include "images/Images.h"
#include "utils/Constants.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

void Game::statistics_Init() {

    this->statisticsScreenVars.reset();
    this->gameState = GameState::Stats;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void Game::statistics() { 

    uint16_t gamesWon = 0;
    uint16_t gamesPlayed = 0;
    uint16_t currentStreak = 0;
    uint16_t maxStreak = 0;
    uint16_t percent = 0;

    if (this->gamePlayVars.mode == GameMode::English) {

        gamesWon = this->cookie->gamesWon_EN;
        gamesPlayed = this->cookie->gamesPlayed_EN;
        currentStreak = this->cookie->currentStreak_EN;
        maxStreak = this->cookie->maxStreak_EN;

    }
    else {

        gamesWon = this->cookie->gamesWon_FR;
        gamesPlayed = this->cookie->gamesPlayed_FR;
        currentStreak = this->cookie->currentStreak_FR;
        maxStreak = this->cookie->maxStreak_FR;

    }

    percent = (gamesPlayed == 0 ? 0 : (gamesWon * 100) / gamesPlayed);

    if (PC::buttons.pressed(BTN_A)) {
        
        switch (this->statisticsScreenVars.mode) {

            case StatisticsMode::PageOne:
                this->statisticsScreenVars.mode = StatisticsMode::PageTwo;
                break;

            case StatisticsMode::PageTwo:
                this->gameState = GameState::Title_Init; 
                break;

        }

    }



    // Press B to reset ..

    if (PC::buttons.repeat(BTN_B, 1)) {

        this->gamePlayVars.cancelButton++;

        if (this->gamePlayVars.cancelButton == 64) {
            this->cookie->initialise(this->gamePlayVars.mode);
        }

    }
    else {

        this->gamePlayVars.cancelButton = false;

    }

        
    switch (this->statisticsScreenVars.mode) {

        case StatisticsMode::PageOne:

            if (this->gamePlayVars.mode == GameMode::English) {
                PD::drawBitmap(0, 9, Images::Statistics_Header_EN);
                PD::drawBitmap(0, 22, Images::Statistics_EN);
            }
            else {
                PD::drawBitmap(0, 9, Images::Statistics_Header_FR);
                PD::drawBitmap(0, 22, Images::Statistics_FR);
            }

            PD::setColor(Constants::White);
            PD::setCursor(16, 29);
            if (gamesPlayed < 100) { PD::print("0"); }
            if (gamesPlayed < 10)  { PD::print("0"); }
            PD::print(gamesPlayed);

            PD::setCursor(71, 29);
            if (percent < 100) { PD::print("0"); }
            if (percent < 10)  { PD::print("0"); }
            PD::print(percent);

            PD::setCursor(16, 54);
            if (currentStreak < 100) { PD::print("0"); }
            if (currentStreak < 10)  { PD::print("0"); }
            PD::print(currentStreak);

            PD::setCursor(71, 54);
            if (maxStreak < 100) { PD::print("0"); }
            if (maxStreak < 10)  {PD::print("0"); }
            PD::print(maxStreak);

            break;

        case StatisticsMode::PageTwo:

            if (this->gamePlayVars.mode == GameMode::English) {
                PD::drawBitmap(0, 9, Images::Statistics_Header_EN);
            }
            else {
                PD::drawBitmap(0, 9, Images::Statistics_Header_FR);
            }

            PD::drawBitmap(2, 29, Images::Statistics_Vert);

            for (uint8_t i = 0; i < 6; i++) {

                uint8_t width = this->cookie->getPercent(this->gamePlayVars.mode, i);
                uint16_t val = this->cookie->getPercentVal(this->gamePlayVars.mode, i);
                uint8_t textWidth = this->textWidth(val);

                if (val != 0 && textWidth > width) width = textWidth + 1;

                if (i + 1 == this->statisticsScreenVars.numberOfAttempts) {
                    PD::setColor(Constants::Orange, Constants::Black);
                }
                else {
                    PD::setColor(Constants::DarkGrey, Constants::Black);
                }

                PD::fillRect(12, 29 + (i * 8), width == 0 ? 1 : width, 6);

                if (val > 0) {

                    uint8_t digits[5];
                    Utils::extractDigits(digits, val);
                    bool firstDigitRendered = false;

                    for (int8_t j = 4; j >= 0; j--) {

                        if (digits[j] != 0 || firstDigitRendered) {

                            PD::drawBitmap(12 + width - (j * 4) - 5, 29 + (i * 8), Images::Stats[digits[j]]);
                            firstDigitRendered = true;

                        }

                    }

                }

            }

            break;

    }

}

uint8_t Game::textWidth(uint16_t number) {

    if (number >= 100000) return 6 * 4;
    if (number >= 10000) return 5 * 4;
    if (number >= 1000) return 4 * 4;
    if (number >= 100) return 3 * 4;
    if (number >= 10) return 2 * 4;
    if (number >= 1) return 1 * 4;
    return 0;

}