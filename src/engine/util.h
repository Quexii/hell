#ifndef ENGINE_UTIL_H
#define ENGINE_UTIL_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t start, target;
} FrameLimiter;

typedef struct {
    uint64_t last, start, timer, frames;
    double delta;
    float fps;
} FrameTimer;

// FrameLimiter

static inline void FrameLimiter_Sync(FrameLimiter *ft, double fps) {
    ft->target = (uint64_t) (1000000000.0 / fps);
}

static inline void FrameLimiter_Begin(FrameLimiter *ft) {
    ft->start = SDL_GetTicksNS();
}

static inline void FrameLimiter_End(FrameLimiter *ft) {
    uint64_t elapsed = SDL_GetTicksNS() - ft->start;

    if (elapsed >= ft->target)
        return;

    uint64_t rem = ft->target - elapsed;

    if (rem > 1000000ULL)
        SDL_DelayNS(rem - 1000000ULL);
}

// FrameTimer

static inline void FrameTimer_Init(FrameTimer *ft) {
    uint64_t now = SDL_GetTicksNS();

    *ft = (FrameTimer){ .last = now, .start = now, .timer = now };
}

static inline void FrameTimer_Update(FrameTimer *ft) {
    uint64_t now = SDL_GetTicksNS();
    ft->delta = (now - ft->last) / 1e9f;
    ft->last = now;
    ft->start = now;
    ft->frames++;

    uint64_t elapsed = now - ft->timer;
    if (elapsed >= 1e9) {
        ft->fps = ft->frames / (elapsed / 1e9f);
        ft->frames = 0;
        ft->timer = now;
    }
}

static inline float FrameTimer_GetFrameTime(FrameTimer *ft) {
    return (SDL_GetTicksNS() - ft->start) / 1e9f;
}

static inline bool
Asset_GetPath(char *out, size_t capacity, const char *directory, const char *name) {
    if (!out || !capacity || !directory || !name)
        return false;

    const char *base = SDL_GetBasePath();
    if (!base)
        return false;

    int n = snprintf(out, capacity, "%sassets/%s/%s", base, directory, name);

    return n >= 0 && (size_t) n < capacity;
}
#endif /* ifndef ENGINE_UTIL_H */
