#ifndef SWEET_VALENTINE_SPLASH_SCREEN
#define SWEET_VALENTINE_SPLASH_SCREEN

#include "constants.h"
#include "SplashText.h"
#include "TinyHeart.h"

class SplashScreen
{
    private:

        uint8_t     numTinyHearts;
        uint8_t     numDeadTinyHearts;
        SplashText* text;
        TinyHeart*  tinyheart[MAX_NUM_TINYHEARTS];
    
        void initSplashText();
        TinyHeart* spawnTinyHeart(uint8_t rank);
        void killAllTinyHearts();
    
    public:

        SplashScreen();
        ~SplashScreen();
        void stop();
        void tick();
        void tinyHeartDied(uint8_t rank);
};

#endif