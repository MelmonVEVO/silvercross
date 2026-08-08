#ifndef ENTITY_H
#define ENTITY_H

#include "constants.h"
#include "primitives.h"
#include "utils.h"
#include <raylib.h>

typedef enum {
    ENTITY_PLAYER,
    ENTITY_BULLET,
    ENTITY_ENEMY,
    ENTITY_MEDAL,
    ENTITY_BOMB,
    ENTITY_TYPE_COUNT,
} EntityType;

typedef struct {

    Vector2 additional_velocity;
    f32 invincibility_time;
    f32 hyper_gauge;
    f32 collision_recovery_time;
    u8 life;
    u8 bombs;
    bool focusing;
    bool hyper_active;
    bool counterbomb_active;
    u8 counterbomb_frames;
    f32 firing_time;
} PlayerData;

typedef struct BulletConfig {
    f32 initial_speed;
    f32 acceleration;
    f32 angular_velocity;
    Vector2 gravity;
    f32 initial_ttl;
    /* f32 acceleration_start_time; */
    const AnimatedTexture2D *bullet_texture;
    u8 flags;
    u8 texture_row;
} BulletConfig;

typedef struct {
    BulletConfig config;
    f32 ttl;
    f32 acceleration;
    AnimatedTexture2DInstance texture_instance;
} BulletData;

union EntityAs {
    PlayerData player;
    BulletData bullet;
    struct {
    } enemy;
    struct {
    } medal;
    struct {
    } bomb;
};

// In case you need to keep a reference to another
// entity for more than one frame.
typedef struct {
    u32 idx;
    u64 generation;
} EntityHandle;

typedef struct {
    EntityType type;
    u64 generation;

    f32 hp;
    bool is_alive;
    f32 time_alive;
    Vector2 position, velocity, collision;
    EntityHandle parent;

    union EntityAs as;
} Entity;

#define ENTITY_HANDLE_NONE                                                \
    ((EntityHandle){.idx = MAX_ENTITIES, .generation = 0})

typedef struct {
    // Called every frame, uses a fixed delta.
    void (*process)(Entity *self, f32 delta);
    // Called every frame, uses raylib's delta.
    void (*draw)(Entity *self, f32 delta);
    // Called when the entity is spawned in.
    void (*init)(Entity *self);
    // Called when the entity's HP reaches 0.
    // and is_alive is false.
    // By default, just despawns it.
    void (*die)(Entity *self);
    // Called when the entity collides with another.
    void (*hit)(Entity *self, Entity *other);
    // Called when the entity takes HP damage.
    void (*damage)(Entity *self);
} EntityBehaviours;

void entity_process_noop(Entity *self, f32 delta);
void entity_draw_noop(Entity *self, f32 delta);
void entity_init_noop(Entity *self);
void entity_die_noop(Entity *self);
void entity_hit_noop(Entity *self, Entity *other);
void entity_damage_noop(Entity *self);

static inline bool entity_handle_is_none(EntityHandle handle) {
    return handle.idx >= MAX_ENTITIES;
}

static inline bool entity_handle_equal(EntityHandle a, EntityHandle b) {
    return a.idx == b.idx && a.generation == b.generation;
}

// Gets an entity from its handle. Returns NULL if either
// - Type mismatch
// - Generation mismatch
// - idx out of bounds
Entity *get_entity(EntityHandle handle, EntityType type);
Entity *get_entity_agnostic(EntityHandle handle);
Entity *spawn_entity(EntityType type);
void destroy_entity(EntityHandle handle);
// Use with caution.
void destroy_entity_ptr(Entity *entity);
EntityHandle entity_handle_from_ptr(Entity *entity);

extern const EntityBehaviours entity_behaviours[ENTITY_TYPE_COUNT];
#define entity_process(SELF, DELTA)                                       \
    entity_behaviours[(SELF)->type].process((SELF), (DELTA))
#define entity_draw(SELF, DELTA)                                          \
    entity_behaviours[(SELF)->type].draw((SELF), (DELTA))
#define entity_init(SELF) entity_behaviours[SELF->type].init(SELF)
#define entity_die(SELF) entity_behaviours[SELF->type].die(SELF)
#define entity_hit(SELF, OTHER)                                           \
    entity_behaviours[(SELF)->type].hit((SELF), (OTHER))
#define entity_damage(SELF) entity_behaviours[SELF->type].damage(SELF)
#define entity_has_behaviour(SELF, BEHAVIOUR)                             \
    (entity_behaviours[(SELF)->type].BEHAVIOUR != NULL)

void reset_entities(void);
void process_entities(f32 delta);
void draw_entities(f32 delta);
u32 entity_count(void);

typedef struct {
    Entity entities[MAX_ENTITIES];
    u32 free_next[MAX_ENTITIES];
    u32 free_head;
    u32 live_entity_count;
} Entities;

#endif // ENTITY_H
