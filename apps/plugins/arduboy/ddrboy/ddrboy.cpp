#include <stdlib.h>
#include <math.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include "globals.h"
#include "game.h"
#include "../lib/ab_math.h"
#include "sounds.h"

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
Game game(arduboy, sound);

void setup() {
	arduboy.begin();
	arduboy.setFrameRate(FRAMERATE);
	arduboy.setTextWrap(true);
	arduboy.initRandomSeed();
	game.read_highscore();
	game.sound_on = arduboy.audio.enabled();
}

void loop() {
	if (!(arduboy.nextFrame())) return;
	arduboy.pollButtons();
	arduboy.clear();

	switch (game.game_state) {
		
		case 0:
			game.menu();
			break;
		
		case 1:
			game.play();
			break;

		case 2:
			game.win();
			break;
			
		case 3:
			game.select_difficulty();
			break;

		default:
			break;
	}

	// increase the timer if it is active (1000 because ms)
	arduboy.display();
}
