#pragma once

#include "Arduboy2Ext.h"
#include "Enums.h"

class EEPROM_Utils {

    public: 

        EEPROM_Utils() {};
        
        static void initEEPROM(bool forceClear, GameMode gameMode, bool clearBoth);
        static void increaseCorrectWords(GameMode gameMode, uint8_t numberOfGuesses);
        static void resetWiningStreak(GameMode gameMode);
        static void setMode(GameMode gameMode);
        static GameMode getMode();
        static uint8_t getPercent(GameMode gameMode, uint8_t val);
        static uint16_t getPercentVal(GameMode gameMode, uint8_t val);
};


/* ----------------------------------------------------------------------------
 *   Is the EEPROM initialised?
 */
void EEPROM_Utils::initEEPROM(bool forceClear, GameMode gameMode, bool clearBoth) {

    byte c1 = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Char1));
    byte c2 = eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Char2));

    uint16_t zero = 0;

    if (forceClear || c1 != 'L' || c2 != 'D') { 

        eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Char1), 'L');
        eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Char2), 'D');
        eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Mode), static_cast<uint8_t>(gameMode));

        if ((forceClear && gameMode == GameMode::English) || clearBoth) { 

            EEPROM.put(Constants::EEPROM_Games_Won_EN, zero);
            EEPROM.put(Constants::EEPROM_Games_Played_EN, zero);
            EEPROM.put(Constants::EEPROM_Current_Streak_EN, zero);
            EEPROM.put(Constants::EEPROM_Max_Streak_EN, zero);

            for (uint8_t i = 0; i < 6; i++) {

                EEPROM.put(Constants::EEPROM_Distribution_EN + (i * 2), zero);

            }

        }

        if ((forceClear && gameMode == GameMode::French) || clearBoth) { 

            EEPROM.put(Constants::EEPROM_Games_Won_FR, zero);
            EEPROM.put(Constants::EEPROM_Games_Played_FR, zero);
            EEPROM.put(Constants::EEPROM_Current_Streak_FR, zero);
            EEPROM.put(Constants::EEPROM_Max_Streak_FR, zero);

            for (uint8_t i = 0; i < 6; i++) {

                EEPROM.put(Constants::EEPROM_Distribution_FR + (i * 2), zero);

            }

        }

    }

}

void EEPROM_Utils::increaseCorrectWords(GameMode gameMode, uint8_t numberOfGuesses) {

    uint16_t gamesWon = 0;
    uint16_t gamesPlayed = 0;
    uint16_t currentStreak = 0;
    uint16_t maxStreak = 0;
    uint16_t guesses = 0;

    if (gameMode == GameMode::English) {

        EEPROM.get(Constants::EEPROM_Games_Won_EN, gamesWon);
        EEPROM.get(Constants::EEPROM_Games_Played_EN, gamesPlayed);
        EEPROM.get(Constants::EEPROM_Current_Streak_EN, currentStreak);
        EEPROM.get(Constants::EEPROM_Max_Streak_EN, maxStreak);
        EEPROM.get(Constants::EEPROM_Distribution_EN + (numberOfGuesses * 2), guesses);

    }
    else {

        EEPROM.get(Constants::EEPROM_Games_Won_FR, gamesWon);
        EEPROM.get(Constants::EEPROM_Games_Played_FR, gamesPlayed);
        EEPROM.get(Constants::EEPROM_Current_Streak_FR, currentStreak);
        EEPROM.get(Constants::EEPROM_Max_Streak_FR, maxStreak);
        EEPROM.get(Constants::EEPROM_Distribution_FR + (numberOfGuesses * 2), guesses);

    }

    gamesWon++;
    gamesPlayed++;
    currentStreak++;
    guesses++;

    if (currentStreak > maxStreak) maxStreak = currentStreak;

    if (gameMode == GameMode::English) {

        EEPROM.put(Constants::EEPROM_Games_Won_EN, gamesWon);
        EEPROM.put(Constants::EEPROM_Games_Played_EN, gamesPlayed);
        EEPROM.put(Constants::EEPROM_Current_Streak_EN, currentStreak);
        EEPROM.put(Constants::EEPROM_Max_Streak_EN, maxStreak);
        EEPROM.put(Constants::EEPROM_Distribution_EN + (numberOfGuesses * 2), guesses);

    }
    else {

        EEPROM.put(Constants::EEPROM_Games_Won_FR, gamesWon);
        EEPROM.put(Constants::EEPROM_Games_Played_FR, gamesPlayed);
        EEPROM.put(Constants::EEPROM_Current_Streak_FR, currentStreak);
        EEPROM.put(Constants::EEPROM_Max_Streak_FR, maxStreak);
        EEPROM.put(Constants::EEPROM_Distribution_FR + (numberOfGuesses * 2), guesses);

    }

}

void EEPROM_Utils::resetWiningStreak(GameMode gameMode) {

    uint16_t gamesPlayed = 0;
    uint16_t currentStreak = 0;

    if (gameMode == GameMode::English) {

        EEPROM.get(Constants::EEPROM_Games_Played_EN, gamesPlayed);
        gamesPlayed++;
        EEPROM.put(Constants::EEPROM_Games_Played_EN, gamesPlayed);
        EEPROM.put(Constants::EEPROM_Current_Streak_EN, currentStreak);

    }
    else {

        EEPROM.get(Constants::EEPROM_Games_Played_FR, gamesPlayed);
        gamesPlayed++;
        EEPROM.put(Constants::EEPROM_Games_Played_FR, gamesPlayed);
        EEPROM.put(Constants::EEPROM_Current_Streak_FR, currentStreak);

    }

}

void EEPROM_Utils::setMode(GameMode gameMode) {

    eeprom_update_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Mode), static_cast<uint8_t>(gameMode));

}

GameMode EEPROM_Utils::getMode() {

    return static_cast<GameMode>(eeprom_read_byte(reinterpret_cast<uint8_t *>(Constants::EEPROM_Mode)));

}

uint8_t EEPROM_Utils::getPercent(GameMode gameMode, uint8_t val) {

    uint16_t high = 0;
    uint16_t guesses = 0;
    uint16_t thisGuess = 0;

    switch (gameMode) {

        case GameMode::English:

            for (uint8_t i = 0; i < 6; i++) {

                EEPROM.get(Constants::EEPROM_Distribution_EN + (i * 2), guesses);
                if (guesses > high) high = guesses;
                if (i == val) thisGuess = guesses;
            }

            if (high == 0) return 0;

            return (thisGuess * 110 / high);

        case GameMode::French:

            for (uint8_t i = 0; i < 6; i++) {

                EEPROM.get(Constants::EEPROM_Distribution_FR + (i * 2), guesses);
                if (guesses > high) high = guesses;
                if (i == val) thisGuess = guesses;

            }

            if (high == 0) return 0;

            return (thisGuess * 110 / high);
    }

    return 0;

}

uint16_t EEPROM_Utils::getPercentVal(GameMode gameMode, uint8_t val) {

    uint16_t score = 0;

    switch (gameMode) {

        case GameMode::English:
            EEPROM.get(Constants::EEPROM_Distribution_EN + (val * 2), score);
            return score;

        case GameMode::French:
            EEPROM.get(Constants::EEPROM_Distribution_FR + (val * 2), score);
            return score;

    }

    return 0;

}