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
    // Stationary targets do not move or pathfind
    if (type == EnemyType::Target) {
        return;
    }

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
    // Stationary targets don't attack
    if (type == EnemyType::Target) {
        return false;
    }

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
        goal = FindNearestWalkableAround(goal, 3);
    }

    // compute new path with A*
    path = AStarTilePath(myTile, goal);
    pathIndex = 0;
}

void Enemy::MoveAlongPath(float dt)
{
    if (path.empty() || pathIndex >= (int)path.size()) {
        return;
    }

    IPoint nextTile = path[pathIndex];
    D2D_POINT_2F targetPos = TileCenter(nextTile);

    float dx = targetPos.x - pos.x;
    float dy = targetPos.y - pos.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    if (dist < 0.05f) {
        pathIndex++;
        return;
    }

    float vx = (dx / dist) * moveSpeed;
    float vy = (dy / dist) * moveSpeed;

    D2D_POINT_2F nextPos = { pos.x + vx * dt, pos.y + vy * dt };
    if (canMove(nextPos, {0.3f, 0.3f})) {
        pos = nextPos;
    } else {
        // If blocked, re-path on next update cycle
        timeSinceRepath = repathInterval;
    }
}