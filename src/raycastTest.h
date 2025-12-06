#pragma once

#include <Windows.h>
#include <SDL3/SDL.h>
#include <unordered_map>
#include <d2d1.h>

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
    "~~~~~~~~~~~~~~~~!!!@!!!!"
    "~..............!!......!"
    "~..............!@......!"
    "~..............!!......@"
    "~..............!!......!"
    "~....N......N..........!"
    "~..............!!!!@!!.!"
    "~..............!!!!!!!.!"
    "~..............#!!!!!!.@"
    "~..............!!......!"
    "~...N....N.....!!..N..!!"
    "~.....................!!"
    "~..............!!..N..!!"
    "~..............!!.....!#"
    "~...........N..!!#!!!.!!"
    "~..............!!!!!!.!!"
    "!.!!!!!!.!!!!!!........!"
    "!.!....=.=..........=..!"
    "@.!.=..=.=..!!..=...=..!"
    "!...........!#..==..=..#"
    "!.!!!!!!.=..!!.........!"
    "!.!!!!!!.=..!!....=....!"
    "!......................^"
    "!!!#!!!!!!#!!!@!!!!#!!!!";


const D2D1_BITMAP_PROPERTIES bmpProps =
    D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                          D2D1_ALPHA_MODE_PREMULTIPLIED));

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

// get pixel from a SDL surface
SDL_Color GetPixelColor(SDL_Surface* surface, int x, int y);