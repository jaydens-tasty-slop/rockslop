#include <Gamebuino-Meta.h>
#include "GameEngine.h"
#include "constants.h"
#include "HeartBrick.h"
#include "Paddle.h"
#include "Ball.h"
#include "Renderer.h"
#include "Application.h"

const uint8_t GameEngine::WALL_X_OFFSET =  8;
const uint8_t GameEngine::WALL_Y_OFFSET = 20;
const uint8_t GameEngine::BRICK_PADDING =  1;

uint8_t GameEngine::destroyedBricks = 0;
bool GameEngine::ballReady = false;

HeartBrick* GameEngine::wall[WALL_WIDTH][WALL_HEIGHT];
Paddle* GameEngine::paddle = NULL;
Ball* GameEngine::ball = NULL;

uint8_t GameEngine::mode = GameEngine::MODE_STANDBY;

void GameEngine::init() {
    initWall();
    initPaddle();
}

void GameEngine::initWall() {
    HeartBrick* brick;
    size_t x,y;
    for (x=0; x<WALL_WIDTH; x++) {
        for (y=0; y<WALL_HEIGHT; y++) {
            brick = new HeartBrick(
                HEART_BRICK_WIDTH,
                HEART_BRICK_HEIGHT,
                WALL_X_OFFSET + x*(HEART_BRICK_WIDTH  + BRICK_PADDING),
                SCREEN_HEIGHT - WALL_Y_OFFSET - (y + 1)*(HEART_BRICK_HEIGHT + BRICK_PADDING),
                0,
                0
            );
            wall[x][y] = brick;
            Renderer::addListener(brick);
        }
    }
}

void GameEngine::initPaddle() {
    paddle = new Paddle(
        PADDLE_WIDTH,
        PADDLE_HEIGHT,
        -PADDLE_WIDTH,
        10,
        4,
        0
    );

    Renderer::addListener(paddle);
}

void GameEngine::initBall() {
    ball = new Ball(
        BALL_WIDTH,
        BALL_HEIGHT,
        paddle->getX() + .5*(PADDLE_WIDTH - BALL_WIDTH),
        paddle->getY() + PADDLE_HEIGHT + 2,
        0,
        0
    );

    Renderer::addListener(ball);

    ballReady = true;
}

void GameEngine::tick() {
    paddle->tick();

    switch (mode) {
        case MODE_STANDBY:
            // (upstream writes through a null ball until the paddle has
            // slid into place; a no-op on its MCU, a crash here)
            if (ball) ball->setX(paddle->getX() + .5*(PADDLE_WIDTH - BALL_WIDTH));
            break;
        case MODE_STARTED:
            ball->tick();
            // (the checks can end the game, which deletes paddle and ball;
            // upstream goes on reading through the null pointers)
            checkCollisionWithScreenEdges();
            if (!ball) break;
            checkCollisionWithPaddle();
            if (!ball) break;
            checkCollisionWithWall();
            break;
    }
}

void GameEngine::checkCollisionWithScreenEdges() {
    int16_t x = ball->getX();
    int16_t y = ball->getY();
    int16_t vx = ball->getVx();
    int16_t vy = ball->getVy();

    if (x <= 0) {
        ball->setX(0);
        ball->setVx(-vx);
    } else if (x + BALL_WIDTH >= SCREEN_WIDTH-1) {
        ball->setX(SCREEN_WIDTH - 1 - BALL_WIDTH);
        ball->setVx(-vx);
    }

    if (y <= 0) {
        destroyWall();
    } else if (y + BALL_HEIGHT > SCREEN_HEIGHT-1) {
        ball->setY(SCREEN_HEIGHT - 1 - BALL_HEIGHT);
        ball->setVy(-vy);
    }
}

