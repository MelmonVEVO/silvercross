#include "assets.h"
#include "raylib.h"
#include "utils.h"

Assets assets = {};

void load_assets(void) {
    assets.fonts.fusion = LoadFontEx(
        "fonts/fusion-pixel-10px-proportional-ja.ttf", 14, 0, 0);
    assets.textures.bullets1 =
        load_animated_texture("sprites/bullets1.png", 3, 9, 12);
    assets.sfx.medal_collect = LoadSound("audio/sfx/cursor05.wav");
    assets.textures.playershots =
        load_animated_texture("sprites/playershots.png", 1, 2, 1);
    assets.textures.tempplayer = LoadTexture("sprites/player.png");
    SetSoundPitch(assets.sfx.medal_collect, 1.35f);
    SetSoundVolume(assets.sfx.medal_collect, 0.05f);
    assets.textures.medal = LoadTexture("sprites/medal.png");
    assets.sfx.bonk = LoadSound("audio/sfx/bonk.wav");
    assets.textures.hud = LoadTexture("sprites/hud.png");
    assets.textures.seal = LoadTexture("sprites/seal.png");
    assets.textures.playershot_impact =
        load_animated_texture("sprites/playerbullet_impact.png", 7, 1, 30);
    assets.textures.star = LoadTexture("sprites/bullet_star.png");
}
