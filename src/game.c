#include "assets.h"
#include "bullet.h"
#include "constants.h"
#include "entity.h"
#include "primitives.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>

bool paused = false;

static void process_paused_game(void) {}

void draw_game(f32 delta) { draw_entities(delta); }

// XXX: temporary
BulletConfig test_bullet_config = (BulletConfig){
    .flags = BULLETFLAG_ROTATE_TEXTURE,
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 2,
    .initial_speed = 40.0f,
    .initial_ttl = 10.0f,
};

f32 cd = 0;
f32 r = 0;

void process_game(void) {
    static f32 accumulator = 0;
    if (paused) {
        process_paused_game();
        return;
    }

    accumulator += GetFrameTime();

    const f32 fixed_delta = 1.0f / (f32)FRAMERATE;

    while (accumulator >= fixed_delta) {
        assert(!paused);
        if (cd <= 0) {
            bullet_fire_ring((Vector2){VIEWPORT_WIDTH / 2.0f, 40}, r,
                             &test_bullet_config, 450, 0, TRJ_DEFAULT);
            cd += 0.03f;
        }
        r = fmodf(r + (15.0f * fixed_delta), 360.0f);
        cd -= fixed_delta;
        process_entities(fixed_delta);
        accumulator -= fixed_delta;
    }
}
