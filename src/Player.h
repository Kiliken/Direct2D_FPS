#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <Windows.h>
#include <d2d1.h>

class Player
{
public:
    explicit Player() {}
    ~Player() {}

    void Update() {}

    D2D_POINT_2F pos = {12.0f, 12.0f};
    float angle = 0.0f; // radians
    float moveSpeed = 4.0f; // tiles per second basis; scaled by dt
    float rotSpeed = 1.8f;  // radians per second
    float size_f = 0.375f; //player radius
};
