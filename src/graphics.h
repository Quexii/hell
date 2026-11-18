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

static inline void RDG_Line(Batch *batch, float x, float y, float ex, float ey, Vec3 color) {
    VertexDebug_W *out = Batch_Reserve(batch, 2);
    if (out) {
        out[0] = (VertexDebug_W){
            { x, y, 0.0f },
            color
        };
        out[1] = (VertexDebug_W){
            { ex, ey, 0.0f },
            color
        };
    }
}

static inline void RDG_Rect(Batch *batch, float x, float y, float w, float h, Vec3 color) {
    VertexDebug_W *out = Batch_Reserve(batch, 8);
    if (out) {
        out[0] = (VertexDebug_W){
            { x, y, 0.0f },
            color
        };
        out[1] = (VertexDebug_W){
            { x + w, y, 0.0f },
            color
        };

        out[2] = (VertexDebug_W){
            { x + w, y, 0.0f },
            color
        };
        out[3] = (VertexDebug_W){
            { x + w, y + h, 0.0f },
            color
        };

        out[4] = (VertexDebug_W){
            { x + w, y + h, 0.0f },
            color
        };
        out[5] = (VertexDebug_W){
            { x, y + h, 0.0f },
            color
        };

        out[6] = (VertexDebug_W){
            { x, y + h, 0.0f },
            color
        };
        out[7] = (VertexDebug_W){
            { x, y, 0.0f },
            color
        };
    }
}

static inline void RDG_Circle(Batch *batch, float x, float y, float r, Vec3 color) {
    const uint32_t segments = 40;
    VertexDebug_W *out = Batch_Reserve(batch, segments * 2);

    if (out) {
        for (uint32_t i = 0; i < segments; i++) {
            float a0 = ToRadians(((float) i / segments) * 360.0f);
            float a1 = ToRadians(((float) (i + 1) / segments) * 360.0f);

            out[i * 2] = (VertexDebug_W){
                { x + sinf(a0) * r, y + cosf(a0) * r, 0.0f },
                color
            };

            out[i * 2 + 1] = (VertexDebug_W){
                { x + sinf(a1) * r, y + cosf(a1) * r, 0.0f },
                color
            };
        }
    }
}

#endif // GAME_GRAPHICS_H
