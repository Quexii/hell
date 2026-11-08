#include "graphics.h"

#include "bullets.h"
#include "engine/font.h"
#include "engine/gpu/gpu.h"
#include "engine/mesh.h"
#include "engine/ui.h"

#include <SDL3/SDL_gpu.h>
#include <stdbool.h>

#define SAMPLER_DESC_LINEAR                                                                        \
    FILTERMODE_LINEAR, FILTERMODE_LINEAR, SAMPLINGMODE_LINEAR, SAMPLERADDRESSMODE_CLAMP_TO_EDGE,   \
        SAMPLERADDRESSMODE_CLAMP_TO_EDGE, SAMPLERADDRESSMODE_CLAMP_TO_EDGE

#define SAMPLER_DESC_NEAREST                                                                       \
    FILTERMODE_LINEAR, FILTERMODE_NEAREST, SAMPLINGMODE_NEAREST, SAMPLERADDRESSMODE_CLAMP_TO_EDGE, \
        SAMPLERADDRESSMODE_CLAMP_TO_EDGE, SAMPLERADDRESSMODE_CLAMP_TO_EDGE

bool Graphics_Init_Resources(Graphics *g, Window *window, Device device) {
    Shader shVertBullets = GPU_LoadShader(
               device,
               &(ShaderDesc){ .path = "sprite.vert.spv",
                              .stage = SHADERSTAGE_VERTEX,
                              .main = "main",
                              .uniformBuffers = 1,
                              .storageBuffers = 1 }
           ),
           shFragBullets = GPU_LoadShader(
               device,
               &(ShaderDesc){ .path = "sprite.frag.spv",
                              .stage = SHADERSTAGE_FRAGMENT,
                              .main = "main",
                              .samplers = 1 }
           ),
           shVertUI = GPU_LoadShader(
               device,
               &(ShaderDesc){
                   .path = "ui.vert.spv",
                   .stage = SHADERSTAGE_VERTEX,
                   .main = "main",
                   .uniformBuffers = 1,
               }
           ),
           shFragUI = GPU_LoadShader(
               device,
               &(ShaderDesc){ .path = "ui.frag.spv",
                              .stage = SHADERSTAGE_FRAGMENT,
                              .main = "main",
                              .samplers = 1 }
           );

    if (!shVertBullets.handle || !shFragBullets.handle || !shVertUI.handle || !shFragUI.handle) {
        if (shVertBullets.handle)
            SDL_ReleaseGPUShader(device, shVertBullets.handle);
        if (shFragBullets.handle)
            SDL_ReleaseGPUShader(device, shFragBullets.handle);
        if (shVertUI.handle)
            SDL_ReleaseGPUShader(device, shVertUI.handle);
        if (shFragUI.handle)
            SDL_ReleaseGPUShader(device, shFragUI.handle);
    }

    VertexLayout layoutBullets = VERTEX_LAYOUT(
        VERTEX_ATTR(MeshVertex, pos, 0, 0),
        VERTEX_ATTR(MeshVertex, uv, 1, 0),
        VERTEX_ATTR(BulletData, position, 2, 1),
        VERTEX_ATTR(BulletData, size, 3, 1),
        VERTEX_ATTR(BulletData, rotation, 4, 1),
        VERTEX_ATTR(BulletData, spriteID, 5, 1),
        VERTEX_ATTR(BulletData, tint, 6, 1),
    );

    VertexLayout layoutUI = VERTEX_LAYOUT(
        VERTEX_ATTR(UIVertex, position, 0, 0),
        VERTEX_ATTR(UIVertex, uv, 1, 0),
        VERTEX_ATTR(UIVertex, color, 2, 0),
        VERTEX_ATTR(UIVertex, mode, 3, 0)
    );

    g->bulletPipeline = GPU_CreatePipeline(
        device,
        window->handle,
        &(PipelineDesc){
            .vertexShader = &shVertBullets,
            .fragmentShader = &shFragBullets,
            .layout = &layoutBullets,
            .blend = GPU_BLEND_ALPHA,
            .buffers =
                (VertexBufferDesc[]){
                                     VERTEX_BUFFER(MeshVertex, 0, SDL_GPU_VERTEXINPUTRATE_VERTEX),
                                     VERTEX_BUFFER(BulletData, 1, SDL_GPU_VERTEXINPUTRATE_INSTANCE),
                                     },
            .numBuffers = 2,
    }
    );

    g->uiPipeline = GPU_CreatePipeline(
        device,
        window->handle,
        &(PipelineDesc){
            .vertexShader = &shVertUI,
            .fragmentShader = &shFragUI,
            .layout = &layoutUI,
            .blend = GPU_BLEND_ALPHA,
            .buffers =
                (VertexBufferDesc[]){
                                     VERTEX_BUFFER(UIVertex, 0, SDL_GPU_VERTEXINPUTRATE_VERTEX),
                                     },
            .numBuffers = 1
    }
    );

    GPU_ReleaseShader(&shVertBullets);
    GPU_ReleaseShader(&shFragBullets);
    GPU_ReleaseShader(&shVertUI);
    GPU_ReleaseShader(&shFragUI);

    if (!g->uiPipeline || !g->bulletPipeline)
        return false;

    g->samplerLinear = GPU_CraeteSampler(device, &(SamplerDesc){ SAMPLER_DESC_LINEAR });
    g->samplerNearest = GPU_CraeteSampler(device, &(SamplerDesc){ SAMPLER_DESC_NEAREST });

    if (!g->samplerLinear.handle || !g->samplerNearest.handle)
        return false;

    if (!Font_LoadStatic(
            &g->fontCinzel, &g->fontCinzelTexture, &g->samplerLinear, "cinzel-medium.fnt"
        ))
        return false;

    if (!Font_LoadStatic(
            &g->fontRajdhani, &g->fontRajdhaniTexture, &g->samplerLinear, "rajdhani-medium.fnt"
        ))
        return false;

    if (!Font_LoadStatic(
            &g->fontMonsterrat,
            &g->fontMontserratTexture,
            &g->samplerLinear,
            "montserrat-medium.fnt"
        ))
        return false;

    return true;
}

