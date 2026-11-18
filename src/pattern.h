#ifndef GAME_PATTERN_H
#define GAME_PATTERN_H

#include "bullets.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct Pattern Pattern;
typedef struct PatternTimer PatternTimer;
typedef struct PatternDesc PatternDesc;

typedef void (*PatternEmitFn)(Pattern *pattern, PatternTimer *tm, BulletSim *bullets);

typedef struct {
    PatternEmitFn fn;
    float interval;
} SchedDesc;

struct PatternDesc {
    const SchedDesc *scheds;
    uint32_t count;
    float duration;
};

struct PatternTimer {
    SchedDesc desc;

    float timer;
    float elapsed;
    uint32_t emitCount;
};

struct Pattern {
    PatternTimer *timers;
    uint32_t timerCount;

    Vec2 origin;
    float time;
    float duration;

    void *userdata;
};

Pattern Pattern_Create(const PatternDesc *desc, Vec2 origin);

bool Pattern_Update(Pattern *pattern, BulletSim *bullets, float dt);

void Pattern_Destroy(Pattern *pattern);

#endif
