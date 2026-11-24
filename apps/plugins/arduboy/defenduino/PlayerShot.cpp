#include "PlayerShot.h"
#include "Globals.h"

const unsigned char spriteRight[] PROGMEM = { 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x00, 0x00, 0x1, 0x1, 0x1, 0x00, 0x1, 0x1, 0x00, 0x1, 0x1, 0x1, 0x00, 0x00, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x00, 0x00, 0x00, 0x1, 0x1, 0x1, 0x1, 0x00, 0x1, 0x1, 0x1, 0x1, 0x00, 0x00, 0x1, 0x1, 0x1, 0x00, 0x1, 0x1, 0x1, 0x00, 0x1, 0x00, 0x1, 0x1, 0x00, 0x1, 0x1, 0x1, 0x00, 0x00, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, };

#define FLAG_SHRINKING 0x4
#define FLAG_TIP_ON_SCREEN 0x8

void PlayerShot::render(Vector2Int screenPos)
{
	if (screenPos.x < 0 || screenPos.x >= SCREEN_WIDTH)
	{
		setFlag(&flags, FLAG_SHRINKING);
		unsetFlag(&flags, FLAG_TIP_ON_SCREEN);
	}

	if (velocity.x > 0)
	{
		arduboy.drawBitmap(screenPos.x - length, screenPos.y, (const uint8_t *)spriteRight, length, 1, WHITE);
	}
	else
	{
		arduboy.drawBitmap(screenPos.x, screenPos.y, (const uint8_t *)spriteRight, length, 1, WHITE);
	}
}

void PlayerShot::fire(float xVelocity)
{
	length = 1;
	velocity.x = xVelocity;
	unsetFlag(&flags, FLAG_SHRINKING);
	setFlag(&flags, FLAG_TIP_ON_SCREEN);

}

void PlayerShot::update()
{
	MovingGameObject::update();

	if (!isFlagSet(flags, FLAG_SHRINKING))
	{
		length += 2;
		if (length >= 64)
		{
			length = 64;
		}
	}
	else
	{
		if (length <= 2)
		{
			length = 0;
			setActive(false);
		}
		else
		{
			length -= 2;
		}
	}
}

Rect PlayerShot::getCollisionRect()
{
	uint8_t iVel = abs(velocity.x) / 10.0;
	if (iVel < 1)
	{
		iVel = 1;
	}

	iVel += 2;

	if (velocity.x < 0)
	{
		return Rect(worldPos.getPixelX(), worldPos.getPixelY(), iVel, 1);
	}
	else
	{
		return Rect(worldPos.getPixelX() - iVel, worldPos.getPixelY(), iVel, 1);
	}
}

bool PlayerShot::tipOnScreen()
{
	return isFlagSet(flags, FLAG_TIP_ON_SCREEN);
}