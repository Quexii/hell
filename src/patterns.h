#ifndef BULLET_PATTERNS_H
#define BULLET_PATTERNS_H

#include "bullets.h"
#include "engine/maths.h"
#include "pattern.h"

#include <math.h>

static inline void Pattern_Mirrored(Pattern *p, BulletSim *bullets) {
    const float offset = fabsf(sinf(p->nextEmit) * 0.5f);
    const float speed = 100.f;

    for (uint32_t i = 0; i < 8; i++) {
        float base = RAD90 + RAD45 * (float) i;

        for (uint32_t side = 0; side < 2; side++) {
            float angle = base + (side == 0 ? offset : -offset);

            Vec2 dir = AngleToDirection(angle);

            BulletSim_Spawn(
                bullets,
                &(BulletDesc){
                    .position = p->origin,
                    .velocity = { dir.x * speed, dir.y * speed                 },
                    .size = { 24.f,          p->emission % 2 ? 12.f : 24.f },
                    .rotationMode = BULLET_ROTATION_VELOCITY,
                    .spriteID = p->emission % 2,
                    .tint = RGBA8(255, 255, 255, 255)
            }
            );
        }
    }
}

#endif // BULLET_PATTERNS_H
