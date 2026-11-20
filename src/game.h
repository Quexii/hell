#ifndef GAME_H
#define GAME_H

#include "engine/gpu/gpu.h"
#include "engine/renderer.h"
#include "engine/ui.h"
#include "graphics.h"
#include "level.h"
#include "playfield.h"

#include <stdint.h>

typedef struct {
    Graphics gfx;
    Buffer ssboBulletUVs, ssboEntityUVs;

    Batch backdropBatch, entitiesBatch, bulletBatch, uiBatch, dbgBatch, *batches[4];
    Renderer2D r2d;
    UIContext uiCtx;
    UI ui;
    Playfield playfield;
    Level level;
    float spawnTimer, dtTimer;
    uint32_t spawnedCount;
} Game;

#endif // GAME_H
