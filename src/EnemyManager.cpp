#include "EnemyManager.h"
#include <algorithm>


// constructor containing rng initialization
EnemyManager::EnemyManager() : rng((unsigned)std::random_device{}()) {}
EnemyManager::~EnemyManager() {}


// Reset enemy manager state
void EnemyManager::Reset()
{
    enemies.clear();
    spawnAccumulator = 0.0f;
}


// Try to spawn a new enemy
void EnemyManager::TrySpawn(const D2D_POINT_2F &playerPos)
{
    if ((int)enemies.size() >= maxEnemies) return;
    // find a random floor tile not near player
    std::uniform_int_distribution<int> distX(1, mapWidth - 2);
    std::uniform_int_distribution<int> distY(1, mapHeight - 2);
    for (int attempts = 0; attempts < 200; ++attempts)
    {
        int tx = distX(rng);
        int ty = distY(rng);
        if (getTile(tx, ty) != '.') continue;
        D2D_POINT_2F p = { (float)tx + 0.5f, (float)ty + 0.5f };
        float dx = p.x - playerPos.x;
        float dy = p.y - playerPos.y;
        float d2 = dx*dx + dy*dy;
        if (d2 < 9.0f) continue; // too close (less than 3 tiles)
        bool occupied = false;
        for (auto &e : enemies) {
            if (std::fabs(e.pos.x - p.x) < 0.1f && std::fabs(e.pos.y - p.y) < 0.1f) { occupied = true; break; }
        }
        if (occupied) continue;
        enemies.emplace_back(p);
        break;
    }
}


// Update all enemies
void EnemyManager::Update(float dt, const D2D_POINT_2F &playerPos)
{
    spawnAccumulator += dt;
    if (spawnAccumulator >= 5.0f)
    {
        spawnAccumulator = 0.0f;
        TrySpawn(playerPos);
    }

    for (auto &e : enemies)
    {
        e.Update(dt);
        e.TryAttack(playerPos);
    }
}


// Render enemies as billboards (temporary implementation)
void EnemyManager::RenderBillboards(ID2D1HwndRenderTarget *rt,
                                    ID2D1SolidColorBrush *enemyBrush,
                                    const std::vector<float> &depthBuffer,
                                    int width, int height,
                                    float halfH,
                                    const D2D_POINT_2F &playerPos,
                                    float playerAngle,
                                    float planeHalf)
{
    for (const auto &e : enemies)
    {
        float dxw = e.pos.x - playerPos.x;
        float dyw = e.pos.y - playerPos.y;
        // transform to camera space
        float sinA = std::sin(playerAngle);
        float cosA = std::cos(playerAngle);
        float cx =  dxw *  cosA + dyw *  sinA; // forward
        float cy = -dxw *  sinA + dyw *  cosA; // right

        if (cx <= 0.1f) continue; // behind player

        float spriteScreenX = (width / 2.0f) * (1.0f + (cy / (cx * planeHalf)));
        int spriteHeight = (int)(height / cx);
        int spriteWidth  = spriteHeight; // square
        int drawTop = (int)(halfH - spriteHeight / 2.0f);
        int drawBottom = (int)(halfH + spriteHeight / 2.0f);
        int drawLeft = (int)(spriteScreenX - spriteWidth / 2.0f);
        int drawRight = (int)(spriteScreenX + spriteWidth / 2.0f);

        if (drawRight < 0 || drawLeft >= width) continue;
        if (drawTop < 0) drawTop = 0;
        if (drawBottom >= height) drawBottom = height - 1;

        int l = (drawLeft < 0 ? 0 : drawLeft);
        int r = (drawRight > (width - 1) ? (width - 1) : drawRight);
        for (int sx = l; sx <= r; ++sx)
        {
            if (cx < depthBuffer[sx])
            {
                rt->DrawLine(
                    D2D1::Point2F((FLOAT)sx, (FLOAT)drawTop),
                    D2D1::Point2F((FLOAT)sx, (FLOAT)drawBottom),
                    enemyBrush,
                    1.0f);
            }
        }
    }
}
