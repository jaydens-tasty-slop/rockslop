#ifndef TANKS_PHYSICSBODY_H
#define TANKS_PHYSICSBODY_H

#include <stdint.h>
#include "objlist.h"

#include <stdbool.h>

#include "collision.h"
#include "fwd.h"
#include "physics.h"

enum { KIND_TANK, KIND_SHELL, KIND_MINE, KIND_DETECTOR };

class PhysicsBody {
public:
    PhysicsBody(uint width, uint height);
    virtual ~PhysicsBody();

    int position_x;
    int position_y;
    int velocity_x{0};
    int velocity_y{0};
    const uint width;
    const uint height;
    PhysicsBody *parent{nullptr};
    bool active{true};

    bool tile_collisions;
    // Whether or not to collide with holes
    bool respect_holes;

    // An array of physics objects, sorted from least to greatest Y position
    static ObjList objects;

    uint center_x() const;
    uint center_y() const;

    bool detect_collision(PhysicsBody *other) const;
    bool is_point_inside(int x, int y) const;
    direction_t process_tile_collision();
    bool center_distance_less_than(PhysicsBody *other, uint dis) const;
    bool collides_line(struct line_seg *seg) const;

    void tick();

    static void sort();
    static void remove_all();
    static void remove_inactive();

    virtual void process() = 0;
    virtual int kind() const = 0;

    virtual void handle_tile_collision(direction_t dir);
    virtual void handle_explosion();

    // Polymorphic ping-pong (aka "visitor pattern," apparently)
    virtual void handle_collision(PhysicsBody *other) = 0;
    virtual void collide([[maybe_unused]] Tank *tank);
    virtual void collide([[maybe_unused]] Shell *shell);
    virtual void collide([[maybe_unused]] Mine *mine);
    virtual void collide([[maybe_unused]] MineDetector *detector);
};

#endif //TANKS_PHYSICSBODY_H
