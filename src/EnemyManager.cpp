#include "EnemyManager.h"
#include <algorithm>

// constructor containing rng initialization
EnemyManager::EnemyManager() : rng((unsigned)std::random_device{}()) {}
EnemyManager::~EnemyManager() 
{
    if (enemyBmp)
    {
        enemyBmp->Release(); // D2D resources use Release()
        enemyBmp = nullptr;
    }
}

// Reset enemy manager state
void EnemyManager::Reset()
{
    enemies.clear();
    spawnAccumulator = 0.0f;
    spawningEnabled = true;
}

// New: helper to find a random free floor not near player or other enemies
bool EnemyManager::FindRandomFreeFloor(const D2D_POINT_2F &playerPos, D2D_POINT_2F &outPos)
{
    std::uniform_int_distribution<int> distX(1, mapWidth - 2);
    std::uniform_int_distribution<int> distY(1, mapHeight - 2);

    for (int attempts = 0; attempts < 200; ++attempts)
    {
        int tx = distX(rng);
        int ty = distY(rng);
        if (getTile(tx, ty) != '.')
            continue;

        D2D_POINT_2F p = {(float)tx + 0.5f, (float)ty + 0.5f};

        float dx = p.x - playerPos.x;
        float dy = p.y - playerPos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < 9.0f)
            continue; // too close (less than 3 tiles)

        bool occupied = false;
        for (auto &e : enemies)
        {
            if (std::fabs(e.pos.x - p.x) < 0.1f && std::fabs(e.pos.y - p.y) < 0.1f)
            {
                occupied = true;
                break;
            }
        }
        if (occupied)
            continue;

        outPos = p;
        return true;
    }
    return false;
}

// New: initialize stationary targets at random valid positions
void EnemyManager::InitializeTargets(int count, const D2D_POINT_2F &playerPos)
{
    for (int i = 0; i < count; ++i)
    {
        D2D_POINT_2F pos;
        if (FindRandomFreeFloor(playerPos, pos))
        {
            enemies.emplace_back(Enemy(pos, EnemyType::Target));
        }
    }
}

// Try to spawn a new enemy (walker)
void EnemyManager::TrySpawn(const D2D_POINT_2F &playerPos)
{
    if (!spawningEnabled)
        return;

    if ((int)enemies.size() >= maxEnemies)
        return;

    D2D_POINT_2F p;
    if (FindRandomFreeFloor(playerPos, p))
    {
        enemies.emplace_back(Enemy(p, EnemyType::Walker));
    }
}

// Update all enemies
void EnemyManager::Update(float dt, const D2D_POINT_2F &playerPos)
{
    spawnAccumulator += dt;
    if (spawningEnabled && spawnAccumulator >= 5.0f)
    {
        spawnAccumulator = 0.0f;
        TrySpawn(playerPos);
    }

    for (auto &e : enemies)
    {
        e.Update(dt, playerPos);
        e.TryAttack(playerPos);
    }
}

// Create the D2D1 bitmap of the billboard renderer
void EnemyManager::CreateBillboardRenderer(ID2D1HwndRenderTarget *rt, int width, int height)
{
    enemyBmpSize = D2D1::SizeU(width, height);
    enemyBmpPx = std::vector<BYTE>(width * height * 4);
    std::fill(enemyBmpPx.begin(), enemyBmpPx.end(), 0x00);

    rt->CreateBitmap(
        enemyBmpSize,
        enemyBmpPx.data(), // pointer to pixel buffer
        width * 4,         // pitch (stride in bytes)
        &bmpProps,
        &enemyBmp);
}

