#pragma once

#include "Vector.h"

class Ball
{
private:
    static constexpr float _friction = .60;           // percentage to reduce velocity by every second
    static constexpr uint8_t _powerChangePerSecond = 100;
    static constexpr float _aimChangePerSecond = 1.75;
    static constexpr float _minVelocityThreshold = 4;
    static constexpr float _minVelocitySecondsThreshold = 1; // stop the ball when velocity < threshold for this many seconds

    bool _powerIncreasing = true;
    float _minVelocitySeconds = 0; // how long velocity has been below the minThreshold

public:
    float X = 0, Y = 0;
    Vector Velocity = {0, 0};   // used for when the ball is in motion
    float Direction = 0;        // used for choosing which direction to hit the ball (units = radians)
    float Power = DefaultPower; // how hard to hit the ball

    static constexpr uint8_t Radius = 2;
    static constexpr uint8_t MinPower = 20;
    static constexpr uint8_t MaxPower = 150;
    static constexpr uint8_t DefaultPower = (MaxPower + MinPower) / 2;

    Ball() = default;
    Ball(float x, float y) : X(x), Y(y) {}

    void RotateDirectionClockwise(float secondsDelta) {
        Direction -= _aimChangePerSecond * secondsDelta;
        if (Direction < 0)
            Direction = TWO_PI + Direction;
    }

    void RotateDirectionCounterClockwise(float secondsDelta) {
        Direction += _aimChangePerSecond * secondsDelta;
        if (Direction > TWO_PI)
            Direction = Direction - TWO_PI;
    }

    void ResetPower()
    {
        Power = DefaultPower;
        _powerIncreasing = true;
    }

    void TickPower(float secondsDelta)
    {
        if (_powerIncreasing)
        {
            Power += _powerChangePerSecond * secondsDelta;
            if (Power > MaxPower)
            {
                Power = MaxPower;
                _powerIncreasing = false;
            }
        }
        else
        {
            Power -= _powerChangePerSecond * secondsDelta;
            if (Power < MinPower)
            {
                Power = MinPower;
                _powerIncreasing = true;
            }
        }
    }

    void StartHit()
    {
        Velocity.x = cos(Direction) * Power;
        Velocity.y = -sin(Direction) * Power;
        _minVelocitySeconds = 0;
    }

    void Move(float secondsDelta)
    {
        X += Velocity.x * secondsDelta;
        Y += Velocity.y * secondsDelta;

        ApplyFriction(secondsDelta);
    }

    void ApplyFriction(float secondsDelta)
    {
        Velocity.x -= Velocity.x * _friction * secondsDelta;
        Velocity.y -= Velocity.y * _friction * secondsDelta;

        if (Velocity.Length() < _minVelocityThreshold)
            _minVelocitySeconds += secondsDelta;
        else
            _minVelocitySeconds = 0;
    }

    bool IsStopped()
    {
        return _minVelocitySeconds >= _minVelocitySecondsThreshold;
    }
};