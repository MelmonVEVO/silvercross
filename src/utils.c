#include "utils.h"
#include "constants.h"
#include "primitives.h"
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

void accelerate(Vector2 *velocity, Vector2 positive_direction,
                const float acceleration, const float delta) {
    f32 speed = Vector2DotProduct(*velocity, positive_direction);
    speed += acceleration * delta;
    *velocity = Vector2Scale(positive_direction, speed);
}

void move(Vector2 *position, const Vector2 velocity, float delta) {
    *position = Vector2Add(*position, Vector2Scale(velocity, delta));
}

Rectangle create_centred_rectangle(const float x, const float y,
                                   const Vector2 sizes) {
    return (Rectangle){x - (sizes.x * 0.5f), y - (sizes.y * 0.5f), sizes.x,
                       sizes.y};
}

void draw_progress_bar(Rectangle bar, float percentage, Colour bgcolour,
                       Colour fillcolour) {
    DrawRectangleRec(bar, bgcolour);
    float fill_width = bar.width * percentage;
    DrawRectangleRec((Rectangle){bar.x, bar.y, fill_width, bar.height},
                     fillcolour);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf(TERM_INFO "[INFO] " TERM_NORMAL);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, TERM_WARNING "[WARNING] " TERM_NORMAL);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, TERM_ERROR "[ERROR] " TERM_NORMAL);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void draw_centred_texture(Texture2D texture, Vector2 at) {
    draw_centred_texture_ex(texture, at, 0, 1.0f, WHITE);
}

void draw_centred_texture_ex(Texture2D texture, Vector2 at, float rotation,
                             float scale, Colour tint) {
    const Rectangle source =
        (Rectangle){0, 0, texture.width, texture.height};
    const Rectangle dest = (Rectangle){at.x, at.y, texture.width * scale,
                                       texture.height * scale};
    const Vector2 origin = {
        floorf(dest.width / 2.0f),
        floorf(dest.height / 2.0f)}; // should I keep the floorf?
    DrawTexturePro(texture, source, dest, origin, rotation, tint);
}

void draw_animated_texture_ex_nostep(AnimatedTexture2DInstance *instance,
                                     Vector2 position, float rotation,
                                     float scale, Colour tint) {
    assert(instance->texture->fps > 0);
    assert(instance->texture->frames > 0);
    if (instance->texture->fps == 0 || instance->texture->frames == 0) {
        log_error(
            "The texture identified with %u has a 0 frame or 0 fps set. "
            "Check the texture configurations!",
            instance->texture->texture_atlas.id);
        return;
    }
    const float seconds_per_frame = 1.0f / instance->texture->fps;
    const Vector2 sprite_sizes =
        animated_texture_frame_size(instance->texture);
    const float sprite_width = sprite_sizes.x;
    const float sprite_height = sprite_sizes.y;
    const int frame_to_draw =
        (int)floorf(instance->animation_time / seconds_per_frame);
    Rectangle src = (Rectangle){frame_to_draw * sprite_width,
                                sprite_height * instance->row,
                                sprite_width, sprite_height};
    Rectangle target =
        (Rectangle){roundf(position.x), roundf(position.y),
                    sprite_width * scale, sprite_height * scale};
    DrawTexturePro(instance->texture->texture_atlas, src, target,
                   (Vector2){floorf(target.width / 2.0f),
                             floorf(target.height / 2.0f)},
                   rotation, tint);
}

f32 total_animation_time(const AnimatedTexture2D *texture) {
    const f32 seconds_per_frame = 1.0f / texture->fps;
    return seconds_per_frame * texture->frames;
}

i32 draw_animated_texture_ex(AnimatedTexture2DInstance *instance,
                             float delta, Vector2 position, float rotation,
                             float scale, Colour tint) {
    const f32 new_animation_time =
        fmodf(instance->animation_time + delta,
              total_animation_time(instance->texture));
    draw_animated_texture_ex_nostep(instance, position, rotation, scale,
                                    tint);
    int result = new_animation_time < instance->animation_time;
    instance->animation_time = new_animation_time;
    return result;
}

// BUG: There's a good chance this is bugged since I've not tested it lmao
void draw_animated_texture_pro_nostep(AnimatedTexture2DInstance *instance,
                                      Rectangle dest, Vector2 origin,
                                      float rotation, Colour tint) {
    assert(instance->texture->fps > 0);
    assert(instance->texture->frames > 0);
    if (instance->texture->fps == 0 || instance->texture->frames == 0) {
        log_error(
            "The texture identified with %u has a 0 frame or 0 fps set. "
            "Check the texture configurations!",
            instance->texture->texture_atlas.id);
        return;
    }
    const float seconds_per_frame = 1.0f / instance->texture->fps;
    const Vector2 sprite_sizes =
        animated_texture_frame_size(instance->texture);
    const float sprite_width = sprite_sizes.x;
    const float sprite_height = sprite_sizes.y;
    const int frame_to_draw =
        (int)floorf(instance->animation_time / seconds_per_frame);
    Rectangle src = (Rectangle){frame_to_draw * sprite_width,
                                sprite_height * instance->row,
                                sprite_width, sprite_height};
    DrawTexturePro(instance->texture->texture_atlas, src, dest, origin,
                   rotation, tint);
}

