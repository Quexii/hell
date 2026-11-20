#include "game.h"

#include "bullets.h"
#include "engine/application.h"
#include "engine/gpu/gpu.h"
#include "engine/maths.h"
#include "engine/renderer.h"
#include "engine/ui.h"
#include "graphics.h"
#include "level.h"
#include "playfield.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_timer.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    Vec4 uv[7];
} SSBO_BulletUVs;

typedef struct {
    Vec4 uv[100];
} SSBO_EntityUVs;

static Game game = { 0 };

static bool Game_Init(Application *app) {
    Game *g = app->userdata;
    Device device = app->window->gpuDevice;

    if (!Graphics_Init_Resources(&g->gfx, app->window, device))
        return false;

    g->ssboBulletUVs = GPU_CreateBuffer(
        device, &(BufferDesc){ .size = sizeof(SSBO_BulletUVs), .usage = BUFFERUSAGE_GFX_STORE_READ }
    );
    g->ssboEntityUVs = GPU_CreateBuffer(
        device, &(BufferDesc){ .size = sizeof(SSBO_EntityUVs), .usage = BUFFERUSAGE_GFX_STORE_READ }
    );
    if (!g->ssboBulletUVs.handle || !g->ssboEntityUVs.handle)
        return false;

    g->uiCtx = (UIContext){
        .ui = &g->ui,
        .app = app,
    };

    if (!Batch_Create(
            &g->entitiesBatch,
            device,
            &(BatchDesc){ .pipeline = g->gfx.bulletPipeline,
                          .texture = &g->gfx.atlasGeneric,
                          .sampler = &g->gfx.samplerLinear,
                          .lookup = &g->ssboEntityUVs,
                          .stride = sizeof(BulletData),
                          .capacity = 1000,
                          .instanced = true,
                          .target = &g->playfield.target }
        ))
        return false;

    if (!Batch_Create(
            &g->bulletBatch,
            device,
            &(BatchDesc){ .pipeline = g->gfx.bulletPipeline,
                          .texture = &g->gfx.atlasBullets,
                          .sampler = &g->gfx.samplerLinear,
                          .lookup = &g->ssboBulletUVs,
                          .capacity = MAX_BULLETS,
                          .stride = sizeof(BulletData),
                          .instanced = true,
                          .target = &g->playfield.target }
        ))
        return false;

    if (!Batch_Create(
            &g->uiBatch,
            device,
            &(BatchDesc){
                .pipeline = g->gfx.uiPipeline,
                .capacity = 10000,
                .stride = sizeof(UIVertex),
                .instanced = false,
                .explicitDrawFn = UI_Draw,
                .explicitDrawData = &g->uiCtx,
                .texture = &g->gfx.white,
                .sampler = &g->gfx.samplerNearest,
            }
        ))
        return false;

    if (!Batch_Create(
            &g->dbgBatch,
            device,
            &(BatchDesc){
                .pipeline = g->gfx.debugWorldPipeline,
                .capacity = 60000,
                .stride = sizeof(VertexDebug_W),
                .instanced = false,
                .target = &g->playfield.target,
            }
        ))
        return false;

    g->batches[0] = &g->entitiesBatch;
    g->batches[1] = &g->bulletBatch;
    g->batches[2] = &g->dbgBatch;
    g->batches[3] = &g->uiBatch;
    g->r2d = R2D_Create(g->batches, app->window, 4);

    if (!UI_Init(&g->ui, &g->uiBatch, 10000))
        return false;

    Batch_SetViewport(&g->entitiesBatch, (float) PLAYFIELD_WIDTH, (float) PLAYFIELD_HEIGHT);
    Batch_SetViewport(&g->bulletBatch, (float) PLAYFIELD_WIDTH, (float) PLAYFIELD_HEIGHT);
    Batch_SetViewport(&g->dbgBatch, (float) PLAYFIELD_WIDTH, (float) PLAYFIELD_HEIGHT);
    Batch_SetViewport(&g->uiBatch, (float) app->window->width, (float) app->window->height);

    if (!Playfield_Init(&g->playfield, device))
        return false;

    if (!BulletSim_Create(&g->bullets, MAX_BULLETS))
        return false;

    Level_LoadEnemies(&g->level);
    Level_LoadStages(&g->level);
    Level_StartStage(&g->level, STAGE_ID_TEST);

    printf("%i\n", g->level.stages->eventCount);
    g->level.player.position = (Vec2){ PLAYFIELD_WIDTH / 2.0f, PLAYFIELD_HEIGHT - 40.f };

    return true;
}

