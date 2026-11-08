#ifndef GAME_PLAYFIELD_H
#define GAME_PLAYFIELD_H

#include "engine/gpu/gpu.h"
#include <stdint.h>

#define PLAYFIELD_WIDTH 480
#define PLAYFIELD_HEIGHT 640
#define PLAYFIELD_RENDER_WIDTH 960
#define PLAYFIELD_RENDER_HEIGHT 1280

typedef struct {
    Texture target;
    uint32_t width, height;
} Playfield;

bool Playfield_Init(Playfield *p, Device device);

#endif // GAME_PLAYFIELD_H
