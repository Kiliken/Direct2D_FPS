#include "Enemy.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <d2d1.h>


// Update enemy state
void Enemy::Update(float dt)
{
    timeSinceAttack += dt;
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
