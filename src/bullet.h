#ifndef BULLET_H
#define BULLET_H

#include "entity.h"

#define PATTERNFLAG_CONSTRAIN_RANDOMISATION (1 << 0)
#define PATTERNFLAG_LOCK_EMITTER_ROTATION (1 << 1)
#define PATTERNFLAG_HAS_BURST (1 << 2)

// The texture will be rotated to match direction
#define BULLETFLAG_ROTATE_TEXTURE (1u << 0)
// The bullet has a larger hitbox
#define BULLETFLAG_BIG (1u << 1)
// The bullet belongs to the player
#define BULLETFLAG_PLAYER (1u << 2)
// The bullet will be drawn over low priority bullets
#define BULLETFLAG_HIGH_PRIORITY (1u << 3)
#define BULLETFLAG_HAS_MIN_SPEED (1u << 4)
#define BULLETFLAG_HAS_MAX_SPEED (1u << 5)

void fire_pattern(Vector2 position, f32 angle,
                  const PatternConfig *pattern);

void fire_pattern_burst_shot(Vector2 position, f32 angle,
                             const PatternConfig *pattern,
                             f32 burst_progress);

void bullet_fire_one(Vector2 initial_position, f32 direction,
                     const BulletConfig *args, f32 offset,
                     f32 angle_randomisation);

void bullet_fire_ring(Vector2 initial_position, f32 direction,
                      const BulletConfig *config, int bullets_in_ring,
                      f32 ring_offset, Trajectory trajectory,
                      f32 speed_randomisation, f32 angle_randomisation);

void bullet_fire_arc(Vector2 initial_position, f32 direction,
                     const BulletConfig *config, int bullets_in_arc,
                     f32 arc_offset, f32 arc_angle, Trajectory trajectory,
                     f32 speed_randomisation, f32 angle_randomisation,
                     bool constrain_randomisation);

u32 cancel_bullets(bool spawn_crystals, bool spawn_the_particle);

void process_bullet(Entity *self, f32 delta);
void draw_bullet(Entity *self, f32 delta);
void hit_bullet(Entity *self, Entity *other);

#endif // BULLET_H
