
#include "LeWord.h"
#include "Pokitto.h"
#include "images/Images.h"
#include "utils/Constants.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;


void Game::game_Init() {

    this->gamePlayVars.checkState = CheckState::Normal;
    this->gamePlayVars.cancelButton = 0;
    this->gamePlayVars.keyboard.reset();
    this->gamePlayVars.guesses.reset();

    this->gameState = GameState::Game_Play;

    char status = ' ';
    

    // Pick a random word ..

    #ifndef USE_BRINE

        switch (this->gamePlayVars.mode) {

            case GameMode::English:
                {
                    this->file.openRO("music/LeWord00.img");

                    while (true) {

                        uint32_t word = random(0, Dictionary::English_NumberOfWords) * 6;   
// printf("word : %i\n", word);                        
                        this->file.seek(word);
                        this->file.read(&status, 1);
                        this->file.read(&this->gamePlayVars.selectedWord, 5);

// for (uint8_t i = 0; i < 5; i++) {
// printf("%i ", this->gamePlayVars.selectedWord[i]);
// }        
// printf("\n");

                        if (status == '1') break;

                    }

                }
                break;

            case GameMode::French:
                {
                    file.openRO("music/LeWord01.img");

                    while (true) {

                        uint32_t word = random(0, Dictionary::French_NumberOfWords) * 6;   
// printf("word : %i\n", word);                        
                        this->file.seek(word);
                        this->file.read(&status, 1);
                        this->file.read(&this->gamePlayVars.selectedWord, 5);

                        if (status == '1') break;

                    }

                    this->file.openRO("/music/LeWord01.img");

                }
                break;

        }                

    #else

        switch (this->gamePlayVars.mode) {

            case GameMode::English:
                this->file.openRO("music/LeWord00.img");
                break;

            case GameMode::French:
                this->file.openRO("music/LeWord01.img");
                break;

        }                


    #endif

}

