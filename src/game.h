#ifndef GAME_H
#define GAME_H

#include "entity.h"
#include "primitives.h"

typedef enum {
    FPS_120 = 120,
    FPS_60 = 60,
} FPSOption;

typedef enum {
    DIFFICULTY_HARD,
    DIFFICULTY_SOFTENED,
} DifficultyOption;

typedef struct {
    FPSOption fps_option;
    DifficultyOption difficulty;
} Options;

extern Options options;

void add_score(u32 extra_score);
void reset_game(void);
void show_boss_hp_bar(EntityHandle boss, f32 max_hp, u8 patterns_left,
                      f32 *timer);
void hide_boss_hp_bar(void);
void decrement_boss_hp_bar_patterns(void);

void process_game(void);
void draw_game(f32 delta);

#endif // GAME_H
