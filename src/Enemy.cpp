#include "Enemy.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <d2d1.h>
#include "raycastTest.h" // for canMove, getTile

static inline float length2(float x, float y) { return x*x + y*y; }

static inline float length(float x, float y) { return std::sqrt(length2(x,y)); }


// Helper: find nearest walkable tile around a center within a given radius
static IPoint FindNearestWalkableAround(const IPoint& center, int maxRadius)
{
    if (IsWalkable(center.x, center.y)) return center;
    for (int r = 1; r <= maxRadius; ++r) {
        // scan a diamond ring (Manhattan distance r)
        for (int dx = -r; dx <= r; ++dx) {
            int dy1 = r - std::abs(dx);
            int dy2 = -dy1;
            int tx1 = center.x + dx;
            int ty1 = center.y + dy1;
            int tx2 = center.x + dx;
            int ty2 = center.y + dy2;
            if (IsWalkable(tx1, ty1)) return {tx1, ty1};
            if (IsWalkable(tx2, ty2)) return {tx2, ty2};
        }
    }
    return center; // fallback to center; A* will return empty if not walkable
}


// Update enemy state and pathfollow toward player
void Enemy::Update(float dt, const D2D_POINT_2F &playerPos)
{
    timeSinceAttack += dt;
    timeSinceRepath += dt;

    IPoint myTile = WorldToTile(pos);
    IPoint playerTile = WorldToTile(playerPos);

    // Repath conditions
    if (!haveLastPlayerTile || lastPlayerTile != playerTile || timeSinceRepath >= repathInterval || path.empty() || pathIndex >= (int)path.size()) {
        EnsurePath(myTile, playerTile);
        lastPlayerTile = playerTile;
        haveLastPlayerTile = true;
        timeSinceRepath = 0.0f;
    }

    MoveAlongPath(dt);
}


// Attempt to attack the player
bool Enemy::TryAttack(const D2D_POINT_2F &playerPos)
{
    float dx = pos.x - playerPos.x;
    float dy = pos.y - playerPos.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    if (dist <= attackRange && timeSinceAttack >= attackInterval)
    {
        timeSinceAttack = 0.0f;
        SDL_Log("Enemy attacked!");
        return true;
    }
    return false;
}


void Enemy::EnsurePath(const IPoint& myTile, const IPoint& playerTile)
{
    char myChar = InBounds(myTile.x, myTile.y) ? getTile(myTile.x, myTile.y) : '?';
    char plChar = InBounds(playerTile.x, playerTile.y) ? getTile(playerTile.x, playerTile.y) : '?';

    // Debug: show what tiles we’re reading from raycastTest
    SDL_Log("Enemy EnsurePath: myTile=(%d,%d,'%c') playerTile=(%d,%d,'%c')",
            myTile.x, myTile.y, myChar, playerTile.x, playerTile.y, plChar);

    IPoint goal = playerTile;
    if (!IsWalkable(goal.x, goal.y)) {
        // Player might be standing at a non-floor tile edge; pick nearest floor tile
        goal = FindNearestWalkableAround(playerTile, /*maxRadius=*/4);
        char gChar = InBounds(goal.x, goal.y) ? getTile(goal.x, goal.y) : '?';
        SDL_Log("EnsurePath: player tile not walkable, using nearest walkable goal=(%d,%d,'%c')",
                goal.x, goal.y, gChar);
    }

    if (!IsWalkable(myTile.x, myTile.y)) {
        SDL_Log("EnsurePath: Enemy start tile not walkable, clearing path");
        path.clear();
        pathIndex = 0;
        return;
    }

    path = AStarTilePath(myTile, goal);
    pathIndex = 0;

    SDL_Log("EnsurePath: path length = %d", (int)path.size());
}


void Enemy::MoveAlongPath(float dt)
{
    if (path.empty() || pathIndex >= (int)path.size()) return;

    // Next waypoint (tile center)
    const IPoint nextTile = path[pathIndex];
    D2D_POINT_2F target = TileCenter(nextTile);

    float dx = target.x - pos.x;
    float dy = target.y - pos.y;
    float dist = length(dx, dy);

    // Consider tile reached if close to center
    const float arriveRadius = 0.20f; // tiles (increased to reduce jitter)
    if (dist <= arriveRadius) {
        ++pathIndex;
        if (pathIndex >= (int)path.size()) return;
        D2D_POINT_2F nextTarget = TileCenter(path[pathIndex]);
        dx = nextTarget.x - pos.x;
        dy = nextTarget.y - pos.y;
        dist = length(dx, dy);
    }

    if (dist > 1e-5f) {
        float vx = (dx / dist) * moveSpeed;
        float vy = (dy / dist) * moveSpeed;
        D2D_POINT_2F desired = { pos.x + vx * dt, pos.y + vy * dt };

        // simple collision check, similar to player
        D2D_POINT_2F size = {0.35f, 0.35f};
        if (canMove(desired, size)) {
            pos = desired;
        } else {
            // try axis-wise movement to slide along walls (limit slide distance to reduce jitter)
            D2D_POINT_2F desiredX = { pos.x + vx * dt, pos.y };
            D2D_POINT_2F desiredY = { pos.x, pos.y + vy * dt };
            bool moved = false;
            if (canMove(desiredX, size)) { pos = desiredX; moved = true; }
            if (!moved && canMove(desiredY, size)) { pos = desiredY; moved = true; }
            if (!moved) {
                // Blocked; path will be reconsidered next tick
            }
        }
    }
}