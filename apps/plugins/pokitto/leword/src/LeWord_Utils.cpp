#include "LeWord.h"
#include "Pokitto.h"
#include "images/Images.h"
#include "utils/Constants.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

void Game::moveCursor(Direction direction) {

    switch (direction) {

        case Direction::Right:
    
            switch (this->gamePlayVars.keyboard.yCursor) {

                case 0:
                    if (this->gamePlayVars.keyboard.xCursor < 9) {
                        
                        this->gamePlayVars.keyboard.xCursor++;

                    }
                    else {

                        this->gamePlayVars.keyboard.xCursor = 0;

                    }
                    break;

                case 1:
                case 2:
                    if (this->gamePlayVars.keyboard.xCursor < 8) {
                        
                        this->gamePlayVars.keyboard.xCursor++;

                    }
                    else {

                        this->gamePlayVars.keyboard.xCursor = 0;

                    }
                    break;

            }

            break;

        case Direction::Left:
    
            switch (this->gamePlayVars.keyboard.yCursor) {

                case 0:
                    if (this->gamePlayVars.keyboard.xCursor > 0) {

                        this->gamePlayVars.keyboard.xCursor--;

                    }
                    else {

                        this->gamePlayVars.keyboard.xCursor = 9;

                    }

                    break;

                case 1:
                case 2:
                    if (this->gamePlayVars.keyboard.xCursor > 0) {

                        this->gamePlayVars.keyboard.xCursor--;

                    }
                    else {

                        this->gamePlayVars.keyboard.xCursor = 8;

                    }

                    break;

            }
             
            break;

        case Direction::Down:
    
            switch (this->gamePlayVars.keyboard.yCursor) {

                case 0:
                    if (this->gamePlayVars.keyboard.xCursor == 9) {
                        
                        this->gamePlayVars.keyboard.yCursor++;
                        this->gamePlayVars.keyboard.xCursor--;

                    }
                    else {
                        this->gamePlayVars.keyboard.yCursor++;
                    }
                    break;

                case 1:
                    this->gamePlayVars.keyboard.yCursor++;
                    break;

            }

            break;

        case Direction::Up:
    
            if (this->gamePlayVars.keyboard.yCursor == 0) {

                this->gamePlayVars.keyboard.state = KeyboardState::StartHiding;
                break;

            }
            else {

                switch (this->gamePlayVars.keyboard.yCursor) {

                    case 0:
                    case 2:
                        if (this->gamePlayVars.keyboard.xCursor == 9) {
                            this->gamePlayVars.keyboard.yCursor--;
                            this->gamePlayVars.keyboard.xCursor--;
                        }
                        else {
                            this->gamePlayVars.keyboard.yCursor--;
                        }
                        break;

                    case 1:
                        this->gamePlayVars.keyboard.yCursor--;
                        break;


                }


            }

            break;


    }
    
}

