#ifndef GAME_H
#define GAME_H

#include "bullets.h"
#include "engine/gpu/gpu.h"
#include "engine/renderer.h"
#include "engine/ui.h"
#include "graphics.h"
#include "pattern.h"
#include "playfield.h"

#include <stdint.h>

typedef struct {
    Graphics gfx;
    Buffer ssboBulletUVs, ssboEntityUVs;

    Batch backdropBatch, entitiesBatch, bulletBatch, uiBatch, *batches[3];
    Renderer2D r2d;
    UIContext uiCtx;
    UI ui;
    Playfield playfield;
    BulletSim bullets;
    Pattern pattern;
    float spawnTimer, dtTimer;
    uint32_t spawnedCount;
} Game;

#endif // GAME_H
