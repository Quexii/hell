#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include "engine/application.h"
#include "engine/gpu/gpu.h"
#include "engine/maths.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct Stage Stage;
typedef struct TimelineEvent TimelineEvent;
typedef struct Enemy Enemy;
typedef struct EnemyCommand EnemyCommand;
typedef struct EnemyController EnemyController;
typedef struct Level Level;
typedef struct Player Player;
typedef struct Pattern Pattern;
typedef struct PatternDesc PatternDesc;

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
} CommandOp;

typedef enum { STAGE_ID_TEST = 0, STAGE_ID_LAST } StageId;

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

#endif // GAME_LEVEL_H
