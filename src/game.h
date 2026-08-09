#ifndef GAME_H
#define GAME_H

#include "primitives.h"

void add_score(u32 extra_score);
void reset_game(void);

void process_game(void);
void draw_game(f32 delta);

#endif // GAME_H
