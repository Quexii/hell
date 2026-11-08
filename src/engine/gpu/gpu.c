#include "gpu.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_image/SDL_image.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static SDL_GPUColorTargetBlendState GPU_GetBlendState(const PipelineDesc *desc) {
    SDL_GPUColorTargetBlendState state = { 0 };

    if (desc->blend == GPU_BLEND_DISABLED)
        return state;

    state.enable_blend = true;

    BlendDesc blend = { .colorOp = SDL_GPU_BLENDOP_ADD,
                        .alphaOp = SDL_GPU_BLENDOP_ADD,
                        .srcColor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                        .dstColor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .srcAlpha = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                        .dstAlpha = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA };

    if (desc->blend == GPU_BLEND_CUSTOM)
        blend = desc->customBlend;

    state.color_blend_op = blend.colorOp;
    state.alpha_blend_op = blend.alphaOp;

    state.src_color_blendfactor = blend.srcColor;
    state.dst_color_blendfactor = blend.dstColor;
    state.src_alpha_blendfactor = blend.srcAlpha;
    state.dst_alpha_blendfactor = blend.dstAlpha;

    return state;
}

GraphicsPipeline GPU_CreatePipeline(Device device, SDL_Window *window, const PipelineDesc *desc) {
    if (!device || !window || !desc || !desc->vertexShader || !desc->fragmentShader ||
        !desc->vertexShader->handle || !desc->fragmentShader->handle || !desc->layout ||
        (desc->layout->count && !desc->layout->attributes)) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "GPU_CreatePipeline: Invalid descriptor");
        return NULL;
    }

    uint32_t count = desc->layout->count;

    SDL_GPUVertexAttribute *attributes = NULL;

    if (count > 0) {
        attributes = SDL_calloc(count, sizeof(*attributes));

        if (!attributes) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to allocate vertex attributes");
            return NULL;
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        const VertexAttribute *attr = &desc->layout->attributes[i];

        attributes[i] = (SDL_GPUVertexAttribute){ .buffer_slot = attr->slot,
                                                  .location = attr->location,
                                                  .format = attr->format,
                                                  .offset = attr->offset };
    }

    SDL_GPUVertexBufferDescription buffers[desc->numBuffers];

    for (uint32_t i = 0; i < desc->numBuffers; i++) {
        buffers[i].slot = desc->buffers[i].slot;
        buffers[i].input_rate = desc->buffers[i].inputRate;
        buffers[i].pitch = desc->buffers[i].pitch;
        buffers[i].instance_step_rate = 0;
    }

    SDL_GPUColorTargetDescription colorTarget = {
        .format = SDL_GetGPUSwapchainTextureFormat(device, window),
        .blend_state = GPU_GetBlendState(desc)
    };

    SDL_GPUGraphicsPipelineCreateInfo ci = {
        .vertex_shader = desc->vertexShader->handle,
        .fragment_shader = desc->fragmentShader->handle,

        .primitive_type = desc->primitive,

        .vertex_input_state = { .num_vertex_buffers = desc->numBuffers,
                               .vertex_buffer_descriptions = buffers,
                               .num_vertex_attributes = count,
                               .vertex_attributes = attributes },

        .target_info = { .num_color_targets = 1, .color_target_descriptions = &colorTarget }
    };

    GraphicsPipeline pipeline = SDL_CreateGPUGraphicsPipeline(device, &ci);

    SDL_free(attributes);

    if (!pipeline) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER,
            "Failed -> SDL_CreateGPUGraphicsPipeline(...)\n%s",
            SDL_GetError()
        );
    }

    return pipeline;
}

void GPU_BindPipeline(RenderPass *pass, GraphicsPipeline pipeline) {
    if (!pass || !pass->handle || !pipeline)
        return;

    SDL_BindGPUGraphicsPipeline(pass->handle, pipeline);
}

void GPU_ReleasePipeline(Device device, GraphicsPipeline pipeline) {
    if (pipeline)
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
}

