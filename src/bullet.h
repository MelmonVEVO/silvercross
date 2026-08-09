#ifndef BULLET_H
#define BULLET_H

#include "entity.h"

// Trajectory controls the initial angle of bullets fired.
// Default fires the bullets in a usual manner.
// Fixed fires all bullets in one single direction.
// Aimed fires the bullets rotated towards the player.
// Random randomises the bullet trajectory.
// Always makes a bullet always fire in a specific angle.
typedef enum {
    TRJ_DEFAULT,
    TRJ_FIXED,
    TRJ_AIMED,
    TRJ_RANDOM,
} Trajectory;

/*
 * BP_ONE: Fires a single bullet.
 * BP_RING: Fires a ring of bullets.
 * BP_ARC: Fires bullets in a constrained arc.
 * BP_RANDOM_SPHERE: Fires bullets within a circle randomly.
 * BP_COMBINATION: Fires multiple bullet patterns at once.
 */
typedef enum {
    BP_ONE,
    BP_RING,
    BP_ARC,
} BulletPattern;

typedef struct PatternConfig {
    BulletPattern pattern_type;
    unsigned int bullets_in_pattern;
    const struct BulletConfig *bullet_config;
    float pattern_length;
    float spawn_offset;
    struct {
        unsigned int number_of_shots;
        float total_time;
        unsigned int end_bullets_in_pattern;
        float end_bullet_speed_modifier;
    } burst_data;
    float trj_always_angle;
    Trajectory trajectory;
    float speed_randomisation;
    float angle_randomisation;
    u8 constrain_randomisation;
    u8 lock_emitter_rotation;
} PatternConfig;

// The texture will be rotated to match direction
#define BULLETFLAG_ROTATE_TEXTURE (1u << 0)
// The bullet has a larger hitbox
#define BULLETFLAG_BIG (1u << 1)
// The bullet belongs to the player
#define BULLETFLAG_PLAYER (1u << 2)
// The bullet will be drawn over low priority bullets
#define BULLETFLAG_HIGH_PRIORITY (1u << 3)

void fire_pattern(Vector2 position, float angle,
                  const PatternConfig *pattern);

void fire_pattern_burst_shot(Vector2 position, float angle,
                             const PatternConfig *pattern,
                             float burst_progress);

void bullet_fire_one(Vector2 initial_position, float direction,
                     const BulletConfig *args, float offset);

void bullet_fire_ring(Vector2 initial_position, float direction,
                      const BulletConfig *config, int bullets_in_ring,
                      float ring_offset, Trajectory trajectory);

void bullet_fire_arc(Vector2 initial_position, float direction,
                     const BulletConfig *config, int bullets_in_arc,
                     float arc_offset, float arc_angle,
                     Trajectory trajectory);

u32 cancel_bullets(bool spawn_crystals, bool spawn_the_particle);

void process_bullet(Entity *self, f32 delta);
void draw_bullet(Entity *self, f32 delta);
void hit_bullet(Entity *self, Entity *other);

#endif // BULLET_H
