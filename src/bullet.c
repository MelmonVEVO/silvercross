#include "bullet.h"
#include "assets.h"
#include "entity.h"
#include "particle.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "utils.h"
#include <assert.h>

static const ParticleConfig CANCELLED_BULLET_PARTICLE = (ParticleConfig){
    .static_texture_atlas = &assets.textures.star,
    .static_texture_rows = 1,
    .static_texture_columns = 1,
    .static_texture_pick_to = 0,
    .static_texture_pick_from = 0,
    .initial_scale = 1.0f,
    .end_scale = 0.2f,
    .initial_texture_rotational_randomness = 360.0f,
    .texture_rotation_speed = 50.0f,
    /* .texture_rotation_speed_randomness = 50.0f, */
    .base_lifetime = 0.5f,
    .lifetime_randomness_seconds = 0.2f,
    .flags = PARTICLEFLAG_HIGH_DRAW_PRIORITY,
};

void process_bullet(Entity *self, f32 delta) {
    move(&self->position, self->velocity, delta);
    accelerate(&self->velocity, self->as.bullet.acceleration, delta);
    move(&self->velocity, self->as.bullet.config.gravity, delta);

    self->velocity = Vector2Rotate(
        self->velocity,
        self->as.bullet.config.angular_velocity * DEG2RAD * delta);
    self->as.bullet.ttl -= delta;
    if ((self->as.bullet.ttl <= 0) ||
        (self->position.y < -10.0f &&
         self->as.bullet.config.flags & BULLETFLAG_PLAYER)) {
        spawn_particle(&CANCELLED_BULLET_PARTICLE, self->position,
                       self->velocity, WHITE);
        destroy_entity_ptr(self);
    }
}

void draw_bullet(Entity *self, f32 delta) {
    float rotation =
        (self->as.bullet.config.flags & BULLETFLAG_ROTATE_TEXTURE)
            ? atan2f(self->velocity.y, self->velocity.x) * RAD2DEG
            : 0;
    Colour tint = self->as.bullet.config.flags & BULLETFLAG_PLAYER
                      ? Fade(WHITE, 0.8f)
                      : WHITE;
    draw_animated_texture_ex(&self->as.bullet.texture_instance, delta,
                             self->position, rotation, 1, tint);
}

static void fire(Vector2 initial_position, f32 direction,
                 const BulletConfig *config) {
    assert(config);
    if (!config)
        log_error("Config not passed in to spawn a bullet!");
    Entity *bullet = spawn_entity(ENTITY_BULLET);
    if (!bullet) {
        log_warning("Exhausted pool while trying to spawn a bullet.");
        return;
    }
    bullet->as.bullet = (BulletData){};
    BulletData *data = &bullet->as.bullet;
    data->config = *config;
    bullet->position = initial_position;
    bullet->velocity =
        VEC2FROMANGLE(direction, bullet->as.bullet.config.initial_speed);
    bullet->hp = 1;
    bullet->collision = data->config.flags & BULLETFLAG_BIG
                            ? (Vector2){4, 4}
                            : (Vector2){2, 2};
    data->texture_instance.texture = data->config.bullet_texture;
    data->texture_instance.animation_time =
        random_float() * total_animation_time(data->config.bullet_texture);
    data->texture_instance.row = data->config.texture_row;
    data->ttl = data->config.initial_ttl;
}

static const ParticleConfig player_bullet_impact_particle =
    (ParticleConfig){
        .base_lifetime = 5.0f,
        .flags = PARTICLEFLAG_DESPAWN_AFTER_ANIMATING |
                 PARTICLEFLAG_HIGH_DRAW_PRIORITY,
        .animated_texture = &assets.textures.playershot_impact,
        .initial_scale = 1.0f,
        .end_scale = 1.0f,
    };

static const ParticleConfig player_bullet_impact_small_particle =
    (ParticleConfig){
        .flags = PARTICLEFLAG_HIGH_DRAW_PRIORITY,
        .base_lifetime = 0.3f,
        .lifetime_randomness_seconds = 0.3f,
        .initial_scale = 1.0f,
    };

void hit_bullet(Entity *self, Entity *other) {
    if ((other->type == ENTITY_PLAYER &&
         self->as.bullet.config.flags & BULLETFLAG_PLAYER) ||
        (other->type == ENTITY_ENEMY &&
         !(self->as.bullet.config.flags & BULLETFLAG_PLAYER)) ||
        (other->type == ENTITY_BOMB &&
         self->as.bullet.config.flags & BULLETFLAG_PLAYER))
        return;
    if (self->as.bullet.config.flags & BULLETFLAG_PLAYER) {
        Vector2 particle_position = Vector2Add(
            self->position, (Vector2){-2.0f + (4.0f * random_float()),
                                      -2.0f + (4.0f * random_float())});
        spawn_particle(&player_bullet_impact_particle, particle_position,
                       Vector2Zero(), WHITE);
        burst_particles(&player_bullet_impact_small_particle,
                        particle_position, 70.0f, 0, 360.0f, 40, 32.0f,
                        10.0f, Fade(WHITE, 0.4f));
    } else if (other->type == ENTITY_BOMB) {
        spawn_particle(&CANCELLED_BULLET_PARTICLE, self->position,
                       self->velocity, WHITE);
    }
    destroy_entity_ptr(self);
}

