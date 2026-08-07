#include "utils.h"
#include "constants.h"
#include "primitives.h"
#include <raymath.h>
#include <stdio.h>

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

Rectangle create_centred_rectangle(const float x, const float y,
                                   const Vector2 sizes) {
    return (Rectangle){x - (sizes.x * 0.5f), y - (sizes.y * 0.5f), sizes.x,
                       sizes.y};
}

bool test_rectangle_offscreen(Rectangle rect) {
    f32 xend = rect.x + rect.width;
    f32 yend = rect.y + rect.height;

    return rect.x >= VIEWPORT_WIDTH || xend < 0 ||
           rect.y >= VIEWPORT_HEIGHT || yend < 0;
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
