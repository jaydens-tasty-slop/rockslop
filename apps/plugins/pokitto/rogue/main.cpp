#include "Pokitto.h"
#include "PokittoCookie.h"

#include "src/Game.h"
#include "src/GameCookie.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

Game game;
GameCookie cookie;


int main() {


    // Initialise pokitto ..

    cookie.begin("ROGUE", sizeof(cookie), (char*)&cookie);

    PC::begin();
    PD::loadRGBPalette(palettePico);   
    PD::persistence = true;
    PD::setColor(7);  // YP3: white, not dark grey (5), on the mono LCD
    PD::setInvisibleColor(14);
    PD::setFont(font5x7);


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



