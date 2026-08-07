#include "entity.h"
#include "constants.h"
#include "player.h"
#include "utils.h"
#include <assert.h>
#include <stddef.h>

const EntityBehaviours entity_behaviours[ENTITY_TYPE_COUNT] = {
    player_behaviours, {}, {}, {}, {},
};
Entities entities;

static bool is_handle_valid(EntityHandle handle) {
    return handle.idx < MAX_ENTITIES;
}

void process_entities(f32 delta) {
    Entity *current;
    for (i32 i = 0; i < MAX_ENTITIES; i++) {
        current = &entities.entities[i];
        if (!current->is_alive)
            continue;
        entity_process(current, delta);
    }
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
