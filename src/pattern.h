#ifndef GAME_PATTERN_H
#define GAME_PATTERN_H

#include "bullets.h"
typedef struct Pattern Pattern;

typedef void (*PatternEmitFn)(Pattern *pattern, BulletSim *bullets);

struct Pattern {
    PatternEmitFn emit;

    Vec2 origin;

    float time;
    float interval;
    float nextEmit;

    uint32_t emission;
    void *userdata;
};

static inline Pattern Pattern_Create(PatternEmitFn emit, Vec2 origin, float interval) {
    return (Pattern){ .emit = emit, .origin = origin, .interval = interval, .nextEmit = 0.f };
}

void Pattern_Update(Pattern *pattern, BulletSim *bullets, float dt);

#endif // GAME_PATTERN_H
