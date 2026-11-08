#include "renderer.h"

#include "gpu/gpu.h"
#include "maths.h"

#include <stdio.h>
#include <stdlib.h>

#define UPLOAD_ALIGN 16
#define ARENA_SLACK 4096

bool Batch_Create(Batch *out, Device device, const BatchDesc *desc) {
    *out = (Batch){ 0 };

    if (!desc->capacity || !desc->stride || !desc->pipeline)
        return false;

    size_t bytes = (size_t) desc->capacity * desc->stride;

    uint8_t *cpu = malloc(bytes);
    if (!cpu)
        return false;

    Buffer gpu =
        GPU_CreateBuffer(device, &(BufferDesc){ .size = bytes, .usage = BUFFERUSAGE_VERTEX });
    if (!gpu.handle) {
        free(cpu);
        return false;
    }

    *out = (Batch){
        .pipeline = desc->pipeline,
        .texture = desc->texture,
        .target = desc->target,
        .sampler = desc->sampler,
        .lookup = desc->lookup,
        .gpu = gpu,
        .cpu = cpu,
        .stride = desc->stride,
        .capacity = desc->capacity,
        .count = 0,
        .instanced = desc->instanced,
        .explicitDrawFn = desc->explicitDrawFn,
        .explicitDrawData = desc->explicitDrawData,
        .cam = Mat4_Identity(),
    };
    return true;
}

void Batch_Clear(Batch *b) {
    b->count = 0;
}

void Batch_SetViewport(Batch *b, float width, float height) {
    b->cam = Mat4_Ortho(0.f, width, height, 0.f, -1.f, 1.f);
}

void *Batch_Reserve(Batch *b, uint32_t n) {
    if (b->count + n > b->capacity)
        return NULL;
    void *p = b->cpu + (size_t) b->count * b->stride;
    b->count += n;
    return p;
}

void Batch_Release(Batch *b) {
    if (b->gpu.handle)
        GPU_ReleaseBuffer(&b->gpu);
    free(b->cpu);
    *b = (Batch){ 0 };
}

Renderer2D R2D_Create(Batch **batches, Window *window, uint32_t batchCount) {
    return (Renderer2D){
        .batches = batches,
        .window = window,
        .batchCount = batchCount,
    };
}

bool R2D_Init(Renderer2D *r, CopyPass *pass) {
    size_t total = ARENA_SLACK;
    for (uint32_t i = 0; i < r->batchCount; i++)
        total += (size_t) r->batches[i]->capacity * r->batches[i]->stride + UPLOAD_ALIGN;

    if (!UploadArena_Init(&r->arena, pass->device, total))
        return false;

    r->quad = Mesh_CreateQuad(pass->device, pass, &r->arena);
    if (!r->quad.vb.handle) {
        UploadArena_Destroy(&r->arena);
        return false;
    }
    return true;
}

void R2D_Begin(Renderer2D *r) {
    for (uint32_t i = 0; i < r->batchCount; i++)
        Batch_Clear(r->batches[i]);
}

void R2D_Upload(Renderer2D *r, CommandBuffer *cmd) {
    if (r->batchCount == 0)
        return;

    UploadAllocation alloc[r->batchCount];
    bool has[r->batchCount];

    if (!UploadArena_Begin(&r->arena))
        return;

    for (uint32_t i = 0; i < r->batchCount; i++) {
        Batch *b = r->batches[i];
        has[i] = b->count > 0 &&
                 UploadArena_Write(
                     &r->arena, b->cpu, (size_t) b->count * b->stride, UPLOAD_ALIGN, &alloc[i]
                 );
    }
    UploadArena_End(&r->arena);

    CopyPass pass = GPU_BeginCopy(cmd);
    if (!pass.handle)
        return;

    for (uint32_t i = 0; i < r->batchCount; i++)
        if (has[i])
            UploadArena_UploadBuffer(&r->arena, &pass, &r->batches[i]->gpu, 0, alloc[i], true);

    GPU_EndCopy(&pass);
}

void R2D_Draw(Renderer2D *r, CommandBuffer *cmd) {
    if (!r || !cmd || !cmd->handle)
        return;

    Texture *currentTarget = NULL;
    bool hasPass = false;

    RenderPass pass = { 0 };

    Frame swapchainFrame = { 0 };
    bool swapchainAcquired = false;

    for (uint32_t i = 0; i < r->batchCount; i++) {
        Batch *b = r->batches[i];

        if (!b || b->count == 0)
            continue;

        if (!hasPass || b->target != currentTarget) {
            if (hasPass) {
                GPU_EndRenderPass(&pass);
                hasPass = false;
            }

            currentTarget = b->target;
            bool renderedBefore = false;

            for (uint32_t j = 0; j < i; j++) {
                Batch *previous = r->batches[j];

                if (previous && previous->count > 0 && previous->target == currentTarget) {
                    renderedBefore = true;
                    break;
                }
            }

            FrameDesc desc = {
                .texture = currentTarget,
                .clearColor = { 0.f, 0.f, 0.f, 1.f },
                .loadOp = renderedBefore ? LOADOP_LOAD : LOADOP_CLEAR,
                .storeOp = STOREOP_STORE
            };

            Frame frame = { 0 };

            if (!currentTarget && swapchainAcquired) {
                frame = swapchainFrame;
                frame.desc = &desc;
            } else {
                frame = GPU_BeginFrame(cmd, r->window->handle, &desc);

                if (!frame.swapchainTexture) {
                    SDL_Log("R2D_Draw: Failed to acquire render target: %s", SDL_GetError());
                    return;
                }

                if (!currentTarget) {
                    swapchainFrame = frame;
                    swapchainAcquired = true;
                }
            }

            pass = GPU_BeginRenderPass(&frame);

            if (!pass.handle) {
                SDL_Log("R2D_Draw: Failed to begin render pass: %s", SDL_GetError());
                return;
            }

            hasPass = true;

            GPU_PushVertexData(cmd, 0, &b->cam, sizeof(Mat4));
        }

        if (b->explicitDrawFn) {
            b->explicitDrawFn(r, &pass, b->explicitDrawData);
            continue;
        }

        GPU_BindPipeline(&pass, b->pipeline);

        if (b->instanced)
            Mesh_Bind(&pass, &r->quad);

        GPU_BindVBO(&pass, &b->gpu, 0, b->instanced ? 1 : 0);

        if (b->texture && b->sampler) {
            GPU_BindFragmentSampler(
                &pass, (Texture *[]){ b->texture }, (Sampler *[]){ b->sampler }, 1, 0
            );
        }

        if (b->lookup)
            GPU_BindVertexStorageBuffer(&pass, b->lookup, 0);

        if (b->instanced)
            Mesh_Draw(&pass, &r->quad, b->count);
        else
            GPU_DrawPrimitives(&pass, b->count, 1, 0, 0);
    }

    if (hasPass)
        GPU_EndRenderPass(&pass);
}

void R2D_Destroy(Renderer2D *r) {
    if (r->quad.vb.handle)
        Mesh_Destroy(&r->quad);
    if (r->arena.buffer.handle)
        UploadArena_Destroy(&r->arena);
}
