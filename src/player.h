#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

void process_player(Entity *self, f32 delta);
void draw_player(Entity *self, f32 delta);
void init_player(Entity *self);
void hit_player(Entity *self, Entity *other);

// Returns the angle in degrees pointing from some position to the player.
float front_towards_player(Vector2 position);
Vector2 player_position(void);

u8 get_player_health(void);
u8 get_player_bombs(void);

#endif // PLAYER_H
