#ifndef ENGINE_RENDERER_H
#define ENGINE_RENDERER_H

#include "gpu/window.h"
#include "gpu/gpu.h"
#include "maths.h"
#include "mesh.h"

#include <stdbool.h>
#include <stdint.h>

#define RENDER_LAYERS 16

typedef struct Renderer2D Renderer2D;

typedef struct {
    Mat4 model, view, proj;
} Matrices;

typedef void (*ExplicitDrawFn)(Renderer2D *r, RenderPass *pass, void *user);

typedef struct {
    GraphicsPipeline pipeline;
    Texture *texture, *target;
    Sampler *sampler;
    Buffer *lookup;
    uint32_t stride, capacity, lookupSize;
    bool instanced;
    ExplicitDrawFn explicitDrawFn;
    void *explicitDrawData;
} BatchDesc;

typedef struct {
    GraphicsPipeline pipeline;
    Texture *texture, *target;
    Sampler *sampler;
    Buffer *lookup;

    Buffer gpu;
    uint8_t *cpu;
    uint32_t stride, capacity, count;
    bool instanced;
    ExplicitDrawFn explicitDrawFn;
    Mat4 cam;
    void *explicitDrawData;
} Batch;

struct Renderer2D {
    Batch **batches;
    uint32_t batchCount;
    Window *window;
    Mesh quad;
    UploadArena arena;
};

bool Batch_Create(Batch *out, Device device, const BatchDesc *desc);
void Batch_Clear(Batch *b);
void Batch_SetViewport(Batch *b, float width, float height);
void *Batch_Reserve(Batch *b, uint32_t n);
void Batch_Release(Batch *b);

Renderer2D R2D_Create(Batch **batches, Window *window, uint32_t batchCount);
bool R2D_Init(Renderer2D *r, CopyPass *pass);
void R2D_Begin(Renderer2D *r);
void R2D_Upload(Renderer2D *r, CommandBuffer *cmd);
void R2D_Draw(Renderer2D *r, CommandBuffer *cmd);
void R2D_Destroy(Renderer2D *r);
#endif // ENGINE_RENDERER_H
