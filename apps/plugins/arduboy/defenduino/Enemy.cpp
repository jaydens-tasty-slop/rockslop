#include "Globals.h"
#include "Enemy.h"

bool Enemy::collisionCheck(uint8_t width, uint8_t height, PlayerShot* pPlayerShots, PlayerShip* pPlayerShip)
{
	Rect thisRect = Rect(worldPos.getPixelX(), worldPos.getPixelY(), width, height);	
	for (int i = 0; i < TOTAL_PLAYER_SHOTS; i++)
	{
		if (pPlayerShots[i].isActive() && pPlayerShots[i].tipOnScreen() && arduboy.collide(pPlayerShots[i].getCollisionRect(), thisRect))
		{
			pPlayerShots[i].setActive(false);
			return true;
		}
	}
	
	if (pPlayerShip->isActive() and !pPlayerShip->isExploding() && arduboy.collide(pPlayerShip->getCollisionRect(), thisRect))
	{
		pPlayerShip->destroy();
		return true;
	}

	return false;
}

void Enemy::destroy(uint16_t score, bool isCounted)
{
	unsetFlag(&flags, FLAG_ACTIVE);
	explodeObject(&flags, worldPos, PARTICLES_EXPLOSION);
	pGameState->addToScore(score);

	if (isCounted)
	{
		pGameState->onCountedEnemyDeath();
	}
}