#include "enemy.h"
#include "assets.h"
#include "bullet.h"
#include "constants.h"
#include "entity.h"
#include "game.h"
#include "medal.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "utils.h"
#include <assert.h>

f32 boss_timer = 0;

// -- Luciko --

BulletConfig luciko_pat1_spiral_bullet = (BulletConfig){
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 0,
    .initial_speed = 200.0f,
    .min_speed = 80.0f,
    .acceleration = -280.0f,
    .initial_ttl = 5.0f,
};

PatternConfig luciko_pat1_spiral = (PatternConfig){
    .pattern_type = BP_RING,
    .bullets_in_pattern = 2,
    .bullet_config = &luciko_pat1_spiral_bullet,
    .spawn_offset = 4.0f,
    .trajectory = TRJ_DEFAULT,
};

EmitterConfig luciko_pat1_spiral_emitter = (EmitterConfig){
    .number_of_volleys = -1,
    .pattern = &luciko_pat1_spiral,
    .rotation_range = 360.0f,
    .rotation_speed = 750.0f,
    .rotation_type = ROT_CONTINUOUS,
    .start_rotation = 90.0f,
    .volley_rate = 70.0f,
    .time_until_start = 0.5f,
    .enabled_by_default = true,
};

BulletConfig luciko_pat1_arc_bullet = (BulletConfig){
    .bullet_texture = &assets.textures.bullets1,
    .flags = BULLETFLAG_BIG | BULLETFLAG_HIGH_PRIORITY,
    .texture_row = 1,
    .initial_speed = 130.0f,
    .initial_ttl = 5.0f,
};

PatternConfig luciko_pat1_arc = (PatternConfig){
    .pattern_type = BP_ARC,
    .bullet_config = &luciko_pat1_arc_bullet,
    .trajectory = TRJ_DEFAULT,
    .bullets_in_pattern = 7,
    .pattern_length = 16.0f,
    .burst_data =
        {
            .number_of_shots = 3,
            .end_bullet_speed_modifier = 130.0f,
            .end_bullets_in_pattern = 7,
            .total_time = 0.65f,
        },
    .flags = PATTERNFLAG_HAS_BURST,
};

EmitterConfig luciko_pat1_arc_emitter = (EmitterConfig){
    .pattern = &luciko_pat1_arc,
    .rotation_range = 360.0f,
    .rotation_type = ROT_TOWARDS_PLAYER,
    .number_of_volleys = -1,
    .time_until_start = 0.83333f,
    .volley_rate = 0.38f,
    .enabled_by_default = true,
};
#define LUCIKO_HOME_POSITION (Vector2){VIEWPORT_WIDTH / 2.0f, 60.0f}
#define LUCIKO_PHASE1_HP 900.0f
#define LUCIKO_PHASE2_HP 780.0f
#define LUCIKO_PHASE3_HP 780.0f
#define LUCIKO_PHASE4_HP 780.0f
#define LUCIKO_PHASE1_TIME 25
#define LUCIKO_PHASE2_TIME 22
#define LUCIKO_PHASE3_TIME 22
#define LUCIKO_PHASE4_TIME 22

