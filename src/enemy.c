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
#include <stdio.h>

f32 boss_timer = 0;

// TODO: add more enemies lol

// -- Impdrop --
// Standard zako that floats in and fires a few aimed shots.

// --

// -- Canister --
// Canisters explode into bullets on death.
// Has two behaviour modes. Standard canisters merely fly across the
// screen. Luciko canisters fly and explode after a short time. If a
// canister is destroyed by the player, it also spawns medals.

static const BulletConfig canister_bullet = {
    .initial_speed = 80.0f,
    .initial_ttl = 8.0f,
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 7,
};

static const PatternConfig canister_pattern = {
    .pattern_type = BP_RING,
    .bullet_config = &canister_bullet,
    .bullets_in_pattern = 12,
    .angle_randomisation = 20.0f,
    .speed_randomisation = 45.0f,
};

#define LUCIKO_CANISTER_BLOWUP_TIME 0.5f
#define CANISTER_GRAVITY 280.0f

static void process_canister(Entity *self, f32 delta) {
    move(&self->position, self->velocity, delta);
    move(&self->velocity, Vector2Scale(VECTOR2DOWN, CANISTER_GRAVITY),
         delta);
    if (self->as.enemy.canister_data.luciko_canister &&
        self->time_alive > LUCIKO_CANISTER_BLOWUP_TIME) {
        self->as.enemy.canister_data.died_from_timeout = true;
        self->hp = 0;
    }
    if (self->position.y > VIEWPORT_HEIGHT + 5.0f)
        self->queue_destroy = true;
}

static void init_canister(Entity *self) {
    self->hp = 10.0f;
    self->collision = (Vector2){15.0f, 15.0f};
}

static void draw_canister(Entity *self) {
    DrawCircle(self->position.x, self->position.y, 4.0f, YELLOW);
}

static void die_canister(Entity *self) {
    fire_pattern(self->position, 0, &canister_pattern);
    if (self->as.enemy.canister_data.died_from_timeout)
        return;
    spawn_medals(self->position, 6);
    add_score(600);
}

// -- Luciko's Options

#define LUCIKO_OPTION_TIME_TO_POSITION 0.3f
#define LUCIKO_OPTION_GRAVITY (Vector2){0, -2840.0f}
#define LUCIKO_OPTION_WAKE_UP_TIME 2.5f
#define LUCIKO_OPTION_SEAL_RADIUS 80.0f

static const BulletConfig luciko_pattern3_field_bullet = (BulletConfig){
    .initial_speed = 350.0f,
    .initial_ttl = 5.0f,
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 2,
    .flags = BULLETFLAG_ROTATE_TEXTURE,
};

static const BulletConfig luciko_pattern3_aimed_bullet = (BulletConfig){
    .initial_speed = 170.0f,
    .initial_ttl = 5.0f,
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 0,
};

static const PatternConfig luciko_pattern3_field_pattern = (PatternConfig){
    .pattern_type = BP_ARC,
    .bullets_in_pattern = 7,
    .pattern_length = 45.0f,
    .bullet_config = &luciko_pattern3_field_bullet,
};

static const EmitterConfig luciko_pattern3_field_emitter_left =
    (EmitterConfig){
        .volley_rate = 25.0f,
        .start_rotation = 40.0f,
        .rotation_type = ROT_TOWARDS_PLAYER,
        .rotation_range = 360.0f,
        .time_until_start = LUCIKO_OPTION_TIME_TO_POSITION + 0.6f,
        .pattern = &luciko_pattern3_field_pattern,
    };

static const EmitterConfig luciko_pattern3_field_emitter_right =
    (EmitterConfig){
        .volley_rate = 25.0f,
        .start_rotation = -40.0f,
        .rotation_type = ROT_TOWARDS_PLAYER,
        .rotation_range = 360.0f,
        .time_until_start = LUCIKO_OPTION_TIME_TO_POSITION + 0.6f,
        .pattern = &luciko_pattern3_field_pattern,
    };

