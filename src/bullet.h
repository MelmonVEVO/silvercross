#ifndef BULLET_H
#define BULLET_H

#include "entity.h"

void process_bullet(Entity *self, f32 delta);
void draw_bullet(Entity *self, f32 delta);
void init_bullet(Entity *self);
void hit_bullet(Entity *self, Entity *other);

#endif // BULLET_H
