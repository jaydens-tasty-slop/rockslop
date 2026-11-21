#include "tank.h"

#include "ai.h"
#include "mine_detector.h"
#include "game.h"

const struct Tank::type_data Tank::types[NUM_TANK_TYPES] = {
        {5, 1, 2, (uint8_t)TANK_SPEED_HIGH},
        {1, 1, 0, (uint8_t)0},
        {1, 1, 0, (uint8_t)TANK_SPEED_SLOW},
        {1, 0, 0, (uint8_t)TANK_SPEED_SLOW},
        {1, 1, 4, (uint8_t)TANK_SPEED_HIGH},
        {3, 1, 0, (uint8_t)TANK_SPEED_NORMAL},
        {2, 2, 0, (uint8_t)0},
        {5, 1, 2, (uint8_t)TANK_SPEED_HIGH},
        {5, 1, 2, (uint8_t)TANK_SPEED_NORMAL},
        {2, 0, 2, (uint8_t)TANK_SPEED_BLACK},
};

Tank::Tank(const struct serialized_tank *ser_tank, uint8_t id):
    PhysicsBody(TANK_SIZE, TANK_SIZE),
    type(ser_tank->type),
    id(id),
    // add 1 because the level system uses coordinates from the first non-border block
    start_x(ser_tank->start_x + 1),
    start_y(ser_tank->start_y + 1) {

    tile_collisions = true;
    respect_holes = true;

    position_x = TILE_TO_X_COORD(start_x);
    position_y = TILE_TO_Y_COORD(start_y);

    if(id == 0) {
        game.player = this;
    }

    game.num_tanks++;
}

Tank::~Tank() {
    game.num_tanks--;
    if(this == game.player) {
        game.player = nullptr;
    }
}

void Tank::kill() {
    if(this != game.player) {
        game.total_kills++;
        game.kills[type]++;
        game.alive_tanks[id] = false;
    }

    active = false;
}

void Tank::process() {

    ai_process_move(this);
    ai_process_fire(this);

    if(shot_cooldown) {
        shot_cooldown--;
    }
    if(mine_cooldown) {
        mine_cooldown--;
    }

}

void Tank::fire_shell() {
    if(!can_shoot()) return;
    Shell *shell = new (std::nothrow) Shell(this);
    if(!shell) {
            return;
    }

    num_shells++;
    shot_cooldown = SHOT_COOLDOWN;
}

bool Tank::can_shoot() const {
    return !shot_cooldown && num_shells < max_shells();
}

void Tank::lay_mine() {
    if(!can_lay_mine()) return;
    Mine *mine = new (std::nothrow) Mine(this);
    if(!mine) {
            return;
    }

    num_mines++;
    mine_cooldown = MINE_COOLDOWN;
}

bool Tank::can_lay_mine() const {
    return !mine_cooldown && num_mines < max_mines();
}

void Tank::set_velocity(int velocity) {
    if(velocity == 0) {
        velocity_x = 0;
        velocity_y = 0;
    } else {
        velocity_x = velocity * fast_cos(tread_rot) / TRIG_SCALE;
        velocity_y = velocity * fast_sin(tread_rot) / TRIG_SCALE;
    }
}

void Tank::handle_collision(PhysicsBody *other) {
    other->collide(this);
}

void Tank::handle_explosion() {
    kill();
}

void Tank::collide(Tank *tank) {
    //Figure out if the four corners are colliding
    bool top_right = tank->is_point_inside(position_x + width, position_y);
    bool bottom_right = tank->is_point_inside(position_x + width, position_y + height);
    bool top_left = tank->is_point_inside(position_x, position_y);
    bool bottom_left = tank->is_point_inside(position_x, position_y + height);

    uint dis_up = -1;
    uint dis_down = -1;
    uint dis_left = -1;
    uint dis_right = -1;

    if(!(top_right || bottom_right || top_left || bottom_left)) return;

    if((top_right || bottom_right)) {
        dis_right = position_x + width - tank->position_x;
    }
    if((top_left || bottom_left)) {
        dis_left = tank->position_x + tank->width - position_x;
    }
    if((top_left || top_right)) {
        dis_up = tank->position_y + tank->height - position_y;
    }
    if((bottom_left || bottom_right)) {
        dis_down = position_y + height - tank->position_y;
    }

    //pick the direction with the smallest distance
    if(dis_up < dis_left && dis_up < dis_right) {
        velocity_y += dis_up / 2;
        tank->velocity_y -= dis_up / 2;
    }
    if(dis_left < dis_up && dis_left < dis_down) {
        velocity_x += dis_left / 2;
        tank->velocity_x -= dis_left / 2;
    }
    if(dis_down < dis_left && dis_down < dis_right) {
        velocity_y -= dis_down / 2;
        tank->velocity_y += dis_down / 2;
    }
    if(dis_right < dis_up && dis_right < dis_down) {
        velocity_x -= dis_right / 2;
        tank->velocity_x += dis_right / 2;
    }
}

void Tank::collide(Shell *shell) {
    shell->collide(this);
}

void Tank::collide(MineDetector *detector) {
    detector->collide(this);
}
