
#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <Windows.h>
#include <wincodec.h>
#include <unordered_map>
#include <d2d1.h>
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include "raycastTest.h"
#include "Player.h"
#include "EnemyManager.h"

// Window and Render Stuff
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static ID2D1Factory *pFactory = NULL;
static ID2D1HwndRenderTarget *pRenderTarget = NULL;

// Game Stuff
static Player *player = NULL;
static EnemyManager enemyManager;
static Uint64 ticks_prev = 0;
static float dt = 0.0f;

SDL_Surface *textureBitmap = NULL;

// Brushes
ID2D1SolidColorBrush *ceilBrush = NULL;
ID2D1SolidColorBrush *floorBrush = NULL;
ID2D1SolidColorBrush *wallBrush = NULL;
ID2D1SolidColorBrush *enemyBrush = NULL;

// Wall Bitmap
ID2D1Bitmap *bitmap = NULL;
D2D1_SIZE_U size;
std::vector<BYTE> pixels;

// Crosshair
D2D1_ELLIPSE crosshair;
D2D1_RECT_F crossCenter;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("D2DFPS", 1280, 720, SDL_WINDOW_ALWAYS_ON_TOP, &window, &renderer))
    {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_HideCursor();

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

    D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
    const int width = static_cast<int>(rtSize.width);
    const int height = static_cast<int>(rtSize.height);

    size = D2D1::SizeU(width, height);
    pixels = std::vector<BYTE>(width * height * 4);

    player = new Player();
    ticks_prev = SDL_GetTicks();
    enemyManager.Reset();

    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.15f, 1.0f), &ceilBrush);
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.2f, 0.22f, 1.0f), &floorBrush);
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.9f, 0.9f, 1.0f), &wallBrush);
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 1.0f), &enemyBrush);

    textureBitmap = SDL_LoadBMP("../Assets/walls.bmp");

    pRenderTarget->CreateBitmap(
        size,
        pixels.data(), // pointer to pixel buffer
        width * 4,     // pitch (stride in bytes)
        &bmpProps,
        &bitmap);

    enemyManager.CreateBillboardRenderer(pRenderTarget, width, height);

    crosshair = D2D1::Ellipse(
        D2D1::Point2F((FLOAT)(width / 2), (FLOAT)(height / 2)),
        25.0f,
        25.0f);

    crossCenter = D2D1::RectF(width / 2 - 1, height / 2 - 1, width / 2 + 1, height / 2 + 1);

    // textureBitmap = SDL_ConvertSurface(textureBitmap, SDL_PIXELFORMAT_BGRA32);

    // pRenderTarget->CreateBitmap(D2D1::SizeU(textureBitmap->w, textureBitmap->h), textureBitmap->pixels, textureBitmap->pitch, &bmpProps, &wallTexture);

    if (!mapCheck())
    {
        fprintf(stderr, "Map is invalid!\n");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }

    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        if (pRenderTarget)
        {
            pRenderTarget->Resize(D2D1::SizeU(event->window.data1, event->window.data2));
        }
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        // mouse horizontal movement rotates view
        const float sensitivity = 0.0025f; // radians per pixel
        player->angle += event->motion.xrel * sensitivity;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    // delta time
    const Uint64 now = SDL_GetTicks();
    dt = (float)(now - ticks_prev) / 1000.0f;
    ticks_prev = now;

    // movement relative to facing
    D2D_POINT_2F forward = {std::cos(player->angle), std::sin(player->angle)};
    D2D_POINT_2F right = {-std::sin(player->angle), std::cos(player->angle)};

    D2D_POINT_2F desired = {0.0f, 0.0f};
    D2D_POINT_2U texture_coords = {0.0f, 0.0f};

    D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
    const int width = static_cast<int>(rtSize.width);
    const int height = static_cast<int>(rtSize.height);

    // Inputs
    {
        const bool *key_states = SDL_GetKeyboardState(NULL);
        D2D_POINT_2F mousePos = D2D1::Point2F(0.0f, 0.0f);
        Uint32 mouseInputs = SDL_GetMouseState(&mousePos.x, &mousePos.y);

        if (mouseInputs & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))
        {
            // 1. Calculate the ray direction for the center of the screen
            // The center ray is simply the player's view direction.
            D2D_POINT_2F rayDir = {std::cos(player->angle), std::sin(player->angle)};

            // 2. Check for an enemy hit along the ray
            D2D_POINT_2F hitPos;
            const float enemyCheckProximity = 0.5f; // A small radius around the hit point to confirm the enemy's center is close

            // The wall rendering already calculates the perpWallDist for the center ray (x=width/2, camX=0).
            // We use the new checkEnemyHit for accurate sprite hit.
            float hitDistance = checkEnemyHit(player->pos, rayDir, hitPos, enemyManager);

            // 3. Compare hit distance with the closest enemy.
            // A basic check to see if an enemy was hit (hitDistance is not the initial 1e30f value).
            if (hitDistance < 1e30f)
            {
                // An enemy was hit. Remove it.
                // The hitPos is where the *ray* hit the enemy bounding circle.
                // We use a small proximity check to remove the enemy whose center is closest to the hitPos.
                if (enemyManager.RemoveEnemyAt(hitPos, enemyCheckProximity))
                {
                    SDL_Log("Enemy destroyed at distance: %f", hitDistance);
                }
            }
        }

        if (key_states[SDL_SCANCODE_W])
        {
            desired.x += forward.x;
            desired.y += forward.y;
        }
        if (key_states[SDL_SCANCODE_S])
        {
            desired.x -= forward.x;
            desired.y -= forward.y;
        }
        if (key_states[SDL_SCANCODE_A])
        {
            desired.x -= right.x;
            desired.y -= right.y;
        }
        if (key_states[SDL_SCANCODE_D])
        {
            desired.x += right.x;
            desired.y += right.y;
        }

        // optional keyboard rotation
        if (key_states[SDL_SCANCODE_Q])
        {
            player->angle -= player->rotSpeed * dt;
        }
        if (key_states[SDL_SCANCODE_E])
        {
            player->angle += player->rotSpeed * dt;
        }
        if (key_states[SDL_SCANCODE_ESCAPE])
        {
            return SDL_APP_SUCCESS;
        }

        // Mouse position reset
        if (mousePos.x < 50.0f || mousePos.y < 50.0f || mousePos.x > width - 50.0f || mousePos.y > height - 50.0f)
        {
            SDL_WarpMouseInWindow(window, width / 2.0f, height / 2.0f);
        }
    }

    // Update
    // normalize desired
    float len = std::sqrt(desired.x * desired.x + desired.y * desired.y);
    if (len > 0.0001f)
    {
        desired.x /= len;
        desired.y /= len;
    }

    D2D_POINT_2F nextPos = {player->pos.x + desired.x * player->moveSpeed * dt,
                            player->pos.y + desired.y * player->moveSpeed * dt};
    // collision: player radius ~0.25 tiles
    D2D_POINT_2F size = {0.35f, 0.35f};
    if (canMove(nextPos, size))
    {
        player->pos = nextPos;
    }

    enemyManager.Update(dt, player->pos);

    // Draw
    pRenderTarget->BeginDraw();
    {
        pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        // draw ceiling and floor as two rects
        const float halfH = rtSize.height * 0.5f;
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, rtSize.width, halfH), ceilBrush);
        pRenderTarget->FillRectangle(D2D1::RectF(0, halfH, rtSize.width, rtSize.height), floorBrush);

        // raycasting
        const float fov = 60.0f * (3.14159265f / 180.0f); // 60 degrees
        const float planeHalf = std::tan(fov * 0.5f);
        static std::vector<float> depthBuffer;
        depthBuffer.assign(width, 1e30f);
        std::fill(pixels.begin(), pixels.end(), 0x00);

        for (int x = 0; x < width; ++x)
        {
            // camera space x in [-1,1]
            float camX = ((2.0f * x) / (float)width) - 1.0f;
            // ray direction
            D2D_POINT_2F dir = {
                std::cos(player->angle) + planeHalf * camX * (-std::sin(player->angle)),
                std::sin(player->angle) + planeHalf * camX * (std::cos(player->angle))};

            // DDA setup
            int mapX = (int)player->pos.x;
            int mapY = (int)player->pos.y;
            float sideDistX;
            float sideDistY;
            float deltaDistX = (dir.x == 0.0f) ? 1e30f : std::fabs(1.0f / dir.x);
            float deltaDistY = (dir.y == 0.0f) ? 1e30f : std::fabs(1.0f / dir.y);
            int stepX;
            int stepY;
            int hit = 0;
            int side = 0;
            int tile = 0;

            if (dir.x < 0)
            {
                stepX = -1;
                sideDistX = (player->pos.x - mapX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (mapX + 1.0f - player->pos.x) * deltaDistX;
            }
            if (dir.y < 0)
            {
                stepY = -1;
                sideDistY = (player->pos.y - mapY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (mapY + 1.0f - player->pos.y) * deltaDistY;
            }

            while (!hit)
            {
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                }
                else
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }

                tile = getTile(mapX, mapY);

                if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight)
                {
                    hit = 1;
                    break;
                }
                if (tile != '.')
                {
                    hit = 1;
                }
            }

            float perpWallDist;
            if (side == 0)
                perpWallDist = (sideDistX - deltaDistX);
            else
                perpWallDist = (sideDistY - deltaDistY);
            if (perpWallDist < 0.0001f)
                perpWallDist = 0.0001f;

            // line height
            int lineHeight = (int)(height / perpWallDist);
            int drawStart = -lineHeight / 2 + (int)halfH;
            int drawEnd = lineHeight / 2 + (int)halfH;
            if (drawStart < 0)
                drawStart = 0;
            if (drawEnd >= height)
                drawEnd = height - 1;

            // wall texture
            int wallTextureNum = (int)wallTypes.find(tile)->second;

            // calculate value of wallX
            double wallX;
            if (side == 0)
                wallX = player->pos.y + perpWallDist * dir.y;
            else
                wallX = player->pos.x + perpWallDist * dir.x;
            wallX -= floor(wallX);

            // x coordinate on the texture
            int texX = int(wallX * double(texture_wall_size));
            if (side == 0 && dir.x > 0)
                texX = texture_wall_size - texX - 1;
            if (side == 1 && dir.y < 0)
                texX = texture_wall_size - texX - 1;

            // How much to increase the texture coordinate per screen pixel
            double step = 1.0 * double(texture_wall_size) / lineHeight;
            // Starting texture coordinate
            double texPos = (drawStart - height / 2 + lineHeight / 2) * step;

            // shade by side
            float shade = (side == 1) ? 0.75f : 1.0f;

            texture_coords.x = texX + (wallTextureNum % 4) * texture_wall_size; // this should be wallTextureNum % 4 * texture_wall_size

            // for (int y = drawStart; y < drawEnd; y++)
            for (int y = 0; y < height; y++)
            {
                int currentPixel = (y * width + x) * 4;

                if (y <= drawStart || y >= drawEnd)
                {
                    pixels[currentPixel + 0] = 0x00;
                    pixels[currentPixel + 1] = 0x00;
                    pixels[currentPixel + 2] = 0x00;
                    pixels[currentPixel + 3] = 0x00;
                    continue;
                }

                // Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
                int texY = (int)texPos & (texture_wall_size - 1);
                texPos += step;

                texture_coords.y = texY + (wallTextureNum / 4) * texture_wall_size; // this should be wallTextureNum / 4 * texture_wall_size

                SDL_Color pixelColor = GetPixelColor(textureBitmap, texture_coords.x, texture_coords.y);

                pixels[currentPixel + 0] = BYTE(pixelColor.b * shade);
                pixels[currentPixel + 1] = BYTE(pixelColor.g * shade);
                pixels[currentPixel + 2] = BYTE(pixelColor.r * shade);
                pixels[currentPixel + 3] = 0xFF;
            }
            depthBuffer[x] = perpWallDist;
        }

        bitmap->CopyFromMemory(nullptr, pixels.data(), width * 4);
        pRenderTarget->DrawBitmap(
            bitmap,
            D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height));

        enemyManager.RenderBillboards(pRenderTarget, enemyBrush, textureBitmap, depthBuffer, width, height, halfH, player->pos, player->angle, planeHalf);

        pRenderTarget->DrawEllipse(crosshair, enemyBrush);
        pRenderTarget->DrawRectangle(crossCenter, enemyBrush);
    }

    pRenderTarget->EndDraw();

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (textureBitmap)
    {
        SDL_DestroySurface(textureBitmap);
        textureBitmap = nullptr;
    }
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    delete player;
    player = nullptr;

    // delete &enemyManager; // crash risk

    if (bitmap)
    {
        bitmap->Release();
        bitmap = nullptr;
    }
    if (ceilBrush)
    {
        ceilBrush->Release();
        ceilBrush = nullptr;
    }
    if (floorBrush)
    {
        floorBrush->Release();
        floorBrush = nullptr;
    }
    if (wallBrush)
    {
        wallBrush->Release();
        wallBrush = nullptr;
    }
    if (pRenderTarget)
    {
        pRenderTarget->Release();
        pRenderTarget = nullptr;
    }
    if (pFactory)
    {
        pFactory->Release();
        pFactory = nullptr;
    }
    if (enemyBrush)
    {
        enemyBrush->Release();
        enemyBrush = nullptr;
    }
}
