#ifndef GAME_COLLIDER_H
#define GAME_COLLIDER_H

#include "engine/maths.h"

#define COLLIDER_GRID_SIZE 56
#define COLLIDER_GRID_COLUMNS ((PLAYFIELD_WIDTH + COLLIDER_GRID_SIZE - 1) / COLLIDER_GRID_SIZE)
#define COLLIDER_GRID_ROWS ((PLAYFIELD_HEIGHT + COLLIDER_GRID_SIZE - 1) / COLLIDER_GRID_SIZE)

typedef struct Collider Collier;
typedef struct CollisionCell CollisionCell;
typedef struct CollisionEntry CollisionEntry;
typedef struct CollisionGrid CollisionGrid;
typedef struct Level Level;

typedef enum { COLLIDER_NONE, COLLIDER_CIRCLE, COLLIDER_OBB } ColliderType;

typedef enum {
    COLLIDER_LAYER_PLAYER = 0,
    COLLIDER_LAYER_PLAYER_BULLET,
    COLLIDER_LAYER_PLAYER_GRAZE,
    COLLIDER_LAYER_PLAYER_PICKUP,
    COLLIDER_LAYER_ENEMY,
    COLLIDER_LAYER_ENEMY_BULLET,
    COLLIDER_LAYER_PICKUP,
    COLLIDER_LAYER_LAST,
} ColliderLayer;

typedef void (*CollisionHandler)(uint32_t a, uint32_t b, void *userdata);

struct Collider {
    ColliderType type;
    ColliderLayer layer;

    union {
        struct {
            float radius;
        } circle;

        struct {
            Vec2 halfSize;
            float rotation;
        } obb;
    };
};

struct CollisionCell {
    uint32_t offset;
    uint32_t count;
};

struct CollisionEntry {
    const Collier *collider;
    const Vec2 *position;
    void *owner;
    uint32_t index;
};

struct CollisionGrid {
    uint32_t columns, rows;
    float cellSize;
    uint32_t capacity;
    uint32_t indexCapacity;

    CollisionCell *cells;
    uint32_t *indices;

    uint32_t *counts;
    uint32_t *cursor;

    CollisionEntry *entries;
    uint32_t entryCount;
};

bool CollisionGrid_Init(
    CollisionGrid *grid, uint32_t columns, uint32_t rows, float cellSize, uint32_t capacity
);

void CollisionGrid_Clear(CollisionGrid *grid);

void CollisionGrid_Destroy(CollisionGrid *grid);

void CollisionGrid_Begin(CollisionGrid *grid);

bool CollisionGrid_Add(
    CollisionGrid *grid, const Collier *collider, const Vec2 *position, void *owner, uint32_t index
);

bool CollisionGrid_End(CollisionGrid *grid);

typedef void (*CollisionEntryHandler)(
    const CollisionEntry *a, const CollisionEntry *b, void *ownerA, void *ownerB, void *userdata
);

void CollisionGrid_CheckEntries(
    const CollisionGrid *grid, CollisionEntryHandler handler, void *userdata
);

bool CollisionGrid_Build(
    CollisionGrid *grid, const Collier *colliders, const Vec2 *positions, uint32_t count
);

void CollisionGrid_Check(
    const CollisionGrid *grid,
    const Collier *colliders,
    const Vec2 *positions,
    uint32_t count,
    CollisionHandler handler,
    void *userdata
);

bool Collider_Intersects_Circle_Circle(Collier *a, Collier *b, Vec2 *pos1, Vec2 *pos2);

bool Collider_Intersects_Circle_OBB(Collier *a, Collier *b, Vec2 *pos1, Vec2 *pos2);

bool Collider_Intersects_OBB_OBB(Collier *a, Collier *b, Vec2 *pos1, Vec2 *pos2);

bool Collider_Intersects(Collier *a, Collier *b, Vec2 *pos1, Vec2 *pos2);

bool ColliderLayer_Intersects(ColliderLayer a, ColliderLayer b);

#endif // GAME_COLLIDER_H
