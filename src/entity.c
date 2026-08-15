#include "entity.h"
#include "assets.h"
#include "bomb.h"
#include "bullet.h"
#include "constants.h"
#include "enemy.h"
#include "medal.h"
#include "particle.h"
#include "player.h"
#include "raylib.h"
#include "utils.h"
#include <assert.h>
#include <raymath.h>
#include <stddef.h>

#ifdef DEBUG
#include "assets.h"
#endif

void entity_process_noop([[maybe_unused]] Entity *self,
                         [[maybe_unused]] f32 delta) {}
void entity_draw_noop([[maybe_unused]] Entity *self,
                      [[maybe_unused]] f32 delta) {}
void entity_init_noop([[maybe_unused]] Entity *self) {}
void entity_die_noop(Entity *self) { self->is_alive = false; }
void entity_hit_noop([[maybe_unused]] Entity *self,
                     [[maybe_unused]] Entity *other) {}
void entity_damage_noop(Entity *self, f32 amount) { self->hp -= amount; }

const EntityBehaviours entity_behaviours[ENTITY_TYPE_COUNT] = {
    [ENTITY_PLAYER] =
        {
            .process = process_player,
            .draw = draw_player,
            .init = init_player,
            .die = entity_die_noop,
            .hit = hit_player,
            .damage = entity_damage_noop,
        },
    [ENTITY_BULLET] =
        {
            .process = process_bullet,
            .draw = draw_bullet,
            .init = entity_init_noop,
            .die = entity_die_noop,
            .hit = hit_bullet,
            .damage = entity_damage_noop,
        },
    [ENTITY_MEDAL] =
        {
            .process = process_medal,
            .draw = draw_medal,
            .init = init_medal,
            .die = entity_die_noop,
            .hit = hit_medal,
            .damage = entity_damage_noop,
        },
    [ENTITY_ENEMY] =
        {
            .process = process_enemy,
            .draw = draw_enemy,
            .init = entity_init_noop,
            .die = die_enemy,
            .hit = hit_enemy,
            .damage = damage_enemy,
        },
    [ENTITY_BOMB] =
        {
            .process = process_bomb,
            .draw = draw_bomb,
            .init = init_bomb,
            .die = entity_die_noop,
            .damage = entity_damage_noop,
        },
};

typedef struct {
    Entity entities[MAX_ENTITIES];
    u32 free_next[MAX_ENTITIES];
    u32 free_head;
    u32 live_entity_count;
} Entities;

static Entities entities;

// BUG: I don't know how I managed to fuck up spatial partitioning this
// bad, using a naive IxJ approach is genuinely an improvement, as proved
// by THE LUMINOUS RUIN. Too late now. I'll look at it if I have time at
// the end of the jam.
// INFO: What I could change it to: iterate through the grid, for each
// entity whose top left corner inhabits the cell, if not already checked,
// check right and down. OR SOMETHING LIKE THAT.
// I'd also like to change the update function so it doesn't rebuild the
// entire grid from scratch every frame, but I've profiled the thing and
// most of the performance bottleneck is within record_collisions.
typedef struct {
    u32 entity_idxs[MAX_ENTITIES];
    int count;
} PartitionCell;

#define SPATIAL_PARTITION_CELL_SIZE 16
#define PARTITION_GRID_WIDTH (VIEWPORT_WIDTH / SPATIAL_PARTITION_CELL_SIZE)
#define PARTITION_GRID_HEIGHT                                             \
    (VIEWPORT_HEIGHT / SPATIAL_PARTITION_CELL_SIZE)

typedef struct {
    // Anything out of the viewport? Ignored!
    PartitionCell cells[PARTITION_GRID_HEIGHT][PARTITION_GRID_WIDTH];
} SpatialPartition;

#define MAX_COLLISIONS 5000

static SpatialPartition partition_grid = {};

typedef struct {
    i32 xstart;
    i32 ystart;
} EntityPartitionBounds;

