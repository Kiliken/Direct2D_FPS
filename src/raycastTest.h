#include <Windows.h>
#include <unordered_map>
#include <d2d1.h>

const int screenWidth = 1280;
const int screenHeight = 720;

const float cameraHeight = 0.66f; // height of player camera(1.0 is ceiling, 0.0 is floor)

const int texture_size = 512;      // size(width and height) of texture that will hold all wall textures
const int texture_wall_size = 128; // size(width and height) of each wall type in the full texture

const float fps_refresh_time = 0.1f; // time between FPS text refresh. FPS is smoothed out over this time

// list of wall texture types, in order as they appear in the full texture
enum class WallTexture
{
    Smiley,
    Red,
    Bush,
    Sky,
    Pink,
    Wallpaper,
    Dirt,
    Exit,
};

// valid wall types and their texture for the world map
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

// size of the top-down world map in tiles
const int mapWidth = 24;
const int mapHeight = 24;

// top-down view of world map
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

// get a tile from worldMap. Not memory safe.
char getTile(int x, int y)
{
    return worldMap[y * mapWidth + x];
}

// checks worldMap for errors
// returns: true on success, false on errors found
bool mapCheck()
{
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

// check if a rectangular thing with given size can move to given position without colliding with walls or
// being outside of the map
// position is considered the middle of the rectangle
bool canMove(D2D_POINT_2F position, D2D_POINT_2F size)
{
    // create the corners of the rectangle
    D2D_POINT_2F upper_left;
    upper_left.x = position.x - size.x / 2.0f;
    upper_left.y = position.y - size.y / 2.0f;

    D2D_POINT_2F lower_right;
    lower_right.x = position.x + size.x / 2.0f;
    lower_right.y = position.y + size.y / 2.0f;

    if (upper_left.x < 0 || upper_left.y < 0 || lower_right.x >= mapWidth || lower_right.y >= mapHeight)
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

// rotate a given vector with given float value in radians and return the result
// see: https://en.wikipedia.org/wiki/Rotation_matrix#In_two_dimensions
D2D_POINT_2F rotateVec(D2D_POINT_2F vec, float value)
{
    D2D_POINT_2F buffer;
    buffer.x = vec.x * std::cos(value) - vec.y * std::sin(value);
    buffer.y = vec.x * std::sin(value) + vec.y * std::cos(value);
    return buffer;
}