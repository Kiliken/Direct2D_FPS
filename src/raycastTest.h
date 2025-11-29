#pragma once
#include <Windows.h>
#include <unordered_map>
#include <d2d1.h>

extern const int screenWidth;
extern const int screenHeight;

extern const float cameraHeight; // height of player camera(1.0 is ceiling, 0.0 is floor)

extern const int texture_size;      // size(width and height) of texture that will hold all wall textures
extern const int texture_wall_size; // size(width and height) of each wall type in the full texture

extern const float fps_refresh_time; // time between FPS text refresh. FPS is smoothed out over this time

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
extern const std::unordered_map<char, WallTexture> wallTypes;

// size of the top-down world map in tiles
extern const int mapWidth;
extern const int mapHeight;

// top-down view of world map
extern const char worldMap[];

// get a tile from worldMap. Not memory safe.
char getTile(int x, int y);

// checks worldMap for errors
// returns: true on success, false on errors found
bool mapCheck();

// check if a rectangular thing with given size can move to given position without colliding with walls or
// being outside of the map
// position is considered the middle of the rectangle
bool canMove(D2D_POINT_2F position, D2D_POINT_2F size);

// rotate a given vector with given float value in radians and return the result
D2D_POINT_2F rotateVec(D2D_POINT_2F vec, float value);