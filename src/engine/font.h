#ifndef ENGINE_FONT_H
#define ENGINE_FONT_H

#include "gpu/gpu.h"
#include "maths.h"
#include <stdbool.h>
#include <stdint.h>

#define FONT_MAX_GLYPHS 128

typedef struct {
    Rect uv;
    Vec2 size, offset;
    float advance;
    bool valid;
} Glyph;

typedef struct {
    Glyph glyphs[FONT_MAX_GLYPHS];

    float lineHeight, baseline;

    uint32_t atlasWidth, atlasHeight;

    Texture *texture;
    Sampler *sampler;
} Font;

bool Font_LoadStatic(Font *font, Texture *texture, Sampler *sampler, const char *metricesPath);

#endif // ENGINE_FONT_H
