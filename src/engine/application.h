#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "gpu/gpu.h"
#include "gpu/window.h"

#include <SDL3/SDL_events.h>
#include <stdbool.h>

typedef struct Application Application;

typedef struct {
    bool (*init)(Application *app);
    bool (*upload)(Application *app, CopyPass *pass);

    void (*event)(Application *app, const SDL_Event *event);
    void (*update)(Application *app, float dt);

    void (*prepare)(Application *app, CommandBuffer *cmd);

    void (*render)(Application *app, RenderPass *pass);
    void (*cleanup)(Application *app);
} ApplicationVTable;

struct Application {
    Window *window;
    void   *userdata;

    const char *title;
    int         width, height;

    bool              running;
    ApplicationVTable vtable;
};

Application Application_Create(void);

#endif
