#ifndef WINDOW_H
#define WINDOW_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <stdint.h>
#include "../util.h"

typedef struct {
    SDL_DisplayID id;
    const SDL_DisplayMode *mode;
    float refreshRate;
    double precciseRefreshRate;
} Display;

typedef struct {
    SDL_Window    *handle;
    const char    *title;
    int32_t        width, height;
    Display display;
    FrameLimiter frameLimiter;
    FrameTimer frameTimer;
    SDL_GPUDevice *gpuDevice;
} Window;

Window *Window_Create(const char *title, int32_t width, int32_t height);
void    Window_Init(Window *window);
Display GetWindowDisplay(Window *window);
void    Window_Destroy(Window *window);

#endif /* ifndef WINDOW_H */
