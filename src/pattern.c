#include "pattern.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Pattern Pattern_Create(PatternTimer timers[], uint32_t timerCount, Vec2 origin) {
    if (timerCount <= 0 || !timers) {
        fprintf(stderr, "Invalid `timerCount=%i` or `timers%p` when creating a pattern!\n", timerCount, (void*)timers);
        exit(EXIT_FAILURE);
    }

    PatternTimer *timersCopy = malloc(sizeof(PatternTimer) * timerCount);
    memcpy(timersCopy, timers, sizeof(PatternTimer) * timerCount);

    if (!timersCopy) {
        fprintf(stderr, "Failed to copy timers when creating a pattern!\n");
        exit(EXIT_FAILURE);
    }

    return (Pattern){ .timers = timersCopy, .timerCount = timerCount, .origin = origin };
}

void Pattern_Update(Pattern *p, BulletSim *bullets, float dt) {
    if (!p || p->timerCount == 0 || !p->timers)
        return;

    p->time += dt;

    for (uint32_t i = 0; i < p->timerCount; i++) {
        PatternTimer *pt = &p->timers[i];

        pt->timer += dt;
        while (pt->timer >= pt->desc.interval) {
            pt->desc.fn(p, pt, bullets);
            pt->desc.emitCount++;
            pt->elapsed += pt->desc.interval;
            pt->timer -= pt->desc.interval;
        }
    }
}

void Pattern_Destroy(Pattern *pattern) {
    if (!pattern || !pattern->timers) {
        fprintf(stderr, "Unable to destroy pattern! (self=%p)\n", (void*)pattern);
        return;
    }
    free(pattern->timers);
}
