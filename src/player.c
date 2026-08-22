#include "player.h"
#include "assets.h"
#include "bullet.h"
#include "constants.h"
#include "entity.h"
#include "medal.h"
#include "utils.h"
#include <assert.h>
#include <float.h>
#include <raylib.h>
#include <raymath.h>

EntityHandle player_ref;

static BulletConfig player_bullet_args = (BulletConfig){
    .initial_speed = 1200.0f,
    .initial_ttl = 1.0f,
    .flags = BULLETFLAG_ROTATE_TEXTURE | BULLETFLAG_PLAYER,
    .texture_row = 0,
    .bullet_texture = &assets.textures.playershots,
};

const Vector2 options_locs[4] = {
    (Vector2){-20.0f, 21.0f},
    (Vector2){-30.0f, 13.0f},
    (Vector2){20.0f, 21.0f},
    (Vector2){30.0f, 13.0f},
};

Vector2 player_position(void) {
    Entity *player = get_entity(player_ref, ENTITY_PLAYER);
    return player->position;
}

static void player_fire(Entity *player, f32 delta) {
    static f32 fire_rate = 0;
    if (fire_rate <= 0) {
        bullet_fire_one(
            (Vector2){player->position.x - 7.0f, player->position.y},
            -90.0f, &player_bullet_args, 0, 0);
        bullet_fire_one(
            (Vector2){player->position.x + 7.0f, player->position.y},
            -90.0f, &player_bullet_args, 0, 0);
        bullet_fire_one(Vector2Add(player->as.player.options_position,
                                   options_locs[0]),
                        -92.0f, &player_bullet_args, 0, 0);
        bullet_fire_one(Vector2Add(player->as.player.options_position,
                                   options_locs[1]),
                        -95.0f, &player_bullet_args, 0, 0);
        bullet_fire_one(Vector2Add(player->as.player.options_position,
                                   options_locs[2]),
                        -88.0f, &player_bullet_args, 0, 0);
        bullet_fire_one(Vector2Add(player->as.player.options_position,
                                   options_locs[3]),
                        -85.0f, &player_bullet_args, 0, 0);
        fire_rate = PLAYER_FIRE_RATE;
    }
    fire_rate -= delta;
}

static void fully_kill_player(Entity *self) {
    PlayerData *player_data = &self->as.player;
    player_data->counterbomb_active = false;
    player_data->counterbomb_frames = 1;
    player_data->invincibility_time = PLAYER_INVINCIBILITY_TIME;
    /* player_data->additional_velocity = */
    /*     VEC2FROMANGLE(2 * PI * random_float(), PLAYER_DEATH_YEET_SPEED);
     */
    player_data->life -= 1;
    player_data->bomber_stock =
        MIN(MAX_BOMBS, player_data->bomber_stock + 1);
    cancel_bullets(false, true);
    set_medal_chain_gauge_0();
    // play death sound
    // burst particles
}

