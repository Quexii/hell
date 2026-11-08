#ifndef ENGINE_SPRITE_H
#define ENGINE_SPRITE_H

#include "engine/maths.h"
#include <stdint.h>

typedef struct {
    uint32_t x, y, w, h;
    Vec4 uv;
} SpriteRegion;

#endif /* ifndef ENGINE_SPRITE_H */
