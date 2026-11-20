#ifndef BULLET_PATTERNS_H
#define BULLET_PATTERNS_H

#include "bullets.h"
#include "engine/maths.h"
#include "level.h"
#include "pattern.h"

#include <math.h>

static inline void Pattern_Mirrored(Pattern *p, PatternTimer *tm, BulletSim *bullets) {
    const float offset = fabsf(sinf(tm->elapsed) * 0.5f);
    const float speed = 100.f;

    for (uint32_t i = 0; i < 8; i++) {
        float base = RAD90 + RAD45 * (float) i;

        for (uint32_t side = 0; side < 2; side++) {
            float angle = base + (side == 0 ? offset : -offset);

            Vec2 dir = AngleToDirection(angle);

            Collier col = { 0 };
            col.layer = COLLIDER_LAYER_ENEMY_BULLET;
            if (!(tm->emitCount % 2)) {
                col.type = COLLIDER_CIRCLE;
                col.circle.radius = 24.f * 0.5f;
            } else {
                col.type = COLLIDER_OBB;
                col.obb.halfSize = (Vec2){ 24.f * 0.5f, 12.f * 0.5f };
            }

            BulletSim_Spawn(
                bullets,
                &(BulletDesc){
                    .position = p->origin,
                    .velocity = { dir.x * speed, dir.y * speed                   },
                    .size = { 24.f,          tm->emitCount % 2 ? 12.f : 24.f },
                    .rotationMode = BULLET_ROTATION_VELOCITY,
                    .spriteID = tm->emitCount % 2,
                    .tint = RGBA8(255, 255, 255, 255),
                    .collider = col,
            }
            );
        }
    }
}

static inline void Pattern_Test(Pattern *p, PatternTimer *tm, BulletSim *bullets) {
    const float speed = 80.0f;
    Vec2 dir = AngleToDirection(tm->elapsed);

    BulletSim_Spawn(
        bullets,
        &(BulletDesc){
            .position = p->origin,
            .velocity = { dir.x * speed, dir.y * speed },
            .size = { 4.f * 3,          7.f * 3          },
            .rotationOffset = RAD270,
            .rotationMode = BULLET_ROTATION_VELOCITY,
            .spriteID = BULLETTYPE_KUNAI,
            .tint = RGBA8(255, 255, 255, 255),
            .collider = {
                .type = COLLIDER_OBB,
                .layer = COLLIDER_LAYER_ENEMY_BULLET,
                .obb.halfSize = { 6.f, 10.5f },
            },
    }
    );
}

// desc
static const SchedDesc Pattern_TestSchedules[] = {
    { .fn = Pattern_Test,     .interval = 0.15f },
    { .fn = Pattern_Mirrored, .interval = 0.4f  }
};

static const PatternDesc Pattern_TestDesc = { .scheds = Pattern_TestSchedules,
                                              .count = sizeof(Pattern_TestSchedules) /
                                                       sizeof(Pattern_TestSchedules[0]),
                                              .duration = 5.0f };

#endif // BULLET_PATTERNS_H
