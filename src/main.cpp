
#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <Windows.h>
#include <d2d1.h>
#include "Player.h"
#include <unordered_map>


static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static ID2D1Factory *pFactory = NULL;
static ID2D1HwndRenderTarget *pRenderTarget = NULL;
static D2D_POINT_2F playerPos = {0, 0};
static Player *player = NULL;


D2D_POINT_2F RotatePoint(D2D_POINT_2F p, float value) {
    D2D_POINT_2F ret;
    ret.x = p.x * std::cos(value) - p.y * std::sin(value);
    ret.y = p.x * std::sin(value) + p.y * std::cos(value);
    return ret;
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("D2DFPS", 1280, 720, SDL_WINDOW_ALWAYS_ON_TOP, &window, &renderer))
    {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                             SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                             NULL);

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);

    RECT rc;
    GetClientRect(hwnd, &rc);

    pFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd,
                                         D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)),
        &pRenderTarget);

    player = new Player(*pRenderTarget);

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }

    return SDL_APP_CONTINUE;
}



/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{

    // Inputs
    {
        const bool *key_states = SDL_GetKeyboardState(NULL);

        /* (We're writing our code such that it sees both keys are pressed and cancels each other out!) */
        if (key_states[SDL_SCANCODE_W])
        {
            player->pos.x += player->facingDir.x;
            player->pos.y += player->facingDir.y;
        }

        if (key_states[SDL_SCANCODE_S])
        {
            player->pos.x -= player->facingDir.x;
            player->pos.y -= player->facingDir.y;
        }

        if (key_states[SDL_SCANCODE_A])
        {
           player->facingDir = RotatePoint(player->facingDir, -0.01f);
        }

        if (key_states[SDL_SCANCODE_D])
        {
            player->facingDir = RotatePoint(player->facingDir, 0.01f);
        }
    }

    //Update

    player->Update();



    //Draw
    pRenderTarget->BeginDraw();
    {
        pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

        // Draw a grid background.
        int width = static_cast<int>(rtSize.width);
        int height = static_cast<int>(rtSize.height);


        ID2D1SolidColorBrush *pGridBrush = NULL;
        pRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)),
            &pGridBrush);

        for (int x = 0; x < width; x += 10)
        {
            pRenderTarget->DrawLine(
                D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
                D2D1::Point2F(static_cast<FLOAT>(x), rtSize.height),
                pGridBrush,
                0.5f);
        }

        for (int y = 0; y < height; y += 10)
        {
            pRenderTarget->DrawLine(
                D2D1::Point2F(0.0f, static_cast<FLOAT>(y)),
                D2D1::Point2F(rtSize.width, static_cast<FLOAT>(y)),
                pGridBrush,
                0.5f);
        }

        // Draw two rectangles.
        D2D1_RECT_F rectangle1 = D2D1::RectF(
            rtSize.width / 2 - 50.0f,
            rtSize.height / 2 - 50.0f,
            rtSize.width / 2 + 50.0f,
            rtSize.height / 2 + 50.0f);

        D2D1_RECT_F rectangle2 = D2D1::RectF(
            rtSize.width / 2 - 100.0f,
            rtSize.height / 2 - 100.0f,
            rtSize.width / 2 + 100.0f,
            rtSize.height / 2 + 100.0f);


        // Draw a filled rectangle.
        pRenderTarget->FillRectangle(rectangle1, pGridBrush);

        // Draw the outline of a rectangle.
        pRenderTarget->DrawRectangle(rectangle2, pGridBrush);

        player->Draw(*pRenderTarget);
    }
    pRenderTarget->EndDraw();

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    delete player;
}
