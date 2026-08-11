#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"

void spawn_enemy(EnemyType type, Vector2 at);
void setup_emitter(EmitterLive *emitter, const EmitterConfig *config);

void process_enemy(Entity *self, f32 delta);
void draw_enemy(Entity *self, f32 delta);
void hit_enemy(Entity *self, Entity *other);
void die_enemy(Entity *self);
void damage_enemy(Entity *self, f32 amount);

#endif /* ifndef ENEMY_H */
