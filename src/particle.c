#include "particle.h"
#include "constants.h"
#include "raylib.h"
#include "raymath.h"
#include "utils.h"
#include <assert.h>
#include <stddef.h>

static ParticleLive particles[MAX_PARTICLES] = {0};
static ParticleLive *particle_free;

static ParticleLive *pop_particle(void) {
    ParticleLive *returned_particle = particle_free;
    if (!returned_particle)
        return NULL;
    particle_free = returned_particle->next_free;
    return returned_particle;
}

static void push_particle(ParticleLive *particle) {
    particle->next_free = particle_free;
    particle_free = particle;
}

void reset_particles(void) {
    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        ParticleLive *particle = &particles[i];
        if (particle->current_lifetime <= 0)
            continue;
        particle->current_lifetime = 0;
        push_particle(particle);
    }
}

static float particle_scale(const ParticleLive *particle) {
    const ParticleConfig *configuration = particle->configuration;
    float lifetime_percent = 0.0f;
    assert(particle->starting_lifetime > 0);
    lifetime_percent =
        1.0f - (particle->current_lifetime / particle->starting_lifetime);
    lifetime_percent = Clamp(lifetime_percent, 0.0f, 1.0f);
    return Lerp(configuration->initial_scale, configuration->end_scale,
                lifetime_percent);
}

void spawn_particle(const ParticleConfig *configuration, Vector2 at,
                    Vector2 velocity, Colour tint) {
    assert(configuration);
    ParticleLive *particle = pop_particle();
    if (!particle) {
        log_warning("No particles left, skipping.");
        return;
    }

    particle->configuration = configuration;
    particle->position = at;
    particle->velocity = velocity;
    float lifetime = configuration->base_lifetime +
                     (configuration->lifetime_randomness_seconds -
                      (configuration->lifetime_randomness_seconds * 2.0f *
                       random_float()));
    particle->starting_lifetime = lifetime;
    particle->current_lifetime = lifetime;
    if (configuration->animated_texture) {
        particle->texture_instance.texture =
            configuration->animated_texture;
        particle->texture_instance.animation_time = 0.0f;
        particle->texture_instance.row = 0;
    }
    if (configuration->static_texture_atlas &&
        configuration->static_texture_pick_to >=
            configuration->static_texture_pick_from) {
        const unsigned int pick_count =
            configuration->static_texture_pick_to -
            configuration->static_texture_pick_from + 1;
        particle->static_texture_current =
            configuration->static_texture_pick_from +
            (unsigned int)(random_float() * pick_count);
        if (particle->static_texture_current >
            configuration->static_texture_pick_to) {
            particle->static_texture_current =
                configuration->static_texture_pick_to;
        }
    } else {
        particle->static_texture_current = 0;
    }
    particle->texture_rotation =
        configuration->initial_rotation +
        (360.0f * configuration->initial_texture_rotational_randomness *
         random_float());
    particle->current_angular_velocity =
        configuration->initial_angular_velocity +
        (configuration->angular_velocity_randomness -
         (configuration->angular_velocity_randomness * 2.0f *
          random_float()));
    particle->tint = tint;
}

// This should be called in the draw loop, before drawing the particles.
void process_particles(float delta) {
    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        ParticleLive *particle = &particles[i];
        if (particle->current_lifetime <= 0)
            continue;
        const ParticleConfig *configuration = particle->configuration;

        particle->velocity = Vector2Rotate(
            particle->velocity,
            particle->current_angular_velocity * DEG2RAD * delta);
        accelerate(&particle->velocity, configuration->linear_acceleration,
                   delta);
        move(&particle->velocity, configuration->gravity, delta);
        move(&particle->position, particle->velocity, delta);
        particle->texture_rotation +=
            configuration->texture_rotation_speed * delta;
        if (configuration->flags & PARTICLEFLAG_FIX_ROTATION_BASED_ON_DIR)
            particle->texture_rotation =
                Vector2Angle(particle->velocity, VECTOR2RIGHT) * RAD2DEG;

        particle->current_lifetime -= delta;
        if (particle->current_lifetime <= 0) {
            push_particle(particle);
        }
    }
}

