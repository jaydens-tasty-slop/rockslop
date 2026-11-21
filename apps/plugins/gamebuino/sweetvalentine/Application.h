#ifndef SWEET_VALENTINE_APPLICATION
#define SWEET_VALENTINE_APPLICATION

#include "Background.h"
#include "SplashScreen.h"

class Application
{
    private:

        static Background*   background;
        static SplashScreen* splash;

        static void initBackground();
        static void initSplashScreen();

    public:

        static const uint8_t MODE_SPLASH        = 0;
        static const uint8_t MODE_SPLASH_ENDING = 1;
        static const uint8_t MODE_SPLASH_ENDED  = 2;
        static const uint8_t MODE_GAME          = 3;
        static const uint8_t MODE_GAME_ENDED    = 4;

        static uint8_t mode;

        static void init();
        static void tick();
        static void stopSplash();
        static void tinyHeartDied(uint8_t rank);
        static void killSplashScreen();
        static void startGame();
        static void stopGame();
};

#endif