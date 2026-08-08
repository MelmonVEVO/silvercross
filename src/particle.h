/*
 * particle
 *
 * Spawning and controlling of particles. Since
 * particles are purely visual, they are to be
 * run in draw calls and never use physics-related RNG.
 *
 * Copyright (c) 2026 MELMON PROJECT. All Rights Reserved.
 */
#ifndef PARTICLE_H
#define PARTICLE_H

#include "utils.h"

#define MAX_PARTICLES 4000

#define PARTICLEFLAG_DESPAWN_AFTER_ANIMATING (1u << 0)
#define PARTICLEFLAG_FIX_ROTATION_BASED_ON_DIR (1u << 1)
#define PARTICLEFLAG_ADDITIVE (1u << 2)
#define PARTICLEFLAG_MULTIPLY (1u << 3)
#define PARTICLEFLAG_HIGH_DRAW_PRIORITY (1u << 4)
#define PARTICLEFLAG_FLIP_X (1u << 5)
#define PARTICLEFLAG_FLIP_Y (1u << 6)

// The drawing of a particle texture follows:
// animated_texture != NULL, draw the animated texture, otherwise
// static_texture != NULL, draw the static texture (randomly picking from
// the sheet), otherwise draw a single pixel

typedef struct {
    const AnimatedTexture2D *const animated_texture;
    const Texture2D *const static_texture_atlas;
    // Splitting the texture atlas
    const unsigned int static_texture_rows;
    const unsigned int static_texture_columns;
    // Choosing what images to use from the atlas. Picked from top to
    // bottom, left to right.
    const unsigned int static_texture_pick_from;
    const unsigned int static_texture_pick_to;
    const unsigned char flags;
    const float base_lifetime;
    const float lifetime_randomness_seconds;
    const float initial_angular_velocity;
    const float initial_rotation;
    const float angular_velocity_randomness;
    const float linear_acceleration;
    const Vector2 gravity;
    const float initial_scale;
    const float end_scale;
    const float texture_rotation_speed;
    const float initial_texture_rotational_randomness;
    const float texture_rotation_speed_randomness;
} ParticleConfig;

typedef struct ParticleLive {
    struct ParticleLive *next_free;
    const ParticleConfig *configuration;
    AnimatedTexture2DInstance texture_instance;
    unsigned int static_texture_current;
    float starting_lifetime;
    float current_lifetime;
    float current_angular_velocity;
    Vector2 position;
    Vector2 velocity;
    float texture_rotation;
    Colour tint;
} ParticleLive;

void spawn_particle(const ParticleConfig *configuration, Vector2 at,
                    Vector2 velocity, Colour tint);

// Spawns a burst of a specific particle between circle_start_angle and
// circle_end_angle.
void burst_particles(const ParticleConfig *configuration, Vector2 at,
                     float start_speed, float arc_start_angle,
                     float arc_end_angle, unsigned int count,
                     float speed_randomness, float spacing_randomness,
                     Colour tint);
void process_particles(float delta);
void draw_particle(ParticleLive *particle, float delta);
void reset_particles(void);
void initialise_particle_pool(void);

#endif // PARTICLE_H