static Vector2 get_bullet_start_position(Vector2 origin, float offset,
                                         float rotation) {
    Vector2 additional_vector = VEC2FROMANGLE(rotation, offset);
    return Vector2Add(origin, additional_vector);
}

void bullet_fire_one(Vector2 initial_position, float initial_angle,
                     const BulletConfig *config, float offset) {
    Vector2 start_position = get_bullet_start_position(
        initial_position, offset, DEG2RAD * initial_angle);
    fire(start_position, DEG2RAD * initial_angle, config);
}

void bullet_fire_arc(Vector2 initial_position, float direction,
                     const BulletConfig *config, int bullets_in_arc,
                     float arc_offset, float arc_angle,
                     Trajectory trajectory) {
    if (bullets_in_arc < 2) {
        log_warning(
            "Tried to fire an arc with less than 2 bullets. Please check "
            "the firing configurations!");
        return;
    }
    float angle_per_bullet =
        arc_get_angle_per_thing(bullets_in_arc, DEG2RAD * arc_angle);
    for (int i = 0; i < bullets_in_arc; i++) {
        float bullet_angle = 0;
        Vector2 start_position = get_bullet_start_position(
            initial_position, arc_offset,
            arc_get_thing_angle_for_i(angle_per_bullet, i,
                                      DEG2RAD * direction,
                                      DEG2RAD * arc_angle));
        switch (trajectory) {
        case TRJ_DEFAULT:
            bullet_angle = arc_get_thing_angle_for_i(angle_per_bullet, i,
                                                     DEG2RAD * direction,
                                                     DEG2RAD * arc_angle);
            break;
        case TRJ_FIXED:
            bullet_angle = DEG2RAD * direction;
            break;
        case TRJ_AIMED:
            bullet_angle = DEG2RAD * front_towards_player(start_position);
            break;
        case TRJ_RANDOM:
            bullet_angle = TAU * random_float();
            break;
        default:
        }
        fire(start_position, bullet_angle, config);
    }
}

void bullet_fire_ring(Vector2 initial_position, float direction,
                      const BulletConfig *config, int bullets_in_ring,
                      float ring_offset, Trajectory trajectory) {
    f32 angle_per_bullet =
        RAD2DEG * ring_get_angle_per_thing(bullets_in_ring);
    bullet_fire_arc(initial_position, direction, config, bullets_in_ring,
                    ring_offset, 360.0f - angle_per_bullet, trajectory);
}

void fire_pattern(Vector2 position, float angle,
                  const PatternConfig *pattern) {
    assert(pattern);
    if (!pattern) {
        log_error("fire_pattern was called without a pattern.");
        return;
    }
    switch (pattern->pattern_type) {
    case (BP_ONE):
        bullet_fire_one(position, angle, pattern->bullet_config,
                        pattern->spawn_offset);
        break;
    case (BP_RING):
        bullet_fire_ring(position, angle, pattern->bullet_config,
                         pattern->bullets_in_pattern,
                         pattern->spawn_offset, pattern->trajectory);
        break;
    case (BP_ARC):
        bullet_fire_arc(position, angle, pattern->bullet_config,
                        pattern->bullets_in_pattern, pattern->spawn_offset,
                        pattern->pattern_length, pattern->trajectory);
        break;
    case (BP_CUSTOM):
        assert(pattern->custom_fire);
        pattern->custom_fire(position, angle, pattern->bullet_config,
                             pattern->spawn_offset);
        break;
    default:
        log_warning("Other unsupported bullet pattern was used in "
                    "fire_pattern_single_shot.");
    }
}

void fire_pattern_burst_shot(Vector2 position, float angle,
                             const PatternConfig *pattern,
                             float burst_progress) {
    assert(pattern);
    assert(pattern->bullet_config);
    if (!pattern) {
        log_error("fire_pattern_burst_shot was called without a pattern.");
        return;
    }

    PatternConfig modified_pattern = *pattern;
    BulletConfig modified_bullet = *pattern->bullet_config;

    if (pattern->burst_data.end_bullets_in_pattern > 0) {
        modified_pattern.bullets_in_pattern = (unsigned int)roundf(
            Lerp((float)pattern->bullets_in_pattern,
                 (float)pattern->burst_data.end_bullets_in_pattern,
                 burst_progress));
    }

    modified_bullet.initial_speed +=
        pattern->burst_data.end_bullet_speed_modifier * burst_progress;
    modified_pattern.bullet_config = &modified_bullet;
    modified_pattern.flags &= (unsigned short)~PATTERNFLAG_HAS_BURST;

    fire_pattern(position, angle, &modified_pattern);
}
