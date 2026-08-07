#ifndef UTILS_H
#define UTILS_H

#include <raylib.h>

// Applies linear acceleration to a velocity.
void accelerate(Vector2 *velocity, const float acceleration,
                const float delta);

// Updates a position vector based on velocity (could also be used for
// applying a vector acceleration to a velocity).
void move(Vector2 *position, const Vector2 velocity, float delta);

void log_info(const char *format, ...);

void log_warning(const char *format, ...);

void log_error(const char *format, ...);

#endif // UTILS_H
