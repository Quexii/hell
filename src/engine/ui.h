#ifndef ENGINE_UI_H
#define ENGINE_UI_H

#include "application.h"
#include "font.h"
#include "gpu/gpu.h"
#include "maths.h"
#include "renderer.h"

#include <stdbool.h>
#include <stdint.h>

#define UI_MAX_CLIP_DEPTH 32

typedef struct UI UI;

typedef enum { UI_DRAW_SOLID, UI_DRAW_TEXTURE, UI_DRAW_TEXT } UIDrawMode;

typedef struct {
    Vec2 position, uv;
    Vec4 color;
    uint32_t mode;
} UIVertex;

typedef struct {
    UI *ui;
    Application *app;
} UIContext;

typedef struct {
    uint32_t firstVertex, vertexCount;

    Texture *texture;
    Sampler *sampler;

    Rect clip;
    bool clipping;
} UICommand;

struct UI {
    Batch *batch;

    UICommand *commands;
    uint32_t count;
    uint32_t capacity;

    Rect clipStack[UI_MAX_CLIP_DEPTH];
    uint32_t clipDepth;
};

bool UI_Init(UI *ui, Batch *batch, uint32_t capacity);
void UI_Begin(UI *ui);
bool UI_PushClip(UI *ui, Rect rect);
void UI_PopClip(UI *ui);
bool UI_Append(
    UI *ui, Rect rect, Rect uv, Vec4 color, UIDrawMode mode, Texture *texture, Sampler *sampler
);

static inline bool UI_Rect(UI *ui, float x, float y, float w, float h, Vec4 color) {
    return UI_Append(
        ui, (Rect){ x, y, w, h }, (Rect){ 0.f, 0.f, 1.f, 1.f }, color, UI_DRAW_SOLID, NULL, NULL
    );
}

static inline bool UI_Texture(
    UI *ui, float x, float y, float w, float h, Texture *texture, Sampler *sampler, Vec4 tint
) {
    return UI_Append(
        ui,
        (Rect){ x, y, w, h },
        (Rect){ 0.f, 0.f, 1.f, 1.f },
        tint,
        UI_DRAW_TEXTURE,
        texture,
        sampler
    );
}

static inline void
UI_Text(UI *ui, const Font *font, const char *text, Vec2 position, float fontSize, Vec4 color) {
    float scale = fontSize / font->lineHeight;

    float startX = position.x;
    float x = startX;
    float y = position.y;

    for (const unsigned char *p = (const unsigned char *) text; *p; p++) {
        if (*p == '\n') {
            x = startX;
            y += font->lineHeight * scale;
            continue;
        }

        if (*p >= FONT_MAX_GLYPHS)
            continue;

        const Glyph *glyph = &font->glyphs[*p];
        if (!glyph->valid)
            continue;

        if (*p != ' ') {
            Rect rect = { x + glyph->offset.x * scale,
                          y + glyph->offset.y * scale,
                          glyph->size.x * scale,
                          glyph->size.y * scale };

            UI_Append(ui, rect, glyph->uv, color, UI_DRAW_TEXT, font->texture, font->sampler);
        }

        x += glyph->advance * scale;
    }
}

void UI_Draw(Renderer2D *r, RenderPass *pass, void *user);
void UI_Destroy(UI *ui);

#endif // ENGINE_UI_H