static void enter_luciko_phase(Entity *self, LucikoPhase phase) {
    set_medal_chain_boss_behaviour(true);
    EmitterLive *emitters = self->as.enemy.current_emitters;
    for (i32 i = 0; i < MAX_EMITTERS; i++) {
        stop_emitter(&emitters[i]);
    }
    switch (phase) {
    case LK_MOVE:
        set_medal_chain_gauge_stop(true);
        self->as.enemy.damage_modifier = 0;
        break;
    case LK_PATTERN1:
        set_medal_chain_gauge_stop(false);
        self->hp = LUCIKO_PHASE1_HP;
        self->as.enemy.damage_modifier = 1.0f;
        setup_emitter(&emitters[0], &luciko_pat1_spiral_emitter);
        setup_emitter(&emitters[1], &luciko_pat1_arc_emitter);
        self->time_alive = 0;
        boss_timer = LUCIKO_PHASE1_TIME;
        break;
    case LK_PATTERN2:
        set_medal_chain_gauge_stop(false);
        self->hp = LUCIKO_PHASE2_HP;
        self->as.enemy.damage_modifier = 1.0f;
        self->time_alive = 0;
        boss_timer = LUCIKO_PHASE2_TIME;
        show_boss_hp_bar(entity_handle_from_ptr(self), LUCIKO_PHASE2_HP, 3,
                         &boss_timer);
        break;
    case LK_PATTERN3:
        set_medal_chain_gauge_stop(false);
        self->hp = LUCIKO_PHASE3_HP;
        self->as.enemy.damage_modifier = 1.0f;
        self->time_alive = 0;
        boss_timer = LUCIKO_PHASE3_TIME;
        show_boss_hp_bar(entity_handle_from_ptr(self), LUCIKO_PHASE3_HP, 2,
                         &boss_timer);
        break;
    case LK_PATTERN4:
        set_medal_chain_gauge_stop(false);
        self->hp = LUCIKO_PHASE4_HP;
        self->as.enemy.damage_modifier = 1.0f;
        self->time_alive = 0;
        boss_timer = LUCIKO_PHASE4_TIME;
        show_boss_hp_bar(entity_handle_from_ptr(self), LUCIKO_PHASE4_HP, 1,
                         &boss_timer);
        break;
    case LK_DYING:
        self->collision = Vector2Zero();
        self->as.enemy.damage_modifier = 0;
        hide_boss_hp_bar();
        set_medal_chain_gauge_stop(true);
        set_medal_chain_boss_behaviour(false);
        break;
    default:
        log_error("Unknown Luciko pattern.");
    }
    self->as.enemy.luciko_data.phase = phase;
}

void init_luciko(Entity *self) {
    assert(self->as.enemy.enemy_type == ENEMY_LUCIKO);
    self->hp = LUCIKO_PHASE1_HP;
    set_medal_chain_gauge_stop(true);
    set_medal_chain_boss_behaviour(true);
    self->collision = (Vector2){30.0f, 48.0f};
    EnemyData *data = &self->as.enemy;
    data->luciko_data.phase = LK_MOVE;
    data->luciko_data.move_location = LUCIKO_HOME_POSITION;
    data->luciko_data.switch_to = LK_PATTERN1;
    data->damage_modifier = 0;
    show_boss_hp_bar(entity_handle_from_ptr(self), LUCIKO_PHASE1_HP, 4,
                     &boss_timer);
}

static void proceed_luciko(Entity *self) {
    EnemyData *data = &self->as.enemy;
    if (data->luciko_data.phase == LK_PATTERN4) {
        enter_luciko_phase(self, LK_DYING);
        return;
    }
    data->luciko_data.move_location = LUCIKO_HOME_POSITION;
    LucikoPhase previous_phase = data->luciko_data.phase;
    data->luciko_data.switch_to = previous_phase + 1;
    enter_luciko_phase(self, LK_MOVE);
}

void die_luciko(Entity *self) {
    self->hp = 0.0001f;
    EnemyData *data = &self->as.enemy;
    if (data->luciko_data.phase != LK_MOVE) {
        cancel_bullets(true, false);
    }
    proceed_luciko(self);
}

static f32 funny_fourier(f32 t) {
    return 120.0f +
           70.0f * (((4.0f / PI) * sinf(2 * PI * t)) +
                    ((4.0f / PI) * (1 / 3.0f) * sinf(6 * PI * t)));
}

