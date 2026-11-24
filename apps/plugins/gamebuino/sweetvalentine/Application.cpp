#include "Application.h"
#include "constants.h"
#include "Background.h"
#include "SplashScreen.h"
#include "Renderer.h"
#include "UserController.h"
#include "GameEngine.h"

// initialization of game mode
uint8_t Application::mode = MODE_SPLASH;

// initialization of assets
Background*   Application::background = NULL;
SplashScreen* Application::splash     = NULL;

// engine initialization
void Application::init() {
    initBackground();
    initSplashScreen();
}

// background initialization
void Application::initBackground() {
    background = new Background();
    Renderer::addListener(background);
}

// splash screen initialization
void Application::initSplashScreen() {
    splash = new SplashScreen();
}

// control loop entry point
void Application::tick() {
    UserController::tick();

    switch (mode) {
        case MODE_SPLASH:
            splash->tick();
            break;
        case MODE_SPLASH_ENDING:
            splash->tick();
            break;
        case MODE_SPLASH_ENDED:
            delete splash;
            splash = NULL;
            startGame();
            break;
        case MODE_GAME:
            GameEngine::tick();
            break;
        case MODE_GAME_ENDED:
            mode = MODE_SPLASH;
            break;
    }

    Renderer::draw();
}

void Application::stopSplash() {
    mode = MODE_SPLASH_ENDING;
    splash->stop();
}

void Application::tinyHeartDied(uint8_t rank) {
    splash->tinyHeartDied(rank);
}

void Application::killSplashScreen() {
    mode = MODE_SPLASH_ENDED;
}

void Application::startGame() {
    mode = MODE_GAME;
    GameEngine::init();
}

void Application::stopGame() {
    initSplashScreen();
    mode = MODE_GAME_ENDED;
}