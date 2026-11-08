#include "playfield.h"

#include <SDL3/SDL_gpu.h>
#include <stdbool.h>

bool Playfield_Init(Playfield *p, Device device) {
    *p = (Playfield){ 0 };
    p->width = PLAYFIELD_WIDTH;
    p->height = PLAYFIELD_HEIGHT;

    p->target.width = PLAYFIELD_RENDER_WIDTH;
    p->target.height = PLAYFIELD_RENDER_HEIGHT;
    p->target.device = device;
    p->target.handle = SDL_CreateGPUTexture(
        device,
        &(SDL_GPUTextureCreateInfo){ .type = SDL_GPU_TEXTURETYPE_2D,
                                     .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                     .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                                              SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                     .width = PLAYFIELD_RENDER_WIDTH,
                                     .height = PLAYFIELD_RENDER_HEIGHT,
                                     .layer_count_or_depth = 1,
                                     .num_levels = 1,
                                     .sample_count = SDL_GPU_SAMPLECOUNT_1 }
    );

    if (!p->target.handle)
        return false;

    return true;
}