static int draw_animated_particle(ParticleLive *particle, float delta) {
    int looped = 0;
    bool not_flipped =
        !(particle->configuration->flags & PARTICLEFLAG_FLIP_X) &&
        !(particle->configuration->flags & PARTICLEFLAG_FLIP_Y);
    if (not_flipped) {
        looped = draw_animated_texture_ex(
            &particle->texture_instance, delta, particle->position,
            particle->texture_rotation, particle_scale(particle),
            particle->tint);
    } else {
        AnimatedTexture2DInstance *instance = &particle->texture_instance;
        assert(instance->texture->fps > 0);
        assert(instance->texture->frames > 0);
        if (instance->texture->fps == 0 ||
            instance->texture->frames == 0) {
            log_error("The texture identified with %u has a 0 frame or 0 "
                      "fps set. Check the texture configurations!",
                      instance->texture->texture_atlas.id);
            return 0;
        }
        const AnimatedTexture2D *texture = instance->texture;
        const float seconds_per_frame = 1.0f / texture->fps;
        const Vector2 frame_size = animated_texture_frame_size(texture);
        const int frame_to_draw =
            (int)floorf(instance->animation_time / seconds_per_frame);
        Rectangle src = (Rectangle){frame_to_draw * frame_size.x,
                                    frame_size.y * instance->row,
                                    frame_size.x, frame_size.y};

        if (particle->configuration->flags & PARTICLEFLAG_FLIP_X)
            src.width *= -1.0f;
        if (particle->configuration->flags & PARTICLEFLAG_FLIP_Y)
            src.height *= -1.0f;

        const float scale = particle_scale(particle);
        const Vector2 scaled_frame_size = Vector2Scale(frame_size, scale);
        Rectangle dest = (Rectangle){
            particle->position.x, particle->position.y,
            fabsf(scaled_frame_size.x), fabsf(scaled_frame_size.y)};
        Vector2 origin = (Vector2){dest.width * 0.5f, dest.height * 0.5f};

        DrawTexturePro(texture->texture_atlas, src, dest, origin,
                       particle->texture_rotation, particle->tint);
        float new_animation_time = fmodf(instance->animation_time + delta,
                                         total_animation_time(texture));
        looped = new_animation_time < instance->animation_time;
        instance->animation_time = new_animation_time;
    }
    if (looped && (particle->configuration->flags &
                   PARTICLEFLAG_DESPAWN_AFTER_ANIMATING)) {
        particle->current_lifetime = 0.0f;
        push_particle(particle);
    }
    return looped;
}

static void draw_static_particle(const ParticleLive *particle) {
    const ParticleConfig *configuration = particle->configuration;
    const Texture2D *atlas = configuration->static_texture_atlas;
    assert(atlas && configuration->static_texture_columns != 0 &&
           configuration->static_texture_rows != 0);
    if (!atlas || configuration->static_texture_columns == 0 ||
        configuration->static_texture_rows == 0) {
        return;
    }

    const float frame_width =
        (float)atlas->width / configuration->static_texture_columns;
    const float frame_height =
        (float)atlas->height / configuration->static_texture_rows;

    const unsigned int column = particle->static_texture_current %
                                configuration->static_texture_columns;
    const unsigned int row = particle->static_texture_current /
                             configuration->static_texture_columns;
    Rectangle src = (Rectangle){column * frame_width, row * frame_height,
                                frame_width, frame_height};
    float scale = particle_scale(particle);
    Rectangle target =
        (Rectangle){particle->position.x, particle->position.y,
                    frame_width * scale, frame_height * scale};
    Vector2 origin = {target.width / 2.0f, target.height / 2.0f};
    DrawTexturePro(*atlas, src, target, origin, particle->texture_rotation,
                   particle->tint);
}

static void draw_pixel_particle(const ParticleLive *particle) {
    float size = MAX(particle_scale(particle), 1.0f);
    Rectangle target = (Rectangle){particle->position.x,
                                   particle->position.y, size, size};
    DrawRectanglePro(target, (Vector2){size / 2.0f, size / 2.0f},
                     particle->texture_rotation, particle->tint);
}

void draw_particle(ParticleLive *particle, float delta) {
    const ParticleConfig *config = particle->configuration;
    assert(config);
    int blend_mode = BLEND_ALPHA;
    if (config->flags & PARTICLEFLAG_MULTIPLY)
        blend_mode = BLEND_MULTIPLIED;
    if (config->flags & PARTICLEFLAG_ADDITIVE)
        blend_mode = BLEND_ADDITIVE;
    BeginBlendMode(blend_mode);
    if (config->animated_texture) {
        draw_animated_particle(particle, delta);
    } else if (config->static_texture_atlas) {
        draw_static_particle(particle);
    } else {
        draw_pixel_particle(particle);
    }
    EndBlendMode();
}

void burst_particles(const ParticleConfig *configuration, Vector2 at,
                     float start_speed, float arc_start_angle,
                     float arc_end_angle, unsigned int count,
                     float speed_randomness, float spacing_randomness,
                     Colour tint) {
    float arc_angle =
        (DEG2RAD * arc_end_angle) - (DEG2RAD * arc_start_angle);
    float angle_per_particle = arc_get_angle_per_thing(count, arc_angle);
    for (unsigned int i = 0; i < count; i++) {
        float particle_angle =
            arc_get_thing_angle_for_i(angle_per_particle, i, 0,
                                      arc_angle) +
            spacing_randomness -
            (spacing_randomness * 2.0f * random_float());
        spawn_particle(
            configuration, at,
            VEC2FROMANGLE(particle_angle,
                          start_speed + speed_randomness -
                              (speed_randomness * 2.0f * random_float())),
            tint);
    }
}

void initialise_particle_pool(void) {
    particle_free = NULL;
    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        particles[i].next_free = particle_free;
        particle_free = &particles[i];
    }
}

void draw_particles(float delta) {
    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        ParticleLive *particle = &particles[i];
        if (particle->current_lifetime <= 0 || !particle->configuration ||
            particle->configuration->flags &
                PARTICLEFLAG_HIGH_DRAW_PRIORITY)
            continue;
        draw_particle(particle, delta);
    }
}

void draw_high_priority_particles(float delta) {
    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        ParticleLive *particle = &particles[i];
        if (particle->current_lifetime <= 0 || !particle->configuration ||
            !(particle->configuration->flags &
              PARTICLEFLAG_HIGH_DRAW_PRIORITY))
            continue;
        draw_particle(particle, delta);
    }
}
