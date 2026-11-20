#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include "collider.h"
#include "engine/application.h"
#include "engine/gpu/gpu.h"
#include "player.h"
#include "world.h"
#include "stage.h"

#include <stdbool.h>
#include <stdint.h>

#define PLAYER_WIDTH 40
#define PLAYER_HEIGHT 60

typedef struct Level Level;

struct Level {
    World world;
    bool levelRunning;
    StageId currentStage;
};

// we dont speak of this 🫩
extern Enemy enemyDefinitions[ENEMY_ID_LAST];
extern Stage stageDefinitions[STAGE_ID_LAST];

void Level_LoadEnemies(void);

void Level_LoadStages(void);

void Level_StartStage(Level *level, StageId stage);

void Level_Tick(Level *level, Application *app, float dt);

void Level_Draw(Level *level, Application *app, CommandBuffer *cmd);

void Level_Destroy(Level *level);

#endif // GAME_LEVEL_H
