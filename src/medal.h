#ifndef MEDAL_H
#define MEDAL_H

#include "entity.h"
#include "primitives.h"

typedef struct {
    f32 chain;
    f32 chain_gauge;
    bool chain_gauge_stopped;
    // When there is a boss, the chain gauge does not
    // drain the chain, instead the medal chain drains at a slower, but
    // constant rate, Ketsui style.
    bool is_boss_time;
} MedalsState;

void set_medal_chain_gauge_stop(bool stop);
void set_medal_chain_gauge_0(void);
void set_medal_chain_boss_behaviour(bool is_boss);

void process_medals_state(f32 delta);
const MedalsState get_current_medals_state(void);
void reset_medals_state(void);

void process_medal(Entity *self, f32 delta);
void draw_medal(Entity *self, f32 delta);
void init_medal(Entity *self);
void hit_medal(Entity *self, Entity *other);

#endif // MEDAL_H
