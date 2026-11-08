#include "game.h"

#include "engine/application.h"
#include "engine/gpu/gpu.h"
#include "engine/maths.h"
#include "engine/renderer.h"
#include "engine/ui.h"
#include "graphics.h"
#include "pattern.h"
#include "patterns.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    Vec4 bulletUVs[BULLETTYPE_LAST];
} SSBO;

static Game game = { 0 };

static bool Game_Init(Application *app) {
    Game *g = app->userdata;
    Device device = app->window->gpuDevice;

    if (!Graphics_Init_Resources(&g->gfx, app->window, device))
        return false;
    g->ssboBuffer = GPU_CreateBuffer(
        device, &(BufferDesc){ .size = sizeof(SSBO), .usage = BUFFERUSAGE_GFX_STORE_READ }
    );
    if (!g->ssboBuffer.handle)
        return false;

    g->uiCtx = (UIContext){
        .ui = &g->ui,
        .app = app,
    };

    if (!Batch_Create(
            &g->bulletBatch,
            device,
            &(BatchDesc){ .pipeline = g->gfx.bulletPipeline,
                          .texture = &g->gfx.atlasBullets,
                          .sampler = &g->gfx.samplerLinear,
                          .lookup = &g->ssboBuffer,
                          .capacity = MAX_BULLETS,
                          .stride = sizeof(BulletData),
                          .instanced = true }
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

    g->batches[0] = &g->bulletBatch;
    g->batches[1] = &g->uiBatch;
    g->r2d = R2D_Create(g->batches, 2);

    if (!UI_Init(&g->ui, &g->uiBatch, 10000))
        return false;

    R2D_SetViewport(&g->r2d, (float) app->window->width, (float) app->window->height);

    if (!BulletSim_Create(&g->bullets, MAX_BULLETS))
        return false;

    g->pattern = Pattern_Create(
        Pattern_Mirrored, (Vec2){ app->window->width / 2.0f, app->window->height / 2.0f }, 0.4f
    );

    return true;
}

static bool Game_Upload(Application *app, CopyPass *pass) {
    Game *g = app->userdata;

    if (!R2D_Init(&g->r2d, pass))
        return false;

    SSBO ssbo = {
        .bulletUVs = {
{ 0.000000, 0.000000, 0.187500, 0.187500 },
{ 0.187500, 0.000000, 0.375000, 0.187500 },
        },
    };

    UploadAllocation ssboAlloc;

    if (!UploadArena_Begin(&g->r2d.arena))
        return false;

    bool ok = UploadArena_Write(&g->r2d.arena, &ssbo, sizeof(ssbo), 16, &ssboAlloc);
    UploadArena_End(&g->r2d.arena);

    if (!ok)
        return false;

    if (!UploadArena_UploadBuffer(&g->r2d.arena, pass, &g->ssboBuffer, 0, ssboAlloc, false))
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
            R2D_SetViewport(&g->r2d, (float) width, (float) height);
    }

    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
        app->running = false;
}

static void Game_Update(Application *app, float dt) {
    Game *g = app->userdata;

    g->pattern.origin = (Vec2){
        .x = app->window->width / 2.0f,
        .y = app->window->height / 2.0f
    };
    Pattern_Update(&g->pattern, &g->bullets, dt);

    BulletSim_Update(&g->bullets, dt, app->window->width, app->window->height);

    g->dtTimer += dt;
}

static void Game_Prepare(Application *app, CommandBuffer *cmd) {
    Game *g = app->userdata;
    BulletSim *sim = &g->bullets;

    R2D_Begin(&g->r2d);

    if (sim->count > 0) {
        BulletData *out = Batch_Reserve(&g->bulletBatch, sim->count);

        if (out) {
            for (uint32_t i = 0; i < sim->count; i++) {
                out[i] = (BulletData){
                    .position = sim->positions[i],
                    .rotation = sim->rotations[i],
                    .size = sim->sizes[i],
                    .spriteID = sim->spriteIDs[i],
                    .tint = MutCol_A(sim->tints[i], (uint8_t) Clamp(255 * sim->life[i], 0, 255))
                };
            }
        } else {
            SDL_Log("Bullet batch full: count=%u capacity=%u", sim->count, g->bulletBatch.capacity);
        }
    }

    UI_Begin(&g->ui);
    UI_PushClip(&g->ui, (Rect){ 20.f, 20.f, 180.f, 110.f });
    UI_Rect(&g->ui, 10.f, 10.f, 180.f, 80.f, (Vec4){ 1.0f, 0.5f, 0.8f, 1.0f });
    UI_Rect(&g->ui, 10.f, 110.f, 180.f, 80.f, (Vec4){ 1.0f, 0.8f, 0.5f, 1.0f });
    UI_Rect(&g->ui, 140.f, 80.f, 180.f, 80.f, (Vec4){ 0.5f, 0.8f, 0.1f, 1.0f });
    UI_PopClip(&g->ui);
    UI_Texture(
        &g->ui,
        40.f,
        160.f,
        180.f,
        180.f,
        &g->gfx.fontMontserratTexture,
        &g->gfx.samplerLinear,
        (Vec4){ 1.0f, 1.0f, 1.0f, 1.0f }
    );
    UI_Text(&g->ui, &g->gfx.fontRajdhani, "Hello, World!", (Vec2){300.f, 100.f}, 48.f, (Vec4){1.0f, 1.0f, 1.0f, 1.0f});

    R2D_Upload(&g->r2d, cmd);
}

static void Game_Render(Application *app, RenderPass *pass) {
    Game *g = app->userdata;
    R2D_Draw(&g->r2d, pass);
}

static void Game_Cleanup(Application *app) {
    Game *g = app->userdata;
    Device device = app->window->gpuDevice;

    BulletSim_Destory(&g->bullets);
    UI_Destroy(&g->ui);
    R2D_Destroy(&g->r2d);
    Batch_Release(&g->uiBatch);
    Batch_Release(&g->bulletBatch);

    if (g->ssboBuffer.handle)
        GPU_ReleaseBuffer(&g->ssboBuffer);

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
