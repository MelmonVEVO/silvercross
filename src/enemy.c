#include "enemy.h"
#include "assets.h"
#include "bullet.h"
#include "constants.h"
#include "entity.h"
#include "player.h"
#include "raylib.h"
#include <assert.h>

// -- Test enemy --

BulletConfig test_enemy_bullet = (BulletConfig){
    .bullet_texture = &assets.textures.bullets1,
    .texture_row = 2,
    .initial_speed = 110.0f,
    .initial_ttl = 5.0f,
    .flags = BULLETFLAG_ROTATE_TEXTURE,
};

void init_test_enemy(Entity *self) {
    assert(self->as.enemy.enemy_type == ENEMY_TEST_ENEMY);
    self->as.enemy.time_until_shoot = 0.3f;
    self->hp = 40;
    self->collision = (Vector2){64.0f, 64.0f};
}

void process_test_enemy(Entity *self, f32 delta) {
    f32 *time = &self->as.enemy.time_until_shoot;
    *time -= delta;
    if (*time <= 0) {
        bullet_fire_one(self->position,
                        front_towards_player(self->position),
                        &test_enemy_bullet, 0);
        *time += 0.3f;
    }

    self->velocity = Vector2Scale((Vector2){sinf(3.0f * self->time_alive),
                                            cosf(1.6f * self->time_alive)},
                                  16.0f);

    move_entity(self, delta);
}

void draw_test_enemy(Entity *self) {
    DrawRectangle(self->position.x - 32, self->position.y - 32, 64, 64,
                  RED);
}

// -- Regular enemy stuff --

void spawn_enemy(EnemyType type, Vector2 at) {
    Entity *enemy = spawn_entity(ENTITY_ENEMY);
    enemy->as.enemy = (EnemyData){};
    enemy->as.enemy.enemy_type = type;
    switch (type) {
    case ENEMY_TEST_ENEMY:
        init_test_enemy(enemy);
        break;
    default:
    }
    enemy->position = at;
}

void process_enemy(Entity *self, f32 delta) {
    switch (self->as.enemy.enemy_type) {
    case ENEMY_TEST_ENEMY:
        process_test_enemy(self, delta);
        break;
    default:
    }
}

void draw_enemy(Entity *self, f32 delta) {
    switch (self->as.enemy.enemy_type) {
    case ENEMY_TEST_ENEMY:
        draw_test_enemy(self);
        break;
    default:
    }
}

void hit_enemy(Entity *self, Entity *other) {
    if (other->type != ENTITY_BULLET ||
        !(other->as.bullet.config.flags & BULLETFLAG_PLAYER))
        return;
    Entity *medal = spawn_entity(ENTITY_MEDAL);
    medal->position = self->position;
    entity_damage(self, PLAYER_BULLET_DAMAGE);
}

void die_enemy(Entity *self) {}

void damage_enemy(Entity *self, f32 amount) {}
