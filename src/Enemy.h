#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <d2d1helper.h>

struct Enemy {
    D2D_POINT_2F pos;
    int hp = 1;
    int damage = 1;
    float attackInterval = 2.0f; // seconds
    float timeSinceAttack = 0.0f;
    float attackRange = 1.2f; // tiles

    Enemy() : pos{0.f, 0.f} {}
    explicit Enemy(D2D_POINT_2F p) : pos{p} {}

    void Update(float dt);
    bool TryAttack(const D2D_POINT_2F &playerPos);
};