Shader GPU_LoadShader(Device device, ShaderDesc *desc) {
    const char *base = SDL_GetBasePath();
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%sassets/shaders/%s", base, desc->path);

    printf("%s\n", path);

    size_t codeSize;
    void *code = SDL_LoadFile(path, &codeSize);
    SDL_GPUShaderCreateInfo ci = { .code = code,
                                   .code_size = codeSize,
                                   .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                   .num_samplers = desc->samplers,
                                   .num_storage_buffers = desc->storageBuffers,
                                   .num_uniform_buffers = desc->uniformBuffers,
                                   .num_storage_textures = desc->storageTextures,
                                   .stage = (SDL_GPUShaderStage) desc->stage,
                                   .entrypoint = desc->main };

    SDL_GPUShader *handle = SDL_CreateGPUShader(device, &ci);
    SDL_free(code);

    if (!handle) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER, "Failed -> SDL_CreateGPUShader(...)\n%s", SDL_GetError()
        );
    }

    return (Shader){ .handle = handle, .device = device };
}

void GPU_ReleaseShader(Shader *shader) {
    SDL_ReleaseGPUShader(shader->device, shader->handle);
}

Buffer GPU_CreateBuffer(Device device, BufferDesc *desc) {
    SDL_GPUBuffer *handle = SDL_CreateGPUBuffer(
        device,
        &(SDL_GPUBufferCreateInfo){
            .size = desc->size,
            .usage = desc->usage,
        }
    );

    if (!handle) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER, "Failed -> SDL_CreateGPUBuffer(...)\n%s", SDL_GetError()
        );
    }

    return (Buffer){ .handle = handle, .device = device };
}

void GPU_ReleaseBuffer(Buffer *buffer) {
    SDL_ReleaseGPUBuffer(buffer->device, buffer->handle);
}

Texture GPU_LoadTexture(CopyPass *pass, const char *path) {
    const char *base = SDL_GetBasePath();
    char p[PATH_MAX];
    snprintf(p, sizeof(p), "%sassets/sprites/%s", base, path);

    int w, h;
    SDL_GPUTexture *handle = IMG_LoadGPUTexture(pass->device, pass->handle, p, &w, &h);
    if (!handle) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER, "Failed -> IMG_LoadGPUTexture(%s)\n%s", path, SDL_GetError()
        );
    }

    return (Texture){ .handle = handle, .device = pass->device, .width = w, .height = h };
}

void GPU_ReleaseTexture(Texture *texture) {
    SDL_ReleaseGPUTexture(texture->device, texture->handle);
}

Sampler GPU_CraeteSampler(Device device, SamplerDesc *desc) {
    SDL_GPUSamplerCreateInfo ci = { .min_filter = (SDL_GPUFilter) desc->min,
                                    .mag_filter = (SDL_GPUFilter) desc->mag,
                                    .address_mode_u = (SDL_GPUSamplerAddressMode) desc->adU,
                                    .address_mode_v = (SDL_GPUSamplerAddressMode) desc->adV,
                                    .address_mode_w = (SDL_GPUSamplerAddressMode) desc->adW };

    SDL_GPUSampler *handle = SDL_CreateGPUSampler(device, &ci);
    if (!handle) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER, "Failed -> SDL_CreateGPUSampler(...)\n%s", SDL_GetError()
        );
    }

    return (Sampler){ .device = device, .handle = handle };
}

void GPU_ReleaseSampler(Sampler *sampler) {
    SDL_ReleaseGPUSampler(sampler->device, sampler->handle);
}

TransferBuffer GPU_CreateTransferBuffer(Device device, TransferDesc *desc) {
    SDL_GPUTransferBufferCreateInfo ci = { .usage = (SDL_GPUTransferBufferUsage) desc->usage,
                                           .size = desc->size };
    SDL_GPUTransferBuffer *handle = SDL_CreateGPUTransferBuffer(device, &ci);
    if (!handle) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER,
            "Failed -> SDL_CreateGPUTransferBuffer(...)\n%s",
            SDL_GetError()
        );
    }
    return (TransferBuffer){ .handle = handle, .device = device, .size = desc->size };
}

void *GPU_MapTransfer(TransferBuffer *buffer, bool cycle) {
    return SDL_MapGPUTransferBuffer(buffer->device, buffer->handle, cycle);
}

void GPU_UnmapTransfer(TransferBuffer *buffer) {
    SDL_UnmapGPUTransferBuffer(buffer->device, buffer->handle);
}

void GPU_ReleaseTransferBuffer(TransferBuffer *buffer) {
    SDL_ReleaseGPUTransferBuffer(buffer->device, buffer->handle);
}

