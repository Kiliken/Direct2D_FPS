// main.cpp (UI overlay loads PNG via WIC, fixed no-goto-init-skip, no bmpProps collision)

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

// ------------------------------------------------------------
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
static bool gameClear = false;

SDL_Surface *textureBitmap = NULL;

// Brushes
ID2D1SolidColorBrush *ceilBrush = NULL;
ID2D1SolidColorBrush *floorBrush = NULL;
ID2D1SolidColorBrush *wallBrush = NULL;
ID2D1SolidColorBrush *enemyBrush = NULL;

// Wall Bitmap (screen-sized)
ID2D1Bitmap *bitmap = NULL;
D2D1_SIZE_U size;
std::vector<BYTE> pixels;

// Crosshair
D2D1_ELLIPSE crosshair;
D2D1_RECT_F crossCenter;

// ------------------------------------------------------------
// UI overlay (PNG via WIC)
static ID2D1Bitmap *overlayBmpA = NULL;
static ID2D1Bitmap *overlayBmpB = NULL;
static ID2D1Bitmap *overlayBmpCurrent = NULL;
static float uiFlashTimer = 0.0f;
static const float uiFlashDuration = 0.08f; // 80ms (tweak: 0.05~0.12)

// Left mouse edge detection
static bool prevLeftDown = false;

// WIC factory + COM init flag
static IWICImagingFactory *gWicFactory = NULL;
static bool gComInitialized = false;

static D2D1_BITMAP_PROPERTIES uiBmpProps =
    D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                          D2D1_ALPHA_MODE_PREMULTIPLIED));

