#ifndef GAME_H
#define GAME_H

#include "constants.h"
#include "entity.h"
#include "primitives.h"

typedef struct {
} World;

void process_game(void);
void draw_game(f32 delta);

#endif // GAME_H