CheckState Game::checkWord() {

    uint8_t guessWord[5];
    uint8_t testWord[5];
    uint8_t answer[5];

    for (uint8_t i = 0; i < 5; i++) {
        guessWord[i] = this->gamePlayVars.guesses.chars[this->gamePlayVars.guesses.yCursor][i];
        answer[i] = this->gamePlayVars.selectedWord[i];
        this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][i] = GuessState::Incorrect;
    }



    // Is the word correct?

    bool isCorrectWord = true;

    for (uint8_t i = 0; i < 5; i++) {
        if (guessWord[i] != answer[i]) {
            isCorrectWord = false;
        }
    }
    
    if (isCorrectWord) {

        for (uint8_t i = 0; i < 5; i++) {
            this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][i] = GuessState::Correct; 
        }

        return CheckState::CorrectWord;

    }
 

    // QWERT?

    if (guessWord[0] == 81 && guessWord[1] == 87 && guessWord[2] == 69 && guessWord[3] == 82 && guessWord[4] == 84) {

        return CheckState::InvalidWord_Hanski;

    }



    // Get start of words using char one and two of guess ..

    uint32_t alphaStart = 0;
    uint16_t startPos = ((guessWord[0] - 65) * 26) + (guessWord[1] - 65);
    char status = ' ';


    switch (this->gamePlayVars.mode) {

        case GameMode::English:
            alphaStart = Dictionary::English_AlphaMap[startPos];
            break;

        case GameMode::French:
            alphaStart = Dictionary::French_AlphaMap[startPos];
            break;

    }

    
    // No matching letters?

    if (alphaStart == 0xFFFF) {

        return CheckState::InvalidWord; 

    }

    alphaStart = alphaStart * 6;

    while (true) {

        file.seek(alphaStart);
        file.read(&status, 1);
        file.read(&testWord, 5);

        alphaStart = alphaStart + 6;

        if (testWord[0] > guessWord[0] || (testWord[0] == guessWord[0] && testWord[1] > guessWord[1])) {
            
            return CheckState::InvalidWord;

        }


        // Valid word!  What are correct?

        bool isValid = true;

        for (uint8_t i = 0; i < 5; i++) {

            if (testWord[i] != guessWord[i]) {
                isValid = false;
                break;
            };

        }

        if (isValid) {

            
            // Correct letters ..

            for (uint8_t i = 0; i < 5; i++) {

                if (guessWord[i] == answer[i]) { 
                    this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][i] = GuessState::Correct; 
                    answer[i] = ' ';
                }

            }

            for (uint8_t i = 0; i < 5; i++) {

                if (guessWord[i] != this->gamePlayVars.selectedWord[i]) { 

                    for (uint8_t j = 0; j < 5; j++) {

                        if (this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][j] != GuessState::Correct && testWord[i] == answer[j]) { 

                            this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][i] = GuessState::WrongPosition;
                            answer[j] = ' ';
                            break;
                    
                        }
                    
                    }


                }

            }


            // If the letter is not found but already exists, we cannot remove it from the keyboard ..

            for (uint8_t i = 0; i < 5; i++) {

                if (this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][i] == GuessState::Incorrect) {

                    for (uint8_t j = 0; j < 5; j++) {

                        if (guessWord[i] == this->gamePlayVars.selectedWord[j]) { 

                            this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][i] = GuessState::Duplicate;
                    
                        }
                    
                    }

                }

            }


            // Disable the keys for letters not found in solution ..

            uint8_t letterCount = 0;

            for (uint8_t i = 0; i < 5; i++) {

                switch (this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][i]) {

                    case GuessState::Incorrect: 
                        this->gamePlayVars.keyboard.keys[guessWord[i] - 65] = KeyState::Invisible;
                        break;

                    case GuessState::Correct: 
                        this->gamePlayVars.keyboard.keys[guessWord[i] - 65] = KeyState::Correct;
                        letterCount++;
                        break;

                    case GuessState::WrongPosition: 
                        if (this->gamePlayVars.keyboard.keys[guessWord[i] - 65] == KeyState::Visible) {
                            this->gamePlayVars.keyboard.keys[guessWord[i] - 65] = KeyState::WrongPosition;
                        }
                        letterCount++;
                        break;
            
                }

            }


            // Clear duplicate marks ..

            for (uint8_t j = 0; j < 5; j++) {

                if (this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][j] == GuessState::Duplicate) { 

                    this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][j] = GuessState::Incorrect;
            
                }
            
            }


            // Play tune ..

            switch (letterCount) {

                case 1:
                    this->sounds.playSoundEffect(Sounds::Effects::Ding_00, this->cookie->sfx);
                    break;

                case 2:
                    this->sounds.playSoundEffect(Sounds::Effects::Ding_01, this->cookie->sfx);
                    break;

                case 3:
                    this->sounds.playSoundEffect(Sounds::Effects::Ding_02, this->cookie->sfx);
                    break;

                case 4:
                    this->sounds.playSoundEffect(Sounds::Effects::Ding_03, this->cookie->sfx);
                    break;

            }

            return CheckState::RealWord;

        }

    }

}
