#pragma once

#include "Pokitto.h"
#include "PokittoCookie.h"
#include "Enums.h"
#include "Structs.h"


class GameCookie : public Pokitto::Cookie {

	public:

		uint8_t initialised;
		uint8_t track;
		GameMode gameMode;
		SoundSettings sfx;

		uint16_t gamesWon_EN = 0;
		uint16_t gamesPlayed_EN = 0;
		uint16_t currentStreak_EN = 0;
		uint16_t maxStreak_EN = 0;
		uint16_t distribution_EN[6] = { 0, 0, 0, 0, 0, 0 };

		uint16_t gamesWon_FR = 0;
		uint16_t gamesPlayed_FR = 0;
		uint16_t currentStreak_FR = 0;
		uint16_t maxStreak_FR = 0;
		uint16_t distribution_FR[6] = { 0, 0, 0, 0, 0, 0 };

	public:

		void initialise() {

			this->initialised = COOKIE_INITIALISED;
			this->sfx = SoundSettings::Both;

			this->gamesWon_EN = 0;
			this->gamesPlayed_EN = 0;
			this->currentStreak_EN = 0;
			this->maxStreak_EN = 0;

			this->gamesWon_FR = 0;
			this->gamesPlayed_FR = 0;
			this->currentStreak_FR = 0;
			this->maxStreak_FR = 0;

			for (uint8_t i = 0; i < 6; i++) {

				this->distribution_EN[i] = 0;
				this->distribution_FR[i] = 0;

			}

			this->saveCookie();

		}

		void initialise(GameMode gameMode) {

			switch (gameMode) {

				case GameMode::English:

					this->initialised = COOKIE_INITIALISED;
					this->gamesWon_EN = 0;
					this->gamesPlayed_EN = 0;
					this->currentStreak_EN = 0;
					this->maxStreak_EN = 0;

					for (uint8_t i = 0; i < 6; i++) {
						this->distribution_EN[i] = 0;
					}

					break;
				
				case GameMode::French:

					this->gamesWon_FR = 0;
					this->gamesPlayed_FR = 0;
					this->currentStreak_FR = 0;
					this->maxStreak_FR = 0;
					this->saveCookie();

					for (uint8_t i = 0; i < 6; i++) {
						this->distribution_FR[i] = 0;
					}

					break;

			}

		}

		void increaseCorrectWords(GameMode gameMode, uint8_t numberOfGuesses) {

			if (gameMode == GameMode::English) {

				this->gamesWon_EN++;
				this->gamesPlayed_EN++;
				this->currentStreak_EN++;
				this->distribution_EN[numberOfGuesses]++;
				if (this->currentStreak_EN > this->maxStreak_EN) this->maxStreak_EN = this->currentStreak_EN;

			}
			else {

				this->gamesWon_FR++;
				this->gamesPlayed_FR++;
				this->currentStreak_FR++;
				this->distribution_FR[numberOfGuesses]++;
				if (this->currentStreak_FR > this->maxStreak_FR) this->maxStreak_FR = this->currentStreak_FR;

			}

			this->saveCookie();

		}

		void resetWiningStreak(GameMode gameMode) {

			uint16_t gamesPlayed = 0;
			uint16_t currentStreak = 0;

			if (gameMode == GameMode::English) {

				this->gamesPlayed_EN++;
				this->currentStreak_EN = 0;

			}
			else {

				this->gamesPlayed_FR++;
				this->currentStreak_FR = 0;

			}

			this->saveCookie();

		}

		void setMode(GameMode gameMode) {

			this->gameMode = gameMode;
			this->saveCookie();

		}

		uint8_t getPercent(GameMode gameMode, uint8_t val) {

			uint16_t high = 0;

			switch (gameMode) {

				case GameMode::English:

					for (uint8_t i = 0; i < 6; i++) {
						if (distribution_EN[i] > high) high = distribution_EN[i];
					}

					if (high == 0) return 0;

					return (distribution_EN[val] * 90 / high);

				case GameMode::French:

					for (uint8_t i = 0; i < 6; i++) {
						if (distribution_FR[i] > high) high = distribution_FR[i];
					}

					if (high == 0) return 0;

					return (distribution_FR[val] * 90 / high);

			}

			return 0;

		}

		uint16_t getPercentVal(GameMode gameMode, uint8_t val) {

			uint16_t high = 0;

			switch (gameMode) {

				case GameMode::English:

					return distribution_EN[val];

				case GameMode::French:

					return distribution_FR[val];

			}

			return 0;

		}


};