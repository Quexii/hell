#ifndef GAME_GRAPHICS_H
#define GAME_GRAPHICS_H

#include "engine/font.h"
#include "engine/gpu/gpu.h"
#include "engine/gpu/window.h"

#include <stdbool.h>

typedef struct {
    GraphicsPipeline bulletPipeline, uiPipeline;
    Sampler samplerLinear, samplerNearest;
    Texture white, atlasBullets, atlasGeneric, fontCinzelTexture, fontRajdhaniTexture, fontMontserratTexture;
    Font fontCinzel, fontRajdhani, fontMonsterrat;
} Graphics;

bool Graphics_Init_Resources(Graphics *g, Window *window, Device device);
bool Graphics_Upload_Resources(Graphics *g, CopyPass *pass);
void Graphics_Release_Resources(Graphics *g, Device device);

#endif // GAME_GRAPHICS_H
