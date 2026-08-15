#include "medal.h"
#include "assets.h"
#include "constants.h"
#include "entity.h"
#include "game.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "utils.h"
#include <assert.h>
#include <math.h>

static MedalsState medals_state = {};

static f32 get_medal_value(const Entity *self) {
    switch (self->as.medal.type) {
    case MEDAL_SMALL:
        return MEDAL_SMALL_VALUE;
    case MEDAL_MEDIUM:
        return MEDAL_MEDIUM_VALUE;
    case MEDAL_LARGE:
        return MEDAL_LARGE_VALUE;
    default:
        log_error("This medal doesn't exist!");
        assert(false);
    }
}

void process_medals_state(f32 delta) {
    if (medals_state.chain_gauge_stopped)
        return;
    if (medals_state.is_boss_time) {
        medals_state.chain = MAX(
            0, medals_state.chain - (MEDAL_CHAIN_BOSS_DRAIN_RATE * delta));
        return;
    }
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
    self->velocity.y =
        MIN(self->velocity.y + (MEDAL_GRAVITY * delta), MEDAL_GRAVITY);
    move(&self->position, self->velocity, delta);
    const f32 attract_circle_radius = get_player_attract_circle_radius();
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
}

void init_medal(Entity *self) {
    const f32 angle = (random_float() * (PI / 6)) - (PI / 12);
    self->velocity = Vector2Rotate(
        Vector2Scale(VECTOR2UP, 200.0f + (50.0f * random_float())), angle);
    self->collision = (Vector2){9, 9};
    self->as.medal = (MedalData){};
    self->as.medal.type = MEDAL_MEDIUM; // TODO: different medal values
}

void hit_medal(Entity *self, Entity *other) {
    if (other->type != ENTITY_PLAYER)
        return;

    medals_state.chain_gauge = MEDAL_CHAIN_GAUGE_MAX;
    medals_state.chain += 1.0f;

    PlaySound(assets.sfx.medal_collect);
    add_score((u32)floorf(get_medal_value(self) *
                          (1.0f + (medals_state.chain / 100.0f))));

    destroy_entity_ptr(self);
}

void set_medal_chain_gauge_stop(bool stop) {
    medals_state.chain_gauge_stopped = stop;
}

void set_medal_chain_boss_behaviour(bool is_boss) {
    medals_state.is_boss_time = is_boss;
    if (is_boss) {
        medals_state.chain_gauge = 0;
    }
}

void set_medal_chain_gauge_0(void) { medals_state.chain_gauge = 0; }

void reset_medals_state(void) { medals_state = (MedalsState){}; }