void Game::game() {


    // Press B to quit ..

    if (PC::buttons.repeat(BTN_B, 1)) {

        this->gamePlayVars.cancelButton++;

        if (this->gamePlayVars.cancelButton == 64) {
            this->gamePlayVars.checkState = CheckState::Quit;
        }

    }
    else {

        this->gamePlayVars.cancelButton = false;

    }


    // Pause?

    if (PC::buttons.released(BTN_C)) {

        this->gameState = GameState::Pause_Init;

    }


    switch (this->gamePlayVars.checkState) {

        case CheckState::InvalidWord:
        case CheckState::InvalidWord_Hanski:
            if (PC::buttons.pressed(BTN_A) || PC::buttons.pressed(BTN_B)) {
                this->gamePlayVars.checkState = CheckState::Normal;
            }
            break;

        case CheckState::CorrectWord:
            if (PC::buttons.pressed(BTN_A) || PC::buttons.pressed(BTN_B)) {
                this->cookie->increaseCorrectWords(this->gamePlayVars.mode, this->gamePlayVars.guesses.yCursor);
                this->statisticsScreenVars.numberOfAttempts = this->gamePlayVars.guesses.yCursor + 1;
                this->gameState = GameState::Stats_Init;
            }
            break;

        case CheckState::TooManyAttempts:
            if (PC::buttons.pressed(BTN_A) || PC::buttons.pressed(BTN_B)) {
                this->cookie->resetWiningStreak(this->gamePlayVars.mode);
                this->gameState = GameState::Stats_Init;
            }
            break;

        case CheckState::Quit:
            if (PC::buttons.pressed(BTN_A) || PC::buttons.pressed(BTN_B)) {
                if (this->gamePlayVars.guesses.yCursor > 0) this->cookie->resetWiningStreak(this->gamePlayVars.mode);
                this->statisticsScreenVars.numberOfAttempts = 7;
                this->gameState = GameState::Stats_Init;
            }
            break;

        default:

            if (PC::buttons.pressed(BTN_LEFT))   moveCursor(Direction::Left);
            if (PC::buttons.pressed(BTN_RIGHT))  moveCursor(Direction::Right);

            if (PC::buttons.repeat(BTN_UP, 1)) {
                
                switch (this->gamePlayVars.keyboard.state) {

                    case KeyboardState::Showing:
                    case KeyboardState::StartHiding:
                    case KeyboardState::StartShowing:
                        break;

                    default:

                        if (this->gamePlayVars.guesses.listY < 0) {
                            this->gamePlayVars.guesses.listY = this->gamePlayVars.guesses.listY + 1;
                        }
                        break;

                }

            }        

            if (PC::buttons.pressed(BTN_UP)) {
                
                switch (this->gamePlayVars.keyboard.state) {

                    case KeyboardState::Showing:
                        moveCursor(Direction::Up);
                        break;

                    default:
                        break;

                }

            }        

            if (PC::buttons.repeat(BTN_DOWN, 1))   {
                
                switch (this->gamePlayVars.keyboard.state) {

                    case KeyboardState::Showing:
                    case KeyboardState::StartHiding:
                    case KeyboardState::StartShowing:
                        break;

                    default:

                        if (this->gamePlayVars.guesses.listY > Constants::Scroll_Limits[this->gamePlayVars.guesses.yCursor]) {
                            this->gamePlayVars.guesses.listY = this->gamePlayVars.guesses.listY - 1;
                        }
                        else{
                            this->gamePlayVars.keyboard.state = KeyboardState::StartShowing;
                        }

                        break;

                }

            }

            if (PC::buttons.pressed(BTN_DOWN))   {
                
                switch (this->gamePlayVars.keyboard.state) {

                    case KeyboardState::Showing:
                        moveCursor(Direction::Down);
                        break;

                    default:
                        break;

                }

            }

            if (PC::buttons.pressed(BTN_B)) {

                if (this->gamePlayVars.guesses.xCursor > 0) {

                    this->gamePlayVars.guesses.xCursor--;
                    this->gamePlayVars.guesses.chars[this->gamePlayVars.guesses.yCursor][this->gamePlayVars.guesses.xCursor] = ' ';
                    this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][this->gamePlayVars.guesses.xCursor] = GuessState::Dashed;

                }

            }

            if (PC::buttons.pressed(BTN_A)) {

                if (this->gamePlayVars.keyboard.xCursor == 0 && this->gamePlayVars.keyboard.yCursor == 2) {

                    if (this->gamePlayVars.guesses.xCursor > 0) {

                        this->gamePlayVars.guesses.xCursor--;
                        this->gamePlayVars.guesses.chars[this->gamePlayVars.guesses.yCursor][this->gamePlayVars.guesses.xCursor] = ' ';
                        this->gamePlayVars.guesses.state[this->gamePlayVars.guesses.yCursor][this->gamePlayVars.guesses.xCursor] = GuessState::Dashed;

                    }

                }
                else if (this->gamePlayVars.keyboard.xCursor == 8 && this->gamePlayVars.keyboard.yCursor == 2) {
// printf("BTN_A\n");

                    // Are there any spaces?

                    bool hasSpace = false;

                    for (uint8_t i = 0; i < 5; i++) {

                        if (this->gamePlayVars.guesses.chars[this->gamePlayVars.guesses.yCursor][i] == ' ') {

                            hasSpace = true;
                            break;

                        }

                    }

                    if (!hasSpace) {

                        this->gamePlayVars.checkState = checkWord();

                        switch (this->gamePlayVars.checkState) {

                            case CheckState::CorrectWord:
                                this->sounds.playSoundEffect(Sounds::Effects::Success, this->cookie->sfx);
                                this->gamePlayVars.showInvalidWord_Count = 19;
                                break;

                            case CheckState::InvalidWord:
                            case CheckState::InvalidWord_Hanski:
                                this->gamePlayVars.showInvalidWord_Count = 15;
                                break;

                            case CheckState::RealWord:

                                if (this->gamePlayVars.guesses.yCursor == 5) {

                                    this->gamePlayVars.checkState = CheckState::TooManyAttempts;

                                }
                                else {
                                    
                                    this->gamePlayVars.guesses.xCursor = 0;
                                    this->gamePlayVars.guesses.yCursor = this->gamePlayVars.guesses.yCursor + 1;

                                    if (this->gamePlayVars.guesses.yCursor >= 4) {
                                        this->gamePlayVars.guesses.listY = -((this->gamePlayVars.guesses.yCursor - 3) * Constants::Guess_Spacing);
                                    }

                                }
                                break;

                            default: break;

                        }

                    }

                }
                else {
                    
                    //if (keyboard[Constants::Key_Map[this->gamePlayVars.keyboard.yCursor][this->gamePlayVars.keyboard.xCursor]] != KeyState::Invisible) {

                        if (this->gamePlayVars.guesses.xCursor < 5) {

                            this->gamePlayVars.guesses.chars[this->gamePlayVars.guesses.yCursor][this->gamePlayVars.guesses.xCursor] = 65 + Constants::Key_Map[this->gamePlayVars.keyboard.yCursor][this->gamePlayVars.keyboard.xCursor];

                        }

                        if (this->gamePlayVars.guesses.xCursor < 5) {

                            this->gamePlayVars.guesses.xCursor++;

                            if (this->gamePlayVars.guesses.xCursor == 5) {

                                this->gamePlayVars.keyboard.xCursor = 8;
                                this->gamePlayVars.keyboard.yCursor = 2;

                            }

                        }

                    //}

                }

            }

            break;

    }

    this->renderScreen();



    // Housekeeping ..

    if (this->gamePlayVars.showInvalidWord_Count > 0 && PC::frameCount % 2 == 0) this->gamePlayVars.showInvalidWord_Count--;

}


