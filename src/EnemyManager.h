#pragma once
#include <vector>
#include <random>
#include <d2d1.h>
#include "Enemy.h"
#include "raycastTest.h"

class EnemyManager {
public:
    EnemyManager();
    ~EnemyManager();

    void Reset();
    void Update(float dt, const D2D_POINT_2F &playerPos);
    void RenderBillboards(ID2D1HwndRenderTarget *rt,
                          ID2D1SolidColorBrush *enemyBrush,
                          const std::vector<float> &depthBuffer,
                          int width, int height,
                          float halfH,
                          const D2D_POINT_2F &playerPos,
                          float playerAngle,
                          float planeHalf);

    const std::vector<Enemy>& GetEnemies() const { return enemies; }

private:
    std::vector<Enemy> enemies;
    float spawnAccumulator = 0.0f;
    static const int maxEnemies = 20;
    std::mt19937 rng;

    void TrySpawn(const D2D_POINT_2F &playerPos);
};
