#include "GameCamera.h"
#include "Globals.h"

#define MAX_CAMERA_SPEED 30
#define MAX_CAMERA_SPEED_PIXELS 3

GameCamera::GameCamera() : MovingGameObject()
{

}

void GameCamera::update(PlayerShip* pPlayerShip)
{
	MovingGameObject::update();

	int targetXPos = pPlayerShip->getCameraTarget();

	if (worldPos.x != targetXPos)
	{
		int pixelDistance =  (targetXPos - worldPos.x)/10;
		// TODO: Handle if target is wrapped around but less than camera speed, to avoid "jump" when crossing the border
		if (pixelDistance < 0)
		{
			if (pixelDistance < -MAX_CAMERA_SPEED_PIXELS)
			{
				if (pixelDistance < -HALF_WORLD_WIDTH_PIXELS)
				{
					// Go the other way if it's quicker to just wrap around
					worldPos.x += MAX_CAMERA_SPEED;
				}
				else
				{
					worldPos.x -= MAX_CAMERA_SPEED;
				}
			}
			else
			{
				worldPos.x = targetXPos;
			}
		}
		else
		{
			if (pixelDistance > MAX_CAMERA_SPEED_PIXELS)
			{
				if (pixelDistance > HALF_WORLD_WIDTH_PIXELS)
				{
					// Go the other way if it's quicker to just wrap around
					worldPos.x -= MAX_CAMERA_SPEED;
				}
				else
				{
					worldPos.x += MAX_CAMERA_SPEED;

				}
			}
			else
			{
				worldPos.x = targetXPos;
			}
		}

		if (worldPos.x >= WORLD_WIDTH_UNITS)
		{
			worldPos.x -= WORLD_WIDTH_UNITS;
		}
		else if (worldPos.x < 0)
		{
			worldPos.x += WORLD_WIDTH_UNITS;
		}
	}
}

Vector2Int GameCamera::worldToScreenPos(Vector2Int objPos)
{
	Vector2Int screenPos;
	screenPos.x = objPos.getPixelX() - (worldPos.getPixelX() - HALF_SCREEN_WIDTH);
	screenPos.y = objPos.getPixelY() + HALF_SCREEN_HEIGHT;

	// If far from the camera, flip to the other side for wrapping
	if (screenPos.x < -HALF_WORLD_WIDTH_PIXELS)
	{
		screenPos.x += WORLD_WIDTH_PIXELS;
	}
	else if (screenPos.x > HALF_WORLD_WIDTH_PIXELS)
	{
		screenPos.x -= WORLD_WIDTH_PIXELS;
	}

	return screenPos;
}

