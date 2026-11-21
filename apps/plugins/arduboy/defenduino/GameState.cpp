#include "GameState.h"
#include "Globals.h"
#include "GameOverState.h"

#define LANDER_SPAWN_ALT -200

#define FLAG_FREEZE_ACTORS 0x1
#define FLAG_UI_BOTTOM 0x2
#define FLAG_INTERSTITIAL 0x4

#define SPAWN_INTERVAL 600
#define INITIAL_SPAWN_INTERVAL 120
#define HUMANOID_SPAWN_Y 290

#define SCORE_BOTTOM_Y 57
#define SCANNER_BOTTOM_Y 52

const uint16_t extraLifeSound[] PROGMEM = {
	50,100, 250,100,
	150,100, 350,100,
	250, 100, 450, 100,
	350, 100, 550, 100,
	450, 100, 650, 100,
	550, 100, 750, 100,
	650, 100, 850, 100,
	750, 100, 950, 100,
	TONES_END };

GameState::GameState()
{
	//arduboy.initRandomSeed();
	srand(arduboy.generateRandomSeed());
	onNewLevel();
}

GameCamera* GameState::getCamera()
{
	return &camera;
}


// Tries to start spawning a lander, but no guaranteed to do so if
// no resources are available.
void GameState::startSpawningLander()
{
	if (getInactiveLander() != NULL)
	{
		int worldX = getSafeSpawn();

		Particles* pParticles = getParticles();

		if (pParticles != NULL)
		{
			pParticles->worldPos.x = worldX;
			pParticles->worldPos.y = LANDER_SPAWN_ALT;
			pParticles->show(PARTICLES_SPAWN);
		}
	}
}

// Tries to start spawning a baiter, but not guaranteed to do so if
// no resources are available.
void GameState::startSpawningBaiter()
{
	if (getInactiveBaiter() != NULL)
	{
		int worldX = getSafeSpawn();

		Particles* pParticles = getParticles();

		if (pParticles != NULL)
		{
			pParticles->worldPos.x = worldX;
			pParticles->worldPos.y = 0;
			pParticles->show(PARTICLES_SPAWN_BAITER);
		}

		spawnCountdown = SPAWN_INTERVAL;
	}
}

int GameState::getSafeSpawn()
{
	int worldX = rand() % WORLD_WIDTH_UNITS;
	while (spawnPosTooCloseToPlayer(worldX))
	{
		worldX = rand() % WORLD_WIDTH_UNITS;
	}
	return worldX;
}

bool GameState::spawnPosTooCloseToPlayer(int xPos)
{
	return abs(xPos - playerShip.worldPos.x) < 640
		|| (playerShip.worldPos.x >= ((1024 - 64)*10) && abs(xPos - (playerShip.worldPos.x - 10240)) < 640)
		|| (playerShip.worldPos.x <= 640 && abs(xPos - (playerShip.worldPos.x + 10240)) < 640);

}

void GameState::update()
{
	if (isFlagSet(flags, FLAG_INTERSTITIAL))
	{
		interstitialUpdate();
	}
	else
	{
		inPlayUpdate();
	}

	drawGui();
}

void GameState::interstitialUpdate()
{
	arduboy.setCursor(25, 20);
	arduboy.print(F("ATTACK WAVE "));
	arduboy.print(level);
	arduboy.setCursor(37, 32);
	arduboy.print(F("COMPLETED"));
	arduboy.setCursor(31, 50);

	uint16_t bonus = level * 100;
	if (bonus > 500)
	{
		bonus = 500;
	}

	arduboy.print(F("BONUS X "));
	arduboy.print(bonus);

	uint8_t toDraw = ((remainingHumanoids * 10) - spawnCountdown) / 10;

	for (uint8_t i = 0; i < toDraw; i++)
	{
		uint8_t xPos = 31 + (i * 6);
		arduboy.drawLine(xPos, 59, xPos, 61);
	}

	if (spawnCountdown > 0)
	{
		spawnCountdown--;

		if (spawnCountdown % 10 == 0)
		{
			addToScore(bonus);
		}
	}
	else if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON))
	{
		onNewLevel();
	}
}

