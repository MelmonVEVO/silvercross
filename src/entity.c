#include "entity.h"
#include "constants.h"
#include "player.h"
#include "raylib.h"
#include "utils.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

const EntityBehaviours player_behaviours = {};

const EntityBehaviours entity_behaviours[ENTITY_TYPE_COUNT] = {
    [ENTITY_PLAYER] =
        {
            .process = process_player,
            .draw = draw_player,
            .init = init_player,
        },
};
Entities entities;

typedef struct {
    EntityHandle a;
    EntityHandle b;
} CollisionEvent;

typedef struct {
    u32 entity_idxs[MAX_ENTITIES];
    int count;
} PartitionCell;

typedef struct {
    // Anything out of the viewport? Ignored!
    PartitionCell cells[VIEWPORT_HEIGHT / 5][VIEWPORT_WIDTH / 5];
} SpatialPartition;

#define MAX_COLLISIONS 5000
#define SPATIAL_PARTITION_CELL_SIZE 20.0f

SpatialPartition partition_grid = {};

CollisionEvent collisions[MAX_COLLISIONS];
u32 collision_count;

static bool is_handle_valid(EntityHandle handle) {
    return handle.idx < MAX_ENTITIES;
}

static inline void add_collision(EntityHandle a, EntityHandle b) {
    if (collision_count >= MAX_COLLISIONS) {
        log_error("Trying to record too many collisions!");
        assert(0 && "Trying to record too many collisions!");
        return;
    }
    Entity *a_entity = get_entity_agnostic(a);
    Entity *b_entity = get_entity_agnostic(b);
    if (!a_entity || !b_entity)
        return;
    collisions[collision_count].a = a;
    collisions[collision_count].b = b;
    collision_count++;
}

void record_collisions(void) { collision_count = 0; }

void resolve_collisions(void) {}

void resolve_other(void) {}

void update_partition_grid(void) {
    Entity *current;
    for (i32 i = 0; i < VIEWPORT_HEIGHT / 10; i++) {
        for (i32 j = 0; j < VIEWPORT_WIDTH / 10; j++) {
            partition_grid.cells[i][j].count = 0;
        }
    }

    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        if (!current->is_alive)
            continue;

        i32 xstart, xend, ystart, yend;
        Rectangle entity_bounds = create_centred_rectangle(
            current->position.x, current->position.y,
            (Vector2){current->collision.x, current->collision.y});
        if (test_rectangle_offscreen(entity_bounds))
            continue;

        xstart =
            (i32)floorf(entity_bounds.x / SPATIAL_PARTITION_CELL_SIZE);
        xend = (i32)floorf((entity_bounds.x + entity_bounds.width) /
                           SPATIAL_PARTITION_CELL_SIZE);
        ystart =
            (i32)floorf(entity_bounds.y / SPATIAL_PARTITION_CELL_SIZE);
        yend = (i32)floorf((entity_bounds.y + entity_bounds.height) /
                           SPATIAL_PARTITION_CELL_SIZE);

        for (i32 y = MAX(0, ystart);
             y <= MIN((VIEWPORT_HEIGHT / 10) - 1, yend); y++) {
            for (i32 x = MAX(0, xstart);
                 x <= MIN((VIEWPORT_WIDTH / 10) - 1, xend); x++) {
                partition_grid.cells[y][x]
                    .entity_idxs[partition_grid.cells[y][x].count] = i;
                partition_grid.cells[y][x].count++;
            }
        }
    }
}

void process_entities(f32 delta) {
    Entity *current;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        if (!current->is_alive)
            continue;
        entity_process(current, delta);
    }

    record_collisions();
    resolve_collisions();
    resolve_other();
    update_partition_grid();
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
    entity->is_alive = true;
    entity->generation = generation;
    entity->parent = ENTITY_HANDLE_NONE;

    entities.live_entity_count++;

    if (entity_behaviours[entity->type].init)
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

    unsigned int index = (unsigned int)(offset / sizeof(Entity));
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

    spawn_entity(ENTITY_PLAYER);
}

void draw_entities(f32 delta) {
#ifdef DEBUG
    const Color aqua = {0, 255, 255, 255};
    const i32 cell_size = (i32)SPATIAL_PARTITION_CELL_SIZE;

    for (i32 y = 0; y < VIEWPORT_HEIGHT / cell_size; y++) {
        for (i32 x = 0; x < VIEWPORT_WIDTH / cell_size; x++) {
            PartitionCell *cell = &partition_grid.cells[y][x];
            bool contains = cell->count > 0;

            DrawRectangle(x * cell_size, y * cell_size, cell_size - 1,
                          cell_size - 1, contains ? aqua : DARKBLUE);
        }
    }
#endif

    Entity *current;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        if (!current->is_alive)
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
    if (!entity->is_alive || entity->generation != handle.generation)
        return NULL;

    return entity;
}
