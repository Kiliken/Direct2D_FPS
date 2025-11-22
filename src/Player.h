#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <Windows.h>
#include <d2d1.h>

class Player
{
private:
    ID2D1SolidColorBrush *pBrush;
    ID2D1SolidColorBrush *pBrush2;

public:
    Player(ID2D1HwndRenderTarget &renderer);
    ~Player();

    void Update();
    void Draw(ID2D1HwndRenderTarget &renderer);

    D2D_POINT_2F pos = {200, 200};
    D2D1_ELLIPSE pointer = D2D1::Ellipse(
        pos ,2, 2);
    D2D1_ELLIPSE body = D2D1::Ellipse(
        pos, 20.0f, 20.0f);
    
    D2D_POINT_2F facingDir = {0, -1};
};

Player::Player(ID2D1HwndRenderTarget &renderer)
{
    renderer.CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::NavajoWhite), &pBrush);
    renderer.CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Thistle), &pBrush2);
}

Player::~Player()
{
}

void Player::Update()
{
    body.point = pos;
    pointer.point.x = pos.x + facingDir.x * 20;
    pointer.point.y = pos.y + facingDir.y * 20;

}

void Player::Draw(ID2D1HwndRenderTarget &renderer)
{
    
    renderer.FillEllipse(body, pBrush2);
    renderer.FillEllipse(pointer, pBrush);
}
