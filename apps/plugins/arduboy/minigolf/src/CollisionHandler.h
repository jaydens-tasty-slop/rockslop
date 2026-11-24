#pragma once

#include "Ball.h"
#include "Map.h"
#include <Arduboy2.h>

class CollisionHandler
{
    CollisionHandler() = delete; // enforce this to be a static class

public:
    static void HandleAllCollisions(Ball &ball, const Map &map, float secondsDelta)
    {
        for (auto wall : map.walls)
        {
            if (wall.IsEmpty())
                continue;

            if (IsCollidingWall(ball, wall))
            {
                // handle Wall "end-caps" (should act like a tiny circle collision)
                Vector wallDir1 = {wall.p2.x - wall.p1.x, wall.p2.y - wall.p1.y};
                Vector wallDir2 = {wall.p1.x - wall.p2.x, wall.p1.y - wall.p2.y};
                if (ball.Velocity.DotProduct(wallDir1) > 0)
                {
                    Circle c = Circle(wall.p1.x, wall.p1.y, 1);
                    if (IsCollidingCircle(ball, c))
                        HandleCollisionCircle(ball, c);
                    else
                        HandleCollisionWall(ball, wall);
                }
                else if (ball.Velocity.DotProduct(wallDir2) > 0)
                {
                    Circle c = Circle(wall.p2.x, wall.p2.y, 1);
                    if (IsCollidingCircle(ball, c))
                        HandleCollisionCircle(ball, c);
                    else
                        HandleCollisionWall(ball, wall);
                }
                else
                {
                    HandleCollisionWall(ball, wall);
                }
            }
        }

        for (auto circle : map.circles)
        {
            if (circle.IsEmpty())
                continue;

            if (IsCollidingCircle(ball, circle))
                HandleCollisionCircle(ball, circle);
        }

        for (auto sandTrap : map.sandTraps)
        {
            if (sandTrap.IsEmpty())
                continue;

            if (IsCollidingSandTrap(ball, sandTrap))
                HandleCollisionSandTrap(ball, sandTrap, secondsDelta);
        }

        for (auto treadmill : map.treadmills)
        {
            if (treadmill.IsEmpty())
                continue;

            if (IsCollidingTreadmill(ball, treadmill))
                HandleCollisionTreadmill(ball, treadmill, secondsDelta);
        }
    }

    static bool BallInHole(Ball &ball, const Map &map)
    {
        float dist = Distance(ball.X, ball.Y, map.end.x, map.end.y);
        return dist <= Map::HoleRadius - .5;
    }

    static float Distance(float x1, float y1, float x2, float y2)
    {
        return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }

private:
    static bool IsCollidingWall(const Ball &ball, const Wall &wall)
    {
        // Wall vector
        float dx = static_cast<float>(wall.p2.x - wall.p1.x);
        float dy = static_cast<float>(wall.p2.y - wall.p1.y);

        // Vector from p1 to the ball's center
        float fx = ball.X - static_cast<float>(wall.p1.x);
        float fy = ball.Y - static_cast<float>(wall.p1.y);

        // Project the ball's center onto the line defined by p1 and p2
        float lenSquared = dx * dx + dy * dy;
        float t = (fx * dx + fy * dy) / lenSquared;

        // Clamp t to the range [0, 1] to restrict to the segment
        t = constrain(t, 0.0, 1.0);

        // Find the closest point on the wall segment
        float closestX = wall.p1.x + t * dx;
        float closestY = wall.p1.y + t * dy;

        // Calculate the distance from the ball's center to the closest point
        float distanceToWall = Distance(ball.X, ball.Y, closestX, closestY);

        // Check if the distance is less than or equal to the radius
        return distanceToWall <= Ball::Radius;
    }

