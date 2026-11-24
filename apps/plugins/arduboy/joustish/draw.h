#include <Arduboy2.h>
extern Arduboy2 arduboy;
#include <Arduboy2.h>
extern void drawBackground();
extern void drawGame();

#define ANIM_WALK 16
#define ANIM_SKID  0
#define ANIM_FLY  48
#define ANIM_DEAD 64

#define ANIM_FLIP 8
#define ANIM_FRAME 16

extern uint16_t title_animation;
