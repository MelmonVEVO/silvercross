#include "entity.h"
#include "bullet.h"
#include "constants.h"
#include "player.h"
#include "raylib.h"
#include "utils.h"
#include <assert.h>
#include <stddef.h>

#ifdef DEBUG
#include "assets.h"
#endif

void entity_process_noop([[maybe_unused]] Entity *self,
                         [[maybe_unused]] f32 delta) {}
void entity_draw_noop([[maybe_unused]] Entity *self,
                      [[maybe_unused]] f32 delta) {}
void entity_init_noop([[maybe_unused]] Entity *self) {}
void entity_die_noop([[maybe_unused]] Entity *self) {}
void entity_hit_noop([[maybe_unused]] Entity *self,
                     [[maybe_unused]] Entity *other) {}
void entity_damage_noop([[maybe_unused]] Entity *self) {}

const EntityBehaviours player_behaviours = {};

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
            .init = init_bullet,
            .die = entity_die_noop,
            .hit = hit_bullet,
            .damage = entity_damage_noop,
        },
};
static Entities entities;

typedef struct {
    u32 entity_idxs[MAX_ENTITIES];
    int count;
} PartitionCell;

#define SPATIAL_PARTITION_CELL_SIZE 20
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
    i32 xend;
    i32 ystart;
    i32 yend;
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

CollisionList collision_list = {};

static inline bool is_valid_collision(EntityType a, EntityType b) {
    EntityType a_nrm = (EntityType)MIN(a, b);
    EntityType b_nrm = (EntityType)MAX(a, b);
    switch (a_nrm) {
    case ENTITY_PLAYER:
        return b_nrm == ENTITY_BULLET || b_nrm == ENTITY_ENEMY ||
               b_nrm == ENTITY_MEDAL;
    case ENTITY_BULLET:
        return b_nrm == ENTITY_ENEMY;
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
    if (!a_entity || !b_entity)
        return;
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
                Rectangle a_collision_box = create_centred_rectangle(
                    a_entity_p->position.x, a_entity_p->position.y,
                    a_entity_p->collision);
                for (i32 j = i + 1; j < current->count; j++) {
                    u32 b_entity = current->entity_idxs[j];
                    Entity *b_entity_p = &entities.entities[b_entity];

                    EntityPartitionBounds *a_bounds =
                        &partition_bounds[a_entity];
                    EntityPartitionBounds *b_bounds =
                        &partition_bounds[b_entity];

                    i32 shared_x = MAX(a_bounds->xstart, b_bounds->xstart);
                    i32 shared_y = MAX(a_bounds->ystart, b_bounds->ystart);

                    if (x != shared_x || y != shared_y)
                        continue;

                    if (!is_valid_collision(a_entity_p->type,
                                            b_entity_p->type))
                        continue;

                    Rectangle b_collision_box = create_centred_rectangle(
                        b_entity_p->position.x, b_entity_p->position.y,
                        b_entity_p->collision);
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

void resolve_other(void) {} // TODO: this thing

void update_partition_grid(void) {
    for (i32 i = 0; i < PARTITION_GRID_HEIGHT; i++) {
        for (i32 j = 0; j < PARTITION_GRID_WIDTH; j++) {
            partition_grid.cells[i][j].count = 0;
        }
    }

    Entity *current;
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
            .xend = xend,
            .ystart = ystart,
            .yend = yend,
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
        if (!current->is_alive)
            continue;
        entity_process(current, delta);
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
    const i32 cell_size = SPATIAL_PARTITION_CELL_SIZE;

    for (i32 y = 0; y < PARTITION_GRID_HEIGHT; y++) {
        for (i32 x = 0; x < PARTITION_GRID_WIDTH; x++) {
            PartitionCell *cell = &partition_grid.cells[y][x];

            DrawRectangle(x * cell_size, y * cell_size, cell_size - 1,
                          cell_size - 1, BLUE);
            DrawTextEx(assets.fonts.fusion, TextFormat("%d", cell->count),
                       (Vector2){(x * cell_size) + 2, (y * cell_size) + 2},
                       assets.fonts.fusion.baseSize, 0, BLACK);
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

u32 entity_count(void) { return entities.live_entity_count; }
