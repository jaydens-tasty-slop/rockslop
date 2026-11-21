#include "SplashScreen.h"
#include "constants.h"
#include "TinyHeart.h"
#include "Renderer.h"
#include "Application.h"

SplashScreen::SplashScreen() {
    this->numTinyHearts     = 0;
    this->numDeadTinyHearts = 0;
    this->initSplashText();
}

SplashScreen::~SplashScreen() {
    // -----------------------------------------------------------
    // !!! be careful !!!
    // -----------------------------------------------------------
    // delete[] this->tinyheart;
    // -----------------------------------------------------------
    // don't do that because `tinyheart` was STATICALLY allocated
    // in `SplashScreen.h` :
    //   TinyHeart*  tinyheart[MAX_NUM_TINYHEARTS];
    // -----------------------------------------------------------
    // a big thank you to @minirop for helping me find this error!
    // -----------------------------------------------------------
    Renderer::removeListener(this->text);
    delete this->text;
}

void SplashScreen::initSplashText() {
    this->text = new SplashText(
        SPLASH_TEXT_WIDTH,
        SPLASH_TEXT_HEIGHT,
        .5*(SCREEN_WIDTH - SPLASH_TEXT_WIDTH),
        SCREEN_HEIGHT,
        0,
        0
    );

    this->text->moveToY(.5*(SCREEN_HEIGHT - SPLASH_TEXT_HEIGHT));

    Renderer::addListener(this->text);
}

void SplashScreen::stop() {
    this->text->moveToY(-SPLASH_TEXT_HEIGHT-1);
    size_t i;
    for (i=0; i<this->numTinyHearts; i++) {
        this->tinyheart[i]->die();
    }
}

TinyHeart* SplashScreen::spawnTinyHeart(uint8_t rank) {
    TinyHeart* tinyheart = new TinyHeart(
        rank,
        TINYHEART_WIDTH,
        TINYHEART_HEIGHT,
        random(1, SCREEN_WIDTH - 1 - TINYHEART_WIDTH),
        -TINYHEART_HEIGHT,
        0,
        random(1, 4)
    );

    Renderer::addListener(tinyheart);

    return tinyheart;
}

void SplashScreen::tick() {
    this->text->tick();

    if (Application::mode == Application::MODE_SPLASH_ENDING) {
        if (this->numDeadTinyHearts == this->numTinyHearts) {
            this->killAllTinyHearts();
        }
    }

    if (Application::mode == Application::MODE_SPLASH && this->numTinyHearts < MAX_NUM_TINYHEARTS && random(0, 10) < 1) {
        // (upstream's single statement leaves the order of the two uses of
        // numTinyHearts unspecified)
        uint8_t rank = this->numTinyHearts++;
        this->tinyheart[rank] = this->spawnTinyHeart(rank);
    }

    if (Application::mode == Application::MODE_SPLASH || Application::mode == Application::MODE_SPLASH_ENDING) {
        size_t i;
        for (i=0; i<this->numTinyHearts; i++) {
            this->tinyheart[i]->tick();
        }
    }
}

void SplashScreen::tinyHeartDied(uint8_t rank) {
    Renderer::removeListener(this->tinyheart[rank]);
    this->numDeadTinyHearts++;
}

void SplashScreen::killAllTinyHearts() {
    TinyHeart* heart;
    size_t i;
    for (i=0; i<this->numTinyHearts; i++) {
        heart = this->tinyheart[i];
        delete heart;
        this->tinyheart[i] = NULL;
    }

    this->numTinyHearts     = 0;
    this->numDeadTinyHearts = 0;

    Application::killSplashScreen();
}