void process_player(Entity *player, f32 delta) {
    PlayerData *data = &player->as.player;

    data->options_position = Vector2Lerp(data->options_position,
                                         player->position, 16.0f * delta);
    player->hp = MIN(player->hp + delta, 2.0f);
    data->collision_recovery_time -= delta;
    data->invincibility_time -= delta;

    if (data->counterbomb_active) {
        data->counterbomb_frames--;
        if (data->counterbomb_frames <= 0)
            fully_kill_player(player);
    }

    if (data->life <= 0) {
        data->invincibility_time = 1.0f;
        return;
    }

    player->velocity = Vector2Zero();
    if (is_input_down(INPUT_LEFT))
        player->velocity.x -= 1.0f;
    if (is_input_down(INPUT_RIGHT))
        player->velocity.x += 1.0f;
    if (is_input_down(INPUT_UP))
        player->velocity.y -= 1.0f;
    if (is_input_down(INPUT_DOWN))
        player->velocity.y += 1.0f;

    f32 speed;

    Entity *bomber_entity = get_entity(data->bomber_entity, ENTITY_BOMB);
    if (bomber_entity)
        speed = PLAYER_BOMB_SPEED;
    else if (is_input_down(INPUT_SLOW))
        speed = PLAYER_FOCUS_SPEED;
    else
        speed = PLAYER_SPEED;

    if (Vector2Length(player->velocity) > EPSILON) {
        player->velocity =
            Vector2Scale(Vector2Normalize(player->velocity), speed);
    }
    Vector2 total_velocity =
        Vector2Add(player->velocity, data->additional_velocity);

    player->position.x =
        Clamp(player->position.x + (total_velocity.x * delta),
              PLAYER_MOVEMENT_BOUNDS_UNITS,
              VIEWPORT_WIDTH - PLAYER_MOVEMENT_BOUNDS_UNITS);
    player->position.y =
        Clamp(player->position.y + (total_velocity.y * delta),
              PLAYER_MOVEMENT_BOUNDS_UNITS,
              VIEWPORT_HEIGHT - PLAYER_MOVEMENT_BOUNDS_UNITS);

    if (is_input_just_pressed(INPUT_SHOT))
        data->fire_time = PLAYER_TAP_FIRE_TIME;

    data->fire_time -= delta;

    if (is_input_down(INPUT_SHOT) && data->fire_time <= 0)
        data->fire_time = EPSILON;

    if (data->is_charging_bomb || bomber_entity)
        data->fire_time = 0;

    if (data->fire_time > 0)
        player_fire(player, delta);

    if (Vector2Length(data->additional_velocity) <= 15.0f) {
        data->additional_velocity = Vector2Zero();
    }

    data->additional_velocity = Vector2Lerp(data->additional_velocity,
                                            Vector2Zero(), 15.0f * delta);

    if (is_input_just_pressed(INPUT_BOMB) && data->bomber_stock > 0 &&
        !bomber_entity && !data->is_charging_bomb) {
        data->is_charging_bomb = true;
        data->counterbomb_active = false;
        data->invincibility_time = PLAYER_INVINCIBILITY_TIME;
        data->bomber_charge_time = PLAYER_BOMBER_CHARGE_TIME;
        data->bomber_stock--;
    }

    if (data->is_charging_bomb) {
        data->bomber_charge_time -= delta;
        if (data->bomber_charge_time <= 0) {
            data->is_charging_bomb = false;
            Entity *bomber = spawn_entity(ENTITY_BOMB);
            assert(bomber);
            if (!bomber) {
                log_warning("Could not spawn bomber entity!");
                return;
            }
            data->bomber_entity = entity_handle_from_ptr(bomber);
            add_child(player, bomber);
            bomber->position =
                (Vector2){0, (VIEWPORT_HEIGHT / 2.0f) - VIEWPORT_HEIGHT};
        }
    }
}

void draw_player(Entity *player, f32 delta) {
    static f32 marker_rotation = 0;

    if (player->as.player.life == 0)
        return;

    if (get_entity(player->as.player.bomber_entity, ENTITY_BOMB) ||
        player->as.player.is_charging_bomb) {
        f32 scale =
            player->as.player.is_charging_bomb
                ? quint_ease((PLAYER_BOMBER_CHARGE_TIME -
                              player->as.player.bomber_charge_time) /
                             PLAYER_BOMBER_CHARGE_TIME)
                : 1.0f;
        draw_centred_texture_ex(
            assets.textures.bomb_circle,
            Vector2Add(player->position, (Vector2){0, -32.0f}), 0, scale,
            WHITE);
    }

    f32 invincibility_time = player->as.player.invincibility_time;
    f32 invincibility_opacity =
        sinf(invincibility_time * 120.0f) > 0 ? 0.4 : 0;
    draw_centred_texture(assets.textures.tempplayer, player->position);
    if (invincibility_time > 0) {
        BeginBlendMode(BLEND_ADDITIVE);
        draw_centred_texture_ex(assets.textures.tempplayer,
                                player->position, 0, 1.0f,
                                Fade(WHITE, invincibility_opacity));
        EndBlendMode();
    }
    DrawRectanglePro(
        (Rectangle){player->position.x, player->position.y, 4.0f, 4.0f},
        (Vector2){2.0f, 2.0f}, marker_rotation,
        (Colour){0, 255, 255, 255});
    DrawRectangle(player->position.x - 1, player->position.y - 1, 2, 2,
                  WHITE);
    for (i32 i = 0; i < ARRAYLEN(options_locs); i++) {
        Vector2 position = Vector2Add(player->as.player.options_position,
                                      options_locs[i]);
        DrawCircle(position.x, position.y, 4, WHITE);
    }
    marker_rotation += 400.0f * delta;
    if (invincibility_time > 0) {
        f32 progress =
            ((invincibility_time / PLAYER_INVINCIBILITY_TIME) * 360.0f) -
            90.0f;
        DrawRing(player->position, PLAYER_INVINCIBILITY_RING_RADIUS - 2.0f,
                 PLAYER_INVINCIBILITY_RING_RADIUS, -90.0f, progress, 24,
                 Fade(WHITE, invincibility_opacity + 0.4));
        draw_outlined_text_ex("SAFE", assets.fonts.fusion,
                              (Vector2){floorf(player->position.x - 11.0f),
                                        floorf(player->position.y + 6.0f)},
                              assets.fonts.fusion.baseSize, 0, SKYBLUE,
                              BLACK, 1);
    }
}

