#include "bullet.h"
#include "entity.h"
#include "player.h"
#include "raylib.h"
#include "utils.h"
#include <assert.h>

void process_bullet(Entity *self, f32 delta) {
    move(&self->position, self->velocity, delta);
    accelerate(&self->velocity, self->as.bullet.acceleration, delta);
    move(&self->velocity, self->as.bullet.config.gravity, delta);
    self->as.bullet.ttl -= delta;
    if ((self->as.bullet.ttl <= 0) ||
        (self->position.y < -10.0f &&
         self->as.bullet.config.flags & BULLETFLAG_PLAYER) ||
        (test_rectangle_offscreen((Rectangle){
            self->position.x - 1, self->position.y - 1, 2, 2})))
        destroy_entity_ptr(self);
}

void draw_bullet(Entity *self, f32 delta) {
    float rotation =
        (self->as.bullet.config.flags & BULLETFLAG_ROTATE_TEXTURE)
            ? atan2f(self->velocity.y, self->velocity.x) * RAD2DEG
            : 0;
    draw_animated_texture_ex(&self->as.bullet.texture_instance, delta,
                             self->position, rotation, 1, WHITE);
}

static void fire(Vector2 initial_position, f32 direction,
                 const BulletConfig *config) {
    assert(config);
    if (!config)
        log_error("Config not passed in to spawn a bullet!");
    Entity *bullet = spawn_entity(ENTITY_BULLET);
    if (!bullet) {
        log_warning("Exhausted pool while trying to spawn a bullet.");
        return;
    }
    bullet->as.bullet = (BulletData){};
    BulletData *data = &bullet->as.bullet;
    data->config = *config;
    bullet->position = initial_position;
    bullet->velocity =
        VEC2FROMANGLE(direction, bullet->as.bullet.config.initial_speed);
    bullet->hp = 1;
    bullet->collision = data->config.flags & BULLETFLAG_BIG
                            ? (Vector2){4, 4}
                            : (Vector2){2, 2};
    data->texture_instance.texture = data->config.bullet_texture;
    data->texture_instance.animation_time =
        random_float() * total_animation_time(data->config.bullet_texture);
    data->texture_instance.row = data->config.texture_row;
    data->ttl = data->config.initial_ttl;
}

void init_bullet(Entity *self) {}

void hit_bullet(Entity *self, Entity *other) {
    if ((other->type == ENTITY_PLAYER &&
         self->as.bullet.config.flags & BULLETFLAG_PLAYER) ||
        (other->type == ENTITY_ENEMY &&
         !(self->as.bullet.config.flags & BULLETFLAG_PLAYER)))
        return;
    destroy_entity_ptr(self);
}

static Vector2 get_bullet_start_position(Vector2 origin, float offset,
                                         float rotation) {
    Vector2 additional_vector = VEC2FROMANGLE(rotation, offset);
    return Vector2Add(origin, additional_vector);
}

void bullet_fire_one(Vector2 initial_position, float initial_angle,
                     const BulletConfig *config, float offset) {
    Vector2 start_position = get_bullet_start_position(
        initial_position, offset, DEG2RAD * initial_angle);
    fire(start_position, DEG2RAD * initial_angle, config);
}

void bullet_fire_arc(Vector2 initial_position, float direction,
                     const BulletConfig *config, int bullets_in_arc,
                     float arc_offset, float arc_angle,
                     Trajectory trajectory) {
    if (bullets_in_arc < 2) {
        log_warning(
            "Tried to fire an arc with less than 2 bullets. Please check "
            "the firing configurations!");
        return;
    }
    float angle_per_bullet =
        arc_get_angle_per_thing(bullets_in_arc, DEG2RAD * arc_angle);
    for (int i = 0; i < bullets_in_arc; i++) {
        float bullet_angle = 0;
        Vector2 start_position = get_bullet_start_position(
            initial_position, arc_offset,
            arc_get_thing_angle_for_i(angle_per_bullet, i,
                                      DEG2RAD * direction,
                                      DEG2RAD * arc_angle));
        switch (trajectory) {
        case TRJ_DEFAULT:
            bullet_angle = arc_get_thing_angle_for_i(angle_per_bullet, i,
                                                     DEG2RAD * direction,
                                                     DEG2RAD * arc_angle);
            break;
        case TRJ_FIXED:
            bullet_angle = DEG2RAD * direction;
            break;
        case TRJ_AIMED:
            bullet_angle = DEG2RAD * front_towards_player(start_position);
            break;
        case TRJ_RANDOM:
            bullet_angle = TAU * random_float();
            break;
        default:
            break;
        }
        fire(start_position, bullet_angle, config);
    }
}

void bullet_fire_ring(Vector2 initial_position, float direction,
                      const BulletConfig *config, int bullets_in_ring,
                      float ring_offset, Trajectory trajectory) {
    f32 angle_per_bullet =
        RAD2DEG * ring_get_angle_per_thing(bullets_in_ring);
    bullet_fire_arc(initial_position, direction, config, bullets_in_ring,
                    ring_offset, 360.0f - angle_per_bullet, trajectory);
}
