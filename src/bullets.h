#ifndef GAME_BULLETS_H
#define GAME_BULLETS_H

#include "engine/maths.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_BULLETS 100000

enum { BULLETTYPE_CIRCLE, BULLETTYPE_HEX, BULLETTYPE_LAST };

typedef struct {
    Vec2 position, size;
    float rotation;
    uint32_t spriteID, tint;
} BulletData;

typedef enum {
    BULLET_ROTATION_FIXED,
    BULLET_ROTATION_VELOCITY,
    BULLET_ROTATION_SPIN
} BulletRotationMode;

typedef struct {
    Vec2 position, velocity, size;
    float rotation, angularVelocity;
    BulletRotationMode rotationMode;
    uint32_t spriteID, tint;
} BulletDesc;

typedef struct {
    Vec2 *positions, *velocities, *sizes;
    float *rotations, *angularVelocities, *life;
    BulletRotationMode *rotationModes;
    uint32_t *spriteIDs, *tints, count, capacity;
} BulletSim;

bool BulletSim_Create(BulletSim *sim, uint32_t capacity);
void BulletSim_Destory(BulletSim *sim);

bool BulletSim_Spawn(BulletSim *sim, BulletDesc *desc);
void BulletSim_Remove(BulletSim *sim, uint32_t index);

void BulletSim_Update(BulletSim *sim, float dt, float w, float h);

#endif // GAME_BULLETS_H
