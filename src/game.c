#include "entity.h"
#include "primitives.h"
#include <assert.h>
#include <raylib.h>

bool paused = false;

static void process_paused_game(void) {}

void draw_game(f32 delta) { draw_entities(delta); }

void process_game(void) {
    static f32 accumulator = 0;
    if (paused) {
        process_paused_game();
        return;
    }

    accumulator += GetFrameTime();

    const f32 fixed_delta = 1.0f / 120.0f;

    while (accumulator >= fixed_delta) {
        assert(!paused);
        process_entities(fixed_delta);
        accumulator -= fixed_delta;
    }
}
