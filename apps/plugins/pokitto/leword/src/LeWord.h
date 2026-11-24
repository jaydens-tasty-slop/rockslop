#pragma once

#include "Pokitto.h"
#include <LibAudio>
#include <LibSchedule>
#include "images/Images.h"
#include "utils/Enums.h"
#include "utils/GameCookie.h"
#include "utils/Structs.h"
#include "src/sounds/Sounds.h"
#include "utils/Utils.h"
#include "dictionary/Dictionary.h"

using PC=Pokitto::Core;
using PD=Pokitto::Display;
using PB=Pokitto::Buttons;

class Game {

    private:
        TitleScreenVars titleScreenVars;
        SplashScreenVars splashScreenVars;
        StatisticsScreenVars statisticsScreenVars;

        GamePlayVars gamePlayVars;
        GameState gameState = GameState::SplashScreen_Init;
        GameCookie *cookie;

        File file;
        Sounds sounds;
        Letter letters[Constants::NumberOfLetters];
        
    public:
        void setup(GameCookie *cookie);
        void loop();

    private:
        void splashScreen_Init();
        void splashScreen();
        void game_Init();
        void game();
        void pause_Init();
        void pause();
        void drawSolution();
        void drawGuesses(int8_t yOffset);
        void drawKeyboard(uint8_t xOffset, uint8_t yOffset);
        void statistics_Init();
        void statistics(); 
        void title_Init();
        void title(); 
        void renderScreen(); 
        void renderBackground(); 
        void moveCursor(Direction direction);
        uint8_t textWidth(uint16_t number);
        CheckState checkWord();
};

