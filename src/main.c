#include "assets.h"
#include "constants.h"
#include "enemy.h"
#include "entity.h"
#include "game.h"
#include "particle.h"
#include "primitives.h"
#include "utils.h"
#include <dlfcn.h>
#include <math.h>
#include <raylib.h>

bool is_gameplay = true;

void PROCESS(void) { process_game(); }

void draw_main_menu(f32 delta) {}

#ifdef DEBUG
struct {
    f64 times[120];
    u32 cursor
} frame_times = {};

static inline f64 mean(f64 times[120]) {
    f64 answer = 0;
    for (i32 i = 0; i < 120; i++) {
        answer += times[i];
    }
    answer /= 120.0;
    return answer;
}
#endif

void DRAW(RenderTexture2D target) {
    f32 delta = GetFrameTime();
    f32 screen_scale =
        MIN(floorf((f32)GetScreenWidth() / (f32)VIEWPORT_WIDTH),
            floorf((f32)GetScreenHeight() / (f32)VIEWPORT_HEIGHT));

    BeginTextureMode(target);
    ClearBackground(DARKGRAY);
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

    vpdraw_y = floorf((GetScreenHeight() - vpheight_scaled) * 0.5f);
    vpdraw_x = floorf((GetScreenWidth() - vpwidth_scaled) * 0.5f);

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

    draw_outlined_text_ex(TextFormat("FPS: %d", GetFPS()),
                          assets.fonts.fusion, (Vector2){16.0f, 4},
                          (f32)assets.fonts.fusion.baseSize * 4, 0, WHITE,
                          BLACK, 2);
    draw_outlined_text_ex(TextFormat("ENTITIES: %d", entity_count()),
                          assets.fonts.fusion, (Vector2){16.0f, 48.0f},
                          (f32)assets.fonts.fusion.baseSize * 4, 0, WHITE,
                          BLACK, 2);
#ifdef DEBUG
    draw_outlined_text_ex(
        TextFormat("%.04f ms", mean(frame_times.times) * 1000.0f),
        assets.fonts.fusion, (Vector2){16.0f, 128.0f},
        assets.fonts.fusion.baseSize * 2, 0, WHITE, BLACK, 2);
#endif

    EndDrawing();
}

typedef void *magical_main(void *data);

i32 main(void) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(VIEWPORT_WIDTH, VIEWPORT_HEIGHT,
               "JUDGEMENT OF HEAVENLY SILVERCROSS");
    SetWindowMinSize(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    HideCursor();
    ChangeDirectory("res");
    load_assets();
    SetTargetFPS(FRAMERATE);

    RenderTexture2D screen_target =
        LoadRenderTexture(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    SetTextureFilter(screen_target.texture, TEXTURE_FILTER_POINT);

    reset_game();
    spawn_enemy(ENEMY_TEST_ENEMY, (Vector2){VIEWPORT_WIDTH / 2.0f, 60.0f});
    initialise_particle_pool();
    while (!WindowShouldClose()) {
#ifdef DEBUG
        frame_times.times[frame_times.cursor] = GetTime();
#endif
        PROCESS();
#ifdef DEBUG
        frame_times.times[frame_times.cursor] =
            GetTime() - frame_times.times[frame_times.cursor];
        frame_times.cursor = (frame_times.cursor + 1) % 120;
#endif
        DRAW(screen_target);
    }

    return 0;
}
