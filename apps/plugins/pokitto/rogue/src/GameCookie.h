#pragma once

#include "Pokitto.h"
#include "PokittoCookie.h"

class GameCookie : public Pokitto::Cookie {
    
    public:
    
    	uint8_t initialised;
        uint16_t easyScore = 0;


	public:

		void initialise() {

			this->initialised = COOKIE_INITIALISED;

			this->easyScore = 0;
			this->saveCookie();

		}

};
