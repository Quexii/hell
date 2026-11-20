#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include "engine/application.h"
#include "engine/gpu/gpu.h"
#include "engine/maths.h"
#include "playfield.h"

#include <stdbool.h>
#include <stdint.h>

#define PLAYER_WIDTH 40
#define PLAYER_HEIGHT 60
// playfield size GCF
#define COLLIDER_GRID_SIZE 56
#define COLLIDER_GRID_COLUMNS ((PLAYFIELD_WIDTH + COLLIDER_GRID_SIZE - 1) / COLLIDER_GRID_SIZE)
#define COLLIDER_GRID_ROWS ((PLAYFIELD_HEIGHT + COLLIDER_GRID_SIZE - 1) / COLLIDER_GRID_SIZE)

typedef struct Stage Stage;
typedef struct TimelineEvent TimelineEvent;
typedef struct Enemy Enemy;
typedef struct EnemyCommand EnemyCommand;
typedef struct EnemyController EnemyController;
typedef struct Level Level;
typedef struct Player Player;
typedef struct Pattern Pattern;
typedef struct PatternDesc PatternDesc;
typedef struct Collider Collier;
typedef struct CollisionCell CollisionCell;
typedef struct CollisionEntry CollisionEntry;
typedef struct CollisionGrid CollisionGrid;

typedef enum {
    ENEMY_TYPE_BASIC = 0,
    ENEMY_TYPE_BOSS,
    ENEMY_TYPE_MAX,
} EnemyType;

typedef enum {
    ENEMY_ID_TEST = 0,
    ENEMY_ID_LAST,
} EnemyId;

typedef enum {
    OP_MOVE_TO,
    OP_MOVE_BY,
    OP_WAIT,
    OP_FIRE,
    OP_DIE,
} CommandOp;

typedef enum { STAGE_ID_TEST = 0, STAGE_ID_LAST } StageId;

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

struct Stage {
    char *name;
    TimelineEvent *events;
    uint32_t eventCount, current;
};

struct TimelineEvent {
    uint32_t time;
    EnemyId enemy;
    uint32_t health;
    Vec2 position;
};

struct Enemy {
    char *name;
    EnemyType type;
    EnemyCommand *commands;
    uint32_t commandCount;
};

typedef struct {
    Enemy p[ENEMY_ID_LAST];
} EnemyDefinitions;

struct EnemyCommand {
    CommandOp op;

    union {
        struct {
            Vec2 target;
            uint32_t duration;
        } move;

        struct {
            uint32_t ticks;
        } wait;

        struct {
            const PatternDesc *pattern;
        } fire;
    };
};

struct EnemyController {
    uint32_t count, capacity;
    Vec2 *positions;
    uint32_t *healths;
    const Enemy **enemies;
    uint32_t *commandIndices;
    uint32_t *commandTicks;
    Vec2 *moveStarts;
    Pattern *patterns;
};

struct Player {
    Vec2 position;
    uint32_t hearts, bombs;
    float speed;
    Collier collider, grazeCollider, collectCollider;
    float shootTimer, shootInterval;
    // TODO: Add characters with their own abilities...
    // possibly some game mechanics, roguelike/skilltree?
};

struct Level {
    Enemy enemies[ENEMY_ID_LAST];
    Player player;
    Stage stages[STAGE_ID_LAST];
    bool levelRunning;
    StageId currentStage;
    uint32_t ticks;
    EnemyController controller;
    CollisionGrid collisionGrid;
};

void Level_LoadEnemies(Level *level);

void Level_LoadStages(Level *level);

void Level_StartStage(Level *level, StageId stage);

bool EnemyController_Init(EnemyController *e, uint32_t capacity);

bool Level_Spawn(Level *level, TimelineEvent *t);

bool Level_Remove(Level *level, uint32_t i);

void EnemyController_Destroy(EnemyController *e);

void Level_Tick(Level *level, Application *app, float dt);

void Level_Draw(Level *level, Application *app, CommandBuffer *cmd);

void Level_Destroy(Level *level);

bool CollisionGrid_Init(
    CollisionGrid *grid, uint32_t columns, uint32_t rows, float cellSize, uint32_t capacity
);

void CollisionGrid_Clear(CollisionGrid *grid);

void CollisionGrid_Destroy(CollisionGrid *grid);

void CollisionGrid_Begin(CollisionGrid *grid);

bool CollisionGrid_Add(
    CollisionGrid *grid,
    const Collier *collider,
    const Vec2 *position,
    void *owner,
    uint32_t index
);

bool CollisionGrid_End(CollisionGrid *grid);

typedef void (*CollisionEntryHandler)(
    const CollisionEntry *a,
    const CollisionEntry *b,
    void *ownerA,
    void *ownerB,
    Level *level
);

void CollisionGrid_CheckEntries(
    const CollisionGrid *grid,
    CollisionEntryHandler handler,
    Level *level
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

#endif // GAME_LEVEL_H