void Game::renderScreen() {

    drawGuesses(this->gamePlayVars.guesses.listY);

    switch (this->gamePlayVars.checkState) {

        case CheckState::InvalidWord:
            if (this->gamePlayVars.mode == GameMode::English) {
                PD::drawBitmap(14, 72, Images::InvalidWord_EN);
            }
            else {
                PD::drawBitmap(0, 72, Images::InvalidWord_FR);
            }
            break;

        case CheckState::InvalidWord_Hanski:
            PD::drawBitmap(14, 62, Images::InvalidWord_EN_Hanski);
            break;

        case CheckState::CorrectWord:
            if (this->gamePlayVars.mode == GameMode::English) {
                PD::drawBitmap(32, 72, Images::Correct_EN);
            }
            else {
                PD::drawBitmap(32, 27, Images::Correct_FR);
            }
            break;

        case CheckState::TooManyAttempts:
            drawSolution();
            if (this->gamePlayVars.mode == GameMode::English) {
                PD::drawBitmap(9, 61, Images::TooManyAttempts_EN);
            }
            else {
                PD::drawBitmap(1, 61, Images::TooManyAttempts_FR);
            }
            break;

        case CheckState::Quit:
            // PD::setColor(Constants::Black);
            // PD::fillRect(0, 59, 128, 3);
            if (this->gamePlayVars.mode == GameMode::English) {
                PD::drawBitmap(20, 61, Images::Quit_EN);
            }
            else {
                PD::drawBitmap(23, 61, Images::Quit_FR);
            }
            drawSolution();
            break;

        default:
            drawKeyboard(10, this->gamePlayVars.keyboard.yPos);
            break;

    }


    // Show or hide keyboard ..

    switch (this->gamePlayVars.keyboard.state) {

        case KeyboardState::StartHiding:

            this->gamePlayVars.keyboard.yPos++;
            if (this->gamePlayVars.guesses.yCursor >= 4) this->gamePlayVars.guesses.listY++;

            if (this->gamePlayVars.keyboard.yPos == 88) {
                this->gamePlayVars.keyboard.state = KeyboardState::Hiding;
            }
            break;

        case KeyboardState::StartShowing:

            this->gamePlayVars.keyboard.yPos--;
            if (this->gamePlayVars.guesses.yCursor >= 4) this->gamePlayVars.guesses.listY--;

            if (this->gamePlayVars.keyboard.yPos == 57) {
                this->gamePlayVars.keyboard.state = KeyboardState::Showing;
            }
            break;
        
        default:
            break;
    }

}

void Game::drawSolution() {

    // Render words ..

    PD::setColor(Constants::Black);

    for (uint8_t x = 0; x < 5; x++) {

        PD::setColor(Constants::Green, Constants::White);
        PD::fillRect(Constants::Guess_Left + (x * Constants::Guess_Spacing), 72, Constants::Guess_Spacing - 2, Constants::Guess_Spacing - 2);
        PD::setColor(Constants::White, Constants::Green);
        PD::setCursor(Constants::Guess_Left + (x * Constants::Guess_Spacing) + 1, 74);
        PD::print(this->gamePlayVars.selectedWord[x]);

    }

    PD::setColor(Constants::White, Constants::Black);

}


