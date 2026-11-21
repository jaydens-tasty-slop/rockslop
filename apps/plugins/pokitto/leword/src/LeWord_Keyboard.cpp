#include "LeWord.h"
#include "Pokitto.h"
#include "images/Images.h"
#include "utils/Constants.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

void Game::drawKeyboard(uint8_t xOffset, uint8_t yOffset) {

                                  // A  B  C  D  E  F  G  H  I  J  K  L  M  N  O   P  Q  R  S  T  U  V  Q  X  Y  Z
    constexpr uint8_t key_PosX[] = { 1, 6, 4, 3, 3, 4, 5, 6, 8, 7, 8, 9, 8, 7, 9, 10, 1, 4, 2, 5, 7, 5, 2, 3, 6, 2 }; 
    constexpr uint8_t key_PosY[] = { 2, 3, 3, 2, 1, 2, 2, 2, 1, 2, 2, 2, 3, 3, 1,  1, 1, 1, 2, 1, 1, 3, 1, 3, 1, 3 }; 

    constexpr uint8_t key_Width = 10;
    constexpr uint8_t key_Height = 11;

    PD::setColor(Constants::Black);
    //PD::fillRect(xOffset - 1, yOffset + 1, 93, 50);

    for (uint8_t i = 0; i < 26; i++) {

        uint8_t xPos = key_PosX[i] - 1;
        uint8_t yPos = key_PosY[i] - 1;

        uint8_t x = xPos * (key_Width - 1) + (key_PosY[i] >= 2 ? 4 : 0) + (key_PosY[i] == 3 ? -2 : 0);
        uint8_t y = yPos * (key_Height - 1); 

        switch (this->gamePlayVars.keyboard.keys[i]) {

            case KeyState::Visible:
                PD::setColor(Constants::LightGrey, Constants::Black);
                PD::fillRect(x + xOffset + 1, y + yOffset + 2, key_Width - 2, key_Height - 3);
                PD::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_B[i]);
                break;

            case KeyState::WrongPosition:
                PD::setColor(Constants::Orange, Constants::Black);
                PD::fillRect(x + xOffset + 1, y + yOffset + 2, key_Width - 2, key_Height - 3);
                PD::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_W[i]);
                break;

            case KeyState::Correct:
                PD::setColor(Constants::Green, Constants::Black);
                PD::fillRect(x + xOffset + 1, y + yOffset + 2, key_Width - 2, key_Height - 3);
                PD::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_W[i]);
                break;

            case KeyState::Invisible:
                PD::setColor(Constants::DarkGrey, Constants::Transparent);
                //PD::fillRect(x + xOffset + 1, y + yOffset + 2, key_Width - 2, key_Height - 3);
                PD::drawBitmap(x + xOffset, y + 1 + yOffset, Images::Keyboard_W[i]);
                break;
                
        }

        if (xPos == this->gamePlayVars.keyboard.xCursor && yPos == this->gamePlayVars.keyboard.yCursor && this->gamePlayVars.keyboard.state == KeyboardState::Showing) {

            PD::setColor(PC::frameCount % Constants::Key_Flash < Constants::Key_Flash / 2 ? Constants::White : Constants::Black, Constants::Black);
            PD::drawRect(x + xOffset + 1, y + yOffset + 2, key_Width - 3, key_Height - 3);

        }


    }


    // Delete key ..

    PD::setColor(Constants::LightGrey, Constants::Black);
    PD::fillRect(xOffset + 1, 2 * (key_Height - 1) + 2 + yOffset, key_Width, key_Height - 3);
    PD::drawBitmap(xOffset, 2 * (key_Height - 1) + 1 + yOffset, Images::Keyboard_B[26]);

    if (this->gamePlayVars.keyboard.xCursor == 0 && this->gamePlayVars.keyboard.yCursor == 2) {

        PD::setColor(PC::frameCount % Constants::Key_Flash < Constants::Key_Flash / 2 ? Constants::White : Constants::Black, Constants::Black);
        PD::drawRect(xOffset + 1, yOffset + (2 * (key_Height - 1)) + 2, key_Width - 1, key_Height - 3);

    }


    // Enter Key ..

    PD::setColor(Constants::LightGrey, Constants::Black);
    PD::fillRect(7 * (key_Width - 1) + (key_Width - 1) + xOffset + 3, 2 * (key_Height - 1) + 2 + yOffset, key_Width + 5, key_Height - 3);
    PD::drawBitmap(7 * (key_Width - 1) + (key_Width - 1) + xOffset + 2, 2 * (key_Height - 1) + 1 + yOffset, Images::Keyboard_B[27]);

    if (this->gamePlayVars.keyboard.xCursor == 8 && this->gamePlayVars.keyboard.yCursor == 2) {
        PD::setColor(PC::frameCount % Constants::Key_Flash < Constants::Key_Flash / 2 ? Constants::White : Constants::Black, Constants::Black);
        PD::drawRect(7 * (key_Width - 1) + (key_Width - 1) + xOffset + 3, 2 * (key_Height - 1) + 2 + yOffset, key_Width + 4, key_Height - 3);
    }

    PD::setColor(Constants::White);
    PD::drawFastHLine(xOffset, yOffset, 91);
    PD::drawFastVLine(xOffset - 1, yOffset + 1, 50);
    PD::drawFastVLine(xOffset + 91, yOffset + 1, 50);

    switch (this->gamePlayVars.keyboard.state) {

        case KeyboardState::Showing:
        case KeyboardState::StartShowing:
            PD::drawBitmap(xOffset + 86, yOffset - 4, Images::Arrow_Up);
            break;

        case KeyboardState::Hiding:
        case KeyboardState::StartHiding:
            PD::drawBitmap(xOffset + 86, yOffset - 4, Images::Arrow_Down);
            break;

    }

}