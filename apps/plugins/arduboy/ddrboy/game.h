#ifndef GAME_H
#define GAME_H

struct Key {
	uint8_t button;
	int coords[2];
};

class Game {
	Arduboy2 &arduboy;
	ArduboyTones &sound;
	uint8_t reset_highscore_hold = 0;
	unsigned int score = 0; // duration in seconds timer has been running
	unsigned int highscore = 0; // the last score
	uint8_t difficulty = MIN_DIFFICULTY; // 1-4
	Key button_queue[5]; // use 1-6 as UP,LEFT,DOWN,RIGHT instead of 0-5 because an uninitialized int is always 0

	bool anything_pressed();
	void randomize();
	void draw_top_buttons();
	void draw_score();
	void draw_play_buttons();
	void move_play_buttons();
	void play_button_click_checks(uint8_t next_index);
	uint8_t get_next_button_index();
	public:
		Game(Arduboy2 &n_arduboy, ArduboyTones &n_sound) : arduboy(n_arduboy), sound(n_sound) {}
		uint8_t game_state = 0; // 0: menu; 1: play; 2: game over; 3: difficulty
		bool sound_on = false;
		void menu();
		void play();
		void win();
		void select_difficulty();
		void read_highscore();
		void write_highscore();
};

#endif
