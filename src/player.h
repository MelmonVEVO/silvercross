#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

void process_player(Entity *self, f32 delta);
void draw_player(Entity *self, f32 delta);
void init_player(Entity *self);
void hit_player(Entity *self, Entity *other);

// Returns the angle in degrees pointing from some position to the player.
f32 front_towards_player(Vector2 position);
Vector2 player_position(void);
f32 get_player_attract_circle_radius(void);

u8 get_player_life(void);
u8 get_player_bombs(void);
f32 get_player_bomb_progress(void);

#endif // PLAYER_H
