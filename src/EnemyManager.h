#pragma once
#include <vector>
#include <random>
#include <d2d1.h>
#include "Enemy.h"
#include "raycastTest.h"

class EnemyManager
{
public:
    std::vector<Enemy> enemies;

    EnemyManager();
    ~EnemyManager();

    void Reset();

    // New: initialize stationary targets at random valid positions
    void InitializeTargets(int count, const D2D_POINT_2F &playerPos);

    void Update(float dt, const D2D_POINT_2F &playerPos);
    void CreateBillboardRenderer(ID2D1HwndRenderTarget *rt, int width, int height);
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
    bool RemoveEnemyAt(const D2D_POINT_2F &worldPos, float proximity);

    // New: helpers to manage spawning and targets
    void SetSpawningEnabled(bool enabled);
    void DestroyAllEnemies();
    int CountTargets() const;

private:
    float spawnAccumulator = 0.0f;
    static const int maxEnemies = 20;
    std::mt19937 rng;

    bool spawningEnabled = true; // New: control spawn

    // Sprite
    ID2D1Bitmap *enemyBmp = NULL;
    D2D1_SIZE_U enemyBmpSize;
    std::vector<BYTE> enemyBmpPx;

    void TrySpawn(const D2D_POINT_2F &playerPos);
    bool FindRandomFreeFloor(const D2D_POINT_2F &playerPos, D2D_POINT_2F &outPos);
};