CommandBuffer GPU_BeginCommand(Device device) {
    SDL_GPUCommandBuffer *handle = SDL_AcquireGPUCommandBuffer(device);
    if (!handle) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER,
            "Failed -> SDL_AcquireGPUCommandBuffer(...)\n%s",
            SDL_GetError()
        );
    }
    return (CommandBuffer){ .handle = handle, .device = device };
}

void GPU_PushVertexData(CommandBuffer *cmd, uint32_t slot, const void *data, uint32_t length) {
    SDL_PushGPUVertexUniformData(cmd->handle, slot, data, length);
}

void GPU_PushFragmentData(CommandBuffer *cmd, uint32_t slot, const void *data, uint32_t length) {
    SDL_PushGPUFragmentUniformData(cmd->handle, slot, data, length);
}

void GPU_SubmitCommand(CommandBuffer *cmd) {
    SDL_SubmitGPUCommandBuffer(cmd->handle);
}

Frame GPU_BeginFrame(CommandBuffer *cmd, SDL_Window *window, FrameDesc *desc) {
    Frame frame = { .cmd = cmd, .device = cmd->device, .desc = desc };
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            cmd->handle, window, &frame.swapchainTexture, &frame.width, &frame.height
        )) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER,
            "Failed -> SDL_AcquireGPUSwapchainTexture(...)\n%s",
            SDL_GetError()
        );
    }
    return frame;
}

RenderPass GPU_BeginRenderPass(Frame *frame) {
    SDL_GPUColorTargetInfo ci = { .clear_color = frame->desc->clearColor,
                                  .load_op = (SDL_GPULoadOp) frame->desc->loadOp,
                                  .store_op = (SDL_GPUStoreOp) frame->desc->storeOp,
                                  .texture = frame->swapchainTexture };

    SDL_GPURenderPass *handle = SDL_BeginGPURenderPass(frame->cmd->handle, &ci, 1, NULL);
    if (!handle) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER, "Failed -> SDL_BeginGPURenderPass(...)\n%s", SDL_GetError()
        );
    }

    return (RenderPass){ .handle = handle, .device = frame->device, .cmd = frame->cmd };
}

void GPU_BindFragmentSampler(
    RenderPass *pass, Texture *texture[], Sampler *sampler[], uint32_t num, uint32_t slot
) {
    SDL_GPUTextureSamplerBinding tsb[num];
    for (uint32_t i = 0; i < num; i++) {
        tsb[i].sampler = sampler[i]->handle;
        tsb[i].texture = texture[i]->handle;
    }
    SDL_BindGPUFragmentSamplers(pass->handle, slot, tsb, num);
}

void GPU_BindVertexStorageBuffer(RenderPass *pass, Buffer *buffer, uint32_t slot) {
    SDL_BindGPUVertexStorageBuffers(pass->handle, slot, (SDL_GPUBuffer *[1]){ buffer->handle }, 1);
}

void GPU_BindVBO(RenderPass *pass, Buffer *buffer, uint32_t offset, uint32_t slot) {
    SDL_BindGPUVertexBuffers(
        pass->handle,
        slot,
        (SDL_GPUBufferBinding[]){
            { .buffer = buffer->handle, .offset = offset }
    },
        1
    );
}

void GPU_DrawPrimitives(
    RenderPass *pass,
    uint32_t nVertices,
    uint32_t nInstances,
    uint32_t firstVertex,
    uint32_t firstIntance
) {
    SDL_DrawGPUPrimitives(pass->handle, nVertices, nInstances, firstVertex, firstIntance);
}

void GPU_EndRenderPass(RenderPass *pass) {
    SDL_EndGPURenderPass(pass->handle);
}

CopyPass GPU_BeginCopy(CommandBuffer *cmd) {
    SDL_GPUCopyPass *handle = SDL_BeginGPUCopyPass(cmd->handle);
    if (!handle) {
        SDL_LogError(
            SDL_LOG_CATEGORY_RENDER, "Failed -> SDL_BeginGPUCopyPass(...)\n%s", SDL_GetError()
        );
    }
    return (CopyPass){ .handle = handle, .device = cmd->device, .cmd = cmd };
}