bool Graphics_Upload_Resources(Graphics *g, CopyPass *pass) {
    g->atlasBullets = GPU_LoadTexture(pass, "bullet_atlas.png");
    g->white = GPU_LoadTexture(pass, "white.png");
    g->fontCinzelTexture = GPU_LoadTexture(pass, "cinzel-medium.png");
    g->fontRajdhaniTexture = GPU_LoadTexture(pass, "rajdhani-medium.png");
    g->fontMontserratTexture = GPU_LoadTexture(pass, "montserrat.png");

    if (!g->atlasBullets.handle || !g->white.handle)
        return false;

    return true;
}

void Graphics_Release_Resources(Graphics *g, Device device) {
    if (g->fontMontserratTexture.handle)
        GPU_ReleaseTexture(&g->fontMontserratTexture);
    if (g->fontRajdhaniTexture.handle)
        GPU_ReleaseTexture(&g->fontRajdhaniTexture);
    if (g->fontCinzelTexture.handle)
        GPU_ReleaseTexture(&g->fontCinzelTexture);
    if (g->white.handle)
        GPU_ReleaseTexture(&g->white);
    if (g->atlasBullets.handle)
        GPU_ReleaseTexture(&g->atlasBullets);
    if (g->samplerLinear.handle)
        GPU_ReleaseSampler(&g->samplerLinear);
    if (g->samplerNearest.handle)
        GPU_ReleaseSampler(&g->samplerNearest);
    if (g->bulletPipeline)
        GPU_ReleasePipeline(device, g->bulletPipeline);
    if (g->uiPipeline)
        GPU_ReleasePipeline(device, g->uiPipeline);
}
