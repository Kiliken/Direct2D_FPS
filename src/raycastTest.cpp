#include "raycastTest.h"
#include "EnemyManager.h"


char getTile(int x, int y)
{
    return worldMap[y * mapWidth + x];
}

bool mapCheck() {
    // check size
    int mapSize = sizeof(worldMap) - 1; // - 1 because sizeof also counts the final NULL character
    if (mapSize != mapWidth * mapHeight)
    {
        fprintf(stderr, "Map size(%d) is not mapWidth * mapHeight(%d)\n", mapSize, mapWidth * mapHeight);
        return false;
    }

    for (int y = 0; y < mapHeight; ++y)
    {
        for (int x = 0; x < mapWidth; ++x)
        {
            char tile = getTile(x, y);
            // check if tile type is valid
            if (tile != '.' && wallTypes.find(tile) == wallTypes.end())
            {
                fprintf(stderr, "map tile at [%3d,%3d] has an unknown tile type(%c)\n", x, y, tile);
                return false;
            }
            // check if edges are walls
            if ((y == 0 || x == 0 || y == mapHeight - 1 || x == mapWidth - 1) &&
                tile == '.')
            {
                fprintf(stderr, "map edge at [%3d,%3d] is a floor (should be wall)\n", x, y);
                return false;
            }
        }
    }
    return true;
}


D2D_POINT_2F rotateVec(D2D_POINT_2F vec, float value)
{
    return D2D1::Point2F(vec.x * std::cos(value) - vec.y * std::sin(value), vec.x * std::sin(value) + vec.y * std::cos(value));
}

bool canMove(D2D_POINT_2F position, D2D_POINT_2F size)
{
    // create the corners of the rectangle
    D2D_POINT_2F upper_left = D2D1::Point2F(position.x - size.x / 2.0f, position.y - size.y / 2.0f);
    D2D_POINT_2F lower_right = D2D1::Point2F(position.x + size.x / 2.0f, position.y + size.y / 2.0f);

    if (upper_left.x < 0.0 || upper_left.y < 0.0 || lower_right.x >= mapWidth || lower_right.y >= mapHeight)
    {
        return false; // out of map bounds
    }
    
    // loop through each map tile within the rectangle. The rectangle could be multiple tiles in size!
    for (int y = upper_left.y; y <= lower_right.y; ++y)
    {
        for (int x = upper_left.x; x <= lower_right.x; ++x)
        {
            if (getTile(x, y) != '.')
            {
                return false;
            }
        }
    }
    return true;
}


SDL_Color GetPixelColor(SDL_Surface* surface, int x, int y)
{
    SDL_Color color = {0,0,0,0};
    if (!surface) return color;

    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    Uint8* pixels = (Uint8*)surface->pixels;
    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(surface->format);
    int bpp = SDL_BYTESPERPIXEL(surface->format);

    Uint8* p = pixels + y * surface->pitch + x * bpp;

    Uint32 pixelValue = 0;
    switch (bpp) {
        case 1:
            pixelValue = *p;
            break;
        case 2:
            pixelValue = *(Uint16*)p;
            break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                pixelValue = p[0]<<16 | p[1]<<8 | p[2];
            else
                pixelValue = p[0] | p[1]<<8 | p[2]<<16;
            break;
        case 4:
            pixelValue = *(Uint32*)p;
            break;
    }

    SDL_GetRGBA(pixelValue, fmt, NULL, &color.r, &color.g, &color.b, &color.a);

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    return color;
}

// Implement the function to check for the closest enemy hit
float checkEnemyHit(const D2D_POINT_2F &rayPos, const D2D_POINT_2F &rayDir, D2D_POINT_2F &hitPos, EnemyManager &enemyManager)
{
    float closestDist = 1e30f;
    hitPos = {0.0f, 0.0f};

    for (const auto &e : enemyManager.enemies)
    {
        // Vector from ray origin (player pos) to enemy
        D2D_POINT_2F v = {e.pos.x - rayPos.x, e.pos.y - rayPos.y};

        // Calculate the component of v along the ray direction (rayDir is unit length)
        float distAlongRay = v.x * rayDir.x + v.y * rayDir.y;

        // Check if the enemy is in front of the player
        if (distAlongRay <= 0.0f)
            continue;

        // Projection of v onto rayDir is (distAlongRay * rayDir)
        D2D_POINT_2F proj = {distAlongRay * rayDir.x, distAlongRay * rayDir.y};

        // Vector perpendicular from the ray to the enemy center
        D2D_POINT_2F perp = {v.x - proj.x, v.y - proj.y};
        float perpDist2 = perp.x * perp.x + perp.y * perp.y;
        float perpDist = std::sqrt(perpDist2);

        // Assuming enemy size is around 0.5 tiles for a hit
        // This is a simplified check that assumes a circular bounding box of radius 0.5 around the enemy center.
        const float enemyRadius = 0.5f;

        if (perpDist < enemyRadius)
        {
            // The ray hits the billboard's bounding circle.
            // Now find the distance to the hit point on the circle edge
            float distToEdge = distAlongRay - std::sqrt(enemyRadius * enemyRadius - perpDist2);

            if (distToEdge < closestDist)
            {
                closestDist = distToEdge;
                hitPos.x = rayPos.x + closestDist * rayDir.x;
                hitPos.y = rayPos.y + closestDist * rayDir.y;
            }
        }
    }

    return closestDist;
}