// Render enemies as billboards (temporary implementation)
void EnemyManager::RenderBillboards(ID2D1HwndRenderTarget *rt,
                                    ID2D1SolidColorBrush *enemyBrush,
                                    SDL_Surface *mainText,
                                    const std::vector<float> &depthBuffer,
                                    int width, int height,
                                    float halfH,
                                    const D2D_POINT_2F &playerPos,
                                    float playerAngle,
                                    float planeHalf)
{
    std::fill(enemyBmpPx.begin(), enemyBmpPx.end(), 0x00);

    for (const auto &e : enemies)
    {
        float dxw = e.pos.x - playerPos.x;
        float dyw = e.pos.y - playerPos.y;
        // transform to camera space
        float sinA = std::sin(playerAngle);
        float cosA = std::cos(playerAngle);
        float cx = dxw * cosA + dyw * sinA;  // forward
        float cy = -dxw * sinA + dyw * cosA; // right

        if (cx <= 0.1f)
            continue; // behind player

        float spriteScreenX = (width / 2.0f) * (1.0f + (cy / (cx * planeHalf)));
        int spriteHeight = (int)(height / cx);
        int spriteWidth = spriteHeight; // square
        int drawTop = (int)(halfH - spriteHeight / 2.0f);
        int drawBottom = (int)(halfH + spriteHeight / 2.0f);
        int drawLeft = (int)(spriteScreenX - spriteWidth / 2.0f);
        int drawRight = (int)(spriteScreenX + spriteWidth / 2.0f);

        if (drawRight < 0 || drawLeft >= width)
            continue;
        if (drawTop < 0)
            drawTop = 0;
        if (drawBottom >= height)
            drawBottom = height - 1;

        int l = (drawLeft < 0 ? 0 : drawLeft);
        int r = (drawRight > (width - 1) ? (width - 1) : drawRight);

        D2D1_POINT_2U spritePos = D2D1::Point2U(0, 0);
        for (int sx = l; sx <= r; ++sx)
        {

            if (cx < depthBuffer[sx])
            {
                int texX = int(256 * (sx - (-spriteWidth / 2 + spriteScreenX)) * 128 / spriteWidth) / 256;
                // int texX = int(256 * (sx - (-spriteWidth / 2 + spriteScreenX)) * texWidth / spriteWidth) / 256;

                for (int sy = drawTop; sy <= drawBottom; ++sy)
                {
                    int d = (sy) * 256 - height * 128 + spriteHeight * 128; // 256 and 128 factors to avoid floats
                    int texY = ((d * 128) / spriteHeight) / 256;

                    int currentPixel = (sy * width + sx) * 4;

                    SDL_Color pixelColor;
                    if(e.type == EnemyType::Walker)
                        pixelColor = GetPixelColor(mainText, texX, texY + 384);
                    else pixelColor = GetPixelColor(mainText, texX + 128, texY + 384);

                    if (pixelColor.b > 0xEE && pixelColor.r > 0xEE)
                    {
                        continue;
                    }

                    enemyBmpPx[currentPixel + 0] = BYTE(pixelColor.b);
                    enemyBmpPx[currentPixel + 1] = BYTE(pixelColor.g);
                    enemyBmpPx[currentPixel + 2] = BYTE(pixelColor.r);
                    enemyBmpPx[currentPixel + 3] = 0xFF;

                    spritePos.y++;
                }
                spritePos.x++;
            }
        }
    }

    enemyBmp->CopyFromMemory(nullptr, enemyBmpPx.data(), width * 4);
    rt->DrawBitmap(
        enemyBmp,
        D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height));
}

// Remove an enemy if its position is within 'proximity' of 'worldPos'
bool EnemyManager::RemoveEnemyAt(const D2D_POINT_2F &worldPos, float proximity)
{
    float proximitySq = proximity * proximity;

    // Use erase-remove idiom to safely remove the first enemy found
    auto it = std::remove_if(enemies.begin(), enemies.end(),
                             [&](const Enemy &e) {
                                 float dx = e.pos.x - worldPos.x;
                                 float dy = e.pos.y - worldPos.y;
                                 return (dx * dx + dy * dy) < proximitySq;
                             });

    if (it != enemies.end())
    {
        enemies.erase(it, enemies.end());
        return true; // Enemy was removed
    }
    return false; // No enemy removed
}

// New: manage spawning and targets
void EnemyManager::SetSpawningEnabled(bool enabled)
{
    spawningEnabled = enabled;
}

void EnemyManager::DestroyAllEnemies()
{
    enemies.clear();
    // Also clear the billboard buffer so nothing remains drawn this frame
    std::fill(enemyBmpPx.begin(), enemyBmpPx.end(), 0x00);
}

int EnemyManager::CountTargets() const
{
    int cnt = 0;
    for (const auto &e : enemies)
    {
        if (e.type == EnemyType::Target)
            ++cnt;
    }

    SDL_Log("Target count: %d", cnt);
    return cnt;
}