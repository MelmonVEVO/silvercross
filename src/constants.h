#ifndef CONSTANTS_H
#define CONSTANTS_H

#define VECTOR2UP (Vector2){0, -1.0f}
#define VECTOR2DOWN (Vector2){0, 1.0f}
#define VECTOR2LEFT (Vector2){-1.0f, 0}
#define VECTOR2RIGHT (Vector2){1.0f, 0}
#define TAU (PI * 2)

#define VIEWPORT_WIDTH 240
#define VIEWPORT_HEIGHT 320

#define PLAYER_SPEED 158.0f
#define PLAYER_FOCUS_SPEED 90.0f
#define PLAYER_FIRE_TIME 0.3f
#define PLAYER_FIRE_RATE 0.06f
#define PLAYER_MOVEMENT_BOUNDS_UNITS 7.0f
#define PLAYER_COUNTERBOMB_WINDOW_FRAMES 8
#define PLAYER_INVINCIBILITY_TIME 2.0f

#define TERM_INFO "\x1b[36m"
#define TERM_WARNING "\x1b[33m"
#define TERM_ERROR "\x1b[31m"
#define TERM_NORMAL "\x1b[0m"

#define MAX_ENTITIES 120000

#define FRAMERATE 60

#endif // CONSTANTS_H
