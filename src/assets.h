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
    } textures;
} Assets;

extern Assets assets;

void load_assets(void);

#endif
