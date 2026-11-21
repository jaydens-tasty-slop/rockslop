#include "mine.h"

#include "mine_detector.h"
#include "game.h"


Mine::Mine(Tank *tank):
    PhysicsBody(MINE_SIZE, MINE_SIZE),
    countdown(MINE_COUNTDOWN) {
    tile_collisions = false;
    respect_holes = false;

    position_x = tank->position_x + (TANK_SIZE - MINE_SIZE) / 2;
    position_y = tank->position_y + (TANK_SIZE - MINE_SIZE) / 2;

    parent = tank;

    new (std::nothrow) MineDetector(this);
}

Mine::~Mine() {
    if(parent) {
        ((Tank*)parent)->num_mines--;
    }
}

void Mine::process() {
    //Ignore mines which have already finished their countdowns
    if(!countdown) return;

    if(--countdown == EXPLOSION_ANIM) {
        detonate();
    }
    if(countdown == 0) {
        active = false;
    }

//todo: range detection
}

void Mine::detonate() {
    if(countdown < EXPLOSION_ANIM) {
        // Don't explode multiple times
        return;
    }

    countdown = EXPLOSION_ANIM - 1;

    // todo: The original game uses a radius, not a square

    for(uint8_t j = COORD_TO_X_TILE(center_x() - MINE_EXPLOSION_RADIUS);
        j <= COORD_TO_X_TILE(center_x() + MINE_EXPLOSION_RADIUS); j++) {
        if(j < 0 || j >= LEVEL_SIZE_X) continue;
        for(uint8_t k = COORD_TO_Y_TILE(center_y() - MINE_EXPLOSION_RADIUS);
            k <= COORD_TO_Y_TILE(center_y() + MINE_EXPLOSION_RADIUS); k++) {
            if(k < 0 || k >= LEVEL_SIZE_Y) continue;
            if(TILE_TYPE(game.tiles[k][j]) == DESTRUCTIBLE) game.tiles[k][j] = 0;
        }
    }

    // Kill any nearby physics objects
    for(auto & obj : objects) {
        if(obj != this && center_distance_less_than(obj, MINE_EXPLOSION_RADIUS)) {
            obj->handle_explosion();
        }
    }

}

void Mine::handle_explosion() {
    detonate();
}

void Mine::handle_collision(PhysicsBody *other) {
    other->collide(this);
}

void Mine::collide(Shell *shell) {
    shell->collide(this);
}
