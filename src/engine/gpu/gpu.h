#ifndef ENGINE_GPU_H
#define ENGINE_GPU_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_video.h>
#include <SDL3_image/SDL_image.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VERTEX_FORMAT(x)                                                                           \
    _Generic(                                                                                      \
        (x),                                                                                       \
        float: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,                                                  \
        Vec2: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,                                                  \
        Vec3: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,                                                  \
        Vec4: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,                                                  \
        uint32_t: SDL_GPU_VERTEXELEMENTFORMAT_UINT                                                 \
    )

#define VERTEX_ATTR(T, member, location_, slot_)                                                   \
    { .location = (location_),                                                                     \
      .slot = (slot_),                                                                             \
      .format = VERTEX_FORMAT(((T *) 0)->member),                                                  \
      .offset = offsetof(T, member) }

#define VERTEX_LAYOUT(...)                                                                         \
    { .attributes = (VertexAttribute[]){ __VA_ARGS__ },                                            \
      .count = sizeof((VertexAttribute[]){ __VA_ARGS__ }) / sizeof(VertexAttribute) }

#define VERTEX_BUFFER(T, slot_, rate_) { .slot = (slot_), .pitch = sizeof(T), .inputRate = (rate_) }

typedef enum ShaderStage { SHADERSTAGE_VERTEX, SHADERSTAGE_FRAGMENT } ShaderStage;

typedef enum BufferUsage {
    BUFFERUSAGE_VERTEX = 1 << 0,
    BUFFERUSAGE_INDEX = 1 << 1,
    BUFFERUSAGE_INDIRECT = 1 << 2,
    BUFFERUSAGE_GFX_STORE_READ = 1 << 3,
    BUFFERUSAGE_COMP_STORE_READ = 1 << 4,
    BUFFERUSAGE_COMP_STORE_WRITE = 1 << 5
} BufferUsage;

typedef enum TransferBufferUsage {
    TRANSFERBUFFERUSAGE_UPLOAD = 0,
    TRANSFERBUFFERUSAGE_DOWNLOAD = 1
} TransferBufferUsage;

typedef enum LoadOp { LOADOP_LOAD, LOADOP_CLEAR, LOADOP_DONT_CARE } LoadOp;

typedef enum StoreOp {
    STOREOP_STORE,
    STOREOP_DONT_CARE,
    STOREOP_RESOLVE,
    STOREOP_RESOLVE_AND_STORE
} StoreOp;

typedef enum { GPU_BLEND_DISABLED, GPU_BLEND_ALPHA, GPU_BLEND_CUSTOM } BlendMode;

typedef enum { FILTERMODE_NEAREST, FILTERMODE_LINEAR } FilterMode;

typedef enum { SAMPLINGMODE_NEAREST, SAMPLINGMODE_LINEAR } SamplingMode;

typedef enum {
    SAMPLERADDRESSMODE_REPEAT,
    SAMPLERADDRESSMODE_MIRRORED_REPEAT,
    SAMPLERADDRESSMODE_CLAMP_TO_EDGE
} SamplerAddressMode;

typedef SDL_GPUGraphicsPipeline *GraphicsPipeline;
typedef SDL_GPUDevice *Device;

typedef struct {
    uint32_t location;
    uint32_t slot;
    SDL_GPUVertexElementFormat format;
    uint32_t offset;
} VertexAttribute;

typedef struct {
    const VertexAttribute *attributes;
    uint32_t count;
} VertexLayout;

typedef struct {
    SDL_GPUShader *handle;
    Device device;
} Shader;

typedef struct {
    const char *path;
    const char *main;
    const ShaderStage stage;
    const int samplers, storageBuffers, uniformBuffers, storageTextures;
} ShaderDesc;

typedef struct {
    uint32_t size;
    BufferUsage usage;
} BufferDesc;

typedef struct {
    uint32_t size;
    TransferBufferUsage usage;
} TransferDesc;

typedef struct {
    SDL_GPUBlendOp colorOp;
    SDL_GPUBlendOp alphaOp;

    SDL_GPUBlendFactor srcColor;
    SDL_GPUBlendFactor dstColor;

    SDL_GPUBlendFactor srcAlpha;
    SDL_GPUBlendFactor dstAlpha;
} BlendDesc;

typedef struct {
    uint32_t slot, pitch;
    SDL_GPUVertexInputRate inputRate;
} VertexBufferDesc;

typedef struct {
    Shader *vertexShader;
    Shader *fragmentShader;

    SDL_GPUPrimitiveType primitive;
    SDL_GPUVertexInputRate inputRate;

    const VertexLayout *layout;
    VertexBufferDesc *buffers;
    uint32_t numBuffers;

    uint32_t pitch;

    BlendMode blend;
    BlendDesc customBlend;
} PipelineDesc;

typedef struct {
    FilterMode min, mag;
    SamplingMode mipmapMode;
    SamplerAddressMode adU, adV, adW;
} SamplerDesc;

typedef struct {
    SDL_GPUBuffer *handle;
    Device device;
} Buffer;