static EntityPartitionBounds partition_bounds[MAX_ENTITIES] = {};

static bool is_handle_valid(EntityHandle handle) {
    return handle.idx < MAX_ENTITIES;
}

typedef struct {
    u32 a;
    u32 b;
} CollisionEvent;

typedef struct {
    CollisionEvent events[MAX_COLLISIONS];
    u32 count;
} CollisionList;

static CollisionList collision_list = {};
static Rectangle collision_rects[MAX_ENTITIES] = {};

static inline bool is_valid_collision(EntityType a, EntityType b) {
    EntityType a_nrm = (EntityType)MIN(a, b);
    EntityType b_nrm = (EntityType)MAX(a, b);
    switch (a_nrm) {
    case ENTITY_PLAYER:
        return b_nrm == ENTITY_BULLET || b_nrm == ENTITY_ENEMY ||
               b_nrm == ENTITY_MEDAL;
    case ENTITY_BULLET:
        return b_nrm == ENTITY_ENEMY || b_nrm == ENTITY_BOMB;
    case ENTITY_ENEMY:
        return b_nrm == ENTITY_BOMB;
    default:
        return false;
    }
}

static inline void add_collision(u32 a, u32 b) {
    if (collision_list.count == MAX_COLLISIONS) {
        log_error("Tried to have too many collisions!");
        return;
    }

    Entity *a_entity = &entities.entities[a];
    Entity *b_entity = &entities.entities[b];
    u32 lower = MIN(a, b);
    u32 higher = MAX(a, b);

    CollisionEvent event = (CollisionEvent){lower, higher};
    collision_list.events[collision_list.count++] = event;
}

void record_collisions(void) {
    PartitionCell *current;
    for (i32 y = 0; y < PARTITION_GRID_HEIGHT; y++) {
        for (i32 x = 0; x < PARTITION_GRID_WIDTH; x++) {
            current = &partition_grid.cells[y][x];

            if (current->count < 2)
                continue;

            for (i32 i = 0; i < current->count; i++) {
                u32 a_entity = current->entity_idxs[i];
                Entity *a_entity_p = &entities.entities[a_entity];
                Rectangle a_collision_box = collision_rects[a_entity];
                EntityPartitionBounds a_bounds =
                    partition_bounds[a_entity];
                for (i32 j = i + 1; j < current->count; j++) {
                    u32 b_entity = current->entity_idxs[j];
                    Entity *b_entity_p = &entities.entities[b_entity];

                    if (!is_valid_collision(a_entity_p->type,
                                            b_entity_p->type))
                        continue;

                    EntityPartitionBounds b_bounds =
                        partition_bounds[b_entity];

                    i32 shared_x = MAX(a_bounds.xstart, b_bounds.xstart);
                    i32 shared_y = MAX(a_bounds.ystart, b_bounds.ystart);

                    if (x != shared_x || y != shared_y)
                        continue;

                    Rectangle b_collision_box = collision_rects[b_entity];
                    bool colliding = CheckCollisionRecs(a_collision_box,
                                                        b_collision_box);
                    if (!colliding)
                        continue;

                    add_collision(a_entity, b_entity);
                }
            }
        }
    }
}

void resolve_collisions(void) {
    CollisionEvent current;
    for (i32 i = 0; i < collision_list.count; i++) {
        current = collision_list.events[i];
        Entity *a = &entities.entities[current.a];
        Entity *b = &entities.entities[current.b];
        if (entity_has_behaviour(a, hit))
            entity_hit(a, b);
        if (entity_has_behaviour(b, hit))
            entity_hit(b, a);
    }
}

void resolve_other(void) {
    Entity *current;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        if (current->hp <= 0 && current->is_alive) {
            current->is_alive = false;
            entity_die(current);
        }
        if (current->queue_destroy) {
            destroy_entity_ptr(current);
        }
    }
}