void GameState::inPlayUpdate()
{
	bool freezeActors = isFlagSet(flags, FLAG_FREEZE_ACTORS);
	uint8_t expectedLanders = getExpectedLandersForLevel();
	if (!freezeActors)
	{
		if (spawnCountdown == 0)
		{
			/*Serial.print("SpawnedLanders=");
			Serial.println(spawnedLanders);*/
			if (spawnedLanders < expectedLanders)
			{
				spawnWave(expectedLanders);
			}
			else
			{
				startSpawningBaiter();
			}
		}
		else
		{
			spawnCountdown--;
		}
	}

	if (playerShip.isActive())
	{
		playerShip.update();
		camera.update(&playerShip);
		playerShip.render(camera.worldToScreenPos(playerShip.worldPos));

		setFlag(&flags, FLAG_UI_BOTTOM, playerShip.worldPos.y < -160);
	}

	for (uint8_t i = 0; i < TOTAL_PLAYER_SHOTS; i++)
	{
		if (playerShots[i].isActive())
		{
			if (!freezeActors)
			{
				playerShots[i].update();
			}
			playerShots[i].render(camera.worldToScreenPos(playerShots[i].worldPos));
		}
	}

	if (remainingHumanoids > 0)
	{
		landscape.render(camera.worldPos.getPixelX());

		for (uint8_t i = 0; i < TOTAL_HUMANOIDS; i++)
		{
			if (humanoids[i].isActive())
			{
				if (!freezeActors)
				{
					humanoids[i].update(&landscape, &playerShip);
				}
				humanoids[i].render(camera.worldToScreenPos(humanoids[i].worldPos));

				if (!freezeActors)
				{
					humanoids[i].collisionCheck(playerShots, &playerShip);
				}
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_LANDERS; i++)
	{
		if (landers[i].isActive())
		{
			if (!freezeActors)
			{
				landers[i].update(&landscape, &playerShip);
			}

			landers[i].render(camera.worldToScreenPos(landers[i].worldPos));

			if (!freezeActors)
			{
				landers[i].collisionCheck(playerShots, &playerShip);
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_BOMBERS; i++)
	{
		if (bombers[i].isActive())
		{
			if (!freezeActors)
			{
				bombers[i].update(&playerShip);
			}

			bombers[i].render(camera.worldToScreenPos(bombers[i].worldPos));

			if (!freezeActors)
			{
				bombers[i].collisionCheck(playerShots, &playerShip);
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_BAITERS; i++)
	{
		if (baiters[i].isActive())
		{
			if (!freezeActors)
			{
				baiters[i].update(&playerShip);
			}

			baiters[i].render(camera.worldToScreenPos(baiters[i].worldPos));

			if (!freezeActors)
			{
				baiters[i].collisionCheck(playerShots, &playerShip);
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_PODS; i++)
	{
		if (pods[i].isActive())
		{
			if (!freezeActors)
			{
				pods[i].update(&playerShip);
			}

			pods[i].render(camera.worldToScreenPos(pods[i].worldPos));

			if (!freezeActors)
			{
				pods[i].collisionCheck(playerShots, &playerShip);
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_SWARMERS; i++)
	{
		if (swarmers[i].isActive())
		{
			if (!freezeActors)
			{
				swarmers[i].update(&playerShip);
			}

			swarmers[i].render(camera.worldToScreenPos(swarmers[i].worldPos));

			if (!freezeActors)
			{
				swarmers[i].collisionCheck(playerShots, &playerShip);
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_MINES; i++)
	{
		if (mines[i].isActive())
		{
			mines[i].render(camera.worldToScreenPos(mines[i].worldPos));

			if (!freezeActors)
			{
				mines[i].collisionCheck(&playerShip);
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_ENEMY_SHOTS; i++)
	{
		if (enemyShots[i].isActive())
		{
			if (!freezeActors)
			{
				enemyShots[i].update(&playerShip);
			}

			enemyShots[i].render(camera.worldToScreenPos(enemyShots[i].worldPos));

			if (!freezeActors)
			{
				enemyShots[i].collisionCheck(&playerShip);
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_PARTICLES; i++)
	{
		if (particles[i].isActive())
		{
			particles[i].update();
			particles[i].render(camera.worldToScreenPos(particles[i].worldPos));
		}
	}

	if (invertCountdown > 0)
	{
		invertCountdown--;
		arduboy.invert(invertCountdown % 2 != 0);
	}
}

uint8_t GameState::getExpectedLandersForLevel()
{
	return level == 1 ? 15 : 20;
}

void GameState::spawnWave(uint8_t maxForLevel)
{
	uint8_t toSpawn = maxForLevel - spawnedLanders;
	if (toSpawn > 4)
	{
		toSpawn = 4;
	}

	for (uint8_t i = 0; i < toSpawn; i++)
	{
		startSpawningLander();
	}

	spawnCountdown = SPAWN_INTERVAL;
}

PlayerShot* GameState::getPlayerShot()
{
	PlayerShot* pShot = NULL;
	for (uint8_t i = 0; i < TOTAL_PLAYER_SHOTS; i++)
	{
		if (!playerShots[i].isActive())
		{
			pShot = &playerShots[i];
			break;
		}
	}

	return pShot;
}

EnemyShot* GameState::getEnemyShot()
{
	EnemyShot* pShot = NULL;
	for (uint8_t i = 0; i < TOTAL_ENEMY_SHOTS; i++)
	{
		if (!enemyShots[i].isActive())
		{
			pShot = &enemyShots[i];
			break;
		}
	}

	return pShot;
}

Mine* GameState::getMine()
{
	Mine* pMine = NULL;
	for (uint8_t i = 0; i < TOTAL_MINES; i++)
	{
		if (!mines[i].isActive())
		{
			pMine = &mines[i];
			break;
		}
	}

	return pMine;
}

Particles* GameState::getParticles()
{
	Particles* pParticles = NULL;
	for (uint8_t i = 0; i < TOTAL_PARTICLES; i++)
	{
		if (!particles[i].isActive())
		{
			pParticles = &particles[i];
			break;
		}
	}

	return pParticles;
}

Swarmer* GameState::getSwarmer()
{
	Swarmer* pSwarmer = NULL;
	for (uint8_t i = 0; i < TOTAL_SWARMERS; i++)
	{
		if (!swarmers[i].isActive())
		{
			pSwarmer = &swarmers[i];
			break;
		}
	}

	return pSwarmer;
}

void GameState::lostLife()
{
	unsetFlag(&flags, FLAG_FREEZE_ACTORS);
	lives--;

	if (lives > 0)
	{
		onNewLife();
	}
	else
	{
		setState(STATE_GAME_OVER);
	}
}

void GameState::freezeActors()
{
	setFlag(&flags, FLAG_FREEZE_ACTORS);
}

Lander* GameState::getInactiveLander()
{
	Lander* pLander = NULL;
	for (uint8_t i = 0; i < TOTAL_LANDERS; i++)
	{
		if (!landers[i].isActive())
		{
			pLander = &landers[i];
			break;
		}
	}
	return pLander;
}

Baiter* GameState::getInactiveBaiter()
{
	Baiter* pBaiter = NULL;
	for (uint8_t i = 0; i < TOTAL_BAITERS; i++)
	{
		if (!baiters[i].isActive())
		{
			pBaiter = &baiters[i];
			break;
		}
	}
	return pBaiter;
}

void GameState::completeSpawningLander(int xPos, int yPos)
{
	// Find the first inactive lander in the list, activate and place it.
	Lander* pLander = getInactiveLander();
	if (pLander != NULL)
	{
		pLander->setActive(true);
		pLander->worldPos.x = xPos;
		pLander->worldPos.y = yPos;
		spawnedLanders++;
		liveEnemies++;
	}
	/*else
	{
		Serial.println("No available lander!");
	}*/
}

void GameState::completeSpawningBaiter(int xPos, int yPos)
{
	// Find the first inactive baiter in the list, activate and place it.
	Baiter* pBaiter = getInactiveBaiter();
	if(pBaiter != NULL)
	{
		pBaiter->setActive(true);
		pBaiter->worldPos.x = xPos;
		pBaiter->worldPos.y = yPos;
	}
/*	else
	{
		Serial.println("No available baiter!");
	}*/
}

// Get the index of a humanoid that can be captured, or NO_HUMANOID_FOUND if there is none.
uint8_t GameState::getCapturableHumanoidAtPosition(int xPos)
{
	uint8_t index = NO_HUMANOID_FOUND;

	for (uint8_t i = 0; i < TOTAL_HUMANOIDS; i++)
	{
		if (humanoids[i].isCapturable() && humanoids[i].worldPos.getPixelX() == xPos)
		{
			index = i;
			break;
		}
	}

	return index;
}

Humanoid* GameState::getHumanoid(uint8_t index)
{
	return &humanoids[index];
}

void GameState::drawGui()
{
	uint8_t guiY = isFlagSet(flags, FLAG_UI_BOTTOM) ? SCORE_BOTTOM_Y : 0;
	smallFont.setCursor(0, guiY);
	smallFont.print(score);
	drawScanner();

	String sLives(lives-1);
	uint8_t xPos = 127 - (sLives.length() * 4);
	smallFont.setCursor(xPos, guiY);
	smallFont.print(sLives);

	String sBombs(smartBombs);
	xPos -= ((sBombs.length()+1) * 4);
	smallFont.setCursor(xPos, guiY);
	smallFont.print(sBombs);
}

void GameState::drawScanner()
{
	uint8_t scannerY = isFlagSet(flags, FLAG_UI_BOTTOM) ? SCANNER_BOTTOM_Y : 0;

	arduboy.drawRect(31, scannerY, 66, 12, WHITE);
	scannerY++;
	arduboy.fillRect(32, scannerY, 64, 10, BLACK);

	// Draw landscape on scanner
	if (remainingHumanoids > 0)
	{
		int landscapeIdx = camera.worldPos.getPixelX() - 512;
		if (landscapeIdx < 0)
		{
			landscapeIdx += 1024;
		}

		// Make sure we're on a four-position boundary which is quicker to uncompress
		landscapeIdx -= landscapeIdx % 4;

		for (uint8_t i = 0; i < 64; i++)
		{
			byte height = landscape.getHeight(landscapeIdx);

			arduboy.drawPixel(32 + i, scannerY + (height / 6.4));

			landscapeIdx += 16;
			if (landscapeIdx >= 1024)
			{
				landscapeIdx = 0;
			}
		}

		for (uint8_t i = 0; i < TOTAL_HUMANOIDS; i++)
		{
			if (humanoids[i].isActive())
			{
				plotOnScanner(scannerY, &humanoids[i]);
			}
		}
	}

	for (uint8_t i = 0; i < TOTAL_LANDERS; i++)
	{
		if (landers[i].isActive())
		{
			plotOnScanner(scannerY, &landers[i]);
		}
	}

	for (uint8_t i = 0; i < TOTAL_BOMBERS; i++)
	{
		if (bombers[i].isActive())
		{
			plotOnScanner(scannerY, &bombers[i]);
		}
	}


	for (uint8_t i = 0; i < TOTAL_PODS; i++)
	{
		if (pods[i].isActive())
		{
			plotOnScanner(scannerY, &pods[i]);
		}
	}

	for (uint8_t i = 0; i < TOTAL_SWARMERS; i++)
	{
		if (swarmers[i].isActive())
		{
			plotOnScanner(scannerY, &swarmers[i]);
		}
	}

	for (uint8_t i = 0; i < TOTAL_BAITERS; i++)
	{
		if (baiters[i].isActive())
		{
			plotOnScanner(scannerY, &baiters[i]);
		}
	}

	// Playership
	uint8_t yPos = scannerY + ((playerShip.worldPos.getPixelY() + 32) / 6.4);

	// Different xpos depending on left or right of cam.
	uint8_t xPos = playerShip.facingRight() ? 62 : 65;
	arduboy.drawLine(xPos, yPos, xPos+1, yPos, WHITE);

}

void GameState::plotOnScanner(uint8_t scannerY, GameObject* pGameObject)
{
	int8_t xPos = ((pGameObject->worldPos.getPixelX() - camera.worldPos.getPixelX()) / 16);
	if (xPos >= 32)
	{
		xPos -= 64;
	}
	else if (xPos <= -32)
	{
		xPos += 64;
	}
	arduboy.drawPixel(xPos + 64, scannerY + ((pGameObject->worldPos.getPixelY() + 32) / 6.4), WHITE);
}

void GameState::onCountedEnemyDeath(uint8_t total)
{
	if (liveEnemies < total)
	{
		liveEnemies = 0;
	}
	else
	{
		liveEnemies -= total;
	}

	if (liveEnemies == 0 && spawnedLanders == getExpectedLandersForLevel())
	{
		// Start interstitial
		setFlag(&flags, FLAG_INTERSTITIAL);
		unsetFlag(&flags, FLAG_UI_BOTTOM);
		unsetFlag(&flags, FLAG_FREEZE_ACTORS);
		spawnCountdown = remainingHumanoids * 10;
		invertCountdown = 0;
		arduboy.invert(false);

		// If the last enemy was killed by the player crashing into it, cancel the explosion but still take away a life.
		// They might get an extra one from the surviving human bonus, so don't go to game over until after that.
		if (playerShip.isExploding())
		{
			playerShip.cancelExplosion();
			lives--;
		}
	}
}

void GameState::addToScore(uint16_t toAdd)
{
	score += toAdd;

	if (score > nextBonus)
	{
		nextBonus += 10000;
		lives++;
		smartBombs++;
		sound.tones(extraLifeSound);
	}
}

void GameState::onNewLevel()
{
	if (lives == 0)
	{
		setState(STATE_GAME_OVER);
	}
	else
	{
		level++;

		if (level % 5 == 0)
		{
			remainingHumanoids = TOTAL_HUMANOIDS;
		}

		flags = 0;
		spawnedLanders = 0;
		spawnCountdown = INITIAL_SPAWN_INTERVAL;
		liveEnemies = 0;

		playerShip.setActive(true, true);
		playerShip.worldPos.x = 0;
		playerShip.worldPos.y = 0;
		camera.worldPos.x = 320;

		for (uint8_t i = 0; i < TOTAL_HUMANOIDS; i++)
		{
			if (i < remainingHumanoids)
			{
				humanoids[i].worldPos.x = rand() % 10240;
				humanoids[i].worldPos.y = HUMANOID_SPAWN_Y;
				humanoids[i].setActive(true);
			}
			else
			{
				humanoids[i].setActive(false);
			}
		}

		for (uint8_t i = 0; i < TOTAL_PARTICLES; i++)
		{
			particles[i].setActive(false);
		}

		for (uint8_t i = 0; i < TOTAL_PLAYER_SHOTS; i++)
		{
			playerShots[i].setActive(false);
		}

		for (uint8_t i = 0; i < TOTAL_ENEMY_SHOTS; i++)
		{
			enemyShots[i].setActive(false);
		}

		for (uint8_t i = 0; i < TOTAL_MINES; i++)
		{
			mines[i].setActive(false);
		}
		
		for (uint8_t i = 0; i < TOTAL_PODS; i++)
		{
			pods[i].setActive(false);
		}

		for (uint8_t i = 0; i < TOTAL_BAITERS; i++)
		{
			baiters[i].setActive(false);
		}

		spawnBombers();
		spawnPods();
	}
}

void GameState::spawnBombers()
{
	uint8_t expectedBombers;
	if (level == 1)
	{
		expectedBombers = 0;
	}
	else if (level == 2)
	{
		expectedBombers = 3;
	}
	else if (level == 3)
	{
		expectedBombers = 4;
	}
	else
	{
		expectedBombers = 5;
	}

	for (uint8_t i = 0; i < expectedBombers; i++)
	{
		bombers[i].setActive(true);
	}
	distributeActiveBombers();

	liveEnemies += expectedBombers;
}

void GameState::spawnPods()
{
	uint8_t expectedPods;
	if (level == 1)
	{
		expectedPods = 0;
	}
	else if (level == 2)
	{
		expectedPods = 1;
	}
	else if (level == 3)
	{
		expectedPods = 3;
	}
	else
	{
		expectedPods = 4;
	}

	for (uint8_t i = 0; i < expectedPods; i++)
	{
		pods[i].setActive(true);
	}
	distributeActivePods();


	liveEnemies += expectedPods * 5; // * 5 to account for spawned swarmers
}

void GameState::onNewLife()
{
	playerShip.worldPos.x = 0;
	playerShip.worldPos.y = 0;
	playerShip.setActive(true, true);
	camera.worldPos.x = 320;

	// All remaining enemies should be moved
	for (uint8_t i = 0; i < TOTAL_LANDERS; i++)
	{
		if (landers[i].isActive())
		{
			landers[i].worldPos.x = getSafeSpawn();
			landers[i].worldPos.y = LANDER_SPAWN_ALT;
		}

		if (!landers[i].isMutant())
		{
			landers[i].startSeeking();
		}
	}

	distributeActiveBombers();
	distributeActivePods();

	// All remaining humanoids should be placed on the ground.
	for (uint8_t i = 0; i < TOTAL_HUMANOIDS; i++)
	{
		if (humanoids[i].isActive())
		{
			humanoids[i].worldPos.y = HUMANOID_SPAWN_Y;
			humanoids[i].startWalking();
		}
	}

	// All shots should be removed
	for (uint8_t i = 0; i < TOTAL_PLAYER_SHOTS; i++)
	{
		playerShots[i].setActive(false);
	}

	for (uint8_t i = 0; i < TOTAL_ENEMY_SHOTS; i++)
	{
		enemyShots[i].setActive(false);
	}

	for (uint8_t i = 0; i < TOTAL_MINES; i++)
	{
		mines[i].setActive(false);
	}
}

void GameState::distributeActiveBombers()
{
	uint8_t spawnedInGroup = 0;
	Vector2Int spawn;
	bool goRight = rand() % 2 == 0;

	for (int i = 0; i < TOTAL_BOMBERS; i++)
	{
		if (bombers[i].isActive())
		{
			if (spawnedInGroup == 0)
			{
				spawn.x = getSafeSpawn();
				spawn.y-=240;
			}

			bombers[i].onSpawn(spawn, goRight);

			spawnedInGroup++;
			if (spawnedInGroup == 3)
			{
				spawnedInGroup = 0;
				goRight = !goRight;
			}
			else
			{
				spawn.x += 320;
				spawn.y += 240;
			}
		}
	}
}

void GameState::distributeActivePods()
{
	Vector2Int spawn;

	for (int i = 0; i < TOTAL_PODS; i++)
	{
		if (pods[i].isActive())
		{
			spawn.x = getSafeSpawn();
			spawn.y = 0;
			pods[i].onSpawn(spawn, rand() % 2 == 0);
		}
	}
}

void GameState::onSmartBomb()
{
	if (smartBombs > 0 && invertCountdown == 0)
	{
		invertCountdown = 10;
		smartBombs--;

		for (int i = 0; i < TOTAL_LANDERS; i++)
		{
			if (landers[i].isActive() && landers[i].isVisible())
			{
				landers[i].destroy();
			}
		}

		for (int i = 0; i < TOTAL_BOMBERS; i++)
		{
			if (bombers[i].isActive() && bombers[i].isVisible())
			{
				bombers[i].destroy();
			}
		}

		for (int i = 0; i < TOTAL_PODS; i++)
		{
			if (pods[i].isActive() && pods[i].isVisible())
			{
				pods[i].destroy(true, &playerShip);
			}
		}


		for (int i = 0; i < TOTAL_SWARMERS; i++)
		{
			if (swarmers[i].isActive() && swarmers[i].isVisible())
			{
				swarmers[i].destroy(true);
			}
		}

		for (int i = 0; i < TOTAL_BAITERS; i++)
		{
			if (baiters[i].isActive() && baiters[i].isVisible())
			{
				baiters[i].destroy();
			}
		}

		for (int i = 0; i < TOTAL_ENEMY_SHOTS; i++)
		{
			enemyShots[i].setActive(false);
		}

		for (int i = 0; i < TOTAL_MINES; i++)
		{
			mines[i].setActive(false);
		}
	}
}

void GameState::onHumanoidDestroyed()
{
	remainingHumanoids--;

	if (remainingHumanoids == 0)
	{
		invertCountdown = 10;

		for (int i = 0; i < TOTAL_LANDERS; i++)
		{
			if (landers[i].isActive())
			{
				landers[i].becomeMutant();
			}
		}
	}
}