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

void accelerate(Vector2 *velocity, const float acceleration,
                const float delta) {
    if (Vector2LengthSqr(*velocity) == 0.0f)
        return;
    *velocity =
        Vector2Add(*velocity, Vector2Scale(Vector2Normalize(*velocity),
                                           acceleration * delta));
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

static bool is_latin_word_codepoint(int codepoint) {
    return (codepoint >= 0x21 && codepoint <= 0x7e) ||
           (codepoint >= 0xc0 && codepoint <= 0x24f) ||
           (codepoint >= 0x300 && codepoint <= 0x36f) ||
           (codepoint >= 0x1e00 && codepoint <= 0x1eff) ||
           (codepoint >= 0xa720 && codepoint <= 0xa7ff) ||
           (codepoint >= 0xab30 && codepoint <= 0xab6f);
}

Rectangle draw_text_with_overflow(const char *text, Font font,
                                  Vector2 position, float font_size,
                                  float spacing, Colour colour,
                                  float max_width, int newline_height) {
    Rectangle bounds = {position.x, position.y, 0.0f, 0.0f};
    if (text == NULL || text[0] == '\0' || font_size <= 0.0f ||
        max_width <= 0.0f)
        return bounds;

    if (font.texture.id == 0)
        font = GetFontDefault();

    int codepoint_count = 0;
    int *codepoints = LoadCodepoints(text, &codepoint_count);
    if (codepoints == NULL || codepoint_count == 0) {
        UnloadCodepoints(codepoints);
        return bounds;
    }

    size_t wrapped_capacity = (size_t)codepoint_count * 2 + 1;
    int *wrapped_text = malloc(wrapped_capacity * sizeof(*wrapped_text));
    if (wrapped_text == NULL) {
        UnloadCodepoints(codepoints);
        log_error("Failed to allocate wrapped text buffer");
        return bounds;
    }

    int wrapped_count = 0;
    int line_start = 0;
    bool line_has_text = false;

    for (int i = 0; i < codepoint_count;) {
        if (codepoints[i] == '\n') {
            wrapped_text[wrapped_count++] = '\n';
            line_start = wrapped_count;
            line_has_text = false;
            i++;
            continue;
        }

        int separator_start = i;
        while (i < codepoint_count &&
               (codepoints[i] == ' ' || codepoints[i] == '\t' ||
                codepoints[i] == 0x3000))
            i++;
        int separator_count = i - separator_start;

        if (i == codepoint_count || codepoints[i] == '\n')
            continue;

        int token_start = i;
        int codepoint = codepoints[i++];
        bool latin_run = is_latin_word_codepoint(codepoint);

        if (latin_run) {
            while (i < codepoint_count) {
                codepoint = codepoints[i];
                if (!is_latin_word_codepoint(codepoint))
                    break;
                i++;
            }
        }
        int token_count = i - token_start;
        int before_token = wrapped_count;

        if (line_has_text) {
            for (int j = 0; j < separator_count; j++)
                wrapped_text[wrapped_count++] =
                    codepoints[separator_start + j];
        }
        for (int j = 0; j < token_count; j++)
            wrapped_text[wrapped_count++] = codepoints[token_start + j];

        float candidate_width =
            MeasureTextCodepoints(font, wrapped_text + line_start,
                                  wrapped_count - line_start, font_size,
                                  spacing)
                .x;
        if (line_has_text && candidate_width > max_width) {
            wrapped_count = before_token;
            wrapped_text[wrapped_count++] = '\n';
            line_start = wrapped_count;
            for (int j = 0; j < token_count; j++)
                wrapped_text[wrapped_count++] =
                    codepoints[token_start + j];
        }
        line_has_text = true;
    }

    int first = 0;
    int line = 0;
    for (int i = 0; i <= wrapped_count; i++) {
        if (i < wrapped_count && wrapped_text[i] != '\n')
            continue;

        int line_length = i - first;
        if (line_length > 0) {
            Vector2 line_size =
                MeasureTextCodepoints(font, wrapped_text + first,
                                      line_length, font_size, spacing);
            bounds.width = MAX(bounds.width, line_size.x);
            DrawTextCodepoints(
                font, wrapped_text + first, line_length,
                (Vector2){position.x,
                          position.y + (float)(line * newline_height)},
                font_size, spacing, colour);
        }
        line++;
        first = i + 1;
    }
    bounds.height = font_size + (float)((line - 1) * newline_height);

    free(wrapped_text);
    UnloadCodepoints(codepoints);
    return bounds;
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
