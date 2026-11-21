#include <Arduboy2.h>

#pragma once

struct TitleScreenVars {

    uint8_t delay = 0;
    uint8_t charIndex = 0;
    uint8_t frameIndex = 0;
    
    void reset() {

        // this->cursor = GameMode::English;

    }

};

struct StatisticsScreenVars {

    StatisticsMode mode = StatisticsMode::PageOne;
    uint8_t numberOfAttempts = 0;
    
    void reset() {

        this->mode = StatisticsMode::PageOne;

    }

};

struct KeyboardVars {

    uint8_t xCursor = 0;
    uint8_t yCursor = 0;
    uint8_t yPos = 33;

    KeyboardState state;
    KeyState keys[26];

    void reset() {

        for (uint8_t i = 0; i < 26; i++) {
            keys[i] = KeyState::Visible;
        }

    }

};

struct GuessesVar {

    char chars[6][5];
    GuessState state[6][5];
    uint8_t xCursor = 0;
    uint8_t yCursor = 0;
    int8_t listY = 0;

    void reset() {

        for (uint8_t y = 0; y < 6; y++) {

            for (uint8_t x = 0; x < 5; x++) {
                this->chars[y][x] = ' ';
                this->state[y][x] = GuessState::Dashed;
            }

        }

        this->xCursor = 0;
        this->yCursor = 0;
        this->listY = 0;

    }

};

struct GamePlayVars {

    GameMode mode = GameMode::English;
    KeyboardVars keyboard;
    GuessesVar guesses;

    uint8_t showInvalidWord_Count = 0;
    CheckState checkState = CheckState::Normal;
    uint8_t cancelButton = 0;

    #ifdef USE_BRINE
    char selectedWord[5] = { 'B', 'R', 'I', 'N', 'E' };
    #else
    char selectedWord[5] = { ' ', ' ', ' ', ' ', ' ' };
    #endif

};
