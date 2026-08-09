#include "game.h"
#include "assets.h"
#include "constants.h"
#include "entity.h"
#include "medal.h"
#include "player.h"
#include "primitives.h"
#include "raymath.h"
#include "utils.h"
#include <assert.h>
#include <raylib.h>

static u32 score = 0;
static bool paused = false;

static void process_paused_game(void) {}

static f32 display_score = 0;
static void draw_hud(f32 delta) {
    display_score = Lerp(display_score, (f32)score, 6.0f * delta);
    draw_outlined_text_ex(
        TextFormat("%07d", (i32)display_score), assets.fonts.fusion,
        (Vector2){4, 2}, assets.fonts.fusion.baseSize, 0, WHITE, BLACK, 1);

    const MedalsState medal_state = get_current_medals_state();
    draw_outlined_text_ex(
        TextFormat("%03d", (i32)floorf(medal_state.chain)),
        assets.fonts.fusion, (Vector2){4, 14},
        assets.fonts.fusion.baseSize, 0, WHITE, BLACK, 1);

    Rectangle chain_gauge_bar = (Rectangle){
        .x = 2,
        .y = 28,
        .width = 50,
        .height = 2,
    };
    draw_progress_bar(
        chain_gauge_bar,
        Clamp(medal_state.chain_gauge / MEDAL_CHAIN_GAUGE_MAX, 0, 1.0f),
        BLACK, GREEN);

    draw_outlined_text_ex(TextFormat("%d", get_player_health()),
                          assets.fonts.fusion, (Vector2){4, 30},
                          assets.fonts.fusion.baseSize, 0, RED, BLACK, 1);
    draw_outlined_text_ex(TextFormat("%d", get_player_bombs()),
                          assets.fonts.fusion, (Vector2){12, 30},
                          assets.fonts.fusion.baseSize, 0, BLUE, BLACK, 1);
}

static void draw_pause_overlay() {
    DrawRectangle(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT,
                  Fade(BLACK, 0.5f));
    draw_outlined_text_ex("PAUSED.", assets.fonts.fusion, (Vector2){8, 8},
                          assets.fonts.fusion.baseSize, 0, WHITE, BLACK,
                          1);
}

void draw_game(f32 delta) {
    draw_entities(delta);
    draw_hud(delta);
    if (paused) {
        draw_pause_overlay();
    }
}

void process_game(void) {
    static f32 accumulator = 0;

    if (IsKeyPressed(KEY_ESCAPE)) {
        paused = !paused;
    }

    if (paused) {
        process_paused_game();
        return;
    }

    accumulator += GetFrameTime();

    const f32 fixed_delta = 1.0f / (f32)FRAMERATE;

    while (accumulator >= fixed_delta) {
        assert(!paused);
        process_entities(fixed_delta);
        process_medals_state(fixed_delta);
        accumulator -= fixed_delta;
    }
}

void add_score(u32 extra_score) { score += extra_score; }

void reset_game(void) {
    score = 0;
    display_score = 0;
    reset_entities();
    reset_medals_state();
}
