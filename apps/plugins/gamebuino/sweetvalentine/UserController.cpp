#include "UserController.h"
#include "Application.h"
#include "GameEngine.h"

void UserController::tick() {
    if (gb.buttons.pressed(BUTTON_A)) {
        switch (Application::mode) {
            case Application::MODE_SPLASH:
                Application::stopSplash();
                break;
            case Application::MODE_GAME:
                if (GameEngine::mode == GameEngine::MODE_STANDBY) {
                    GameEngine::start();
                }
                break;
        }
    } else if (gb.buttons.pressed(BUTTON_B) && AUTHORIZED_SUSPENSION) {
        switch (GameEngine::mode) {
            case GameEngine::MODE_STARTED:
                GameEngine::suspend();
                break;
            case GameEngine::MODE_SUSPENDED:
                GameEngine::resume();
                break;
        }
    } else if (gb.buttons.repeat(BUTTON_LEFT, 0)) {
        GameEngine::movePaddleRight();
    } else if (gb.buttons.repeat(BUTTON_RIGHT, 0)) {
        GameEngine::movePaddleLeft();
    }
}