void GPU_UploadToBuffer(
    CopyPass *pass,
    Buffer *buffer,
    TransferBuffer *trans,
    uint32_t size,
    uint32_t offset,
    uint32_t locOffset
) {
    SDL_GPUTransferBufferLocation loc = {
        .transfer_buffer = trans->handle,
        .offset = locOffset,
    };

    SDL_GPUBufferRegion region = { .buffer = buffer->handle, .size = size, .offset = offset };

    SDL_UploadToGPUBuffer(pass->handle, &loc, &region, true);
}

void GPU_UploadBufferMapped(
    CopyPass *pass,
    Buffer *buffer,
    TransferBuffer *trans,
    void *data,
    uint32_t size,
    uint32_t offset,
    uint32_t locOffset,
    bool cycle
) {
    void *mem = GPU_MapTransfer(trans, cycle);
    memcpy(mem, data, size);
    GPU_UnmapTransfer(trans);
    GPU_UploadToBuffer(pass, buffer, trans, size, offset, locOffset);
}

void GPU_EndCopy(CopyPass *pass) {
    SDL_EndGPUCopyPass(pass->handle);
}

bool UploadArena_Init(UploadArena *arena, Device device, uint32_t capacity) {
    if (!arena || !device || capacity == 0)
        return false;

    *arena = (UploadArena){ .device = device, .capacity = capacity };

    arena->buffer = GPU_CreateTransferBuffer(
        device, &(TransferDesc){ .usage = TRANSFERBUFFERUSAGE_UPLOAD, .size = capacity }
    );

    if (!arena->buffer.handle) {
        *arena = (UploadArena){ 0 };
        return false;
    }

    return true;
}

void UploadArena_Destroy(UploadArena *arena) {
    if (!arena)
        return;

    if (arena->mapped)
        UploadArena_End(arena);

    if (arena->buffer.handle)
        GPU_ReleaseTransferBuffer(&arena->buffer);

    *arena = (UploadArena){ 0 };
}

bool UploadArena_Begin(UploadArena *arena) {
    if (!arena || !arena->buffer.handle || arena->mapped)
        return false;

    arena->mapped = SDL_MapGPUTransferBuffer(arena->device, arena->buffer.handle, true);

    if (!arena->mapped)
        return false;

    arena->offset = 0;
    return true;
}

void UploadArena_End(UploadArena *arena) {
    if (!arena || !arena->mapped)
        return;

    SDL_UnmapGPUTransferBuffer(arena->device, arena->buffer.handle);
    arena->mapped = NULL;
}

bool UploadArena_Alloc(
    UploadArena *arena, uint32_t size, uint32_t alignment, UploadAllocation *allocation, void **ptr
) {
    if (!arena || !arena->mapped || !allocation || !ptr || size == 0)
        return false;

    if (!alignment || (alignment & (alignment - 1)))
        return false;

    uint64_t aligned = ((uint64_t) arena->offset + alignment - 1) & ~((uint64_t) alignment - 1);

    if (aligned + size > arena->capacity)
        return false;

    allocation->offset = (uint32_t) aligned;
    allocation->size = size;

    *ptr = arena->mapped + aligned;
    arena->offset = (uint32_t) (aligned + size);

    return true;
}

bool UploadArena_Write(
    UploadArena *arena,
    const void *data,
    uint32_t size,
    uint32_t alignment,
    UploadAllocation *allocation
) {
    if (!data)
        return false;

    void *ptr;

    if (!UploadArena_Alloc(arena, size, alignment, allocation, &ptr))
        return false;

    memcpy(ptr, data, size);
    return true;
}

bool UploadArena_UploadBuffer(
    UploadArena *arena,
    CopyPass *pass,
    Buffer *destination,
    uint32_t destinationOffset,
    UploadAllocation allocation,
    bool cycle
) {
    if (!arena || !arena->buffer.handle || !pass || !destination)
        return false;

    if (arena->mapped || allocation.size == 0)
        return false;

    if ((uint64_t) allocation.offset + allocation.size > arena->offset)
        return false;

    if ((uint64_t) destinationOffset + allocation.size > UINT32_MAX)
        return false;

    SDL_UploadToGPUBuffer(
        pass->handle,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = arena->buffer.handle,
                                          .offset = allocation.offset },
        &(SDL_GPUBufferRegion){
            .buffer = destination->handle, .offset = destinationOffset, .size = allocation.size },
        cycle
    );

    return true;
}
