#include "enemy.h"
#include "assets.h"
#include "bullet.h"
#include "constants.h"
#include "entity.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "utils.h"
#include <assert.h>

// -- Luciko --

BulletConfig luciko_pat1_spiral_bullet = (BulletConfig){
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 0,
    .initial_speed = 90.0f,
    .initial_ttl = 10.0f,
};

PatternConfig luciko_pat1_spiral = (PatternConfig){
    .pattern_type = BP_ONE,
    .bullet_config = &luciko_pat1_spiral_bullet,
    .spawn_offset = 4.0f,
    .trajectory = TRJ_DEFAULT,
};

EmitterConfig luciko_pat1_spiral_emitter = (EmitterConfig){
    .number_of_volleys = -1,
    .pattern = &luciko_pat1_spiral,
    .rotation_range = 360.0f,
    .rotation_speed = 100.0f,
    .rotation_type = ROT_CONTINUOUS,
    .start_rotation = 90.0f,
    .volley_rate = 24.0f,
    .time_until_start = 0.5f,
};

BulletConfig luciko_pat1_arc_bullet = (BulletConfig){
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 1,
    .initial_speed = 130.0f,
    .initial_ttl = 5.0f,
};

PatternConfig luciko_pat1_arc = (PatternConfig){
    .pattern_type = BP_ARC,
    .bullet_config = &luciko_pat1_arc_bullet,
    .trajectory = TRJ_DEFAULT,
    .bullets_in_pattern = 7,
    .pattern_length = 40.0f,
    .burst_data =
        {
            .number_of_shots = 3,
            .end_bullet_speed_modifier = 130.0f,
            .end_bullets_in_pattern = 7,
            .total_time = 0.85f,
        },
    .flags = PATTERNFLAG_HAS_BURST,
};

EmitterConfig luciko_pat1_arc_emitter = (EmitterConfig){
    .pattern = &luciko_pat1_arc,
    .rotation_range = 360.0f,
    .rotation_type = ROT_TOWARDS_PLAYER,
    .number_of_volleys = -1,
    .time_until_start = 0.5f,
    .volley_rate = 0.3f,
};

// -- Test enemy --

BulletConfig test_enemy_bullet = (BulletConfig){
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 3,
    .initial_speed = 110.0f,
    .initial_ttl = 5.0f,
    .flags = BULLETFLAG_ROTATE_TEXTURE,
};

PatternConfig test_enemy_pattern = (PatternConfig){
    .pattern_type = BP_RING,
    .bullet_config = &test_enemy_bullet,
    .bullets_in_pattern = 10,
    .trajectory = TRJ_DEFAULT,
};

EmitterConfig test_enemy_emitter = (EmitterConfig){
    .pattern = &test_enemy_pattern,
    .enabled_by_default = true,
    .rotation_range = 360.0f,
    .rotation_speed = 30.0f,
    .rotation_type = ROT_CONTINUOUS,
    .volley_rate = 4.0f,
};

void init_test_enemy(Entity *self) {
    assert(self->as.enemy.enemy_type == ENEMY_TEST_ENEMY);
    self->hp = 40;
    self->collision = (Vector2){64.0f, 64.0f};
    self->as.enemy.seal_circle_radius = 100.0f;
    setup_emitter(&self->as.enemy.current_emitters[0],
                  &test_enemy_emitter);
}

void process_test_enemy(Entity *self, f32 delta) {
    self->velocity = Vector2Scale((Vector2){sinf(3.0f * self->time_alive),
                                            cosf(1.6f * self->time_alive)},
                                  16.0f);

    move_entity(self, delta);
}

void draw_test_enemy(Entity *self) {
    DrawRectangle(self->position.x - 32, self->position.y - 32, 64, 64,
                  RED);
}

// -- Regular enemy stuff --

// Returns the position of an enemy's emitter in world space.
static Vector2 emitter_world_pos(Entity *enemy, EmitterLive *emitter) {
    return Vector2Add(entity_world_position(enemy),
                      emitter->config->local_position);
}

