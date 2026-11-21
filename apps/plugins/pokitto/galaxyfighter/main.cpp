#include "Pokitto.h"
#include "PokittoCookie.h"
#include <LibAudio>
#include "src/Game.h"

#include "src/entities/Entities.h"
#include "src/data/Formations.h"
#include "src/images/Images.h"
#include "src/utils/Constants.h"
#include "src/utils/Utils.h"
#include "src/utils/Enums.h"


using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;


Game game;
GameCookie cookie;

int main() {

    pk_images_init(pk_assets_load("images.bin"));


    // Initialise pokitto ..

    cookie.begin("GALAXY", sizeof(cookie), (char*)&cookie);

    PC::begin();
    PD::loadRGBPalette(palettePico);   
    PD::persistence = true;
    PD::setColor(5);
    PC::setFrameRate(200);
    PD::setFont(fontC64);

    // Kick off the random number generator ..
    
    srand(PC::getTime());



    // Has the cookie been initialised?

    if (cookie.initialised != COOKIE_INITIALISED) {

        cookie.initialise();

    }


    // Play game!

    game.setup(&cookie);

    while (PC::isRunning()) {
    
    if (!PC::update()) continue;
    
        game.loop();
    
    }
    
    return 0;

}

    