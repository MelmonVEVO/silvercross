#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"

typedef struct {
    struct {
        Font fusion;
    } fonts;
} Assets;

extern Assets assets;

void load_assets(void);

#endif
