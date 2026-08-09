#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"
#include "utils.h"

typedef struct {
    struct {
        Font fusion;
    } fonts;
    struct {
        AnimatedTexture2D bullets1;
        AnimatedTexture2D bullets2;
        AnimatedTexture2D playershots;
        Texture2D tempplayer;
        Texture2D medal;
        Texture2D hud;
    } textures;
    struct {
        Sound medal_collect;
        Sound bonk;
    } sfx;
} Assets;

extern Assets assets;

void load_assets(void);

#endif