void GameEngine::checkCollisionWithPaddle() {
    int16_t px = paddle->getX() + .5*PADDLE_WIDTH;
    int16_t py = paddle->getY() + PADDLE_HEIGHT;
    int16_t bx = ball->getX() + .5*BALL_WIDTH;
    int16_t by = ball->getY();
    int16_t dx = bx - px;

    if (abs(dx) < (.5*PADDLE_WIDTH - 2) && by <= py) {
        int16_t bvx = ball->getVx();
        int16_t bvy = ball->getVy();
        ball->setY(py);
        ball->setVx(max(-5, min(5, bvx + 3*2*dx/PADDLE_WIDTH)));
        ball->setVy(-bvy);
    }
}

void GameEngine::checkCollisionWithWall() {
    int16_t bx = ball->getX() + .5*BALL_WIDTH;
    int16_t by = ball->getY() + .5*BALL_HEIGHT;
    uint8_t bw = HEART_BRICK_WIDTH + BRICK_PADDING;
    uint8_t bh = HEART_BRICK_HEIGHT + BRICK_PADDING;
    int16_t x = bx - WALL_X_OFFSET;
    int16_t y = SCREEN_HEIGHT - WALL_Y_OFFSET - by;

    if (x >= 0 && x <= bw * WALL_WIDTH && y >= 0 && y <= bh *  WALL_HEIGHT) {
        uint8_t bi = x / bw;
        uint8_t bj = y / bh;

        if (wall[bi][bj] != NULL && bi < WALL_WIDTH && bj < WALL_HEIGHT) {
            HeartBrick* brick = wall[bi][bj];
            performCollisionWithBrick(brick, bi, bj);
        }
    }
}

void GameEngine::performCollisionWithBrick(HeartBrick* brick, uint8_t i, uint8_t j) {
    int16_t bx = ball->getX() + .5*BALL_WIDTH;
    int16_t by = ball->getY() + .5*BALL_HEIGHT;
    int16_t hbx = brick->getX() + .5*HEART_BRICK_WIDTH;
    int16_t hby = brick->getY() + .5*HEART_BRICK_HEIGHT;
    int16_t dx = bx - hbx;
    int16_t dy = by - hby;
    int16_t vx = ball->getVx();
    int16_t vy = ball->getVy();

    int16_t a = 180 * (atan2(dy, dx) / 3.1415926536);

    if (a >= -140 && a < -40 && vy > 0) {
        ball->setVy(-vy);
    } else if (a >= -40 && a < 50 && vx < 0) {
        ball->setVx(-vx);
    } else if (a >= 50 && a < 140 && vy < 0) {
        ball->setVy(-vy);
    } else if (vx > 0) {
        ball->setVx(-vx);
    }

    destroyBrick(brick, i, j);
}

void GameEngine::destroyBrick(HeartBrick* brick, uint8_t i, uint8_t j) {
    Renderer::removeListener(brick);
    wall[i][j] = NULL;
    delete brick;
    destroyedBricks++;
    if (destroyedBricks == WALL_WIDTH * WALL_HEIGHT) {
        stop();
    }
}

void GameEngine::paddleReady() {
    initBall();
}

void GameEngine::start() {
    if (ballReady) {
        mode = MODE_STARTED;
        ball->start();
    }
}

void GameEngine::stop() {
    mode = MODE_STOPPED;
    Renderer::removeListener(paddle);
    Renderer::removeListener(ball);
    delete paddle;
    delete ball;
    paddle = NULL;
    ball = NULL;
    destroyedBricks = 0;
    ballReady = false;
    mode = MODE_STANDBY;
    Application::stopGame();
}

void GameEngine::suspend() {
    mode = MODE_SUSPENDED;
}

void GameEngine::resume() {
    mode = MODE_STARTED;
}

void GameEngine::movePaddleLeft() {
    paddle->left();
}

void GameEngine::movePaddleRight() {
    paddle->right();
}

void GameEngine::destroyWall() {
    size_t x,y;
    for (x=0; x<WALL_WIDTH; x++) {
        for (y=0; y<WALL_HEIGHT; y++) {
            if (wall[x][y] != NULL) {
                destroyBrick(wall[x][y], x, y);
            }
        }
    }
}