void update_partition_grid(void) {
    for (i32 i = 0; i < PARTITION_GRID_HEIGHT; i++) {
        for (i32 j = 0; j < PARTITION_GRID_WIDTH; j++) {
            partition_grid.cells[i][j].count = 0;
        }
    }

    Entity *current;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        if (!current->is_active)
            continue;

        i32 xstart, xend, ystart, yend;
        Vector2 current_position = get_entity_world_position(current);
        Rectangle entity_bounds = create_centred_rectangle(
            current_position.x, current_position.y, current->collision);
        collision_rects[i] = entity_bounds;
        if (test_rectangle_offscreen(entity_bounds))
            continue;

        xstart = (i32)floorf(entity_bounds.x /
                             (float)SPATIAL_PARTITION_CELL_SIZE);
        xend = (i32)floorf((entity_bounds.x + entity_bounds.width) /
                           (float)SPATIAL_PARTITION_CELL_SIZE);
        ystart = (i32)floorf(entity_bounds.y /
                             (float)SPATIAL_PARTITION_CELL_SIZE);
        yend = (i32)floorf((entity_bounds.y + entity_bounds.height) /
                           (float)SPATIAL_PARTITION_CELL_SIZE);

        xstart = MAX(0, xstart);
        xend = MIN(PARTITION_GRID_WIDTH - 1, xend);
        ystart = MAX(0, ystart);
        yend = MIN(PARTITION_GRID_HEIGHT - 1, yend);

        partition_bounds[i] = (EntityPartitionBounds){
            .xstart = xstart,
            .ystart = ystart,
        };

        for (i32 y = ystart; y <= yend; y++) {
            for (i32 x = xstart; x <= xend; x++) {
                PartitionCell *cell = &partition_grid.cells[y][x];
                cell->entity_idxs[cell->count++] = i;
            }
        }
    }
}

void process_entities(f32 delta) {
    Entity *current;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        if (!current->is_active)
            continue;
        entity_process(current, delta);
        current->time_alive += delta;
    }

    update_partition_grid();

    collision_list.count = 0;
    record_collisions();
    resolve_collisions();
    resolve_other();
}

Entity *spawn_entity(EntityType type) {
    assert(type >= 0 || type < ENTITY_TYPE_COUNT);
    if (type < 0 || type >= ENTITY_TYPE_COUNT) {
        log_error("Tried to spawn entity with invalid type %d.", type);
        return NULL;
    }
    if (entities.free_head >= MAX_ENTITIES) {
        log_error("Entity pool exhausted when trying to spawn type %d.",
                  type);
        return NULL;
    }

    u32 index = entities.free_head;
    Entity *entity = &entities.entities[index];
    entities.free_head = entities.free_next[index];

    u32 generation = entity->generation;
    *entity = (Entity){};
    entity->type = type;
    entity->is_active = true;
    entity->is_alive = true;
    entity->generation = generation;
    entity->parent = ENTITY_HANDLE_NONE;

    entities.live_entity_count++;

    if (entity_has_behaviour(entity, init))
        entity_init(entity);

    return entity;
}

EntityHandle entity_handle_from_ptr(Entity *entity) {
    if (!entity)
        return ENTITY_HANDLE_NONE;

    uintptr_t start = (uintptr_t)&entities.entities[0];
    uintptr_t end = (uintptr_t)&entities.entities[MAX_ENTITIES];
    uintptr_t value = (uintptr_t)entity;

    if (value < start || value >= end)
        return ENTITY_HANDLE_NONE;

    uintptr_t offset = value - start;
    if (offset % sizeof(Entity) != 0)
        return ENTITY_HANDLE_NONE;

    u32 index = (u32)(offset / sizeof(Entity));
    return (EntityHandle){
        .idx = index, .generation = entities.entities[index].generation};
}

void destroy_entity(EntityHandle handle) {
    Entity *entity = get_entity_agnostic(handle);
    if (!entity)
        return;
    entities.live_entity_count--;

    u32 index = handle.idx;
    u32 generation = entity->generation + 1;
    *entity = (Entity){};
    entity->generation = generation;
    entity->parent = ENTITY_HANDLE_NONE;

    entities.free_next[index] = entities.free_head;
    entities.free_head = index;

    if (entity_behaviours[entity->type].die)
        entity_die(entity);
}

