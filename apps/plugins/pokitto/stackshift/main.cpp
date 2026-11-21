#include "Pokitto.h"

#include "Pokitto.h"
#include <LibAudio>
#include "src/Game.h"
#include "src/images/Images.h"
#include "src/utils/GameCookie.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;

Game game;
GameCookie cookie;

Audio::Sink<4, PROJ_AUD_FREQ> audio;

int main(){
    

    // Initialise pokitto ..

    pk_images_init(pk_assets_load("images.bin"));
    cookie.begin("STACK", sizeof(cookie), (char*)&cookie);

    PC::begin();
    PD::loadRGBPalette(palettePico);   
    PD::persistence = true;
    PD::setColor(5, 0);
    PD::setInvisibleColor(15);
    PC::setFrameRate(40);
    PD::setFont(fontC64);


    // Has the cookie been initialised?

    if (cookie.initialised != COOKIE_INITIALISED) {

        cookie.initialise();

    }

    game.setup(&cookie);
    
    while( PC::isRunning() ){

        if( !PC::update() )  continue;

        game.loop();

    }
    
    return 0;
}
