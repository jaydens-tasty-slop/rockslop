#pragma once

#include "PlayerShip.h"
#include "GameCamera.h"
#include "Humanoid.h"
#include "PlayerShot.h"
#include "Lander.h"
#include "Landscape.h"
#include "Particles.h"
#include "EnemyShot.h"
#include "Bomber.h"
#include "Mine.h"
#include "Pod.h"
#include "Swarmer.h"
#include "Baiter.h"

#include <Arduboy2.h>

#define TOTAL_PLAYER_SHOTS 4
#define TOTAL_LANDERS 20
#define TOTAL_HUMANOIDS 10
#define TOTAL_PARTICLES 6
#define TOTAL_ENEMY_SHOTS 4
#define TOTAL_BOMBERS 5
#define TOTAL_MINES 6
#define TOTAL_PODS 4
#define TOTAL_SWARMERS 16
#define TOTAL_BAITERS 3

#define NO_HUMANOID_FOUND 255

class GameState
{
public:
	GameState();
	void update();
	PlayerShot* getPlayerShot();
	EnemyShot* getEnemyShot();
	Mine* getMine();
	Swarmer* getSwarmer();

	Particles* getParticles();
	void lostLife();
	GameCamera* getCamera();
	void freezeActors();
	void completeSpawningLander(int xPos, int yPos);
	void completeSpawningBaiter(int xPos, int yPos);
	uint8_t getCapturableHumanoidAtPosition(int xPos);
	Humanoid* getHumanoid(uint8_t index);
	void onCountedEnemyDeath(uint8_t total = 1);
	void addToScore(uint16_t toAdd);
	void onSmartBomb();
	PlayerShip playerShip;
	void onHumanoidDestroyed();
	unsigned long score = 0;
	uint8_t remainingHumanoids = TOTAL_HUMANOIDS;

private:
	GameCamera camera;
	PlayerShot playerShots[TOTAL_PLAYER_SHOTS];
	Lander landers[TOTAL_LANDERS];
	Landscape landscape;
	Humanoid humanoids[TOTAL_HUMANOIDS];
	Particles particles[TOTAL_PARTICLES];
	EnemyShot enemyShots[TOTAL_ENEMY_SHOTS];
	Bomber bombers[TOTAL_BOMBERS];
	Mine mines[TOTAL_MINES];
	Pod pods[TOTAL_PODS];
	Swarmer swarmers[TOTAL_SWARMERS];
	Baiter baiters[TOTAL_BAITERS];

	unsigned long nextBonus = 10000;

	Lander* getInactiveLander();
	Baiter* getInactiveBaiter();
	void startSpawningLander();
	void startSpawningBaiter();
	bool spawnPosTooCloseToPlayer(int xPos);
	int getSafeSpawn();
	void spawnWave(uint8_t maxForLevel);
	void drawGui();
	void drawScanner();
	void plotOnScanner(uint8_t scannerY, GameObject* pGameObject);
	void inPlayUpdate();
	void interstitialUpdate();
	uint8_t getExpectedLandersForLevel();
	void onNewLevel();
	void onNewLife();
	void spawnBombers();
	void spawnPods();
	void distributeActiveBombers();
	void distributeActivePods();

	uint8_t flags = 0;
	uint8_t spawnedLanders = 0;
	uint8_t lives = 3;
	uint8_t smartBombs = 3;
	uint8_t level = 0;
	uint8_t liveEnemies = 0;
	uint16_t spawnCountdown = 0;
	uint8_t invertCountdown = 0;

};