// Helper: Safe release COM
template <typename T>
static void SafeRelease(T *&p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

static ID2D1Bitmap *LoadD2DBitmapFromFile_WIC(const wchar_t *filename)
{
    if (!filename || !pRenderTarget || !gWicFactory)
        return nullptr;

    IWICBitmapDecoder *decoder = nullptr;
    IWICBitmapFrameDecode *frame = nullptr;
    IWICFormatConverter *converter = nullptr;

    auto cleanup = [&]()
    {
        SafeRelease(converter);
        SafeRelease(frame);
        SafeRelease(decoder);
    };

    HRESULT hr = gWicFactory->CreateDecoderFromFilename(
        filename,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder);

    if (FAILED(hr))
    {
        SDL_Log("WIC CreateDecoderFromFilename failed (0x%08X): %ls", (unsigned)hr, filename);
        cleanup();
        return nullptr;
    }

    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr))
    {
        SDL_Log("WIC GetFrame failed (0x%08X): %ls", (unsigned)hr, filename);
        cleanup();
        return nullptr;
    }

    hr = gWicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr))
    {
        SDL_Log("WIC CreateFormatConverter failed (0x%08X): %ls", (unsigned)hr, filename);
        cleanup();
        return nullptr;
    }

    // Convert into 32bpp premultiplied BGRA
    hr = converter->Initialize(
        frame,
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom);

    if (FAILED(hr))
    {
        SDL_Log("WIC converter Initialize failed (0x%08X): %ls", (unsigned)hr, filename);
        cleanup();
        return nullptr;
    }

    UINT w = 0, h = 0;
    hr = converter->GetSize(&w, &h);
    if (FAILED(hr) || w == 0 || h == 0)
    {
        SDL_Log("WIC GetSize failed (0x%08X): %ls", (unsigned)hr, filename);
        cleanup();
        return nullptr;
    }

    std::vector<BYTE> buf;
    buf.resize((size_t)w * (size_t)h * 4);

    hr = converter->CopyPixels(
        nullptr,
        w * 4,
        (UINT)buf.size(),
        buf.data());

    if (FAILED(hr))
    {
        SDL_Log("WIC CopyPixels failed (0x%08X): %ls", (unsigned)hr, filename);
        cleanup();
        return nullptr;
    }

    ID2D1Bitmap *outBmp = nullptr;
    hr = pRenderTarget->CreateBitmap(
        D2D1::SizeU(w, h),
        buf.data(),
        w * 4,
        &uiBmpProps,
        &outBmp);

    if (FAILED(hr))
    {
        SDL_Log("D2D CreateBitmap (overlay) failed (0x%08X): %ls", (unsigned)hr, filename);
        cleanup();
        return nullptr;
    }

    cleanup();
    return outBmp;
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

    SDL_HideCursor();

    // Init COM for WIC (safe even if other parts don't use COM)
    HRESULT comHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(comHr))
    {
        gComInitialized = true;
    }
    else if (comHr == RPC_E_CHANGED_MODE)
    {
        // COM already initialized with a different threading model; still OK for WIC sometimes.
        // We'll continue, but WIC might fail on some setups.
        gComInitialized = false;
        SDL_Log("Warning: CoInitializeEx returned RPC_E_CHANGED_MODE. Continuing.");
    }
    else
    {
        SDL_Log("CoInitializeEx failed: 0x%08X", (unsigned)comHr);
        return SDL_APP_FAILURE;
    }

    /* Get HWND */
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

    // Create WIC factory
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&gWicFactory));

    if (FAILED(hr) || !gWicFactory)
    {
        SDL_Log("Failed to create WIC Imaging Factory: 0x%08X", (unsigned)hr);
        return SDL_APP_FAILURE;
    }

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

    // Load wall texture atlas (still BMP via SDL, since you sample pixels from it)
    textureBitmap = SDL_LoadBMP("../../Assets/walls.bmp");
    if (!textureBitmap)
    {
        SDL_Log("Failed to load walls.bmp: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create screen-sized bitmap we will update each frame
    hr = pRenderTarget->CreateBitmap(
        size,
        pixels.data(),
        width * 4,
        &uiBmpProps,
        &bitmap);

    if (FAILED(hr) || !bitmap)
    {
        SDL_Log("CreateBitmap (screen) failed: 0x%08X", (unsigned)hr);
        return SDL_APP_FAILURE;
    }

    enemyManager.CreateBillboardRenderer(pRenderTarget, width, height);
    enemyManager.InitializeTargets(5, player->pos);

    crosshair = D2D1::Ellipse(
        D2D1::Point2F((FLOAT)(width / 2), (FLOAT)(height / 2)),
        25.0f, 25.0f);

    crossCenter = D2D1::RectF(width / 2 - 1, height / 2 - 1, width / 2 + 1, height / 2 + 1);

    // ------------------------------------------------------------
    // Load overlay PNGs via WIC
    overlayBmpA = LoadD2DBitmapFromFile_WIC(L"../../Assets/ui_a.png");
    overlayBmpB = LoadD2DBitmapFromFile_WIC(L"../../Assets/ui_b.png");

    if (!overlayBmpA || !overlayBmpB)
    {
        SDL_Log("Failed to load overlay PNGs. Ensure ../../Assets/ui_a.png and ../../Assets/ui_b.png exist.");
        return SDL_APP_FAILURE;
    }

    overlayBmpCurrent = overlayBmpA;

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
        return SDL_APP_SUCCESS;
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
        const float sensitivity = 0.0025f;
        player->angle += event->motion.xrel * sensitivity;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    const Uint64 now = SDL_GetTicks();
    dt = (float)(now - ticks_prev) / 1000.0f;
    ticks_prev = now;

    D2D_POINT_2F forward = {std::cos(player->angle), std::sin(player->angle)};
    D2D_POINT_2F right = {-std::sin(player->angle), std::cos(player->angle)};

    D2D_POINT_2F desired = {0.0f, 0.0f};
    D2D_POINT_2U texture_coords = {0, 0};

    D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
    const int width = static_cast<int>(rtSize.width);
    const int height = static_cast<int>(rtSize.height);

    // Inputs
    {
        const bool *key_states = SDL_GetKeyboardState(NULL);
        D2D_POINT_2F mousePos = D2D1::Point2F(0.0f, 0.0f);
        Uint32 mouseInputs = SDL_GetMouseState(&mousePos.x, &mousePos.y);

        const bool leftDown = (mouseInputs & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;

        // Toggle overlay ONCE per click (edge detect)
        if (leftDown && !prevLeftDown)
        {
            overlayBmpCurrent = overlayBmpB; // show POW
            uiFlashTimer = uiFlashDuration;  // start countdown
        }
        prevLeftDown = leftDown;

        // Your existing shooting logic (kept)
        if (leftDown)
        {
            D2D_POINT_2F rayDir = {std::cos(player->angle), std::sin(player->angle)};
            D2D_POINT_2F hitPos;
            const float enemyCheckProximity = 0.5f;

            float hitDistance = checkEnemyHit(player->pos, rayDir, hitPos, enemyManager);

            if (hitDistance < 1e30f)
            {
                if (enemyManager.RemoveEnemyAt(hitPos, enemyCheckProximity))
                {
                    SDL_Log("Enemy destroyed at distance: %f", hitDistance);

                    if (enemyManager.CountTargets() == 0)
                    {
                        gameClear = true;
                        enemyManager.SetSpawningEnabled(false);
                        enemyManager.DestroyAllEnemies();
                        SDL_Log("Game Clear: All targets destroyed. Enemies stopped and cleared.");
                    }
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

        if (mousePos.x < 50.0f || mousePos.y < 50.0f || mousePos.x > width - 50.0f || mousePos.y > height - 50.0f)
        {
            SDL_WarpMouseInWindow(window, width / 2.0f, height / 2.0f);
        }
    }

    // Update
    float len = std::sqrt(desired.x * desired.x + desired.y * desired.y);
    if (len > 0.0001f)
    {
        desired.x /= len;
        desired.y /= len;
    }

    D2D_POINT_2F nextPos = {player->pos.x + desired.x * player->moveSpeed * dt,
                            player->pos.y + desired.y * player->moveSpeed * dt};

    D2D_POINT_2F playerSize = {0.35f, 0.35f};
    if (canMove(nextPos, playerSize))
    {
        player->pos = nextPos;
    }

    enemyManager.Update(dt, player->pos);

    if (uiFlashTimer > 0.0f)
{
    uiFlashTimer -= dt;
    if (uiFlashTimer <= 0.0f)
    {
        overlayBmpCurrent = overlayBmpA; // back to normal UI
        uiFlashTimer = 0.0f;
    }
}

    // Draw
    pRenderTarget->BeginDraw();
    {
        pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        const float halfH = rtSize.height * 0.5f;
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, rtSize.width, halfH), ceilBrush);
        pRenderTarget->FillRectangle(D2D1::RectF(0, halfH, rtSize.width, rtSize.height), floorBrush);

        const float fov = 60.0f * (3.14159265f / 180.0f);
        const float planeHalf = std::tan(fov * 0.5f);

        static std::vector<float> depthBuffer;
        depthBuffer.assign(width, 1e30f);

        std::fill(pixels.begin(), pixels.end(), 0x00);

        for (int x = 0; x < width; ++x)
        {
            float camX = ((2.0f * x) / (float)width) - 1.0f;

            D2D_POINT_2F dir = {
                std::cos(player->angle) + planeHalf * camX * (-std::sin(player->angle)),
                std::sin(player->angle) + planeHalf * camX * (std::cos(player->angle))};

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

            float perpWallDist = (side == 0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
            if (perpWallDist < 0.0001f)
                perpWallDist = 0.0001f;

            int lineHeight = (int)(height / perpWallDist);
            int drawStart = -lineHeight / 2 + (int)halfH;
            int drawEnd = lineHeight / 2 + (int)halfH;
            if (drawStart < 0)
                drawStart = 0;
            if (drawEnd >= height)
                drawEnd = height - 1;

            int wallTextureNum = (int)wallTypes.find(tile)->second;

            double wallX;
            if (side == 0)
                wallX = player->pos.y + perpWallDist * dir.y;
            else
                wallX = player->pos.x + perpWallDist * dir.x;
            wallX -= floor(wallX);

            int texX = int(wallX * double(texture_wall_size));
            if (side == 0 && dir.x > 0)
                texX = texture_wall_size - texX - 1;
            if (side == 1 && dir.y < 0)
                texX = texture_wall_size - texX - 1;

            double step = 1.0 * double(texture_wall_size) / lineHeight;
            double texPos = (drawStart - height / 2 + lineHeight / 2) * step;

            float shade = (side == 1) ? 0.75f : 1.0f;

            texture_coords.x = texX + (wallTextureNum % 4) * texture_wall_size;

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

                int texY = (int)texPos & (texture_wall_size - 1);
                texPos += step;

                texture_coords.y = texY + (wallTextureNum / 4) * texture_wall_size;

                SDL_Color pixelColor = GetPixelColor(textureBitmap, texture_coords.x, texture_coords.y);

                pixels[currentPixel + 0] = BYTE(pixelColor.b * shade);
                pixels[currentPixel + 1] = BYTE(pixelColor.g * shade);
                pixels[currentPixel + 2] = BYTE(pixelColor.r * shade);
                pixels[currentPixel + 3] = 0xFF;
            }

            depthBuffer[x] = perpWallDist;
        }

        bitmap->CopyFromMemory(nullptr, pixels.data(), width * 4);

        pRenderTarget->DrawBitmap(bitmap, D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height));

        enemyManager.RenderBillboards(pRenderTarget, enemyBrush, textureBitmap, depthBuffer,
                                      width, height, halfH, player->pos, player->angle, planeHalf);

        pRenderTarget->DrawEllipse(crosshair, enemyBrush);
        pRenderTarget->DrawRectangle(crossCenter, enemyBrush);

        // UI overlay
        if (overlayBmpCurrent)
        {
            D2D1_SIZE_F uiSize = overlayBmpCurrent->GetSize();
            FLOAT x = 10.0f, y = 10.0f;
            D2D1_RECT_F dst = D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height);
            pRenderTarget->DrawBitmap(overlayBmpCurrent, dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }
    }
    pRenderTarget->EndDraw();

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SafeRelease(overlayBmpA);
    SafeRelease(overlayBmpB);
    overlayBmpCurrent = nullptr;

    SafeRelease(gWicFactory);

    if (gComInitialized)
    {
        CoUninitialize();
        gComInitialized = false;
    }

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

    SafeRelease(bitmap);
    SafeRelease(ceilBrush);
    SafeRelease(floorBrush);
    SafeRelease(wallBrush);
    SafeRelease(enemyBrush);
    SafeRelease(pRenderTarget);
    SafeRelease(pFactory);
}
