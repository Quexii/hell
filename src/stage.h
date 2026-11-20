#ifndef GAME_STAGE_H
#define GAME_STAGE_H

#include "engine/maths.h"
#include "world.h"

typedef struct Stage Stage;
typedef struct TimelineEvent TimelineEvent;

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

#endif // GAME_STAGE_H
