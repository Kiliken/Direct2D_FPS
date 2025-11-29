#include "raycastTest.h"
#include <cmath>
#include <cstdio>

const int screenWidth = 1280;
const int screenHeight = 720;

const float cameraHeight = 0.66f;

const int texture_size = 512;
const int texture_wall_size = 128;

const float fps_refresh_time = 0.1f;

const std::unordered_map<char, WallTexture> wallTypes{
    {'#', WallTexture::Pink},
    {'=', WallTexture::Dirt},
    {'M', WallTexture::Wallpaper},
    {'N', WallTexture::Bush},
    {'~', WallTexture::Sky},
    {'!', WallTexture::Red},
    {'@', WallTexture::Smiley},
    {'^', WallTexture::Exit},
};

const int mapWidth = 24;
const int mapHeight = 24;

const char worldMap[] =
    "~~~~~~~~~~~~~~~~MMM@MMMM"
    "~..............=M......M"
    "~..............=M......M"
    "~..............=@......@"
    "~..............=M......M"
    "~....N......N..........M"
    "~..............=MMM@MM.M"
    "~..............======M.M"
    "~..............=MMMMMM.M"
    "~..............=M......M"
    "~...N....N.....=M..N..M#"
    "~.....................M#"
    "~..............=M..N..M#"
    "~..............=M.....M#"
    "~...........N..=MMMMM.M#"
    "~..............======.=#"
    "#.!!!!!!.!!!!!!........#"
    "#.!....!.!..........=..#"
    "#.!.N..!.!..==..=...=..#"
    "#...........==..==..=..#"
    "#.!!!!!!.!..==.........#"
    "#.######.#..==....=....#"
    "N......................^"
    "########################";

char getTile(int x, int y)
{
    return worldMap[y * mapWidth + x];
}

bool mapCheck()
{
    int mapSize = sizeof(worldMap) - 1;
    if (mapSize != mapWidth * mapHeight)
    {
        std::fprintf(stderr, "Map size(%d) is not mapWidth * mapHeight(%d)\n", mapSize, mapWidth * mapHeight);
        return false;
    }

    for (int y = 0; y < mapHeight; ++y)
    {
        for (int x = 0; x < mapWidth; ++x)
        {
            char tile = getTile(x, y);
            if (tile != '.' && wallTypes.find(tile) == wallTypes.end())
            {
                std::fprintf(stderr, "map tile at [%3d,%3d] has an unknown tile type(%c)\n", x, y, tile);
                return false;
            }
            if ((y == 0 || x == 0 || y == mapHeight - 1 || x == mapWidth - 1) && tile == '.')
            {
                std::fprintf(stderr, "map edge at [%3d,%3d] is a floor (should be wall)\n", x, y);
                return false;
            }
        }
    }
    return true;
}

bool canMove(D2D_POINT_2F position, D2D_POINT_2F size)
{
    D2D_POINT_2F upper_left;
    upper_left.x = position.x - size.x / 2.0f;
    upper_left.y = position.y - size.y / 2.0f;

    D2D_POINT_2F lower_right;
    lower_right.x = position.x + size.x / 2.0f;
    lower_right.y = position.y + size.y / 2.0f;

    if (upper_left.x < 0 || upper_left.y < 0 || lower_right.x >= mapWidth || lower_right.y >= mapHeight)
    {
        return false;
    }
    for (int y = (int)upper_left.y; y <= (int)lower_right.y; ++y)
    {
        for (int x = (int)upper_left.x; x <= (int)lower_right.x; ++x)
        {
            if (getTile(x, y) != '.')
            {
                return false;
            }
        }
    }
    return true;
}

D2D_POINT_2F rotateVec(D2D_POINT_2F vec, float value)
{
    D2D_POINT_2F buffer;
    buffer.x = vec.x * std::cos(value) - vec.y * std::sin(value);
    buffer.y = vec.x * std::sin(value) + vec.y * std::cos(value);
    return buffer;
}
