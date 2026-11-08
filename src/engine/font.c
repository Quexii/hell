#include "font.h"
#include "util.h"

#include <SDL3/SDL_filesystem.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

bool Font_LoadStatic(Font *font, Texture *texture, Sampler *sampler, const char *metricesPath) {
    if (!font || !texture || !sampler || !metricesPath)
        return false;

    char path[PATH_MAX];

    if (!Asset_GetPath(path, sizeof(path), "fonts", metricesPath))
        return false;

    size_t fileSize = 0;
    char *data = SDL_LoadFile(path, &fileSize);

    if (!data) {
        SDL_Log("Failed to load font '%s': %s", path, SDL_GetError());
        return false;
    }

    memset(font, 0, sizeof(*font));

    const char *cursor = data;
    const char *end = data + fileSize;

    uint32_t glyphCount = 0;
    bool commonFound = false;
    bool success = false;

    while (cursor < end) {
        const char *lineEnd = memchr(cursor, '\n', (size_t) (end - cursor));

        if (!lineEnd)
            lineEnd = end;

        size_t length = (size_t) (lineEnd - cursor);

        char line[1024];

        if (length >= sizeof(line)) {
            SDL_Log("Font metrics line too long: %s", path);
            goto cleanup;
        }

        memcpy(line, cursor, length);
        line[length] = '\0';

        cursor = lineEnd < end ? lineEnd + 1 : end;

        if (strncmp(line, "common ", 7) == 0) {
            int atlasW = 0;
            int atlasH = 0;
            int pages = 0;
            int packed = 0;

            int fields = sscanf(
                line,
                "common lineHeight=%f base=%f "
                "ascent=%*f descent=%*f "
                "scaleW=%d scaleH=%d pages=%d packed=%d",
                &font->lineHeight,
                &font->baseline,
                &atlasW,
                &atlasH,
                &pages,
                &packed
            );

            if (fields != 6 || atlasW <= 0 || atlasH <= 0 || pages != 1 ||
                font->lineHeight <= 0.f) {
                SDL_Log("Invalid font metrics in '%s'", path);
                goto cleanup;
            }

            font->atlasWidth = (uint32_t) atlasW;
            font->atlasHeight = (uint32_t) atlasH;

            commonFound = true;
        }

        else if (strncmp(line, "char ", 5) == 0) {
            int id, x, y, w, h;
            int page, channel;
            float xoff, yoff, advance;

            int fields = sscanf(
                line,
                "char id=%d x=%d y=%d "
                "width=%d height=%d "
                "xoffset=%f yoffset=%f "
                "xadvance=%f page=%d chnl=%d",
                &id,
                &x,
                &y,
                &w,
                &h,
                &xoff,
                &yoff,
                &advance,
                &page,
                &channel
            );

            if (fields != 10) {
                SDL_Log("Invalid glyph entry: %s", line);
                goto cleanup;
            }

            if (id < 0 || id >= FONT_MAX_GLYPHS)
                continue;

            if (!commonFound) {
                SDL_Log("Glyph encountered before common metrics");
                goto cleanup;
            }

            if (x < 0 || y < 0 || w < 0 || h < 0 || (int64_t) x + w > font->atlasWidth ||
                (int64_t) y + h > font->atlasHeight) {
                SDL_Log("Invalid glyph bounds for character %d", id);
                goto cleanup;
            }

            float invW = 1.f / (float) font->atlasWidth;
            float invH = 1.f / (float) font->atlasHeight;

            Glyph *glyph = &font->glyphs[id];

            *glyph = (Glyph){
                .uv = { x * invW, y * invH, w * invW, h * invH },
                .size = { (float) w, (float) h },
                .offset = { xoff, yoff },
                .advance = advance,
                .valid = true
            };

            glyphCount++;
        }
    }

    if (!commonFound || glyphCount == 0) {
        SDL_Log("Font contains no usable glyphs: %s", path);
        goto cleanup;
    }

    SDL_Log(
        "Loaded font '%s': %u glyphs, %ux%u atlas, lineHeight=%.2f",
        metricesPath,
        glyphCount,
        font->atlasWidth,
        font->atlasHeight,
        font->lineHeight
    );

    success = true;
    font->texture = texture;
    font->sampler = sampler;

cleanup:
    SDL_free(data);

    if (!success)
        memset(font, 0, sizeof(*font));

    return success;
}
