#include <Arduboy2.h>
#include "Vector2Int.h"

#ifndef   RENDERER_H
#define   RENDERER_H
class Renderer
{
public:
	virtual void render(Vector2Int screenPos);
};
#endif