/*
 * utils
 *
 * A pick-and-mix of useful utility functions and type
 * definitions, including maths, maths, linear algebra
 * and more maths!
 *
 * Copyright (c) 2026 MELMON PROJECT. All Rights Reserved.
 */
#ifndef UTILS_H
#define UTILS_H

#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ARRAYLEN(arr) (sizeof((arr)) / sizeof((arr)[0]))
// Uses radians.
#define VEC2FROMANGLE(angle, magnitude)                                   \
    Vector2Scale((Vector2){cosf(angle), sinf(angle)}, magnitude)

typedef Color Colour; // ;)

typedef struct {
    Texture2D texture_atlas;
    unsigned short frames;
    unsigned short rows;
    unsigned short fps;
} AnimatedTexture2D;

typedef struct {
    const AnimatedTexture2D *texture;
    float animation_time;
    unsigned short row;
} AnimatedTexture2DInstance;

typedef unsigned int Seed;

typedef struct {
    Vector2 start;
    Vector2 control;
    Vector2 end;
} QuadraticBezier;

typedef struct {
    Vector2 start;
    Vector2 start_control;
    Vector2 end_control;
    Vector2 end;
} CubicBezier;

AnimatedTexture2D load_animated_texture(const char *filepath,
                                        size_t frames, size_t rows,
                                        unsigned int fps);

void unload_animated_texture(AnimatedTexture2D *animated_texture);

// Applies linear acceleration to a velocity.
void accelerate(Vector2 *velocity, const float acceleration,
                const float delta);

// Updates a position vector based on velocity (could also be used for
// applying a vector acceleration to a velocity).
void move(Vector2 *position, const Vector2 velocity, float delta);

Rectangle create_centred_rectangle(const float x, const float y,
                                   const Vector2 sizes);

static inline Vector2 rectangle_centre(const Rectangle rectangle) {
    return (Vector2){rectangle.x + (rectangle.width / 2.0f),
                     rectangle.y + (rectangle.height / 2.0f)};
}

void draw_progress_bar(Rectangle bar, float percentage, Colour bgcolour,
                       Colour fillcolour);

void log_info(const char *format, ...);

void log_warning(const char *format, ...);

void log_error(const char *format, ...);

void draw_outlined_text_ex(const char *text, Font font, Vector2 position,
                           float font_size, float spacing, Colour colour,
                           Colour outline_colour, int outline_size);

// Draw text to the screen at position. Text will go to a new line if the
// text's horizontal space would exceed max_width. Text will not break
// words written in Latin characters. Japanese text may be broken.
// Returns a Rectangle that represents the bounding box of the text.
Rectangle draw_text_with_overflow(const char *text, Font font,
                                  Vector2 position, float font_size,
                                  float spacing, Colour colour,
                                  float max_width, int newline_height);

float random_float(void);

void draw_centred_texture(Texture2D texture, Vector2 at);

void draw_centred_texture_ex(Texture2D texture, Vector2 at, float rotation,
                             float scale, Colour tint);

// Draws an animated texture, with no frame time step. The sprite will be
// drawn CENTRAL to the position.
void draw_animated_texture_ex_nostep(AnimatedTexture2DInstance *instance,
                                     Vector2 position, float rotation,
                                     float scale, Colour tint);

// Draws an animated texture and steps through the frame time. The sprite
// will be CENTRAL to the position. Returns 0 if the texture didn't loop, 1
// if it did
int draw_animated_texture_ex(AnimatedTexture2DInstance *instance,
                             float delta, Vector2 position, float rotation,
                             float scale, Colour tint);

void draw_animated_texture_pro_nostep(AnimatedTexture2DInstance *instance,
                                      Rectangle dest, Vector2 origin,
                                      float rotation, Colour tint);

// Note that this performs no centring of destination rectangle.
int draw_animated_texture_pro(AnimatedTexture2DInstance *instance,
                              float delta, Rectangle dest, Vector2 origin,
                              float rotation, Colour tint);

// Returns the size of a single frame in an animated texture.
Vector2 animated_texture_frame_size(const AnimatedTexture2D *texture);

// Returns the total time it takes to animate a texture once.
float total_animation_time(const AnimatedTexture2D *texture);

// Provides the angle in degrees from from to to.
static inline float front_towards_whatever(Vector2 from, Vector2 to) {
    Vector2 direction = Vector2Subtract(to, from);
    float angle = atan2f(direction.y, direction.x);
    return angle * RAD2DEG;
}

// Check if a vector's magnitude is higher than a target.
static inline bool check_magnitudes_higher(Vector2 vec, float target) {
    return Vector2LengthSqr(vec) > target * target;
}

// Check if a vector's magnitude is higher than a target.
static inline bool check_magnitudes_lower(Vector2 vec, float target) {
    return Vector2LengthSqr(vec) < target * target;
}

float ring_get_angle_per_thing(int count_things_in_ring);

float arc_get_angle_per_thing(int count_things_in_arc, float arc_angle);

float ring_get_thing_angle_for_i(float get_angle_per_thing, int i,
                                 float rotation);

float arc_get_thing_angle_for_i(float angle_per_thing, int i,
                                float rotation, float arc_angle);

static inline float inverse_lerp(float min, float max, float val) {
    return Clamp((val - min) / (max - min), 0, 1.0f);
}

static inline Vector2 vector_projection(Vector2 a, Vector2 b) {
    float numerator = Vector2DotProduct(a, b);
    float denominator = Vector2DotProduct(b, b);
    return Vector2Scale(b, numerator / denominator);
}

static inline Vector2 vector_rejection(Vector2 a, Vector2 b) {
    return Vector2Subtract(a, vector_projection(a, b));
}

void draw_cool_hexagon_thing(Vector2 at, Colour colour);

static inline float quint_ease(float x) {
    return Clamp(1 - powf(1 - x, 5), 0, 1.0f);
}

static inline float cubic_ease(float x) {
    return Clamp(1 - powf(1 - x, 3), 0, 1.0f);
}

static inline Vector2 get_spline_point_cubic_bezier(CubicBezier curve,
                                                    float progress) {
    return GetSplinePointBezierCubic(curve.start, curve.start_control,
                                     curve.end_control, curve.end,
                                     progress);
}

static inline Vector2
get_spline_point_quadratic_bezier(QuadraticBezier curve, float progress) {
    return GetSplinePointBezierQuad(curve.start, curve.control, curve.end,
                                    progress);
}

bool test_rectangle_offscreen(Rectangle rect);

// Returns a string of a time formatted as 00:00.00 (min:sec.ms)
// Milliseconds optional.
// This function uses Raylib's TextFormat function, therefore
// it uses that function's internal buffer.
const char *time_format(float time, bool ms);

#endif // UTILS_H
