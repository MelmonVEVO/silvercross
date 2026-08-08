#include "assets.h"
#include "utils.h"

Assets assets = {};

void load_assets(void) {
    assets.fonts.fusion = LoadFontEx(
        "fonts/fusion-pixel-10px-proportional-ja.ttf", 14, 0, 0);
    assets.textures.bullets1 = load_animated_texture(
        "sprites/unowned_enemy_bullet_small.png", 3, 9, 12);
}
