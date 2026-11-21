/* gameover.c */

# include "gameover.h"

void gameover (uint8_t *state) {

	 Mix_PlayMusic(2);

    Pokitto::Display::clear();
    Pokitto::Display::setColor(15);//text color
	Pokitto::Display::print(70,30,"GAME OVER");

	Pokitto::Display::print(30,65,"Ported for Pokitto by");
	Pokitto::Display::print(30,85,"@HomineLudens");
	Pokitto::Display::print(30,100,"@FManga");
	Pokitto::Display::print(30,115,"@DrummyFish");
	Pokitto::Display::print(30,130,"@Jonne");
	Pokitto::Display::update();

	/* Wait */
	Pok_sleep(5000);


	*state = 0;
}
