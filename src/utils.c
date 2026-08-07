#include "utils.h"
#include "constants.h"
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
