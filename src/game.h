#ifndef GAME_H
#define GAME_H

#include "bullets.h"
#include "engine/gpu/gpu.h"
#include "engine/renderer.h"
#include "engine/ui.h"
#include "graphics.h"
#include "pattern.h"

#include <stdint.h>

typedef struct {
    Graphics gfx;
    Buffer ssboBuffer;

    Batch bulletBatch, uiBatch, *batches[2];
    Renderer2D r2d;
    UIContext uiCtx;
    UI ui;
    BulletSim bullets;
    Pattern pattern;
    float spawnTimer, dtTimer;
    uint32_t spawnedCount;
} Game;

#endif // GAME_H
