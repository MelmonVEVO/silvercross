#include "assets.h"
#include "raylib.h"
#include "utils.h"

Assets assets = {};

void load_assets(void) {
    assets.fonts.fusion = LoadFontEx(
        "fonts/fusion-pixel-10px-proportional-ja.ttf", 14, 0, 0);
    assets.textures.bullets1 = load_animated_texture(
        "sprites/unowned_enemy_bullet_small.png", 3, 9, 12);
    assets.sfx.medal_collect = LoadSound("audio/sfx/cursor05.wav");
    assets.textures.playershots =
        load_animated_texture("sprites/playershots.png", 1, 2, 1);
    assets.textures.tempplayer = LoadTexture("sprites/player.png");
    SetSoundPitch(assets.sfx.medal_collect, 1.35f);
    SetSoundVolume(assets.sfx.medal_collect, 0.05f);
}