i32 draw_animated_texture_pro(AnimatedTexture2DInstance *instance,
                              float delta, Rectangle dest, Vector2 origin,
                              float rotation, Colour tint) {
    const f32 new_animation_time =
        fmodf(instance->animation_time + delta,
              total_animation_time(instance->texture));
    draw_animated_texture_pro_nostep(instance, dest, origin, rotation,
                                     tint);
    const i32 result = new_animation_time < instance->animation_time;
    instance->animation_time = new_animation_time;
    return result;
}

AnimatedTexture2D load_animated_texture(const char *filepath,
                                        size_t frames, size_t rows,
                                        unsigned int fps) {
    AnimatedTexture2D texture = {
        .frames = frames, .rows = rows, .fps = fps};
    texture.texture_atlas = LoadTexture(filepath);
    return texture;
}

void unload_animated_texture(AnimatedTexture2D *animated_texture) {
    UnloadTexture(animated_texture->texture_atlas);
    animated_texture->texture_atlas = (Texture2D){0};
    animated_texture->frames = 0;
    animated_texture->rows = 0;
    animated_texture->fps = 0;
}

Vector2 animated_texture_frame_size(const AnimatedTexture2D *texture) {
    return (Vector2){(float)texture->texture_atlas.width / texture->frames,
                     (float)texture->texture_atlas.height / texture->rows};
}

void draw_outlined_text_ex(const char *text, Font font, Vector2 position,
                           float font_size, float spacing, Colour colour,
                           Colour outline_colour, int outline_size) {
    for (int dx = -outline_size; dx <= outline_size; dx++) {
        for (int dy = -outline_size; dy <= outline_size; dy++) {
            if (dx == 0 && dy == 0)
                continue;
            DrawTextEx(font, text,
                       (Vector2){position.x + dx, position.y + dy},
                       font_size, spacing, outline_colour);
        }
    }
    DrawTextEx(font, text, position, font_size, spacing, colour);
}

// Remember that all of these internal helper functions use radians.

float ring_get_angle_per_thing(int count_things_in_ring) {
    return TAU / (float)count_things_in_ring;
}

float arc_get_angle_per_thing(int count_things_in_arc, float arc_angle) {
    return arc_angle / ((float)count_things_in_arc - 1.0);
}

float ring_get_thing_angle_for_i(float get_angle_per_thing, int i,
                                 float rotation) {
    return (get_angle_per_thing * i) + rotation;
}

float arc_get_thing_angle_for_i(float angle_per_thing, int i,
                                float rotation, float arc_angle) {
    return (angle_per_thing * (float)i) + rotation - (arc_angle * 0.5f);
}

void draw_cool_hexagon_thing(Vector2 at, Colour colour) {
    DrawRing(at, 15.0f, 18.0f, 0, 360.0f, 6, Fade(colour, 0.66f));
    DrawRing(at, 11.0f, 13.0f, 0, 360.0f, 6, Fade(colour, 0.33f));
}

const char *time_format(float time, bool ms) {
    const i32 minutes = (int)floorf(time / 60.0f);
    const i32 seconds = (int)floorf(fmodf(time, 60.0f));
    i32 millis = 0;
    if (ms) {
        millis = (int)floorf(100.0f * fmodf(time, 1.0f));
        return TextFormat("%02d:%02d.%02d", minutes, seconds, millis);
    }
    return TextFormat("%02d:%02d", minutes, seconds);
}

bool test_rectangle_offscreen(Rectangle rect) {
    const f32 xend = rect.x + rect.width;
    const f32 yend = rect.y + rect.height;

    return rect.x >= VIEWPORT_WIDTH || xend < 0 ||
           rect.y >= VIEWPORT_HEIGHT || yend < 0;
}

f32 random_float(void) {
    return (float)GetRandomValue(0, INT_MAX) / (float)INT_MAX;
}

static int get_kb_input(GameInput input) {
    switch (input) {
    case INPUT_SHOT:
        return KEY_Z;
    case INPUT_BOMB:
        return KEY_X;
    case INPUT_SLOW:
        return KEY_LEFT_SHIFT;
    case INPUT_PAUSE:
        return KEY_ESCAPE;
    case INPUT_LEFT:
        return KEY_LEFT;
    case INPUT_RIGHT:
        return KEY_RIGHT;
    case INPUT_UP:
        return KEY_UP;
    case INPUT_DOWN:
        return KEY_DOWN;
    default:
        return KEY_NULL;
    }
}

static int get_gamepad_input(GameInput input) {
    switch (input) {
    case INPUT_SHOT:
        return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
    case INPUT_BOMB:
        return GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
    case INPUT_SLOW:
        return GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
    case INPUT_PAUSE:
        return GAMEPAD_BUTTON_MIDDLE_RIGHT;
    case INPUT_LEFT:
        return GAMEPAD_BUTTON_LEFT_FACE_LEFT;
    case INPUT_RIGHT:
        return GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
    case INPUT_UP:
        return GAMEPAD_BUTTON_LEFT_FACE_UP;
    case INPUT_DOWN:
        return GAMEPAD_BUTTON_LEFT_FACE_DOWN;
    default:
        return GAMEPAD_BUTTON_UNKNOWN;
    }
}

bool is_input_down(GameInput input) {
    return IsKeyDown(get_kb_input(input)) ||
           IsGamepadButtonDown(0, get_gamepad_input(input));
}
bool is_input_just_pressed(GameInput input) {
    return IsKeyPressed(get_kb_input(input)) ||
           IsGamepadButtonPressed(0, get_gamepad_input(input));
}
