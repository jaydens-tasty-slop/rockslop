#include "GameOverState.h"
#include "Globals.h"

GameOverState::GameOverState(unsigned long score)
{
	this->score = score;
	highScore = getHighScore();
	if (score > highScore)
	{
		setHighScore(score);
	}
}

void GameOverState::update()
{
	arduboy.setCursor(35, 10);
	arduboy.print(F("GAME OVER"));

	if (score > highScore && arduboy.frameCount % 2 == 0)
	{
		arduboy.setCursor(19, 30);
		arduboy.print("NEW HIGH SCORE!");
	}

	if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON))
	{
		setState(STATE_MENU);
	}
}