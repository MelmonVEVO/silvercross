#ifndef UTILS_H
#define UTILS_H

#include "primitives.h"
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
    u16 frames;
    u16 rows;
    u16 fps;
} AnimatedTexture2D;

typedef struct {
    const AnimatedTexture2D *texture;
    f32 animation_time;
    u16 row;
} AnimatedTexture2DInstance;

typedef u32 Seed;

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
                                        u32 fps);

void unload_animated_texture(AnimatedTexture2D *animated_texture);

// Applies linear acceleration to a velocity.
void accelerate(Vector2 *velocity, const f32 acceleration,
                const f32 delta);

// Updates a position vector based on velocity (could also be used for
// applying a vector acceleration to a velocity).
void move(Vector2 *position, const Vector2 velocity, f32 delta);

Rectangle create_centred_rectangle(const f32 x, const f32 y,
                                   const Vector2 sizes);

static inline Vector2 rectangle_centre(const Rectangle rectangle) {
    return (Vector2){rectangle.x + (rectangle.width / 2.0f),
                     rectangle.y + (rectangle.height / 2.0f)};
}

void draw_progress_bar(Rectangle bar, f32 percentage, Colour bgcolour,
                       Colour fillcolour);

void log_info(const char *format, ...);

void log_warning(const char *format, ...);

void log_error(const char *format, ...);

void draw_outlined_text_ex(const char *text, Font font, Vector2 position,
                           f32 font_size, f32 spacing, Colour colour,
                           Colour outline_colour, i32 outline_size);

f32 random_float(void);

void draw_centred_texture(Texture2D texture, Vector2 at);

void draw_centred_texture_ex(Texture2D texture, Vector2 at, f32 rotation,
                             f32 scale, Colour tint);

// Draws an animated texture, with no frame time step. The sprite will be
// drawn CENTRAL to the position.
void draw_animated_texture_ex_nostep(AnimatedTexture2DInstance *instance,
                                     Vector2 position, f32 rotation,
                                     f32 scale, Colour tint);

// Draws an animated texture and steps through the frame time. The sprite
// will be CENTRAL to the position. Returns 0 if the texture didn't loop, 1
// if it did
i32 draw_animated_texture_ex(AnimatedTexture2DInstance *instance,
                             f32 delta, Vector2 position, f32 rotation,
                             f32 scale, Colour tint);

void draw_animated_texture_pro_nostep(AnimatedTexture2DInstance *instance,
                                      Rectangle dest, Vector2 origin,
                                      f32 rotation, Colour tint);

// Note that this performs no centring of destination rectangle.
i32 draw_animated_texture_pro(AnimatedTexture2DInstance *instance,
                              f32 delta, Rectangle dest, Vector2 origin,
                              f32 rotation, Colour tint);

// Returns the size of a single frame in an animated texture.
Vector2 animated_texture_frame_size(const AnimatedTexture2D *texture);

// Returns the total time it takes to animate a texture once.
f32 total_animation_time(const AnimatedTexture2D *texture);

// Provides the angle in degrees from from to to.
static inline f32 front_towards_whatever(Vector2 from, Vector2 to) {
    Vector2 direction = Vector2Subtract(to, from);
    f32 angle = atan2f(direction.y, direction.x);
    return angle * RAD2DEG;
}

// Check if a vector's magnitude is higher than a target.
static inline bool check_magnitudes_higher(Vector2 vec, f32 target) {
    return Vector2LengthSqr(vec) > target * target;
}

// Check if a vector's magnitude is higher than a target.
static inline bool check_magnitudes_lower(Vector2 vec, f32 target) {
    return Vector2LengthSqr(vec) < target * target;
}

f32 ring_get_angle_per_thing(i32 count_things_in_ring);

f32 arc_get_angle_per_thing(i32 count_things_in_arc, f32 arc_angle);

f32 ring_get_thing_angle_for_i(f32 get_angle_per_thing, i32 i,
                               f32 rotation);

f32 arc_get_thing_angle_for_i(f32 angle_per_thing, i32 i, f32 rotation,
                              f32 arc_angle);

static inline f32 inverse_lerp(f32 min, f32 max, f32 val) {
    return Clamp((val - min) / (max - min), 0, 1.0f);
}

static inline Vector2 vector_projection(Vector2 a, Vector2 b) {
    f32 numerator = Vector2DotProduct(a, b);
    f32 denominator = Vector2DotProduct(b, b);
    return Vector2Scale(b, numerator / denominator);
}

static inline Vector2 vector_rejection(Vector2 a, Vector2 b) {
    return Vector2Subtract(a, vector_projection(a, b));
}

void draw_cool_hexagon_thing(Vector2 at, Colour colour);

static inline f32 quint_ease(f32 x) {
    return Clamp(1 - powf(1 - x, 5), 0, 1.0f);
}

static inline f32 cubic_ease(f32 x) {
    return Clamp(1 - powf(1 - x, 3), 0, 1.0f);
}

static inline Vector2 get_spline_poi32_cubic_bezier(CubicBezier curve,
                                                    f32 progress) {
    return GetSplinePointBezierCubic(curve.start, curve.start_control,
                                     curve.end_control, curve.end,
                                     progress);
}

static inline Vector2
get_spline_poi32_quadratic_bezier(QuadraticBezier curve, f32 progress) {
    return GetSplinePointBezierQuad(curve.start, curve.control, curve.end,
                                    progress);
}

bool test_rectangle_offscreen(Rectangle rect);

// Returns a string of a time formatted as 00:00.00 (min:sec.ms)
// Milliseconds optional.
// This function uses Raylib's TextFormat function, therefore
// it uses that function's i32ernal buffer.
const char *time_format(f32 time, bool ms);

static inline f32 smoothstep(const f32 edge0, const f32 edge1,
                             const f32 x) {
    f32 reduced_x = Clamp((x - edge0) / (edge1 - edge0), 0, 1.0f);

    return (3.0f * reduced_x * reduced_x) -
           (2.0f * reduced_x * reduced_x * reduced_x);
}

#endif // UTILS_H
