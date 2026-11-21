/* Tanks! engine glue and renderer for Rockbox. MIT, see ../LICENSE.
 * The artwork of the original is 3D renders; the tanks, shells and mines are
 * drawn here as small shapes. One tile is 8x8 pixels, the border walls are
 * not drawn (the playfield is 16x17 tiles). */
#include "game.h"
#include "levels.h"
#include "ai.h"
#include "mine.h"
#include "tanks_core.h"

#define TILE_PX 8
#define COL_FLOOR 0x30382c
#define COL_WALL 0x8c8c98
#define COL_WALL_EDGE 0x5c5c68
#define COL_BLOCK 0xa06838
#define COL_HOLE 0x080808

static unsigned seed_state = 1;

extern "C" unsigned tanks_random(void)
{
    seed_state = seed_state * 1103515245u + 12345u;
    return (seed_state >> 8) & 0x7fffff;
}

extern "C" void tanks_seed(unsigned seed) { seed_state = seed | 1; }
extern "C" int tanks_level_count(void) { return LEVEL_COUNT; }
extern "C" const char *tanks_level_name(int level) { return levels[level].name; }
extern "C" void tanks_new_game(void) { game_new(); }
extern "C" void tanks_begin_level(int level) { game_begin_level(level); }
extern "C" void tanks_retry_level(void) { game_start_mission(game.mission); }
extern "C" int tanks_lives(void) { return game.lives; }
extern "C" int tanks_kills(void) { return game.total_kills; }
extern "C" int tanks_enemies(void) { return game.num_tanks > 0 ? game.num_tanks - 1 : 0; }
extern "C" int tanks_level(void) { return game.mission; }

extern "C" int tanks_step(int move, int turn, int fire, int mine, int aim)
{
    struct input_state in;
    in.move = (uint8_t)move;
    in.turn = (int8_t)turn;
    in.fire = fire;
    in.mine = mine;
    if (aim)
        game_aim_assist();
    switch (game_step(&in)) {
    case NEXT_LEVEL: return TS_NEXT_LEVEL;
    case LOSE_LIFE: return TS_LOST_LIFE;
    case LOSE: return TS_GAME_OVER;
    default: return TS_RUNNING;
    }
}

/* world units (256 per tile, first tile is the border) to playfield pixels */
static inline int px(int w) { return (w - TILE_SIZE) * TILE_PX / TILE_SIZE; }

static const unsigned tank_colors[NUM_TANK_TYPES] = {
    0x3c78ff, 0x96641e, 0x969696, 0x3cc8c8, 0xe6d23c,
    0xdc3232, 0x3caa46, 0xaa46c8, 0xf0f0f0, 0x282828,
};

static void draw_tiles(void)
{
    tv_fill(0, 0, PLAY_W, PLAY_H, COL_FLOOR);
    for (int ty = 1; ty < LEVEL_SIZE_Y - 1; ty++) {
        for (int tx = 1; tx < LEVEL_SIZE_X - 1; tx++) {
            tile_t t = game.tiles[ty][tx];
            int x = (tx - 1) * TILE_PX, y = (ty - 1) * TILE_PX;
            if (TILE_HEIGHT(t)) {
                if (TILE_TYPE(t) == DESTRUCTIBLE) {
                    tv_fill(x, y, TILE_PX, TILE_PX, COL_BLOCK);
                    tv_fill(x + 1, y + 1, TILE_PX - 2, TILE_PX - 2, 0xc88c50);
                } else {
                    tv_fill(x, y, TILE_PX, TILE_PX, COL_WALL_EDGE);
                    tv_fill(x, y, TILE_PX - 1, TILE_PX - 1, COL_WALL);
                }
            } else if (TILE_TYPE(t) == HOLE) {
                tv_fill(x + 1, y + 1, TILE_PX - 2, TILE_PX - 2, COL_HOLE);
            }
        }
    }
}

static void draw_tank(const Tank *t, int tick)
{
    int cx = px(t->center_x()), cy = px(t->center_y());
    unsigned col = tank_colors[t->type];
    int c = fast_cos(t->tread_rot), s = fast_sin(t->tread_rot);
    bool horizontal = abs(c) >= abs(s);

    if (t->type == INVISIBLE && (tick & 8))
        col = COL_FLOOR + 0x303030;
    if (horizontal)
        tv_fill(cx - 3, cy - 2, 7, 5, col);
    else
        tv_fill(cx - 2, cy - 3, 5, 7, col);
    tv_fill(cx - 1, cy - 1, 3, 3, t->type == PLAYER ? 0xc8e0ff : 0x202020);
    tv_line(cx, cy, cx + fast_cos(t->barrel_rot) * 6 / TRIG_SCALE,
            cy + fast_sin(t->barrel_rot) * 6 / TRIG_SCALE,
            t->type == PLAYER ? 0xffffff : 0x101010);
}

static void draw_mine(const Mine *m)
{
    int cx = px(m->center_x()), cy = px(m->center_y());

    if (m->countdown > EXPLOSION_ANIM) {
        bool warn = m->countdown < MINE_WARNING && (m->countdown & 2);
        tv_fill(cx - 1, cy - 1, 3, 3, warn ? 0xff3030 : 0xd0c030);
    } else {
        int r = (EXPLOSION_ANIM - (int)m->countdown) * 20 / EXPLOSION_ANIM;
        if (r > 20)
            r = 20;
        tv_fill(cx - r, cy - r, 2 * r + 1, 2 * r + 1,
                m->countdown & 1 ? 0xff9020 : 0xffe060);
    }
}

extern "C" void tanks_draw(int tick)
{
    draw_tiles();

    /* the player's first aim bounce */
    if (game.player) {
        struct line_seg seg;
        raycast(game.player->center_x(), game.player->center_y(),
                game.player->barrel_rot, game.tiles, &seg);
        tv_line(px(seg.x1), px(seg.y1), px(seg.x2), px(seg.y2), 0x506050);
    }

    for (auto &obj : PhysicsBody::objects) {
        switch (obj->kind()) {
        case KIND_MINE:
            draw_mine((const Mine *)obj);
            break;
        default:
            break;
        }
    }
    for (auto &obj : PhysicsBody::objects) {
        if (obj->kind() == KIND_TANK)
            draw_tank((const Tank *)obj, tick);
    }
    for (auto &obj : PhysicsBody::objects) {
        if (obj->kind() == KIND_SHELL)
            tv_fill(px(obj->center_x()) - 1, px(obj->center_y()) - 1, 2, 2, 0xffffff);
    }
}