static float burst_shot_interval(const PatternConfig *pattern) {
    assert(pattern);
    if (pattern->burst_data.number_of_shots <= 1)
        return 0.0f;
    return pattern->burst_data.total_time /
           (float)(pattern->burst_data.number_of_shots - 1);
}

static float burst_shot_progress(const PatternConfig *pattern,
                                 unsigned int shot_index) {
    assert(pattern);
    if (pattern->burst_data.number_of_shots <= 1)
        return 1.0f;
    return (float)shot_index /
           (float)(pattern->burst_data.number_of_shots - 1);
}

static void update_emitter_rotation(Entity *enemy, EmitterLive *emitter,
                                    float delta) {
    assert(emitter->config && "No emitter pattern assigned.");
    bool burst_active = emitter->burst.active;
    bool burst_locks_rotation =
        burst_active && (emitter->config->pattern->flags &
                         PATTERNFLAG_LOCK_EMITTER_ROTATION);

    switch (emitter->config->rotation_type) {
    case ROT_TOWARDS_PLAYER:
        emitter->current_rotation = front_towards_whatever(
            emitter_world_pos(enemy, emitter),
            burst_active ? emitter->burst.player_position_on_burst_start
                         : player_position());
        break;
    case ROT_CONTINUOUS:
        if (!burst_locks_rotation) {
            emitter->current_rotation =
                fmodf(emitter->current_rotation - // BUG: weird behaviour
                                                  // with negative rotation
                                                  // speeds
                          emitter->config->start_rotation +
                          (emitter->config->rotation_speed * delta),
                      emitter->config->rotation_range) +
                emitter->config->start_rotation;
        }
        break;
    case ROT_BOUNCE: // TODO: bounce rotation
        break;
    case ROT_RANDOMISE:
        if (!burst_locks_rotation) {
            emitter->current_rotation =
                (random_float() * emitter->config->rotation_range) +
                emitter->config->start_rotation;
        }
        break;
    default:
        break;
    }
}

static void start_emitter_burst(EmitterLive *emitter) {
    assert(emitter);
    assert(emitter->config);
    assert(emitter->config->pattern);
    if (emitter->config->pattern->burst_data.number_of_shots == 0) {
        log_warning("Tried to start a burst with 0 shots.");
        return;
    }
    emitter->burst.active = true;
    emitter->burst.shot_cooldown = 0.0f;
    emitter->burst.shots_fired = 0;
    emitter->burst.locked_rotation = emitter->current_rotation;
    emitter->burst.player_position_on_burst_start = player_position();
}

static void process_emitter_burst(Entity *enemy, EmitterLive *emitter,
                                  float delta) {
    EnemyData *enemy_data = &enemy->as.enemy;
    assert(enemy_data);
    assert(emitter);
    assert(emitter->config);
    const PatternConfig *pattern = emitter->config->pattern;
    assert(pattern);

    if (!emitter->burst.active)
        return;

    emitter->burst.shot_cooldown -= delta;

    while (emitter->burst.active && emitter->burst.shot_cooldown <= 0.0f) {
        Vector2 pos = emitter_world_pos(enemy, emitter);
        bool lock_burst_rotation =
            (pattern->flags & PATTERNFLAG_LOCK_EMITTER_ROTATION) &&
            emitter->config->rotation_type != ROT_TOWARDS_PLAYER;
        float angle = lock_burst_rotation ? emitter->burst.locked_rotation
                                          : emitter->current_rotation;

        if (!enemy_data->sealed) {
            fire_pattern_burst_shot(
                pos, angle, pattern,
                burst_shot_progress(pattern, emitter->burst.shots_fired));
        }

        emitter->burst.shots_fired++;
        if (emitter->burst.shots_fired >=
            pattern->burst_data.number_of_shots) {
            emitter->burst.active = false;
            break;
        }

        emitter->burst.shot_cooldown += burst_shot_interval(pattern);
    }
}

static float get_volley_cooldown(EmitterLive *emitter_current) {
    if (emitter_current->config->volley_rate == 0.0f) {
        log_error("An emitter has a volley rate of 0. Why?");
        return 0;
    }
    return 1.0f / emitter_current->config->volley_rate;
}

