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