static bool Game_Upload(Application *app, CopyPass *pass) {
    Game *g = app->userdata;

    if (!R2D_Init(&g->r2d, pass))
        return false;

    SSBO_BulletUVs ssboBullets = {
        .uv = {
{ 0.000000f, 0.000000f, 0.125000f, 0.125000f },
{  0.125000f, 0.000000f, 0.250000f, 0.125000f },
{  0.000000f, 0.125000f, 0.062500f, 0.226562f },
{  0.250000f, 0.015625f, 0.281250f, 0.078125f },
{  0.296875f, 0.015625f, 0.328125f, 0.078125f },
{  0.000000f, 0.953125f, 0.046875f, 1.000000f },
{  0.046875f, 0.953125f, 0.093750f, 1.000000f },
        },
    };

    SSBO_EntityUVs ssboSprites = { .uv = { { 0.0f, 0.0f, 0.093750f, 0.093750f } } };

    UploadAllocation ssboBulletsAlloc, ssboSpritesAlloc;

    if (!UploadArena_Begin(&g->r2d.arena))
        return false;

    bool ok =
        UploadArena_Write(
            &g->r2d.arena, &ssboBullets, sizeof(ssboBullets), 16, &ssboBulletsAlloc
        ) &&
        UploadArena_Write(&g->r2d.arena, &ssboSprites, sizeof(ssboSprites), 16, &ssboSpritesAlloc);
    UploadArena_End(&g->r2d.arena);

    if (!ok)
        return false;

    if (!UploadArena_UploadBuffer(
            &g->r2d.arena, pass, &g->ssboBulletUVs, 0, ssboBulletsAlloc, false
        ))
        return false;

    if (!UploadArena_UploadBuffer(
            &g->r2d.arena, pass, &g->ssboEntityUVs, 0, ssboSpritesAlloc, false
        ))
        return false;

    Graphics_Upload_Resources(&g->gfx, pass);
    return true;
}

static void Game_Event(Application *app, const SDL_Event *event) {
    Game *g = app->userdata;

    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        int width = event->window.data1;
        int height = event->window.data2;

        if (width > 0 && height > 0)
            Batch_SetViewport(&g->uiBatch, (float) width, (float) height);
    }

    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
        app->running = false;
}

static void Game_Update(Application *app, float dt) {
    Game *g = app->userdata;

    Level_Tick(&g->level, app, dt);

    BulletSim_Update(&g->bullets, dt, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT);
}

