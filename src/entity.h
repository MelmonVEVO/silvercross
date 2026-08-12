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
    Vector2 options_position;
    f32 invincibility_time;
    f32 hyper_gauge;
    f32 collision_recovery_time;
    u8 life;
    u8 bomber_stock;
    u16 medals_until_next_bomber;
    bool hyper_active;
    bool counterbomb_active;
    u8 counterbomb_frames;
} PlayerData;

typedef struct BulletConfig {
    f32 initial_speed;
    f32 acceleration;
    f32 angular_velocity;
    Vector2 gravity;
    f32 initial_ttl;
    const AnimatedTexture2D *bullet_texture;
    u8 flags;
    u8 texture_row;
} BulletConfig;

typedef struct {
    f32 value;
    bool is_following_player;
} MedalData;

typedef struct {
    BulletConfig config;
    f32 ttl;
    f32 acceleration;
    AnimatedTexture2DInstance texture_instance;
} BulletData;

// Trajectory controls the initial angle of bullets fired.
// Default fires the bullets in a usual manner.
// Fixed fires all bullets in one single direction.
// Aimed fires the bullets rotated towards the player.
// Random randomises the bullet trajectory.
// Always makes a bullet always fire in a specific angle.
typedef enum {
    TRJ_DEFAULT,
    TRJ_FIXED,
    TRJ_AIMED,
    TRJ_RANDOM,
} Trajectory;

/*
 * BP_ONE: Fires a single bullet.
 * BP_RING: Fires a ring of bullets.
 * BP_ARC: Fires bullets in a constrained arc.
 * BP_CUSTOM: Takes a custom function that is ran when firing.
 */
typedef enum {
    BP_ONE,
    BP_RING,
    BP_ARC,
    BP_CUSTOM,
} BulletPattern;

typedef struct PatternConfig {
    BulletPattern pattern_type;
    Trajectory trajectory;
    u8 flags;
    const struct BulletConfig *bullet_config;
    u32 bullets_in_pattern;
    f32 pattern_length;
    f32 spawn_offset;
    void (*custom_fire)(Vector2 initial_position, float initial_angle,
                        const BulletConfig *config, float offset);
    struct {
        u32 number_of_shots;
        f32 total_time;
        u32 end_bullets_in_pattern;
        f32 end_bullet_speed_modifier;
    } burst_data;
    f32 trj_always_angle;
    f32 speed_randomisation;
    f32 angle_randomisation;
} PatternConfig;

typedef enum { ENEMY_TEST_ENEMY, ENEMY_TYPE_COUNT } EnemyType;

// RotationType controls how the emitter "rotates".
typedef enum {
    ROT_NONE,
    ROT_CONTINUOUS,
    ROT_BOUNCE,
    ROT_RANDOMISE,
    ROT_TOWARDS_PLAYER
} RotationType;

typedef struct {
    const PatternConfig *pattern;
    Vector2 local_position;
    int number_of_volleys;
    float time_until_start;
    float volley_rate;
    float start_rotation;
    float rotation_range;
    float rotation_speed;
    RotationType rotation_type;
    bool enabled_by_default;
} EmitterConfig;

typedef struct {
    const EmitterConfig *config;
    bool enabled;
    float cooldown_between_volleys;
    float current_rotation;
    int volleys_left;
    float inverse_rotate;
    struct {
        bool active;
        float shot_cooldown;
        unsigned int shots_fired;
        float locked_rotation;
        Vector2 player_position_on_burst_start;
    } burst;
} EmitterLive;

typedef struct {
    EnemyType enemy_type;
    EmitterLive current_emitters[MAX_EMITTERS];
    f32 seal_circle_radius;
    bool sealed;
    union {};
} EnemyData;

union EntityAs {
    PlayerData player;
    BulletData bullet;
    EnemyData enemy;
    MedalData medal;
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
    // Do not update this manually.
    bool is_active;
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
    void (*damage)(Entity *self, f32 amount);
} EntityBehaviours;

void entity_process_noop(Entity *self, f32 delta);
void entity_draw_noop(Entity *self, f32 delta);
void entity_init_noop(Entity *self);
void entity_die_noop(Entity *self);
void entity_hit_noop(Entity *self, Entity *other);
void entity_damage_noop(Entity *self, f32 amount);

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

// Moves an entity by its position and velocity
void move_entity(Entity *entity, f32 delta);

extern const EntityBehaviours entity_behaviours[ENTITY_TYPE_COUNT];
#define entity_process(SELF, DELTA)                                       \
    entity_behaviours[(SELF)->type].process((SELF), (DELTA))
#define entity_draw(SELF, DELTA)                                          \
    entity_behaviours[(SELF)->type].draw((SELF), (DELTA))
#define entity_init(SELF) entity_behaviours[SELF->type].init(SELF)
#define entity_die(SELF) entity_behaviours[SELF->type].die(SELF)
#define entity_hit(SELF, OTHER)                                           \
    entity_behaviours[(SELF)->type].hit((SELF), (OTHER))
#define entity_damage(SELF, AMOUNT)                                       \
    entity_behaviours[SELF->type].damage(SELF, AMOUNT)
#define entity_has_behaviour(SELF, BEHAVIOUR)                             \
    (entity_behaviours[(SELF)->type].BEHAVIOUR != NULL)

void reset_entities(void);
void process_entities(f32 delta);
void draw_entities(f32 delta);
u32 entity_count(void);
u32 cancel_bullets(bool spawn_crystals, bool spawn_the_particle);
Vector2 entity_world_position(Entity *entity);

#endif // ENTITY_H