static const PatternConfig luciko_pattern3_aimed_pattern = (PatternConfig){
    .pattern_type = BP_ONE,
    .flags = PATTERNFLAG_HAS_BURST,
    .burst_data =
        {
            .number_of_shots = 5,
            .total_time = 0.15f,
            .end_bullets_in_pattern = 1,
        },
    .bullet_config = &luciko_pattern3_aimed_bullet,
};

static const EmitterConfig luciko_pattern3_aimed_emitter_left =
    (EmitterConfig){
        .number_of_volleys = -1,
        .time_until_start = LUCIKO_OPTION_WAKE_UP_TIME,
        .volley_rate = 1.0f,
        .rotation_range = 360.0f,
        .rotation_type = ROT_TOWARDS_PLAYER,
        .pattern = &luciko_pattern3_aimed_pattern,
    };

static const EmitterConfig luciko_pattern3_aimed_emitter_right =
    (EmitterConfig){
        .number_of_volleys = -1,
        .time_until_start = LUCIKO_OPTION_WAKE_UP_TIME + 0.5f,
        .volley_rate = 1.0f,
        .rotation_range = 360.0f,
        .rotation_type = ROT_TOWARDS_PLAYER,
        .pattern = &luciko_pattern3_aimed_pattern,
    };

static void init_luciko_option(Entity *self) {
    self->collision = (Vector2){18.0f, 18.0f};
    self->hp = 240.0f;
}

static void setup_luciko_option(Entity *self) {
    LucikoOptionType type = self->as.enemy.luciko_option_data.type;
    EmitterLive *emitter = &self->as.enemy.current_emitters[0];
    switch (type) {
    case LKO_AIMED_LEFT:
        setup_emitter(emitter, &luciko_pattern3_aimed_emitter_left);
        self->velocity = (Vector2){-255.0f, 420.0f};
        self->as.enemy.seal_circle_radius = LUCIKO_OPTION_SEAL_RADIUS;
        break;
    case LKO_AIMED_RIGHT:
        setup_emitter(emitter, &luciko_pattern3_aimed_emitter_right);
        self->velocity = (Vector2){255.0f, 420.0f};
        self->as.enemy.seal_circle_radius = LUCIKO_OPTION_SEAL_RADIUS;
        break;
    case LKO_LEFT_FIELD:
        self->velocity = (Vector2){-140.0f, 350.0f};
        setup_emitter(emitter, &luciko_pattern3_field_emitter_left);
        break;
    case LKO_RIGHT_FIELD:
        self->velocity = (Vector2){140.0f, 350.0f};
        setup_emitter(emitter, &luciko_pattern3_field_emitter_right);
        break;
    default:
    }
}

static void process_luciko_option(Entity *self, f32 delta) {
    Entity *luciko = get_entity(self->parent, ENTITY_ENEMY);
    if (!luciko || !(luciko->as.enemy.enemy_type == ENEMY_LUCIKO) ||
        !(luciko->as.enemy.luciko_data.phase == LK_PATTERN3))
        self->queue_destroy = true;

    self->as.enemy.damage_modifier =
        self->time_alive > LUCIKO_OPTION_WAKE_UP_TIME ? 1.0f : 0.15f;

    if (self->time_alive > LUCIKO_OPTION_TIME_TO_POSITION)
        return;
    move_entity(self, delta);
    move(&self->velocity, LUCIKO_OPTION_GRAVITY, delta);
}

static void draw_luciko_option(Entity *self) {
    Vector2 pos = get_entity_world_position(self);
    DrawCircle(pos.x, pos.y, 7.0f, COLOUR_RASPBERRY);
}

#define LUCIKO_OPTION_KILL_DAMAGE 120.0f

static void die_luciko_option(Entity *self) {
    Entity *luciko = get_entity(self->parent, ENTITY_ENEMY);
    if (luciko)
        damage_enemy(luciko, LUCIKO_OPTION_KILL_DAMAGE);

    spawn_medals(get_entity_world_position(self), 32);
    add_score(3200);
}

// -- Luciko --

