#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <vector>
#include "Pathfinding.h"

struct Enemy {
    D2D_POINT_2F pos;
    int hp = 1;
    int damage = 1;
    float attackInterval = 2.0f; // seconds
    float timeSinceAttack = 0.0f;
    float attackRange = 1.2f; // tiles

    // Pathfollowing
    float moveSpeed = 1.2f;        // tiles per second
    float repathInterval = 0.25f;  // seconds
    float timeSinceRepath = 0.0f;
    std::vector<IPoint> path;
    int pathIndex = 0;
    bool haveLastPlayerTile = false;
    IPoint lastPlayerTile{0,0};

    Enemy() : pos{0.f, 0.f} {}
    explicit Enemy(D2D_POINT_2F p) : pos{p} {}

    // Update with player position (required for pathfinding)
    void Update(float dt, const D2D_POINT_2F &playerPos);

    bool TryAttack(const D2D_POINT_2F &playerPos);

private:
    void EnsurePath(const IPoint& myTile, const IPoint& playerTile);
    void MoveAlongPath(float dt);
};