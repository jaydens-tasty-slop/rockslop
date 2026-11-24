/* Tanks! CE mission loop for Rockbox; see game.h. MIT. */
#include "game.h"
#include "levels.h"
#include "ai.h"

struct game game;

#define PLAYER_BARREL_ROTATION DEGREES_TO_ANGLE(5)
//1/3 of a second for 90 degree rotation
#define PLAYER_TREAD_ROTATION (DEGREES_TO_ANGLE(90) / (TARGET_TICK_RATE / 3))

int level_count(void)
{
    return LEVEL_COUNT;
}

void game_new(void)
{
    game.lives = 3;
    game.total_kills = 0;
    for (uint i = 0; i < sizeof(game.kills); i++)
        game.kills[i] = 0;
}

static void load_tiles(const struct level_def *def)
{
    for (int y = 0; y < LEVEL_SIZE_Y; y++) {
        for (int x = 0; x < LEVEL_SIZE_X; x++) {
            bool border = x == 0 || y == 0 || x == LEVEL_SIZE_X - 1 ||
                          y == LEVEL_SIZE_Y - 1;
            game.tiles[y][x] = border ? 1 :
                tile_from_char(def->map[y - 1][x - 1]);
        }
    }
}

void game_start_mission(int mission)
{
    const struct level_def *def = &levels[mission];

    PhysicsBody::remove_all();
    game.num_tanks = 0;
    game.player = nullptr;
    game.mission = mission;
    game.level.num_tanks = 0;

    /* tanks: the player first (id 0), then the enemies in map order */
    struct serialized_tank st[MAX_NUM_TANKS];
    int n = 0;
    for (int y = 0; y < LEVEL_SIZE_Y - 2 && n < MAX_NUM_TANKS; y++) {
        for (int x = 0; x < LEVEL_SIZE_X - 2 && n < MAX_NUM_TANKS; x++) {
            int t = tank_from_char(def->map[y][x]);
            if (t < 0)
                continue;
            struct serialized_tank s = { (tank_type_t)t, (uint8_t)x, (uint8_t)y };
            if (t == PLAYER) {
                for (int i = n; i > 0; i--)
                    st[i] = st[i - 1];
                st[0] = s;
            } else {
                st[n] = s;
            }
            n++;
        }
    }
    game.level.num_tanks = n;
    for (int i = 0; i < n; i++) {
        if (!game.alive_tanks[i])
            continue;
        new(std::nothrow) Tank(&st[i], i);
    }
    load_tiles(def);
    game.tick = 0;
}

void game_begin_level(int mission)
{
    for (int i = 0; i < MAX_NUM_TANKS; i++)
        game.alive_tanks[i] = true;
    game_start_mission(mission);
}

static void handle_movement(Tank *player, uint8_t keys)
{
    angle_t target_rot;

    switch (keys) {
    default:
        player->set_velocity(0);
        return;
    case UP:
        target_rot = DEGREES_TO_ANGLE(270);
        break;
    case DOWN:
        target_rot = DEGREES_TO_ANGLE(90);
        break;
    case LEFT:
        target_rot = DEGREES_TO_ANGLE(180);
        break;
    case RIGHT:
        target_rot = DEGREES_TO_ANGLE(0);
        break;
    case UP | RIGHT:
        target_rot = DEGREES_TO_ANGLE(315);
        break;
    case DOWN | RIGHT:
        target_rot = DEGREES_TO_ANGLE(45);
        break;
    case UP | LEFT:
        target_rot = DEGREES_TO_ANGLE(225);
        break;
    case DOWN | LEFT:
        target_rot = DEGREES_TO_ANGLE(135);
    }

    int diff = (int)(player->tread_rot - target_rot);
    if ((uint)abs(diff) > DEGREES_TO_ANGLE(90)) {
        player->tread_rot += DEGREES_TO_ANGLE(180);
        diff = (int)(player->tread_rot - target_rot);
    }
    if (diff < -(int)PLAYER_TREAD_ROTATION) {
        player->tread_rot += PLAYER_TREAD_ROTATION;
    } else if (diff > (int)PLAYER_TREAD_ROTATION) {
        player->tread_rot -= PLAYER_TREAD_ROTATION;
    } else {
        player->tread_rot = target_rot;
    }

    if ((uint)abs(diff) <= DEGREES_TO_ANGLE(45)) {
        player->set_velocity(TANK_SPEED_NORMAL);
    } else {
        player->set_velocity(0);
    }
}

void game_aim_assist(void)
{
    Tank *player = game.player;
    Tank *best = nullptr;
    uint best_d = ~0u;

    if (!player)
        return;
    for (auto &obj : PhysicsBody::objects) {
        if (obj->kind() != KIND_TANK || obj == player || !obj->active)
            continue;
        int dx = (int)obj->center_x() - (int)player->center_x();
        int dy = (int)obj->center_y() - (int)player->center_y();
        uint d = (uint)(dx * dx + dy * dy);
        if (d < best_d) {
            best_d = d;
            best = (Tank *)obj;
        }
    }
    if (best)
        point_at_player(player, best);
}

int game_step(const struct input_state *in)
{
    Tank *player = game.player;

    if (player) {
        handle_movement(player, in->move);
        if (in->fire)
            player->fire_shell();
        if (in->mine)
            player->lay_mine();
        if (in->turn < 0)
            player->barrel_rot -= PLAYER_BARREL_ROTATION;
        else if (in->turn > 0)
            player->barrel_rot += PLAYER_BARREL_ROTATION;
    }

    for (auto &object : PhysicsBody::objects)
        object->process();
    process_collisions();
    for (auto &object : PhysicsBody::objects)
        object->tick();
    PhysicsBody::remove_inactive();
    PhysicsBody::sort();

    game.tick++;
    if (!game.player) {
        game.lives--;
        return game.lives ? LOSE_LIFE : LOSE;
    }
    if (game.num_tanks == 1)
        return NEXT_LEVEL;
    return GAME_RUNNING;
}
