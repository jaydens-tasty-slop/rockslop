#include "GameObject.h"
#include "Globals.h"

GameObject::GameObject()
{
}

bool GameObject::isActive()
{
	return isFlagSet(flags, FLAG_ACTIVE);
}

bool GameObject::isVisible()
{
	return isFlagSet(flags, FLAG_VISIBLE);
}

void GameObject::setActive(bool bIsActive)
{
	if (bIsActive)
	{
		setFlag(&flags, FLAG_ACTIVE);
	}
	else
	{
		unsetFlag(&flags, FLAG_ACTIVE);
	}
}

