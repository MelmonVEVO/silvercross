#ifndef UTILS_H
#define UTILS_H

#include <raylib.h>

typedef Color Colour; // ;)

// Applies linear acceleration to a velocity.
void accelerate(Vector2 *velocity, const float acceleration,
                const float delta);

// Updates a position vector based on velocity (could also be used for
// applying a vector acceleration to a velocity).
void move(Vector2 *position, const Vector2 velocity, float delta);

void log_info(const char *format, ...);

void log_warning(const char *format, ...);

void log_error(const char *format, ...);

Rectangle create_centred_rectangle(const float x, const float y,
                                   const Vector2 sizes);

// Returns true if a rectangle is completely off screen.
bool test_rectangle_offscreen(Rectangle rect);

void draw_outlined_text_ex(const char *text, Font font, Vector2 position,
                           float font_size, float spacing, Colour colour,
                           Colour outline_colour, int outline_size);

#endif // UTILS_H