static void Game_Prepare(Application *app, CommandBuffer *cmd) {
    Game *g = app->userdata;
    BulletSim *sim = &g->bullets;

    R2D_Begin(&g->r2d);

    { // entuties
        BulletData *out = Batch_Reserve(&g->entitiesBatch, 1);
        if (out) {
            out[0] = (BulletData){
                .position = g->level.player.position,
                .size = { 10.f, 10.f },
                .rotation = 0.f,
                .spriteID = 0,
                .tint = RGBA8(255, 255, 255, 255)
            };
        }
    }

    if (sim->count > 0) {
        BulletData *out = Batch_Reserve(&g->bulletBatch, sim->count);

        if (out) {
            for (uint32_t i = 0; i < sim->count; i++) {
                out[i] = (BulletData){
                    .position = sim->positions[i],
                    .rotation = sim->rotations[i],
                    .size = sim->sizes[i],
                    .spriteID = sim->spriteIDs[i],
                    .tint = sim->tints[i], // MutCol_A(sim->tints[i], (uint8_t) Clamp(255 *
                                           // sim->life[i], 0, 255))
                };
            }
        } else {
            SDL_Log("Bullet batch full: count=%u capacity=%u", sim->count, g->bulletBatch.capacity);
        }
    }

    float availW = app->window->width;
    float availH = app->window->height - 40.f;

    float scale = fminf(availW / PLAYFIELD_WIDTH, availH / PLAYFIELD_HEIGHT);
    Rect pfDest = { (availW - PLAYFIELD_WIDTH * scale) * 0.5f,
                    20.f + (availH - PLAYFIELD_HEIGHT * scale) * 0.5f,
                    PLAYFIELD_WIDTH * scale,
                    PLAYFIELD_HEIGHT * scale };
    { // Debug Layer
        RDG_Line(
            &g->dbgBatch,
            PLAYFIELD_WIDTH / 2.f,
            0.f,
            PLAYFIELD_WIDTH / 2.f,
            PLAYFIELD_HEIGHT,
            2,
            (Vec3){ 1.f, 1.f, 1.f }
        );
        RDG_Line(
            &g->dbgBatch,
            0.f,
            PLAYFIELD_HEIGHT / 2.f,
            PLAYFIELD_WIDTH,
            PLAYFIELD_HEIGHT / 2.f,
            2,
            (Vec3){ 1.f, 1.f, 1.f }
        );
    }

    float uiX = pfDest.x + pfDest.w + 20.f;
    float uiY = pfDest.y + 20.f;
    float uiW = (app->window->width - pfDest.w) / 2.f - 40.f;
    float uiH = pfDest.h - 40.f;

    Level_Draw(&g->level, app, cmd);

    UI_Begin(&g->ui);
    UI_GradV(
        &g->ui,
        0.0f,
        0.0f,
        app->window->width,
        app->window->height,
        (Vec4){ 0.1f, 0.1, 0.4f, 1.0f },
        (Vec4){ 0.3f, 0.3f, 0.5f, 1.0f }
    );
    UI_Rect(&g->ui, pfDest.x - 5.f, pfDest.y - 5.f, pfDest.w + 10.f, pfDest.h + 10.f, BLACK);

    UI_Rect(&g->ui, uiX, uiY, uiW, uiH, BLACK);
    UI_GradV(
        &g->ui,
        uiX + 2.f,
        uiY + 2.f,
        uiW - 4.f,
        uiH - 4.f,
        (Vec4){ 0.2, 0.2, 0.3, 1.0 },
        (Vec4){ 0.2, 0.2, 0.4, 1.0 }
    );
    UI_Rect(&g->ui, uiX + 5.f, uiY + 5.f, uiW - 10.f, uiH - 10.f, BLACK);
    
    UI_GradV(
        &g->ui,
        pfDest.x - 3.f,
        pfDest.y - 3.f,
        pfDest.w + 6.f,
        pfDest.h + 6.f,
        (Vec4){ 0.2, 0.2, 0.3, 1.0 },
        (Vec4){ 0.2, 0.2, 0.4, 1.0 }
    );
    UI_Texture(
        &g->ui,
        pfDest.x,
        pfDest.y,
        pfDest.w,
        pfDest.h,
        &g->playfield.target,
        &g->gfx.samplerLinear,
        (Vec4){ 1.0f, 1.0f, 1.0f, 1.0f }
    );

    char text[128];
    sprintf(
        text,
        "FPS: %i\nX: %f, y: %f",
        (int) app->window->frameTimer.fps,
        g->level.player.position.x,
        g->level.player.position.y
    );
    UI_Text(&g->ui, &g->gfx.fontMonsterrat, text, (Vec2){ 10.f, 10.f }, 18.f, WHITE);
    sprintf(text, "Health: %i\nBombs: %i", g->level.player.hearts, g->level.player.bombs);
    UI_Text(&g->ui, &g->gfx.fontMonsterrat, text, (Vec2){ uiX + 16, uiY + 16 }, 22.f, WHITE);

    R2D_Upload(&g->r2d, cmd);
}

static void Game_Render(Application *app, CommandBuffer *cmd) {
    Game *g = app->userdata;

    R2D_Draw(&g->r2d, cmd);
    GPU_SubmitCommand(cmd);
}

static void Game_Cleanup(Application *app) {
    Game *g = app->userdata;
    Device device = app->window->gpuDevice;

    Level_Destroy(&g->level);

    GPU_ReleaseTexture(&g->playfield.target);
    BulletSim_Destory(&g->bullets);
    UI_Destroy(&g->ui);
    R2D_Destroy(&g->r2d);
    Batch_Release(&g->uiBatch);
    Batch_Release(&g->bulletBatch);
    Batch_Release(&g->entitiesBatch);
    Batch_Release(&g->dbgBatch);

    if (g->ssboEntityUVs.handle)
        GPU_ReleaseBuffer(&g->ssboEntityUVs);

    if (g->ssboBulletUVs.handle)
        GPU_ReleaseBuffer(&g->ssboBulletUVs);

    Graphics_Release_Resources(&g->gfx, device);
}

Application Application_Create(void) {
    return (Application){
        .title = "Hell, but bullets!",
        .width = 1280,
        .height = 720,

        .userdata = &game,

        .vtable = { .init = Game_Init,
                   .upload = Game_Upload,
                   .event = Game_Event,
                   .prepare = Game_Prepare,
                   .update = Game_Update,
                   .render = Game_Render,
                   .cleanup = Game_Cleanup }
    };
}