void process_luciko(Entity *self, f32 delta) {
    EnemyData *data = &self->as.enemy;
    switch (data->luciko_data.phase) {
    case LK_MOVE:
        self->position = Vector2Lerp(
            self->position, data->luciko_data.move_location, 6.0f * delta);
        if (Vector2LengthSqr(Vector2Subtract(
                data->luciko_data.move_location, self->position)) < 0.001f)
            enter_luciko_phase(self, data->luciko_data.switch_to);
        break;
    case LK_PATTERN1:
        self->position =
            (Vector2){funny_fourier(self->time_alive * 0.15f),
                      LUCIKO_HOME_POSITION.y +
                          (sinf(self->time_alive * 2.4f) * 7.0f)};
        break;
    case LK_PATTERN2:
        break;
    case LK_PATTERN3:
        break;
    case LK_PATTERN4:
        break;
    case LK_DYING: // TODO: destroy luciko after dying animation
        break;
    default:
        log_warning("Unknown Luciko pattern.");
        assert(false);
    }
    if (get_player_life() == 0)
        return;
    if (data->luciko_data.phase != LK_MOVE &&
        data->luciko_data.phase != LK_DYING) {
        boss_timer -= delta;
        if (boss_timer <= 0) {
            boss_timer = 0;
            cancel_bullets(false, true);
            proceed_luciko(self);
        }
    }
}

// TODO: debug only, remove
static char *phase_text(LucikoPhase phase) {
    switch (phase) {
    case LK_MOVE:
        return "MOVE";
    case LK_PATTERN1:
        return "PATTERN1";
    case LK_PATTERN2:
        return "PATTERN2";
    case LK_PATTERN3:
        return "PATTERN3";
    case LK_PATTERN4:
        return "PATTERN4";
    case LK_DYING:
        return "DYING";
    default:
        return "UNKNOWN";
    }
}

void draw_luciko(Entity *self) {
    DrawRectangle(self->position.x - 12, self->position.y - 24, 24, 48,
                  RED);
    draw_outlined_text_ex(
        TextFormat("%s", phase_text(self->as.enemy.luciko_data.phase)),
        assets.fonts.fusion, self->position, assets.fonts.fusion.baseSize,
        0, WHITE, BLACK, 1);
}

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
    return Vector2Add(get_entity_world_position(enemy),
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
    assert(enemy);
    if (!enemy) {
        log_warning("Could not spawn enemy!");
        return;
    }
    enemy->as.enemy = (EnemyData){};
    enemy->as.enemy.enemy_type = type;
    enemy->as.enemy.damage_modifier = 1.0f;
    switch (type) {
    case ENEMY_TEST_ENEMY:
        init_test_enemy(enemy);
        break;
    case ENEMY_LUCIKO:
        init_luciko(enemy);
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

void stop_emitter(EmitterLive *emitter) { emitter->enabled = false; }

void process_enemy(Entity *self, f32 delta) {
    EnemyData *data = &self->as.enemy;
    switch (data->enemy_type) {
    case ENEMY_TEST_ENEMY:
        process_test_enemy(self, delta);
        break;
    case ENEMY_LUCIKO:
        process_luciko(self, delta);
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
    case ENEMY_LUCIKO:
        draw_luciko(self);
    default:
    }
    if (self->as.enemy.sealed)
        draw_centred_texture_ex(assets.textures.seal, self->position, 0,
                                1.0f, Fade(WHITE, 0.6f));
}

void hit_enemy(Entity *self, Entity *other) {
    switch (other->type) {
    case ENTITY_BULLET:
        if (other->as.bullet.config.flags & BULLETFLAG_PLAYER)
            entity_damage(self, PLAYER_BULLET_DAMAGE);
        break;
    case ENTITY_BOMB:
        // sure, whatever. actually pass in the delta if things get weird
        f32 delta = 1.0f / (f32)options.fps_option;
        entity_damage(self, BOMB_DAMAGE * delta);
        break;
    default:
    }
}

void die_enemy(Entity *self) {
    switch (self->as.enemy.enemy_type) {
    case ENEMY_LUCIKO:
        die_luciko(self);
        break;
    default:
        // TODO: standard enemy explosion
        self->queue_destroy = true;
    }
}

void damage_enemy(Entity *self, f32 amount) {
    self->hp -= amount * self->as.enemy.damage_modifier;
    if (self->hp <= 0)
        die_enemy(self);
}
