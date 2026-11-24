/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

typedef int8_t int8;

struct Position 
{ 
    short x; short y;
    Position() : Position(0,0){};
    Position( short _x, short _y ) : x(_x) , y(_y) {}
};

struct Rectangle
{
    Position pos;
    short width, height;
};

//COLLISION CHECK RESULT
typedef enum __attribute__ ((packed))
{
    FREE,
    HIT_ENEMY,
    HIT_ENEMY_INOFFENSIVE,
    HIT_CHARACTER,
    HIT_TRIGGER,
    HIT_ITEM
} CollisionCheckResult;

//TRIGGER EVENT
typedef enum __attribute__ ((packed))
{
    NO_EVENT,
    START_ANIM,
    STOP_ANIM,
    START_DIALOG_2,
    END_LEVEL
} TriggerEvent;

//HIT TYPE
typedef enum __attribute__ ((packed))
{
    PEPPER_SPRAY
} HitType;

static const short SCREEN_HEIGHT = 64;
static const short SCREEN_WIDTH = 128;
static const short LEVEL_HEIGHT = 13;
static const short MID_WIDTH = SCREEN_WIDTH / 2;
static const short MID_HEIGHT = SCREEN_HEIGHT / 2;
static const short PLAYER_HEIGHT = 20;
static const short PLAYER_WIDTH = 9;
static const short PLAYER_LIFE = 5;
static const short PLAYER_ANIM_NB_FRAMES = 30;
static const short PLAYER_HIT_NB_FRAMES = 30;
static const short PLAYER_MOVE = 1;
static const float PLAYER_JUMP_VELOCITY = 2.0f;
static const float PLAYER_Y_VELOCITY_INC = 0.1f;
static const float PLAYER_FALL_MAX_VELOCITY = 2.0f;
static const short TILE_LENGTH = 10;
static const short TILE_SCREEN_Y_OFFSET = SCREEN_HEIGHT % TILE_LENGTH;
static const short LEVEL_HEIGHT_PIXELS = LEVEL_HEIGHT * TILE_LENGTH;
static const short MAX_ENEMIES_PER_LEVEL = 5;
static const short CHARACTER_MOVE = 1;
static const short ENEMY_MOVE = 1;
static const short ENEMY_FOLLOW_MOVE = 3;
static const short ENEMY_WALK_MAX = 25;
static const short ENEMY_WALK_FRAME_SKIP = 2;
static const short MAX_CHARACTERS_PER_LEVEL = 2;
static const short MAX_TRIGGERS_PER_LEVEL = 3;
static const short TRIGGER_HEIGHT = PLAYER_HEIGHT;
static const short TRIGGER_WIDTH = PLAYER_WIDTH;
static const short DIALOG_DEFAULT_INIT_CURSOR_X = 35;
static const short DIALOG_DEFAULT_CURSOR_Y = 20;
static const short DIALOG_CHAR_WIDTH = 6;
static const short DIALOG_CHAR_HEIGHT = 15;
static const short DIALOG_LETTER_NBFRAMES = 6;
static const short DIALOG_SENTENCE_END_NBFRAMES = 50;
static const short PEPPER_SPRAY_RANGE_X = TILE_LENGTH;
static const short PEPPER_SPRAY_RANGE_Y = TILE_LENGTH * 2;
static const short SNOWFLAKES_NUMBER = 50;
static const short SNOWFLAKES_FRAME_MOVE = 10;
static const short NB_NOTES_IN_SEQUENCE = 5;

template <typename T, short N>
constexpr short arraysize(T(&)[N])
{
    return N;
}

#endif
