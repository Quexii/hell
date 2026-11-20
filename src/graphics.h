#ifndef GAME_GRAPHICS_H
#define GAME_GRAPHICS_H

#include "engine/font.h"
#include "engine/gpu/gpu.h"
#include "engine/gpu/window.h"
#include "engine/maths.h"
#include "engine/renderer.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    Vec3 position;
    Vec3 color;
} VertexDebug_W;

typedef struct {
    GraphicsPipeline bulletPipeline, uiPipeline, debugWorldPipeline;
    Sampler samplerLinear, samplerNearest;
    Texture white, atlasBullets, atlasGeneric, fontCinzelTexture, fontRajdhaniTexture,
        fontMontserratTexture;
    Font fontCinzel, fontRajdhani, fontMonsterrat;
} Graphics;

bool Graphics_Init_Resources(Graphics *g, Window *window, Device device);
bool Graphics_Upload_Resources(Graphics *g, CopyPass *pass);
void Graphics_Release_Resources(Graphics *g, Device device);

static inline void
RDG_Line(Batch *b, float sx, float sy, float ex, float ey, float thickness, Vec3 color) {
    float dx = ex - sx, dy = ey - sy;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-6f)
        return;

    float h = thickness * 0.5f;
    float ux = dx / len * h, uy = dy / len * h;
    float nx = -uy, ny = ux;

    sx -= ux;
    sy -= uy;
    ex += ux;
    ey += uy;

    VertexDebug_W *o = Batch_Reserve(b, 6);
    if (!o)
        return;

    Vec3 p0 = { sx + nx, sy + ny, 0.f }, p1 = { sx - nx, sy - ny, 0.f };
    Vec3 p2 = { ex - nx, ey - ny, 0.f }, p3 = { ex + nx, ey + ny, 0.f };

    o[0] = (VertexDebug_W){ p0, color };
    o[1] = (VertexDebug_W){ p1, color };
    o[2] = (VertexDebug_W){ p2, color };
    o[3] = (VertexDebug_W){ p0, color };
    o[4] = (VertexDebug_W){ p2, color };
    o[5] = (VertexDebug_W){ p3, color };
}

static inline void
RDG_Rect(Batch *b, float x, float y, float w, float h, float rotation, float thickness, Vec3 color) {
    float c = cosf(rotation);
    float s = sinf(rotation);
    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;
    float hx = w * 0.5f;
    float hy = h * 0.5f;
    Vec2 corners[4] = {
        { -hx, -hy },
        { hx, -hy },
        { hx, hy },
        { -hx, hy },
    };

    for (uint32_t i = 0; i < 4; i++) {
        Vec2 a = corners[i];
        Vec2 z = corners[(i + 1) % 4];
        float ax = cx + a.x * c - a.y * s;
        float ay = cy + a.x * s + a.y * c;
        float zx = cx + z.x * c - z.y * s;
        float zy = cy + z.x * s + z.y * c;
        RDG_Line(b, ax, ay, zx, zy, thickness, color);
    }
}

static inline void RDG_Circle(Batch *b, float x, float y, float r, float thickness, Vec3 color) {
    uint32_t segs = (uint32_t) (r * 1.5f);
    if (segs < 12)
        segs = 12;
    if (segs > 128)
        segs = 128;

    float ax = x, ay = y + r;
    for (uint32_t i = 1; i <= segs; i++) {
        float a = ToRadians(((float) i / segs) * 360.0f);
        float bx = x + sinf(a) * r, by = y + cosf(a) * r;
        RDG_Line(b, ax, ay, bx, by, thickness, color);
        ax = bx;
        ay = by;
    }
}

#endif // GAME_GRAPHICS_H