typedef struct {
    SDL_GPUSampler *handle;
    Device device;
} Sampler;

typedef struct {
    SDL_GPUTexture *handle;
    Device device;
    int width, height;
} Texture;

typedef struct {
    SDL_GPUTransferBuffer *handle;
    Device device;
    size_t size;
} TransferBuffer;

typedef struct {
    SDL_GPUCommandBuffer *handle;
    Device device;
} CommandBuffer;

typedef struct {
    SDL_FColor clearColor;
    LoadOp loadOp;
    StoreOp storeOp;
} FrameDesc;

typedef struct {
    Device device;
    CommandBuffer *cmd;
    SDL_GPUTexture *swapchainTexture;
    uint32_t width, height;
    FrameDesc *desc;
} Frame;

typedef struct {
    SDL_GPURenderPass *handle;
    Device device;
    CommandBuffer *cmd;
} RenderPass;

typedef struct {
    SDL_GPUCopyPass *handle;
    Device device;
    CommandBuffer *cmd;
} CopyPass;

typedef struct {
    uint32_t offset;
    uint32_t size;
} UploadAllocation;

typedef struct {
    Device device;
    TransferBuffer buffer;

    uint8_t *mapped;
    uint32_t capacity;
    uint32_t offset;
} UploadArena;

GraphicsPipeline GPU_CreatePipeline(Device device, SDL_Window *window, const PipelineDesc *desc);
void GPU_BindPipeline(RenderPass *pass, GraphicsPipeline pipeline);
void GPU_ReleasePipeline(Device device, GraphicsPipeline pipeline);

Shader GPU_LoadShader(Device device, ShaderDesc *desc);
void GPU_ReleaseShader(Shader *shader);

Buffer GPU_CreateBuffer(Device device, BufferDesc *desc);
void GPU_ReleaseBuffer(Buffer *buffer);

Texture GPU_LoadTexture(CopyPass *pass, const char *path);
void GPU_ReleaseTexture(Texture *texture);

Sampler GPU_CraeteSampler(Device device, SamplerDesc *desc);
void GPU_ReleaseSampler(Sampler *sampler);

TransferBuffer GPU_CreateTransferBuffer(Device device, TransferDesc *desc);
void *GPU_MapTransfer(TransferBuffer *buffer, bool cycle);
void GPU_UnmapTransfer(TransferBuffer *buffer);
void GPU_ReleaseTransferBuffer(TransferBuffer *buffer);

CommandBuffer GPU_BeginCommand(Device device);
void GPU_PushVertexData(CommandBuffer *cmd, uint32_t slot, const void *data, uint32_t length);
void GPU_PushFragmentData(CommandBuffer *cmd, uint32_t slot, const void *data, uint32_t length);
void GPU_SubmitCommand(CommandBuffer *cmd);

Frame GPU_BeginFrame(CommandBuffer *cmd, SDL_Window *window, FrameDesc *desc);

RenderPass GPU_BeginRenderPass(Frame *frame);
void GPU_BindFragmentSampler(
    RenderPass *pass, Texture *texture[], Sampler *sampler[], uint32_t num, uint32_t slot
);
void GPU_BindVertexStorageBuffer(RenderPass *pass, Buffer *buffer, uint32_t slot);
void GPU_BindVBO(RenderPass *pass, Buffer *buffer, uint32_t offset, uint32_t slot);
void GPU_DrawPrimitives(
    RenderPass *pass,
    uint32_t nVertices,
    uint32_t nInstances,
    uint32_t firstVertex,
    uint32_t firstIntance
);
void GPU_EndRenderPass(RenderPass *pass);

CopyPass GPU_BeginCopy(CommandBuffer *cmd);
void GPU_UploadToBuffer(
    CopyPass *pass,
    Buffer *buffer,
    TransferBuffer *trans,
    uint32_t size,
    uint32_t offset,
    uint32_t locOffset
);
void GPU_UploadBufferMapped(
    CopyPass *pass,
    Buffer *buffer,
    TransferBuffer *trans,
    void *data,
    uint32_t size,
    uint32_t offset,
    uint32_t locOffset,
    bool cycle
);
void GPU_EndCopy(CopyPass *pass);

bool UploadArena_Init(UploadArena *arena, Device device, uint32_t capacity);

void UploadArena_Destroy(UploadArena *arena);

bool UploadArena_Begin(UploadArena *arena);
void UploadArena_End(UploadArena *arena);

bool UploadArena_Alloc(
    UploadArena *arena, uint32_t size, uint32_t alignment, UploadAllocation *allocation, void **ptr
);

bool UploadArena_Write(
    UploadArena *arena,
    const void *data,
    uint32_t size,
    uint32_t alignment,
    UploadAllocation *allocation
);

bool UploadArena_UploadBuffer(
    UploadArena *arena,
    CopyPass *pass,
    Buffer *destination,
    uint32_t destinationOffset,
    UploadAllocation allocation,
    bool cycle
);

#endif /* ifndef ENGINE_GPU_H */
