/* Fixed capacity replacement for ezSTL::vector<PhysicsBody*> (MIT, see
 * ../LICENSE): only what the engine uses. */
#ifndef TANKS_OBJLIST_H
#define TANKS_OBJLIST_H

#include <stddef.h>
#include "fwd.h"

#define MAX_OBJECTS 96

class ObjList {
public:
    typedef PhysicsBody **iterator;
    PhysicsBody *items[MAX_OBJECTS];
    size_t count{0};

    size_t size() const { return count; }
    iterator begin() { return items; }
    iterator end() { return items + count; }
    PhysicsBody *&operator[](size_t i) { return items[i]; }
    bool push_back(PhysicsBody *p) {
        if (count >= MAX_OBJECTS) return false;
        items[count++] = p;
        return true;
    }
    iterator erase(iterator it) {
        for (iterator i = it; i + 1 < end(); i++) *i = *(i + 1);
        count--;
        return it;
    }
};

#endif
