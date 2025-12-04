#include "raycastTest.h"


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