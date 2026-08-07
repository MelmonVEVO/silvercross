#include "constants.h"
#include "entity.h"
#include "game.h"
#include "primitives.h"
#include <dlfcn.h>
#include <math.h>
#include <raylib.h>

bool is_gameplay = true;

void load_assets(void) {}

void PROCESS(void) { process_game(); }

void draw_main_menu(f32 delta) {}

void DRAW(RenderTexture2D target) {
    f32 delta = GetFrameTime();
    f32 screen_scale =
        MIN(floorf((f32)GetScreenWidth() / (f32)VIEWPORT_WIDTH),
            floorf((f32)GetScreenHeight() / (f32)VIEWPORT_HEIGHT));

    BeginTextureMode(target);
    ClearBackground(BLACK);
    if (is_gameplay)
        draw_game(delta);
    else
        draw_main_menu(delta);
    EndTextureMode();

    BeginDrawing();
    ClearBackground(WHITE);

    f32 vpwidth_scaled = VIEWPORT_WIDTH * screen_scale;
    f32 vpheight_scaled = VIEWPORT_HEIGHT * screen_scale;
    f32 vpdraw_x;
    f32 vpdraw_y;

    vpdraw_y = (GetScreenHeight() - vpheight_scaled) * 0.5f;
    vpdraw_x = (GetScreenWidth() - vpwidth_scaled) * 0.5f;

    // Shadow
    DrawRectanglePro((Rectangle){vpdraw_x + 10, vpdraw_y + 10,
                                 vpwidth_scaled, vpheight_scaled},
                     (Vector2){0, 0}, 0, (Color){30, 30, 30, 180});

    // Prevent the wallpaper from bleeding into the viewport
    DrawRectanglePro(
        (Rectangle){vpdraw_x, vpdraw_y, vpwidth_scaled, vpheight_scaled},
        (Vector2){0, 0}, 0, BLACK);

    // Viewport
    DrawTexturePro(
        target.texture,
        (Rectangle){0, 0, target.texture.width, -target.texture.height},
        (Rectangle){vpdraw_x, vpdraw_y, vpwidth_scaled, vpheight_scaled},
        (Vector2){0, 0}, 0, WHITE);

    EndDrawing();
}

typedef void *magical_main(void *data);

i32 main(void) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(VIEWPORT_WIDTH, VIEWPORT_HEIGHT, "Magical Girl Shoot");
    SetWindowMinSize(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    HideCursor();
    ChangeDirectory("res");
    load_assets();
    SetTargetFPS(120);

    RenderTexture2D screen_target =
        LoadRenderTexture(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    SetTextureFilter(screen_target.texture, TEXTURE_FILTER_POINT);

    reset_entities();
    while (!WindowShouldClose()) {
        PROCESS();
        DRAW(screen_target);
    }

    return 0;
}
