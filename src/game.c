#include "game.h"
#include "assets.h"
#include "constants.h"
#include "entity.h"
#include "medal.h"
#include "particle.h"
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
    const Rectangle top_src = (Rectangle){
        0,
        0,
        105.0f,
        34.0f,
    };
    DrawTexturePro(assets.textures.hud, top_src, top_src, Vector2Zero(), 0,
                   WHITE);
    DrawRectangle(5, 32, 1, 10, WHITE);

    u8 player_life = get_player_life();
    const Rectangle bottom_src = (Rectangle){
        0,
        35.0f,
        105.0f,
        12.0f,
    };
    const Rectangle bottom_dst = (Rectangle){
        0,
        40.0f + (player_life > 0
                     ? 9.0f + (4.0f * ((f32)player_life - 1.0f))
                     : 0),
        105.0f,
        12.0f,
    };
    DrawTexturePro(assets.textures.hud, bottom_src, bottom_dst,
                   Vector2Zero(), 0, WHITE);

    display_score = Lerp(display_score, (f32)score, 12.0f * delta);
    draw_outlined_text_ex(TextFormat("%08d", (i32)display_score),
                          assets.fonts.fusion, (Vector2){9, -1},
                          assets.fonts.fusion.baseSize, 0, WHITE, BLACK,
                          1);

    const MedalsState medal_state = get_current_medals_state();
    draw_outlined_text_ex(
        TextFormat("%03d", (i32)floorf(medal_state.chain)),
        assets.fonts.fusion, (Vector2){9, 13},
        assets.fonts.fusion.baseSize * 2.0f, 0, WHITE, BLACK, 1);
    draw_outlined_text_ex("chain", assets.fonts.fusion, (Vector2){36, 16},
                          assets.fonts.fusion.baseSize, 0, WHITE, BLACK,
                          1);

    Rectangle chain_gauge_bar = (Rectangle){
        .x = 6,
        .y = 14,
        .width = 55,
        .height = 2,
    };
    draw_progress_bar(
        chain_gauge_bar,
        Clamp(medal_state.chain_gauge / MEDAL_CHAIN_GAUGE_MAX, 0, 1.0f),
        BLACK, GREEN);

    const Rectangle heart_src = (Rectangle){
        0,
        47.0f,
        9.0f,
        9.0f,
    };
    Rectangle heart_dst = (Rectangle){
        1,
        40.0f,
        9.0f,
        9.0f,
    };
    for (i32 i = 0; i < player_life; i++) {
        DrawTexturePro(assets.textures.hud, heart_src, heart_dst,
                       Vector2Zero(), 0, WHITE);
        heart_dst.y += 4.0f;
    }
}

static void draw_pause_overlay() {
    DrawRectangle(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT,
                  Fade(BLACK, 0.5f));
    draw_outlined_text_ex("PAUSED.", assets.fonts.fusion, (Vector2){8, 8},
                          assets.fonts.fusion.baseSize, 0, WHITE, BLACK,
                          1);
}

void draw_game(f32 delta) {
    process_particles(delta);
    draw_particles(delta);
    draw_entities(delta);
    draw_hud(delta);
    draw_high_priority_particles(
        delta); // TODO: remove once draw layers are implemented
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
