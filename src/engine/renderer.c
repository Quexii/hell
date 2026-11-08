#include "renderer.h"

#include "engine/gpu/gpu.h"

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
    };
    return true;
}

void Batch_Clear(Batch *b) {
    b->count = 0;
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

Renderer2D R2D_Create(Batch **batches, uint32_t batchCount) {
    return (Renderer2D){
        .batches = batches,
        .batchCount = batchCount,
        .cam.model = Mat4_Identity(),
        .cam.view = Mat4_Identity(),
        .cam.proj = Mat4_Identity(),
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

void R2D_SetViewport(Renderer2D *r, float width, float height) {
    r->cam.proj = Mat4_Ortho(0, width, height, 0, -1.f, 1.f);
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

void R2D_Draw(Renderer2D *r, RenderPass *pass) {
    bool boundOnce = false;

    for (uint32_t i = 0; i < r->batchCount; i++) {
        Batch *b = r->batches[i];
        if (b->count == 0)
            continue;

        if (b->explicitDrawFn)
            b->explicitDrawFn(r, pass, b->explicitDrawData);
        else {
            GPU_BindPipeline(pass, b->pipeline);

            if (!boundOnce) {
                GPU_PushVertexData(pass->cmd, 0, &r->cam, sizeof(Matrices));
                boundOnce = true;
            }

            if (b->instanced)
                Mesh_Bind(pass, &r->quad);
            GPU_BindVBO(pass, &b->gpu, 0, b->instanced ? 1 : 0);

            if (b->texture && b->sampler)
                GPU_BindFragmentSampler(
                    pass, (Texture *[]){ b->texture }, (Sampler *[]){ b->sampler }, 1, 0
                );
            if (b->lookup)
                GPU_BindVertexStorageBuffer(pass, b->lookup, 0);

            if (b->instanced)
                Mesh_Draw(pass, &r->quad, b->count);
            else
                GPU_DrawPrimitives(pass, b->count, 1, 0, 0);
        }
    }
}

void R2D_Destroy(Renderer2D *r) {
    if (r->quad.vb.handle)
        Mesh_Destroy(&r->quad);
    if (r->arena.buffer.handle)
        UploadArena_Destroy(&r->arena);
}
