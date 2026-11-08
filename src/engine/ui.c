#include "ui.h"

#include "application.h"
#include "maths.h"
#include "renderer.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_rect.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool UI_Init(UI *ui, Batch *batch, uint32_t capacity) {
    ui->batch = batch;
    ui->commands = calloc(capacity, sizeof(UICommand));
    ui->count = 0;
    ui->capacity = capacity;

    if (!ui->commands || ui->capacity == 0) {
        fprintf(
            stderr,
            "Failed to initialize UI; commands: %p, capacity: %u\n",
            (void *) ui->commands,
            ui->capacity
        );
        return false;
    }

    return true;
}

void UI_Begin(UI *ui) {
    ui->count = 0;
    ui->clipDepth = 0;
}

bool UI_PushClip(UI *ui, Rect rect) {
    if (ui->clipDepth >= UI_MAX_CLIP_DEPTH)
        return false;

    if (ui->clipDepth > 0) {
        rect = Rect_Intersect(&rect, &ui->clipStack[ui->clipDepth - 1]);
    }

    ui->clipStack[ui->clipDepth++] = rect;
    return true;
}

void UI_PopClip(UI *ui) {
    if (ui->clipDepth > 0)
        ui->clipDepth--;
}

bool UI_Append(UI *ui, Rect rect, Rect uv, Vec4 color, UIDrawMode mode, Texture *texture, Sampler *sampler) {
    UICommand *last = ui->count ? &ui->commands[ui->count - 1] : NULL;

    const Rect *clip = ui->clipDepth ? &ui->clipStack[ui->clipDepth - 1] : NULL;

    bool sameState = last && last->texture == texture && last->sampler == sampler &&
                     last->clipping == (clip != NULL) && (!clip || Rect_Compare(&last->clip, clip));

    if (!sameState && ui->count >= ui->capacity)
        return false;

    uint32_t firstVertex = ui->batch->count;

    UIVertex *vertices = Batch_Reserve(ui->batch, 6);
    if (!vertices)
        return false;

    if (sameState) {
        last->vertexCount += 6;
    } else {
        UICommand *cmd = &ui->commands[ui->count++];

        *cmd = (UICommand){ .firstVertex = firstVertex,
                            .vertexCount = 6,
                            .texture = texture,
                            .sampler = sampler,
                            .clipping = clip != NULL,
                            .clip = clip ? *clip : (Rect){ 0 } };
    }

    vertices[0] = (UIVertex){
        { rect.x, rect.y },
        { uv.x,   uv.y   },
        color, mode
    };
    vertices[1] = (UIVertex){
        { rect.x + rect.w, rect.y },
        { uv.x + uv.w,     uv.y   },
        color, mode
    };
    vertices[2] = (UIVertex){
        { rect.x + rect.w, rect.y + rect.h },
        { uv.x + uv.w,     uv.y + uv.h     },
        color, mode
    };
    vertices[3] = (UIVertex){
        { rect.x, rect.y },
        { uv.x,   uv.y   },
        color, mode
    };
    vertices[4] = (UIVertex){
        { rect.x, rect.y + rect.h },
        { uv.x,   uv.y + uv.h     },
        color, mode
    };
    vertices[5] = (UIVertex){
        { rect.x + rect.w, rect.y + rect.h },
        { uv.x + uv.w,     uv.y + uv.h     },
        color, mode
    };

    return true;
}

void UI_Draw(Renderer2D *r, RenderPass *pass, void *user) {
    if (!r || !pass || !user) {
        return;
    }

    UIContext *ctx = user;
    if (!ctx->ui)
        return;

    UI *ui = ctx->ui;

    Batch *b = ui->batch;

    if (!b || ui->count == 0)
        return;

    GPU_BindPipeline(pass, b->pipeline);
    GPU_BindVBO(pass, &b->gpu, 0, 0);

    for (uint32_t i = 0; i < ui->count; i++) {
        UICommand *cmd = &ui->commands[i];
        UICommand *last = i > 0 ? &ui->commands[i - 1] : NULL;

        bool textureChanged =
            !last || last->texture != cmd->texture || last->sampler != cmd->sampler;

        bool clipChanged = !last || last->clipping != cmd->clipping ||
                           (cmd->clipping && !Rect_Compare(&last->clip, &cmd->clip));

        if (textureChanged) {
            if (cmd->texture && cmd->sampler) {
                GPU_BindFragmentSampler(
                    pass, (Texture *[]){ cmd->texture }, (Sampler *[]){ cmd->sampler }, 1, 0
                );
            } else {
                GPU_BindFragmentSampler(
                    pass, (Texture *[]){ b->texture }, (Sampler *[]){ b->sampler }, 1, 0
                );
            }
        }

        if (clipChanged) {
            if (cmd->clipping) {
                SDL_Rect clip = {
                    (int) cmd->clip.x, (int) cmd->clip.y, (int) cmd->clip.w, (int) cmd->clip.h
                };
                SDL_SetGPUScissor(pass->handle, &clip);
            } else {
                SDL_SetGPUScissor(
                    pass->handle,
                    &(
                        SDL_Rect
                    ){ .x = 0, .y = 0, .w = ctx->app->window->width, .h = ctx->app->window->height }
                );
            }
        }
        if (cmd->vertexCount > 0) {
            GPU_DrawPrimitives(pass, cmd->vertexCount, 1, cmd->firstVertex, 0);
        }
    }
}

void UI_Destroy(UI *ui) {
    free(ui->commands);
}
