#include "assets.h"

Assets assets = {};

void load_assets(void) {
    assets.fonts.fusion = LoadFontEx(
        "fonts/fusion-pixel-10px-proportional-ja.ttf", 14, 0, 0);
}
