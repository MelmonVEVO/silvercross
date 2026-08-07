#include "constants.h"
#include "entity.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>

EntityHandle player_ref;

Vector2 player_position(void) {
    Entity *player = get_entity(player_ref, ENTITY_PLAYER);
    return player->position;
}

void process_player(Entity *player, f32 delta) {
    player->velocity = Vector2Zero();
    if (IsKeyDown(KEY_LEFT))
        player->velocity.x -= 1.0f;
    if (IsKeyDown(KEY_RIGHT))
        player->velocity.x += 1.0f;
    if (IsKeyDown(KEY_UP))
        player->velocity.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN))
        player->velocity.y += 1.0f;

    float speed;
    if (player->as.player.focusing)
        speed = PLAYER_FOCUS_SPEED;
    else
        speed = PLAYER_SPEED;

    if (Vector2Length(player->velocity) > EPSILON) {
        player->velocity =
            Vector2Scale(Vector2Normalize(player->velocity), speed);
    }
    Vector2 total_velocity = Vector2Add(
        player->velocity, player->as.player.additional_velocity);

    player->position.x =
        Clamp(player->position.x + (total_velocity.x * delta),
              PLAYER_MOVEMENT_BOUNDS_UNITS,
              VIEWPORT_WIDTH - PLAYER_MOVEMENT_BOUNDS_UNITS);
    player->position.y =
        Clamp(player->position.y + (total_velocity.y * delta),
              PLAYER_MOVEMENT_BOUNDS_UNITS,
              VIEWPORT_HEIGHT - PLAYER_MOVEMENT_BOUNDS_UNITS);

    if (Vector2Length(player->as.player.additional_velocity) <= 15.0f) {
        player->as.player.additional_velocity = Vector2Zero();
    }
}

void draw_player(Entity *player, f32 delta) {
    DrawCircle(player->position.x, player->position.y, 8.0f, GREEN);
}

void init_player(Entity *player) {
    player->position = (Vector2){
        floorf(VIEWPORT_WIDTH / 2.0f),
        VIEWPORT_HEIGHT - 45.0f,
    };
}
