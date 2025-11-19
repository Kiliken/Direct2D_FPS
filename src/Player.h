#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <Windows.h>
#include <d2d1.h>

class Player
{
private:
    ID2D1SolidColorBrush *pBrush;
public:
    Player(ID2D1HwndRenderTarget& renderer);
    ~Player();


    void Update();
    void Draw(ID2D1HwndRenderTarget& renderer);


    D2D_POINT_2F pos =  {0,0};
    D2D1_RECT_F collider = D2D1::RectF(
            pos.x,
            pos.y,
            pos.x + 50.0f,
            pos.y + 50.0f);
};

Player::Player(ID2D1HwndRenderTarget& renderer)
{  
    renderer.CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBrush);
}

Player::~Player()
{
}

void Player::Update(){

    collider.top = pos.y;
    collider.left = pos.x;
    collider.bottom = pos.y + 50.0f;
    collider.right = pos.x + 50.0f;
}

void Player::Draw(ID2D1HwndRenderTarget& renderer){ 
    renderer.FillRectangle(collider, pBrush);
}