    static void HandleCollisionWall(Ball &ball, const Wall &wall)
    {
        // Wall direction vector
        Vector wallDir = {wall.p2.x - wall.p1.x, wall.p2.y - wall.p1.y};

        // Wall normal vector (perpendicular to wall direction)
        Vector wallNormal = {-wallDir.y, wallDir.x};
        wallNormal = wallNormal.Normalize();

        // Determine which side of the wall the ball is on
        Vector ballToWallStart = {ball.X - wall.p1.x, ball.Y - wall.p1.y};
        float sideTest = ballToWallStart.DotProduct(wallNormal);

        // If the ball is on the opposite side, reverse the normal
        if (sideTest < 0)
        {
            wallNormal.x = -wallNormal.x;
            wallNormal.y = -wallNormal.y;
        }

        // Reflect the ball's velocity
        float dotProduct = ball.Velocity.DotProduct(wallNormal);
        ball.Velocity.x -= 2 * dotProduct * wallNormal.x;
        ball.Velocity.y -= 2 * dotProduct * wallNormal.y;

        // Calculate the closest point on the wall segment to the ball's position
        float projection = ballToWallStart.DotProduct(wallDir) / wallDir.DotProduct(wallDir);

        // Clamp the projection to the segment [0, 1]
        projection = max(0.0f, min(1.0f, projection));

        // Find the closest point on the wall segment
        Vector closestPoint = {wall.p1.x + projection * wallDir.x, wall.p1.y + projection * wallDir.y};

        // Check if the ball is inside the wall and resolve penetration
        Vector ballToClosestPoint = {ball.X - closestPoint.x, ball.Y - closestPoint.y};
        float distanceToWall = ballToClosestPoint.Length();

        // If the ball has penetrated the wall
        if (distanceToWall < Ball::Radius)
        {
            // Calculate the penetration depth
            float penetrationDepth = Ball::Radius - distanceToWall;

            // Move the ball out of the wall along the wall normal
            ball.X += penetrationDepth * wallNormal.x;
            ball.Y += penetrationDepth * wallNormal.y;
        }
    }

    static bool IsCollidingCircle(const Ball &ball, const Circle &circle)
    {
        Vector circleToBall = {ball.X - circle.location.x, ball.Y - circle.location.y};
        float distance = circleToBall.Length();

        return distance <= circle.radius + Ball::Radius;
    }

    static void HandleCollisionCircle(Ball &ball, const Circle &circle)
    {
        Vector circleToBall = {ball.X - circle.location.x, ball.Y - circle.location.y};
        float distance = circleToBall.Length();

        // Resolve the collision by moving the ball outside of the circle
        Vector correction = circleToBall.Normalize();
        ball.X += correction.x;
        ball.Y += correction.y;

        // Reflect the ball's velocity
        Vector normal = circleToBall.Normalize();
        float dotProduct = ball.Velocity.x * normal.x + ball.Velocity.y * normal.y;
        ball.Velocity.x -= 2 * dotProduct * normal.x;
        ball.Velocity.y -= 2 * dotProduct * normal.y;
    }

    static bool IsCollidingSandTrap(Ball &ball, const SandTrap &sand)
    {
        Rect ballRect = GetBallHitbox(ball);
        Rect sandRect = Rect(sand.x, sand.y, sand.width, sand.height);
        return Arduboy2::collide(ballRect, sandRect);
    }

    static void HandleCollisionSandTrap(Ball &ball, const SandTrap &sand, float secondsDelta)
    {
        for (uint8_t i = 0; i < SandTrap::FrictionMultiplier; i++)
        {
            ball.ApplyFriction(secondsDelta);
        }
    }

    static bool IsCollidingTreadmill(Ball &ball, const Treadmill &treadmill)
    {
        Rect ballRect = GetBallHitbox(ball);
        Rect teadRect = Rect(treadmill.x, treadmill.y, treadmill.width, treadmill.height);
        return Arduboy2::collide(ballRect, teadRect);
    }

    static void HandleCollisionTreadmill(Ball &ball, const Treadmill &treadmill, float secondsDelta)
    {
        auto velocityDelta = Treadmill::Speed * secondsDelta;

        switch (treadmill.direction) {
            case Direction::Up:
                ball.Velocity.y -= velocityDelta;
                break;
            case Direction::Down:
                ball.Velocity.y += velocityDelta;
                break;
            case Direction::Left:
                ball.Velocity.x -= velocityDelta;
                break;
            case Direction::Right:
                ball.Velocity.x += velocityDelta;
                break;
        }
    }

    static Rect GetBallHitbox(const Ball &ball)
    {
        return Rect(ball.X - 1, ball.Y - 1, 2, 2);
    }
};
