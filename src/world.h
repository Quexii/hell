#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "bullets.h"
#include "collider.h"
#include "engine/maths.h"
#include "player.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct Enemy Enemy;
typedef struct EnemyCommand EnemyCommand;
typedef struct EnemyController EnemyController;
typedef struct PatternDesc PatternDesc;
typedef struct Pattern Pattern;
typedef struct World World;
typedef struct TimelineEvent TimelineEvent;
typedef struct Application Application;
typedef struct CommandBuffer CommandBuffer;

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

struct World {
    EnemyController controller;
    BulletSim bullets;
    Player player;
    CollisionGrid collisionGrid;
    uint32_t ticks;
};

bool EnemyController_Init(EnemyController *e, uint32_t capacity);

void EnemyController_Destroy(EnemyController *e);

bool EnemyController_Add(EnemyController *e, Vec2 position, uint32_t health, EnemyId enemyId);

bool EnemyController_Remove(EnemyController *e, uint32_t i);

bool World_Init(World *world);

void World_Destroy(World *world);

bool World_DoEvent(World *world, TimelineEvent *e);

void World_Tick(World *world, float dt);

void World_Render(World *world, Application *app, CommandBuffer *cmd);

#endif // GAME_WORLD_H
