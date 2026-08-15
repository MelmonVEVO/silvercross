#ifndef BOMB_H
#define BOMB_H

#include "entity.h"
#include "primitives.h"

void process_bomb(Entity *self, f32 delta);
void init_bomb(Entity *self);
void draw_bomb(Entity *self, f32 delta);

#endif // BOMB_H
