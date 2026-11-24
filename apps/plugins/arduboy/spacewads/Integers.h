#pragma once

#define NUMBER_OF_STARS 25
#define NUMBER_OF_ASTEROIDS 10
#define NUMBER_OF_BULLETS 5

#define MODE_ACTIVE 4
#define MODE_EXPLOSION_3 3
#define MODE_EXPLOSION_2 2
#define MODE_EXPLOSION_1 1
#define MODE_INACTIVE 0
//Integers and Bools ect.
uint8_t gamestate = 0;


//Player
class Player
{
  public:
    uint8_t x = 0;
    uint8_t y = 0;
    bool pause = false;
    uint8_t mode = 0;  // 0 = inactive, 1 = explosion1, 2 = explostion2, 3 = explosion3, 4 = active
    uint8_t lives = 3;
};
Player player;
//Star                                                                                                                                                     
 struct Star
{
  public:
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t inc = 0;
    bool active;
    bool randomize = true;
};
Star star[NUMBER_OF_STARS];

//Asteroid
struct Asteroid
{
  public:
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t inc = 0;
    uint8_t mode = 0;  // 0 = inactive, 1 = explosion1, 2 = explostion2, 3 = explosion3, 4 = active
    bool randomize = true;
};

Asteroid asteroid[NUMBER_OF_ASTEROIDS];
  
struct Bullet {
  uint8_t x = 0;
  int8_t y = 0;
};

Bullet bullet[NUMBER_OF_BULLETS];