void Game::drawGuesses(int8_t yOffset) {

    for (uint8_t y = 0; y < this->gamePlayVars.guesses.yCursor + 1; y++) {


        // Animate last row?

        int8_t xMove = 0;
        int8_t yMove[5] = { 0, 0, 0, 0, 0 };
        
        switch (this->gamePlayVars.checkState) {

            case CheckState::InvalidWord:
            case CheckState::InvalidWord_Hanski:

                if (y == this->gamePlayVars.guesses.yCursor) {
                    if (this->gamePlayVars.showInvalidWord_Count > 0) {
                        xMove = Constants::shake[this->gamePlayVars.showInvalidWord_Count];
                    }
                }
                break;

            case CheckState::CorrectWord:

                if (y == this->gamePlayVars.guesses.yCursor) {

                    if (this->gamePlayVars.showInvalidWord_Count > 0 && this->gamePlayVars.showInvalidWord_Count < 15) yMove[0] = Constants::shake[this->gamePlayVars.showInvalidWord_Count];
                    if (this->gamePlayVars.showInvalidWord_Count > 1 && this->gamePlayVars.showInvalidWord_Count < 16) yMove[1] = Constants::shake[this->gamePlayVars.showInvalidWord_Count - 1];
                    if (this->gamePlayVars.showInvalidWord_Count > 2 && this->gamePlayVars.showInvalidWord_Count < 17) yMove[2] = Constants::shake[this->gamePlayVars.showInvalidWord_Count - 2];
                    if (this->gamePlayVars.showInvalidWord_Count > 3 && this->gamePlayVars.showInvalidWord_Count < 18) yMove[3] = Constants::shake[this->gamePlayVars.showInvalidWord_Count - 3];
                    if (this->gamePlayVars.showInvalidWord_Count > 4 && this->gamePlayVars.showInvalidWord_Count < 19) yMove[4] = Constants::shake[this->gamePlayVars.showInvalidWord_Count - 4];

                }

                break;

            default: break;

        }


        // Render words ..

        for (uint8_t x = 0; x < 5; x++) {

            switch (this->gamePlayVars.guesses.state[y][x]) {

                case GuessState::Correct:
                    PD::setColor(Constants::Green, Constants::White);
                    PD::fillRect(Constants::Guess_Left + (x * Constants::Guess_Spacing) + xMove, y * Constants::Guess_Spacing + yOffset + yMove[x], Constants::Guess_Spacing - 2, Constants::Guess_Spacing - 2);
                    PD::setColor(Constants::White, Constants::Green);
                    PD::setCursor(Constants::Guess_Left + (x * Constants::Guess_Spacing) + 3 + xMove - 2, y * Constants::Guess_Spacing + 2 + yOffset + yMove[x]);
                    PD::print(this->gamePlayVars.guesses.chars[y][x]);
                    break;

                case GuessState::WrongPosition:
                    PD::setColor(Constants::Orange, Constants::White);
                    PD::fillRect(Constants::Guess_Left + (x * Constants::Guess_Spacing) + xMove, y * Constants::Guess_Spacing + yOffset + yMove[x], Constants::Guess_Spacing - 2, Constants::Guess_Spacing - 2);
                    PD::setColor(Constants::White, Constants::Orange);
                    PD::setCursor(Constants::Guess_Left + (x * Constants::Guess_Spacing) + 3 + xMove - 2, y * Constants::Guess_Spacing + 2 + yOffset + yMove[x]);
                    PD::print(this->gamePlayVars.guesses.chars[y][x]);
                    break;

                case GuessState::Dashed:
                    PD::setColor(Constants::White, Constants::Transparent);
                    PD::setCursor(Constants::Guess_Left + (x * Constants::Guess_Spacing) + 3 + xMove - 2, y * Constants::Guess_Spacing + 2 + yOffset + yMove[x]);
                    PD::print(this->gamePlayVars.guesses.chars[y][x]);
                    PD::setColor(Constants::DarkGrey, Constants::Black);
                    PD::drawFastHLine(Constants::Guess_Left + (x * Constants::Guess_Spacing) + xMove, ((y + 1) * Constants::Guess_Spacing) - 2 + yOffset + yMove[x], Constants::Guess_Spacing - 2);
                    break;

                default:
                    // PD::setColor(Constants::DarkGrey, Constants::White);
                    // PD::fillRect(Constants::Guess_Left + (x * Constants::Guess_Spacing) + xMove, y * Constants::Guess_Spacing + yOffset + yMove[x], Constants::Guess_Spacing - 2, Constants::Guess_Spacing - 2);
                    // PD::setColor(Constants::White, Constants::DarkGrey);
                    PD::setColor(Constants::White, Constants::Transparent);
                    PD::setCursor(Constants::Guess_Left + (x * Constants::Guess_Spacing) + 3 + xMove - 2, y * Constants::Guess_Spacing + 2 + yOffset + yMove[x]);
                    PD::print(this->gamePlayVars.guesses.chars[y][x]);
                    break;

            }

        }

        PD::setColor(Constants::White, Constants::Black);

    }

}
