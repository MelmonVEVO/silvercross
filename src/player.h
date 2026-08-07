#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

void process_player(Entity *self, f32 delta);
void draw_player(Entity *player, f32 delta);
void init_player(Entity *player);

#endif // PLAYER_H
