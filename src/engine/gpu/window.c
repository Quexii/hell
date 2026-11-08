#include "window.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>

Window *Window_Create(const char *title, int32_t width, int32_t height) {
    Window *window = malloc(sizeof(Window));

    window->title  = title;
    window->width  = width;
    window->height = height;

    return window;
}

void Window_Init(Window *window) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Failed -> SDL_Init(SDL_INIT_VIDEO)\n");
        exit(EXIT_FAILURE);
    }

    window->handle =
        SDL_CreateWindow(window->title, window->width, window->height, SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Failed -> SDL_CreateWindow(...)\n");
        Window_Destroy(window);
        exit(EXIT_FAILURE);
    }

    window->gpuDevice = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV,
#ifndef NDEBUG
        true
#else
        false
#endif /* ifdef RELEASE_DEBUG */
        ,
        NULL
    );

    if (!window->gpuDevice) {
        fprintf(stderr, "Failed -> SDL_CreateGPUDevice(...)\n");
        Window_Destroy(window);
        exit(EXIT_FAILURE);
    }

    if (!SDL_ClaimWindowForGPUDevice(window->gpuDevice, window->handle)) {
        fprintf(stderr, "Failed -> SDL_ClaimWindowForGPUDevice(...)\n");
        fprintf(stderr, "%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // SDL_SetGPUSwapchainParameters(window->gpuDevice, window->handle, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_MAILBOX);
    SDL_SetGPUAllowedFramesInFlight(window->gpuDevice, 3);

    window->display      = GetWindowDisplay(window);
    window->frameLimiter = (FrameLimiter){ 0, 0 };
    window->frameTimer   = (FrameTimer){ 0, 0, 0, 0, 0.0, 0.0f };

    FrameTimer_Init(&window->frameTimer);

    FrameLimiter_Sync(&window->frameLimiter, window->display.refreshRate);
}

Display GetWindowDisplay(Window *window) {
    SDL_DisplayID id = SDL_GetDisplayForWindow(window->handle);
    if (!id) {
        fprintf(stderr, "Failed -> SDL_GetDisplayForWindow(window)\n");
        fprintf(stderr, "%s\n", SDL_GetError());
        Window_Destroy(window);
        exit(EXIT_FAILURE);
    }

    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(id);
    if (!mode) {
        fprintf(stderr, "Failed -> SDL_GetCurrentDisplayMode(id)\n");
        fprintf(stderr, "%s\n", SDL_GetError());
        Window_Destroy(window);
        exit(EXIT_FAILURE);
    }

    float  refreshRate        = mode->refresh_rate;
    double preciseRefreshRate = 0.0;

    if (mode->refresh_rate_numerator != 0 && mode->refresh_rate_denominator != 0)
        preciseRefreshRate = (double) mode->refresh_rate_numerator / mode->refresh_rate_denominator;

    return (Display){ .id                  = id,
                      .mode                = mode,
                      .refreshRate         = refreshRate,
                      .precciseRefreshRate = preciseRefreshRate };
}

void Window_Destroy(Window *window) {
    if (window->gpuDevice)
        SDL_DestroyGPUDevice(window->gpuDevice);

    if (window->handle)
        SDL_DestroyWindow(window->handle);

    SDL_Quit();
}
