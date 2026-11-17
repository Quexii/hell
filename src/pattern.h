#ifndef GAME_PATTERN_H
#define GAME_PATTERN_H

#include "bullets.h"

#include <stdint.h>
typedef struct Pattern Pattern;
typedef struct PatternTimer PatternTimer;

typedef void (*PatternEmitFn)(Pattern *pattern, PatternTimer *tm, BulletSim *bullets);

typedef struct {
    PatternEmitFn fn;
    float interval; // replace with more configurations like late intervals or linked intervals
                    // (late fire after timer finishes not when available, linked fires with other
                    // timers, ...)
    uint32_t emitCount;
} SchedDesc;

typedef struct {
    const SchedDesc *scheds;
    uint32_t count;
} PatternDesc;

struct PatternTimer {
    SchedDesc desc;
    float timer, elapsed;
};

struct Pattern {
    PatternTimer *timers;
    uint32_t timerCount;

    Vec2 origin;
    float time;
    void *userdata;
};

Pattern Pattern_Create(PatternTimer timers[], uint32_t timerCount, Vec2 origin);

void Pattern_Update(Pattern *pattern, BulletSim *bullets, float dt);

void Pattern_Destroy(Pattern *pattern);

#endif // GAME_PATTERN_H