static const BulletConfig luciko_pat1_spiral_bullet = (BulletConfig){
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 0,
    .initial_speed = 200.0f,
    .min_speed = 100.0f,
    .acceleration = -280.0f,
    .initial_ttl = 5.0f,
    .flags = BULLETFLAG_HAS_MIN_SPEED,
};

static const PatternConfig luciko_pat1_spiral = (PatternConfig){
    .pattern_type = BP_RING,
    .bullets_in_pattern = 2,
    .bullet_config = &luciko_pat1_spiral_bullet,
    .spawn_offset = 4.0f,
    .trajectory = TRJ_DEFAULT,
};

static const EmitterConfig luciko_pat1_spiral_emitter = (EmitterConfig){
    .number_of_volleys = -1,
    .pattern = &luciko_pat1_spiral,
    .rotation_range = 360.0f,
    .rotation_speed = 750.0f,
    .rotation_type = ROT_CONTINUOUS,
    .start_rotation = 0.0f,
    .volley_rate = 60.0f,
    .time_until_start = 0.5f,
};

static const BulletConfig luciko_pat1_arc_bullet = (BulletConfig){
    .bullet_texture = &assets.textures.bullets1,
    .flags = BULLETFLAG_BIG | BULLETFLAG_HIGH_PRIORITY,
    .texture_row = 1,
    .initial_speed = 130.0f,
    .initial_ttl = 5.0f,
};

