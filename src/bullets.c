#include "bullets.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

bool BulletSim_Create(BulletSim *sim, uint32_t capacity) {
    if (!sim || capacity == 0)
        return false;

    *sim = (BulletSim){ 0 };

    sim->capacity = capacity;
    sim->positions = calloc(capacity, sizeof(Vec2));
    sim->velocities = calloc(capacity, sizeof(Vec2));
    sim->sizes = calloc(capacity, sizeof(Vec2));
    sim->rotations = calloc(capacity, sizeof(float));
    sim->angularVelocities = calloc(capacity, sizeof(float));
    sim->life = calloc(capacity, sizeof(float));
    sim->rotationModes = calloc(capacity, sizeof(BulletRotationMode));
    sim->spriteIDs = calloc(capacity, sizeof(uint32_t));
    sim->tints = calloc(capacity, sizeof(uint32_t));

    if (!sim->capacity || !sim->positions || !sim->sizes || !sim->rotations || !sim->angularVelocities ||
        !sim->life || !sim->rotationModes || !sim->spriteIDs || !sim->tints) {
        BulletSim_Destory(sim);
        return false;
    }

    return true;
}

void BulletSim_Destory(BulletSim *sim) {
    if (!sim)
        return;

    free(sim->positions);
    free(sim->velocities);
    free(sim->sizes);
    free(sim->rotations);
    free(sim->angularVelocities);
    free(sim->life);
    free(sim->rotationModes);
    free(sim->spriteIDs);
    free(sim->tints);

    *sim = (BulletSim){ 0 };
}

bool BulletSim_Spawn(BulletSim *b, BulletDesc *desc) {
    if (b->count >= b->capacity)
        return false;

    uint32_t i = b->count++;

    b->positions[i] = desc->position;
    b->velocities[i] = desc->velocity;
    b->sizes[i] = desc->size;
    b->rotations[i] = desc->rotation;
    b->angularVelocities[i] = desc->angularVelocity;
    b->life[i] = 0.f;
    b->rotationModes[i] = desc->rotationMode;
    b->spriteIDs[i] = desc->spriteID;
    b->tints[i] = desc->tint;

    return true;
}

void BulletSim_Remove(BulletSim *b, uint32_t i) {
    if (i >= b->count)
        return;

    uint32_t last = --b->count;

    if (i == last)
        return;

    b->positions[i] = b->positions[last];
    b->velocities[i] = b->velocities[last];
    b->sizes[i] = b->sizes[last];
    b->rotations[i] = b->rotations[last];
    b->angularVelocities[i] = b->angularVelocities[last];
    b->life[i] = b->life[last];
    b->rotationModes[i] = b->rotationModes[last];
    b->spriteIDs[i] = b->spriteIDs[last];
    b->tints[i] = b->tints[last];
}

void BulletSim_Update(BulletSim *b, float dt, float width, float height) {
    const float margin = 32.f;

    uint32_t i = 0;

    while (i < b->count) {
        b->positions[i].x += b->velocities[i].x * dt;
        b->positions[i].y += b->velocities[i].y * dt;

        switch (b->rotationModes[i]) {
            case BULLET_ROTATION_FIXED:
                break;

            case BULLET_ROTATION_VELOCITY:
                if (b->velocities[i].x != 0.f || b->velocities[i].y != 0.f)
                    b->rotations[i] = atan2f(b->velocities[i].y, b->velocities[i].x);
                break;

            case BULLET_ROTATION_SPIN:
                b->rotations[i] += b->angularVelocities[i] * dt;
                break;
        }
        b->life[i] += dt;

        Vec2 p = b->positions[i];

        if (p.x < -margin || p.x > width + margin || p.y < -margin || p.y > height + margin) {
            BulletSim_Remove(b, i);
            continue;
        }

        i++;
    }
}
