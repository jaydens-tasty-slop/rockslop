#pragma once

#include "Map.h"

struct Vector
{
    float x, y;

    float DotProduct(const Vector &other) const
    {
        return x * other.x + y * other.y;
    }

    float Length() const
    {
        return sqrt(x * x + y * y);
    }

    Vector Normalize() const
    {
        float len = Length();
        return {x / len, y / len};
    }

    Vector operator-(const Point8& p) const {
        return {x - p.x, y - p.y};
    }

    Vector operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }
};