#include "application.h"

#include "input.h"
#include "util.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define TICK_RATE (1.0 / 60.0)
#define MAX_DELTA 0.25
#define MAX_UPDATES 5

#define TIME_US(a, b) ((double) ((b) - (a)) * 1000000.0 / SDL_GetPerformanceFrequency())

typedef struct {
    double acquire;
    double render;
    double submit;
    double prepare;

    uint64_t acquiredFrames;
    uint64_t skippedFrames;
    uint64_t frames;
    double elapsed;
} FrameProfiler;

static void FrameProfiler_Add(
    FrameProfiler *profiler,
    double dt,
    uint64_t t0,
    uint64_t t1,
    uint64_t t2,
    uint64_t t3,
    uint64_t t4
) {
    profiler->prepare += TIME_US(t0, t1);
    profiler->acquire += TIME_US(t1, t2);
    profiler->render += TIME_US(t2, t3);
    profiler->submit += TIME_US(t3, t4);

    profiler->frames++;
    profiler->elapsed += dt;

    if (profiler->elapsed >= 1.0) {
        double count = (double) profiler->frames;

        printf(
            "FPS: %.0f | Prepare: %.2f us | Acquire: %.2f us | "
            "Render: %.2f us | Submit: %.2f us | Acquired frames: %lu | Skipped frames: %lu\n",
            count / profiler->elapsed,
            profiler->prepare / count,
            profiler->acquire / count,
            profiler->render / count,
            profiler->submit / count,
            profiler->acquiredFrames,
            profiler->skippedFrames
        );

        *profiler = (FrameProfiler){ 0 };
    }
}

int main(void) {
    Application app = Application_Create();

    app.running = true;

    app.window = Window_Create(
        app.title ? app.title : "Application",
        app.width > 0 ? app.width : 1280,
        app.height > 0 ? app.height : 720
    );

    if (!app.window)
        return EXIT_FAILURE;

    Window_Init(app.window);

    if (!app.window->gpuDevice)
        goto shutdown;

    if (app.vtable.init && !app.vtable.init(&app))
        goto shutdown;

    if (app.vtable.upload) {
        CommandBuffer cmd = GPU_BeginCommand(app.window->gpuDevice);
        if (!cmd.handle)
            goto shutdown;

        CopyPass pass = GPU_BeginCopy(&cmd);
        if (!pass.handle) {
            SDL_CancelGPUCommandBuffer(cmd.handle);
            goto shutdown;
        }

        bool success = app.vtable.upload(&app, &pass);

        GPU_EndCopy(&pass);
        GPU_SubmitCommand(&cmd);

        if (!success)
            goto shutdown;
    }

    FrameTimer *timer = &app.window->frameTimer;
    double accumulator = 0.0;
    // FrameProfiler profiler = { 0 };

    const double tickTime = 1.0 / 60.0;
    const int maxUpdates = 5;

    Input_Init();

    while (app.running) {
        FrameTimer_Update(timer);
        Input_BeginFrame();

        double dt = timer->delta;
        if (dt > 0.25)
            dt = 0.25;

        accumulator += dt;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                app.running = false;

            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                app.window->width = event.window.data1;
                app.window->height = event.window.data2;
            }

            Input_HandleEvent(&event);

            if (app.vtable.event)
                app.vtable.event(&app, &event);
        }

        if (!app.running)
            break;

        int updates = 0;

        while (accumulator >= tickTime && updates < maxUpdates) {
            if (app.vtable.update)
                app.vtable.update(&app, (float) tickTime);

            accumulator -= tickTime;
            updates++;
        }

        if (updates == maxUpdates && accumulator >= tickTime)
            accumulator = 0.0;

        Device device = app.window->gpuDevice;
        CommandBuffer cmd = GPU_BeginCommand(device);

        if (!cmd.handle)
            break;

        // uint64_t t0 = SDL_GetPerformanceCounter();

        if (app.vtable.prepare)
            app.vtable.prepare(&app, &cmd);

        // uint64_t t1 = SDL_GetPerformanceCounter();
        //
        // FrameDesc desc = {
        //     .clearColor = { 0.f, 0.f, 0.f, 1.f },
        //       .loadOp = LOADOP_CLEAR, .storeOp = STOREOP_STORE
        // };

        if (app.vtable.render)
            app.vtable.render(&app, &cmd);
        // Frame frame = GPU_BeginFrame(&cmd, app.window->handle, &desc);
        //
        // uint64_t t2 = SDL_GetPerformanceCounter(), t3;
        //
        // if (frame.swapchainTexture) {
        //     RenderPass pass = GPU_BeginRenderPass(&frame);
        //
        //     if (pass.handle) {
        //         GPU_EndRenderPass(&pass);
        //     }
        //     profiler.acquiredFrames++;
        //
        //     t3 = SDL_GetPerformanceCounter();
        //     GPU_SubmitCommand(&cmd);
        // } else {
        //     profiler.skippedFrames++;
        //     SDL_CancelGPUCommandBuffer(cmd.handle);
        //
        //     t3 = SDL_GetPerformanceCounter();
        // }
        //
        // uint64_t t4 = SDL_GetPerformanceCounter();
        //
        // FrameProfiler_Add(&profiler, dt, t0, t1, t2, t3, t4);
    }

shutdown:
    if (app.window->gpuDevice)
        SDL_WaitForGPUIdle(app.window->gpuDevice);

    if (app.vtable.cleanup)
        app.vtable.cleanup(&app);

    Window_Destroy(app.window);

    return EXIT_SUCCESS;
}
