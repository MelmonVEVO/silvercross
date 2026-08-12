#include "medal.h"
#include "assets.h"
#include "constants.h"
#include "entity.h"
#include "game.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "utils.h"
#include <math.h>

static MedalsState medals_state = {};

void process_medals_state(f32 delta) {
    if (medals_state.chain_gauge_stopped)
        return;
    medals_state.chain_gauge -= delta;
    if (medals_state.chain > 0 && medals_state.chain_gauge <= 0) {
        f32 drain_rate =
            medals_state.chain_gauge <
                    MEDAL_CHAIN_GAUGE_SUPER_DRAIN_THRESHOLD
                ? MEDAL_CHAIN_DRAIN_RATE +
                      ((fabsf(medals_state.chain_gauge) -
                        fabsf(MEDAL_CHAIN_GAUGE_SUPER_DRAIN_THRESHOLD)) *
                       MEDAL_CHAIN_DRAIN_RATE_EXTRA_PER_SEC)
                : MEDAL_CHAIN_DRAIN_RATE;
        medals_state.chain =
            MAX(0, medals_state.chain - (drain_rate * delta));
    }
}

const MedalsState get_current_medals_state(void) { return medals_state; }

void process_medal(Entity *self, f32 delta) {
    if (self->as.medal.is_following_player) {
        self->velocity =
            Vector2Scale(Vector2Normalize(Vector2Subtract(
                             player_position(), self->position)),
                         FOLLOWING_MEDAL_SPEED);
        move(&self->position, self->velocity, delta);
        return;
    }
    self->as.medal.value =
        MAX(MEDAL_LOWEST_VALUE,
            self->as.medal.value - (MEDAL_VALUE_LOWERING_RATE * delta));
    self->velocity.y =
        MIN(self->velocity.y + (MEDAL_GRAVITY * delta), MEDAL_GRAVITY);
    move(&self->position, self->velocity, delta);
    f32 attract_circle_radius = IsKeyDown(KEY_Z)
                                    ? ATTRACT_CIRCLE_RADIUS
                                    : ATTRACT_CIRCLE_RADIUS_LARGE;
    bool close_enough = Vector2LengthSqr(Vector2Subtract(player_position(),
                                                         self->position)) <
                        attract_circle_radius * attract_circle_radius;
    if (close_enough) {
        self->as.medal.is_following_player = true;
    }

    if (self->position.y > VIEWPORT_HEIGHT + 10.0f) {
        destroy_entity_ptr(self);
    }
}

void draw_medal(Entity *self, [[maybe_unused]] f32 delta) {
    draw_centred_texture(assets.textures.medal, self->position);
    DrawTextEx(assets.fonts.fusion,
               TextFormat("%d", (i32)self->as.medal.value),
               Vector2Add(self->position, (Vector2){-5.0f, -8.0f}),
               assets.fonts.fusion.baseSize, 0, MEDAL_TEXT_COLOUR);
}

void init_medal(Entity *self) {
    const f32 angle = (random_float() * (PI / 6)) - (PI / 12);
    self->velocity = Vector2Rotate(
        Vector2Scale(VECTOR2UP, 200.0f + (50.0f * random_float())), angle);
    self->collision = (Vector2){9, 9};
    self->as.medal = (MedalData){};
    self->as.medal.value = MEDAL_INITIAL_VALUE;
}

void hit_medal(Entity *self, Entity *other) {
    if (other->type != ENTITY_PLAYER)
        return;

    medals_state.chain_gauge = MEDAL_CHAIN_GAUGE_MAX;
    medals_state.chain += 1.0f;

    PlaySound(assets.sfx.medal_collect);
    add_score((u32)floorf(self->as.medal.value *
                          (1.0f + (medals_state.chain / 100.0f))));

    destroy_entity_ptr(self);
}

void set_medal_chain_gauge_stop(bool stop) {
    medals_state.chain_gauge_stopped = stop;
}

void set_medal_chain_gauge_0(void) { medals_state.chain_gauge = 0; }

void reset_medals_state(void) { medals_state = (MedalsState){}; }