void destroy_entity_ptr(Entity *entity) {
    destroy_entity(entity_handle_from_ptr(entity));
}

void reset_entities(void) {
    entities.free_head = 0;
    entities.live_entity_count = 0;
    Entity *current;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        *current = (Entity){};
        entities.free_next[i] = i + 1;
    }
    entities.free_next[MAX_ENTITIES - 1] = MAX_ENTITIES;

    Entity *player = spawn_entity(ENTITY_PLAYER);
    extern EntityHandle player_ref;
    player_ref = entity_handle_from_ptr(player);
}

void draw_entities(f32 delta) {
    Entity *current;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        if (!current->is_active)
            continue;
        entity_draw(current, delta);
    }
}

Entity *get_entity(EntityHandle handle, EntityType type) {
    Entity *entity = get_entity_agnostic(handle);
    if (!entity || entity->type != type)
        return NULL;
    return entity;
}

Entity *get_entity_agnostic(EntityHandle handle) {
    if (!is_handle_valid(handle))
        return NULL;

    Entity *entity = &entities.entities[handle.idx];
    if (!entity->is_active || entity->generation != handle.generation)
        return NULL;

    return entity;
}

u32 entity_count(void) { return entities.live_entity_count; }

void move_entity(Entity *entity, f32 delta) {
    move(&entity->position, entity->velocity, delta);
}

static const ParticleConfig CANCELLED_BULLET_PARTICLE = (ParticleConfig){
    .static_texture_atlas = &assets.textures.star,
    .static_texture_rows = 1,
    .static_texture_columns = 1,
    .static_texture_pick_to = 0,
    .static_texture_pick_from = 0,
    .initial_scale = 1.0f,
    .end_scale = 0.2f,
    .initial_texture_rotational_randomness = 360.0f,
    .texture_rotation_speed = 50.0f,
    /* .texture_rotation_speed_randomness = 50.0f, */
    .base_lifetime = 0.5f,
    .lifetime_randomness_seconds = 0.2f,
    .flags = PARTICLEFLAG_HIGH_DRAW_PRIORITY,
};

u32 cancel_bullets(bool spawn_crystals, bool spawn_the_particle) {
    Entity *current;
    u32 count = 0;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        Vector2 position = current->position;
        if (!(current->type == ENTITY_BULLET) ||
            (current->as.bullet.config.flags & BULLETFLAG_PLAYER))
            continue;

        if (spawn_the_particle) {
            spawn_particle(&CANCELLED_BULLET_PARTICLE, current->position,
                           current->velocity, WHITE);
        }
        destroy_entity_ptr(current);
        if (spawn_crystals) {
            Entity *medal = spawn_entity(ENTITY_MEDAL);
            assert(medal);
            if (!medal) {
                log_warning("Could not spawn medal!");
                continue;
            }
            medal->position = position;
        }
        count++;
    }
    return count;
}

Vector2 get_entity_world_position(Entity *entity) {
    EntityHandle parent = entity->parent;
    if (!is_handle_valid(parent))
        return entity->position;
    Entity *parent_entity = get_entity_agnostic(parent);
    if (!parent_entity) {
        entity->parent = ENTITY_HANDLE_NONE;
        return entity->position;
    }
    return Vector2Add(get_entity_world_position(parent_entity),
                      entity->position);
}

void add_child(Entity *parent_entity, Entity *child_entity) {
    if (!parent_entity || !child_entity) {
        log_warning(
            "Tried to add a child to a parent when either did not exist.");
        return;
    }
    child_entity->position =
        Vector2Subtract(get_entity_world_position(child_entity),
                        get_entity_world_position(parent_entity));
    EntityHandle parent_handle = entity_handle_from_ptr(parent_entity);
    child_entity->parent = parent_handle;
}
