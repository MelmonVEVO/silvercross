#include "bomb.h"
#include "assets.h"
#include "constants.h"
#include "entity.h"
#include "utils.h"
#include <raylib.h>

const Vector2 bomb_area = (Vector2){
    .x = 140.0f,
    .y = VIEWPORT_HEIGHT,
};

AnimatedTexture2DInstance bomb_texture_instance = {
    .texture = &assets.textures.beam,
    .animation_time = 0,
    .row = 0,
};

void init_bomb(Entity *self) {
    self->collision = bomb_area;
    self->hp = 1;
    extern bool screenshake;
    screenshake = true;
}

void process_bomb(Entity *self, f32 delta) {
    if (self->time_alive > BOMB_GO_OUT_TIME) {
        // add death particle
        self->queue_destroy = true;
        extern bool screenshake;
        screenshake = false;
    }
}

void draw_bomb(Entity *self, f32 delta) {
    Vector2 pos = get_entity_world_position(self);
    pos.y -= 32.0f;
    draw_animated_texture_ex(&bomb_texture_instance, delta, pos, 0, 1,
                             WHITE);
}
