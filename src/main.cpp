
#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <Windows.h>
#include <wincodec.h>
#include <unordered_map>
#include <d2d1.h>
#include <cmath>
#include "raycastTest.h"
#include "Player.h"

// Window Stuff
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static ID2D1Factory *pFactory = NULL;
static ID2D1HwndRenderTarget *pRenderTarget = NULL;
static Player *player = NULL;
static Uint64 ticks_prev = 0;
static float dt = 0.0f;

// Image Stuff
D2D1_BITMAP_PROPERTIES bmpProps = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
SDL_Surface *wallbitmap = NULL;
ID2D1Bitmap *wallTexture = NULL;

// Brushes
ID2D1SolidColorBrush *ceilBrush = NULL;
ID2D1SolidColorBrush *floorBrush = NULL;
ID2D1SolidColorBrush *wallBrush = NULL;

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

    player = new Player();
    ticks_prev = SDL_GetTicks();

    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.15f, 1.0f), &ceilBrush);
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.2f, 0.22f, 1.0f), &floorBrush);
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.9f, 0.9f, 1.0f), &wallBrush);

    wallbitmap = SDL_LoadBMP("../Assets/walls.bmp");

    // wallbitmap = SDL_ConvertSurface(wallbitmap, SDL_PIXELFORMAT_BGRA32);

    // pRenderTarget->CreateBitmap(D2D1::SizeU(wallbitmap->w, wallbitmap->h), wallbitmap->pixels, wallbitmap->pitch, &bmpProps, &wallTexture);

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

    // Inputs
    {
        const bool *key_states = SDL_GetKeyboardState(NULL);

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

    // Draw
    pRenderTarget->BeginDraw();
    {
        pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
        const int width = static_cast<int>(rtSize.width);
        const int height = static_cast<int>(rtSize.height);

        // draw ceiling and floor as two rects
        const float halfH = rtSize.height * 0.5f;
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, rtSize.width, halfH), ceilBrush);
        pRenderTarget->FillRectangle(D2D1::RectF(0, halfH, rtSize.width, rtSize.height), floorBrush);

        // raycasting
        const float fov = 60.0f * (3.14159265f / 180.0f); // 60 degrees
        const float planeHalf = std::tan(fov * 0.5f);

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
            double wallX; // where exactly the wall was hit
            if (side == 0)
                wallX = mapY + perpWallDist * dir.y;
            else
                wallX = mapX + perpWallDist * dir.x;
            wallX -= floor((wallX));

            // x coordinate on the texture
            int texX = int(wallX * double(texture_wall_size));
            if (side == 0 && dir.x > 0)
                texX = texture_wall_size - texX - 1;
            if (side == 1 && dir.y < 0)
                texX = texture_wall_size - texX - 1;

            // How much to increase the texture coordinate per screen pixel
            double step = 1.0 * texture_wall_size / lineHeight;
            // Starting texture coordinate
            double texPos = (drawStart - height / 2 + lineHeight / 2) * step;

            // shade by side
            float shade = (side == 1) ? 0.75f : 1.0f;

            texture_coords.x = texX + (wallTextureNum % 4) * texture_wall_size; //this should be wallTextureNum % 4 * texture_wall_size

            for (int y = drawStart; y < drawEnd; y++)
            {
                // Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
                int texY = (int)texPos & (texture_wall_size - 1);
                texPos += step;

                texture_coords.y = texY + (wallTextureNum / 4) * texture_wall_size; //this should be wallTextureNum / 4 * texture_wall_size

                SDL_Color pixelColor = GetPixelColor(wallbitmap, texture_coords.x, texture_coords.y);
                //SDL_Color pixelColor = SDL_Color{255,0,0,1};

                wallBrush->SetColor(D2D1::ColorF(
                    (pixelColor.r / 255.0f) * shade,
                    (pixelColor.g / 255.0f) * shade,
                    (pixelColor.b / 255.0f) * shade,
                    1.0f));

                pRenderTarget->FillRectangle(
                    D2D1::RectF(x, y, x + 1, y + 1),
                    wallBrush);
            }

            // wallBrush->SetColor(D2D1::ColorF(pixelColor.r * shade, pixelColor.g * shade, pixelColor.b * shade, 1.0f));
            // pRenderTarget->DrawLine(
            //     D2D1::Point2F((FLOAT)x, (FLOAT)drawStart),
            //     D2D1::Point2F((FLOAT)x, (FLOAT)drawEnd),
            //     wallBrush,
            //     1.0f);
        }
    }
    pRenderTarget->EndDraw();

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    player = NULL;
    if (pRenderTarget)
    {
        pRenderTarget->Release();
        pRenderTarget = NULL;
    }
    if (pFactory)
    {
        pFactory->Release();
        pFactory = NULL;
    }
    if (ceilBrush)
        ceilBrush->Release();
    if (floorBrush)
        floorBrush->Release();
    if (wallBrush)
        wallBrush->Release();
}