static const PatternConfig luciko_pat1_arc = (PatternConfig){
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

static const EmitterConfig luciko_pat1_arc_emitter = (EmitterConfig){
    .pattern = &luciko_pat1_arc,
    .rotation_range = 360.0f,
    .rotation_type = ROT_TOWARDS_PLAYER,
    .number_of_volleys = -1,
    .time_until_start = 0.83333f,
    .volley_rate = 0.38f,
};

static const BulletConfig luciko_pattern3_random_bullet = (BulletConfig){
    .initial_speed = 80.0f,
    .initial_ttl = 5.0f,
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 6,
    .flags = BULLETFLAG_ROTATE_TEXTURE,
};

static const PatternConfig luciko_pattern3_random_pattern =
    (PatternConfig){
        .pattern_type = BP_ONE,
        .angle_randomisation = 70.0f,
        .flags = PATTERNFLAG_HAS_BURST,
        .bullet_config = &luciko_pattern3_random_bullet,
        .burst_data =
            {
                .number_of_shots = 30,
                .total_time = 0.6f,
                .end_bullets_in_pattern = 1,
            },
    };

static const EmitterConfig luciko_pattern3_random_emitter =
    (EmitterConfig){
        .number_of_volleys = -1,
        .time_until_start = 1.5f,
        .volley_rate = 1.8f,
        .start_rotation = 90.0f,
        .rotation_type = ROT_NONE,
        .pattern = &luciko_pattern3_random_pattern,
        .local_position = (Vector2){0, -5.0f},
    };

#define LUCIKO_HOME_POSITION (Vector2){VIEWPORT_WIDTH / 2.0f, 60.0f}
#define LUCIKO_PHASE1_HP 800.0f
#define LUCIKO_PHASE2_HP 780.0f
#define LUCIKO_PHASE3_HP 1350.0f
#define LUCIKO_PHASE4_HP 780.0f
#define LUCIKO_PHASE1_TIME 26.5f
#define LUCIKO_PHASE2_TIME 22.0f
#define LUCIKO_PHASE3_TIME 30.0f
#define LUCIKO_PHASE4_TIME 22.0f

static void enter_luciko_phase(Entity *self, LucikoPhase phase) {
    set_medal_chain_boss_behaviour(true);
    EmitterLive *emitters = self->as.enemy.current_emitters;
    for (i32 i = 0; i < MAX_EMITTERS; i++) {
        stop_emitter(&emitters[i]);
    }
    self->time_alive = 0;
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
        boss_timer = LUCIKO_PHASE1_TIME;
        break;
    case LK_PATTERN2:
        set_medal_chain_gauge_stop(false);
        self->hp = LUCIKO_PHASE2_HP;
        self->as.enemy.damage_modifier = 1.0f;
        boss_timer = LUCIKO_PHASE2_TIME;
        show_boss_hp_bar(entity_handle_from_ptr(self), LUCIKO_PHASE2_HP, 3,
                         &boss_timer);
        break;
    case LK_PATTERN3:
        set_medal_chain_gauge_stop(false);
        self->hp = LUCIKO_PHASE3_HP;
        boss_timer = LUCIKO_PHASE3_TIME;
        setup_emitter(&emitters[0], &luciko_pattern3_random_emitter);
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

#define LUCIKO_CANISTER_SPAWN_DELAY 0.3f
#define LUCIKO_CANISTER_GROUP_COUNT 6

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
    data->luciko_data.canisters_to_spawn = LUCIKO_CANISTER_GROUP_COUNT;
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
    self->is_alive = true;
}

static f32 funny_fourier(f32 t) {
    return 120.0f +
           70.0f * (((4.0f / PI) * sinf(2 * PI * t)) +
                    ((4.0f / PI) * (1 / 3.0f) * sinf(6 * PI * t)));
}

#define LUCIKO_PATTERN3_GO_DOWN_END_TIME 1.0f
#define LUCIKO_PATTERN3_JUMP_BACK_UP_TIME 0.4f

// maybe?
/* workshop.enemy_position = (Vector2){ */
/*     120.0f + cosf(workshop.movement_time * 0.9f) * 22.0f + */
/*         sinf(workshop.movement_time * 1.7f) * 6.0f, */
/*     80.0f + sinf(workshop.movement_time * 1.2f) * 8.0f + */
/*         cosf(workshop.movement_time * 0.55f) * 3.0f}; */

static inline void process_luciko_pattern3(Entity *self, float delta) {
    self->as.enemy.damage_modifier =
        self->time_alive > LUCIKO_PATTERN3_GO_DOWN_END_TIME +
                               LUCIKO_PATTERN3_JUMP_BACK_UP_TIME
            ? 1.0f
            : 0.2f;
    if (self->time_alive < LUCIKO_PATTERN3_GO_DOWN_END_TIME) {
        f32 progress = smootherstep(0, LUCIKO_PATTERN3_GO_DOWN_END_TIME,
                                    self->time_alive);
        self->position = (Vector2){
            LUCIKO_HOME_POSITION.x,
            LUCIKO_HOME_POSITION.y + (36.0f * progress),
        };
    } else if (self->time_alive >= LUCIKO_PATTERN3_GO_DOWN_END_TIME &&
               self->time_alive < LUCIKO_PATTERN3_GO_DOWN_END_TIME +
                                      LUCIKO_PATTERN3_JUMP_BACK_UP_TIME) {
        // go back up
    } else {
        Vector2 additional = (Vector2){
            9.0f * sinf(2.2f * self->time_alive),
            3.0f * sinf(3.5f * self->time_alive),
        };
        self->position = Vector2Add(LUCIKO_HOME_POSITION, additional);
    }

    if (self->time_alive > LUCIKO_PATTERN3_GO_DOWN_END_TIME &&
        !self->as.enemy.luciko_data.has_spawned_options) {
        self->as.enemy.luciko_data.has_spawned_options = true;

        Entity *left_aimed_option =
            spawn_enemy(ENEMY_LUCIKO_OPTION, self->position);
        left_aimed_option->as.enemy.luciko_option_data.type =
            LKO_AIMED_LEFT;
        setup_luciko_option(left_aimed_option);
        add_child(self, left_aimed_option);

        Entity *right_aimed_option =
            spawn_enemy(ENEMY_LUCIKO_OPTION, self->position);
        right_aimed_option->as.enemy.luciko_option_data.type =
            LKO_AIMED_RIGHT;
        setup_luciko_option(right_aimed_option);
        add_child(self, right_aimed_option);

        Entity *left_field_option =
            spawn_enemy(ENEMY_LUCIKO_OPTION, self->position);
        left_field_option->as.enemy.luciko_option_data.type =
            LKO_LEFT_FIELD;
        setup_luciko_option(left_field_option);
        add_child(self, left_field_option);

        Entity *right_field_option =
            spawn_enemy(ENEMY_LUCIKO_OPTION, self->position);
        right_field_option->as.enemy.luciko_option_data.type =
            LKO_RIGHT_FIELD;
        setup_luciko_option(right_field_option);
        add_child(self, right_field_option);
    }
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
        cancel_bullets(false, true);
        break;
    case LK_PATTERN1:
        self->position =
            (Vector2){funny_fourier(self->time_alive * 0.15f),
                      LUCIKO_HOME_POSITION.y +
                          (sinf(self->time_alive * 2.4f) * 7.0f)};
        break;
    case LK_PATTERN2:
        // Replace this with something better
        self->position =
            (Vector2){120.0f + cosf(self->time_alive * 0.9f) * 22.0f +
                          sinf(self->time_alive * 1.7f) * 6.0f,
                      80.0f + sinf(self->time_alive * 1.2f) * 8.0f +
                          cosf(self->time_alive * 0.55f) * 3.0f};
        u8 *canisters_to_spawn = &data->luciko_data.canisters_to_spawn;
        f32 *canister_spawn_delay =
            &data->luciko_data.canister_spawn_delay;
        *canister_spawn_delay -= delta;
        if (*canister_spawn_delay <= 0) {
            *canister_spawn_delay += LUCIKO_CANISTER_SPAWN_DELAY;
            (*canisters_to_spawn)--;
            if (*canisters_to_spawn == 0) {
                *canisters_to_spawn = LUCIKO_CANISTER_GROUP_COUNT;
                *canister_spawn_delay +=
                    LUCIKO_CANISTER_SPAWN_DELAY * 5.0f;
            }
            Entity *canister = spawn_enemy(ENEMY_CANISTER, self->position);
            assert(canister);
            if (!canister) {
                log_warning("Canister was not spawned!");
                return;
            }
            canister->velocity = VEC2FROMANGLE(
                DEG2RAD * (210.0f + (120.0f * random_float())), 200.0f);
            canister->as.enemy.canister_data.luciko_canister = true;
        }

        break;
    case LK_PATTERN3:
        process_luciko_pattern3(self, delta);
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
            halve_medal_chain();
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
                  COLOUR_RASPBERRY);
    draw_outlined_text_ex(
        TextFormat("%s", phase_text(self->as.enemy.luciko_data.phase)),
        assets.fonts.fusion, self->position, assets.fonts.fusion.baseSize,
        0, WHITE, BLACK, 1);
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

static f32 normalise_rotation(f32 rotation, f32 start, f32 range) {
    if (range <= 0)
        return start;
    float relative = fmodf(rotation - start, range);
    if (relative < 0)
        relative += range;
    return start + relative;
}

static void update_emitter_rotation(Entity *enemy, EmitterLive *emitter,
                                    float delta) {
    assert(emitter->config && "No emitter pattern assigned.");
    bool burst_active = emitter->burst.active;
    bool burst_locks_rotation =
        burst_active && (emitter->config->pattern->flags &
                         PATTERNFLAG_LOCK_EMITTER_ROTATION);
    if (burst_locks_rotation)
        return;

    switch (emitter->config->rotation_type) {
    case ROT_TOWARDS_PLAYER:
        emitter->current_rotation = front_towards_whatever(
            emitter_world_pos(enemy, emitter),
            burst_active ? emitter->burst.player_position_on_burst_start
                         : player_position());
        if (emitter->config->start_rotation != 0)
            emitter->current_rotation += emitter->config->start_rotation;
        break;
    case ROT_CONTINUOUS:
        emitter->current_rotation +=
            emitter->config->rotation_speed * delta;
        emitter->current_rotation = normalise_rotation(
            emitter->current_rotation, emitter->config->start_rotation,
            emitter->config->rotation_range);
        log_info("%f", emitter->current_rotation);
        break;
    case ROT_BOUNCE:
        f32 start = emitter->config->start_rotation;
        f32 end = start + emitter->config->rotation_range;
        emitter->current_rotation +=
            fabsf(emitter->config->rotation_speed) *
            emitter->bounce_rotation_direction * delta;
        if (emitter->current_rotation > end) {
            emitter->current_rotation = end;
            emitter->bounce_rotation_direction = -1.0f;
        } else if (emitter->current_rotation < start) {
            emitter->current_rotation = start;
            emitter->bounce_rotation_direction = 1.0f;
        }
        break;
    case ROT_RANDOMISE:
        emitter->current_rotation =
            (random_float() * emitter->config->rotation_range) +
            emitter->config->start_rotation;
        break;
    case ROT_NONE:
        emitter->current_rotation = emitter->config->start_rotation;
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

        // TODO: rotation interpolation between volleys so there's no
        // jitter
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

Entity *spawn_enemy(EnemyType type, Vector2 at) {
    Entity *enemy = spawn_entity(ENTITY_ENEMY);
    assert(enemy);
    if (!enemy) {
        log_warning("Could not spawn enemy!");
        return NULL;
    }
    enemy->as.enemy = (EnemyData){};
    enemy->as.enemy.enemy_type = type;
    enemy->as.enemy.damage_modifier = 1.0f;
    switch (type) {
    case ENEMY_LUCIKO:
        init_luciko(enemy);
        break;
    case ENEMY_CANISTER:
        init_canister(enemy);
        break;
    case ENEMY_LUCIKO_OPTION:
        init_luciko_option(enemy);
    default:
    }
    enemy->position = at;
    return enemy;
}

void setup_emitter(EmitterLive *emitter, const EmitterConfig *config) {
    *emitter = (EmitterLive){};
    if (config == NULL)
        return;
    emitter->config = config;
    emitter->volleys_left =
        config->number_of_volleys > 0 ? config->number_of_volleys : -1;
    emitter->enabled = true;
    emitter->cooldown_between_volleys =
        emitter->config->time_until_start
            ? emitter->config->time_until_start
            : get_volley_cooldown(emitter);
    emitter->current_rotation = emitter->config->start_rotation;
    emitter->bounce_rotation_direction = 1.0f;
}

void stop_emitter(EmitterLive *emitter) { emitter->enabled = false; }

void process_enemy(Entity *self, f32 delta) {
    EnemyData *data = &self->as.enemy;
    switch (data->enemy_type) {
    case ENEMY_LUCIKO:
        process_luciko(self, delta);
        break;
    case ENEMY_CANISTER:
        process_canister(self, delta);
        break;
    case ENEMY_LUCIKO_OPTION:
        process_luciko_option(self, delta);
        break;
    default:
    }
    Vector2 pos = get_entity_world_position(self);
    data->sealed =
        data->seal_circle_radius > 0 &&
        Vector2LengthSqr(Vector2Subtract(player_position(), pos)) <=
            data->seal_circle_radius * data->seal_circle_radius;
    process_emitters(self, delta);
}

void draw_enemy(Entity *self, f32 delta) {
    switch (self->as.enemy.enemy_type) {
    case ENEMY_LUCIKO:
        draw_luciko(self);
        break;
    case ENEMY_CANISTER:
        draw_canister(self);
        break;
    case ENEMY_LUCIKO_OPTION:
        draw_luciko_option(self);
        break;
    default:
    }
    Vector2 pos = get_entity_world_position(self);
    if (self->as.enemy.sealed)
        draw_centred_texture_ex(assets.textures.seal, pos, 0, 1.0f,
                                Fade(WHITE, 0.6f));
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

static void enemy_standard_death(Entity *self) {
    // TODO: standard enemy explosion
    self->queue_destroy = true;
}

void die_enemy(Entity *self) {
    switch (self->as.enemy.enemy_type) {
    case ENEMY_LUCIKO:
        die_luciko(self);
        break;
    case ENEMY_CANISTER:
        die_canister(self);
        enemy_standard_death(self);
        break;
    case ENEMY_LUCIKO_OPTION:
        die_luciko_option(self);
        enemy_standard_death(self);
        break;
    default:
        enemy_standard_death(self);
    }
}

void damage_enemy(Entity *self, f32 amount) {
    self->hp -= amount * self->as.enemy.damage_modifier;
}
