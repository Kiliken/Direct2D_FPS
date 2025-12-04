#pragma once
#include <vector>
#include <random>
#include <d2d1.h>
#include "Enemy.h"
#include "raycastTest.h"

class EnemyManager
{
public:
    EnemyManager();
    ~EnemyManager();

    void Reset();
    void Update(float dt, const D2D_POINT_2F &playerPos);
    void CreateBillboardRenderer(ID2D1HwndRenderTarget *rt);
    void RenderBillboards(ID2D1HwndRenderTarget *rt,
                          ID2D1SolidColorBrush *enemyBrush,
                          SDL_Surface *mainText,
                          const std::vector<float> &depthBuffer,
                          int width, int height,
                          float halfH,
                          const D2D_POINT_2F &playerPos,
                          float playerAngle,
                          float planeHalf);

    const std::vector<Enemy> &GetEnemies() const { return enemies; }

private:
    std::vector<Enemy> enemies;
    float spawnAccumulator = 0.0f;
    static const int maxEnemies = 20;
    std::mt19937 rng;

    // Sprite
    ID2D1Bitmap *enemyBmp = NULL;
    D2D1_SIZE_U enemyBmpSize = D2D1::SizeU(128, 128);
    std::vector<BYTE> enemyBmpPx;

    void TrySpawn(const D2D_POINT_2F &playerPos);
};