static void process_emitters(Entity *enemy, float delta) {
    for (int i = 0; i < MAX_EMITTERS; i++) {
        EnemyData *enemy_data = &enemy->as.enemy;
        EmitterLive *emitter = &enemy_data->current_emitters[i];

        if (emitter->config == NULL)
            continue;

        if (!emitter->enabled)
            continue;

        if (emitter->config->pattern == NULL) {
            log_error(
                "Tried to fire an emitter which does not have a pattern!");
            continue;
        }

        update_emitter_rotation(enemy, emitter, delta);

        if (emitter->burst.active) {
            process_emitter_burst(enemy, emitter, delta);
            continue;
        }

        if (emitter->volleys_left == 0)
            continue;

        emitter->cooldown_between_volleys -= delta;
        const PatternConfig *pattern = emitter->config->pattern;
        if (!pattern)
            continue;

        if (emitter->cooldown_between_volleys > 0)
            continue;

        if (pattern->flags & PATTERNFLAG_HAS_BURST) {
            start_emitter_burst(emitter);
            if (emitter->burst.active)
                process_emitter_burst(enemy, emitter, 0.0f);
        } else {
            Vector2 pos = emitter_world_pos(enemy, emitter);
            if (!enemy_data->sealed)
                fire_pattern(
                    pos, emitter->current_rotation,
                    pattern); // Fucking look at this fucking nesting
        } // Yep, it keeps going..

        if (emitter->config->number_of_volleys != -1)
            emitter->volleys_left--;
        emitter->cooldown_between_volleys += get_volley_cooldown(emitter);
    } // Nearly there!
} // Let's not do that again.

void spawn_enemy(EnemyType type, Vector2 at) {
    Entity *enemy = spawn_entity(ENTITY_ENEMY);
    enemy->as.enemy = (EnemyData){};
    enemy->as.enemy.enemy_type = type;
    switch (type) {
    case ENEMY_TEST_ENEMY:
        init_test_enemy(enemy);
        break;
    default:
    }
    enemy->position = at;
}

void setup_emitter(EmitterLive *emitter, const EmitterConfig *config) {
    *emitter = (EmitterLive){};
    if (config == NULL)
        return;
    emitter->config = config;
    emitter->volleys_left =
        config->number_of_volleys > 0 ? config->number_of_volleys : -1;
    emitter->enabled = config->enabled_by_default;
    emitter->cooldown_between_volleys =
        emitter->config->time_until_start
            ? emitter->config->time_until_start
            : get_volley_cooldown(emitter);
    emitter->current_rotation = emitter->config->start_rotation;
}

void process_enemy(Entity *self, f32 delta) {
    EnemyData *data = &self->as.enemy;
    switch (data->enemy_type) {
    case ENEMY_TEST_ENEMY:
        process_test_enemy(self, delta);
        break;
    default:
    }
    data->sealed = data->seal_circle_radius > 0 &&
                   Vector2LengthSqr(Vector2Subtract(player_position(),
                                                    self->position)) <=
                       data->seal_circle_radius * data->seal_circle_radius;
    process_emitters(self, delta);
}

void draw_enemy(Entity *self, f32 delta) {
    switch (self->as.enemy.enemy_type) {
    case ENEMY_TEST_ENEMY:
        draw_test_enemy(self);
        break;
    default:
    }
    if (self->as.enemy.sealed)
        draw_centred_texture_ex(assets.textures.seal, self->position, 0,
                                1.0f, Fade(WHITE, 0.6f));
}

void hit_enemy(Entity *self, Entity *other) {
    if (other->type != ENTITY_BULLET ||
        !(other->as.bullet.config.flags & BULLETFLAG_PLAYER))
        return;
    Entity *medal = spawn_entity(ENTITY_MEDAL);
    medal->position = self->position;
    entity_damage(self, PLAYER_BULLET_DAMAGE);
}

void die_enemy(Entity *self) {}

void damage_enemy(Entity *self, f32 amount) {}
