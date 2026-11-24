#ifndef SWEET_VALENTINE_GAME_ENGINE
#define SWEET_VALENTINE_GAME_ENGINE

#include <Gamebuino-Meta.h>
#include "constants.h"
#include "HeartBrick.h"
#include "Paddle.h"
#include "Ball.h"

class GameEngine
{
    private:

        static const uint8_t WALL_X_OFFSET;
        static const uint8_t WALL_Y_OFFSET;
        static const uint8_t BRICK_PADDING;

        static HeartBrick* wall[WALL_WIDTH][WALL_HEIGHT];
        static Paddle* paddle;
        static Ball* ball;
        static uint8_t destroyedBricks;
        static bool ballReady;

        static void initWall();
        static void initPaddle();
        static void initBall();
        static void checkCollisionWithScreenEdges();
        static void checkCollisionWithPaddle();
        static void checkCollisionWithWall();
        static void performCollisionWithBrick(HeartBrick* brick, uint8_t i, uint8_t j);
        static void destroyBrick(HeartBrick* brick, uint8_t i, uint8_t j);
        static void destroyWall();

    public:

        static const uint8_t MODE_STANDBY   = 0;
        static const uint8_t MODE_STARTED   = 1;
        static const uint8_t MODE_SUSPENDED = 2;
        static const uint8_t MODE_STOPPED   = 3;

        static uint8_t mode;
    
        static void init();
        static void tick();
        static void start();
        static void stop();
        static void suspend();
        static void resume();
        static void movePaddleLeft();
        static void movePaddleRight();
        static void paddleReady();
};

#endif