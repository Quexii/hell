#include "pattern.h"

#include <math.h>
#include <stdlib.h>

Pattern Pattern_Create(const PatternDesc *desc, Vec2 origin) {
    Pattern p = { 0 };

    if (!desc || !desc->scheds || desc->count == 0 || !isfinite(desc->duration) ||
        desc->duration <= 0.0f)
        return p;

    for (uint32_t i = 0; i < desc->count; i++) {
        if (!desc->scheds[i].fn || !isfinite(desc->scheds[i].interval) ||
            desc->scheds[i].interval <= 0.0f)
            return p;
    }

    p.timers = calloc(desc->count, sizeof(*p.timers));
    if (!p.timers)
        return (Pattern){ 0 };

    p.timerCount = desc->count;
    p.duration = desc->duration;
    p.origin = origin;

    for (uint32_t i = 0; i < desc->count; i++)
        p.timers[i].desc = desc->scheds[i];

    return p;
}

bool Pattern_Update(Pattern *p, BulletSim *bullets, float dt) {
    if (!p || !p->timers || !p->timerCount || !bullets)
        return true;

    if (dt <= 0.0f)
        return p->time >= p->duration;

    float remaining = p->duration - p->time;

    if (remaining <= 0.0f)
        return true;

    float step = fminf(dt, remaining);

    for (uint32_t i = 0; i < p->timerCount; i++) {
        PatternTimer *tm = &p->timers[i];

        tm->timer += step;

        while (tm->timer >= tm->desc.interval) {
            tm->desc.fn(p, tm, bullets);

            tm->emitCount++;
            tm->elapsed += tm->desc.interval;
            tm->timer -= tm->desc.interval;
        }
    }

    p->time += step;

    return p->time >= p->duration;
}

void Pattern_Destroy(Pattern *p) {
    if (!p)
        return;

    free(p->timers);
    *p = (Pattern){ 0 };
}