void init_player(Entity *player) {
    player->position = (Vector2){
        floorf(VIEWPORT_WIDTH / 2.0f),
        VIEWPORT_HEIGHT - 45.0f,
    };
    player->collision = (Vector2){2.0f, 2.0f};
    player->as.player = (PlayerData){};
    player->hp = 2.0f;
    PlayerData *data = &player->as.player;
    data->medals_until_next_bomber = MEDALS_FOR_NEW_BOMB;
    data->bomber_stock = 3;
    data->life = 3;
    data->invincibility_time = PLAYER_INVINCIBILITY_TIME;
    data->options_position = player->position;
}

static void kill_player(Entity *self) {
    PlayerData *player_data = &self->as.player;
    if (player_data->invincibility_time > 0)
        return;
    if (player_data->counterbomb_active == true)
        return;
    /* unsigned int frames =  TODO: frame shit */
    player_data->counterbomb_active = true;
    player_data->counterbomb_frames = PLAYER_COUNTERBOMB_WINDOW_FRAMES;
}

static void bonk_player(Entity *self, const Vector2 other_position,
                        const bool lethal) {
    PlayerData *data = &self->as.player;
    if (data->invincibility_time > 0 || data->collision_recovery_time > 0)
        return;
    f32 angle = DEG2RAD * front_towards_player(other_position);
    Vector2 additional = VEC2FROMANGLE(angle, PLAYER_COLLIDE_SPEED);
    data->additional_velocity =
        Vector2Add(data->additional_velocity, additional);
    data->collision_recovery_time = 0.2f;
    entity_damage(self, 1.0f);
    if (lethal)
        entity_damage(self, 2.0f);
    if (self->hp <= 0) {
        kill_player(self);
        return;
    }
    PlaySound(assets.sfx.bonk);
}

static void collect_medal(Entity *self) {
    PlayerData *player_data = &self->as.player;
    player_data->medals_until_next_bomber -= 1;
    if (player_data->medals_until_next_bomber == 0) {
        player_data->medals_until_next_bomber = MEDALS_FOR_NEW_BOMB;
        player_data->bomber_stock =
            MIN(player_data->bomber_stock + 1, MAX_BOMBS);
    }
}

void hit_player(Entity *self, Entity *other) {
    if (self->as.player.life == 0)
        return;
    switch (other->type) {
    case ENTITY_BULLET:
        kill_player(self);
        break;
    case ENTITY_ENEMY:
        bonk_player(self, other->position, false); // TODO: lethal bonks
        break;
    case ENTITY_MEDAL:
        collect_medal(self);
        break;
    default:
    }
}

f32 front_towards_player(Vector2 position) {
    return front_towards_whatever(
        position, get_entity(player_ref, ENTITY_PLAYER)->position);
}

u8 get_player_life(void) {
    return get_entity(player_ref, ENTITY_PLAYER)->as.player.life;
}

u8 get_player_bombs(void) {
    return get_entity(player_ref, ENTITY_PLAYER)->as.player.bomber_stock;
}

f32 get_player_attract_circle_radius(void) {
    PlayerData *data = &get_entity(player_ref, ENTITY_PLAYER)->as.player;
    Entity *bomber_entity = get_entity(data->bomber_entity, ENTITY_BOMB);
    return (!is_input_down(INPUT_SHOT) || data->is_charging_bomb ||
                    bomber_entity
                ? ATTRACT_CIRCLE_RADIUS_LARGE
                : ATTRACT_CIRCLE_RADIUS);
}

f32 get_player_bomb_progress(void) {
    PlayerData *data = &get_entity(player_ref, ENTITY_PLAYER)->as.player;
    return (MEDALS_FOR_NEW_BOMB - (f32)data->medals_until_next_bomber) /
           MEDALS_FOR_NEW_BOMB;
}
