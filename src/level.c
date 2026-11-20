#include "level.h"

#include "engine/maths.h"
#include "engine/util.h"
#include "patterns.h"
#include "world.h"

#include <nori.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Enemy enemyDefinitions[ENEMY_ID_LAST];
Stage stageDefinitions[STAGE_ID_LAST];

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void LoadEnemyDef(const char *file, EnemyId id) {
    char path[PATH_MAX];

    if (!Asset_GetPath(path, sizeof(path), "data/entity", file)) {
        fprintf(stderr, "Failed to locate enemy definition: %s\n", file);
        exit(EXIT_FAILURE);
    }

    nori_doc *doc = nori_new();
    if (!doc) {
        fprintf(stderr, "Failed to create new nori doc!\n");
        exit(EXIT_FAILURE);
    }

    nori_add_ref(doc, "Pattern_Test", (void *) &Pattern_TestDesc);

    if (nori_parse_file(doc, path) != NORI_OK) {
        const nori_error *err = nori_get_error(doc);
        fprintf(stderr, "Parse error at %zu:%zu: %s\n", err->line, err->column, err->message);

        nori_free(doc);
        exit(EXIT_FAILURE);
    }

    Enemy loaded = { 0 };

    const char *pName = nori_get_string(nori_doc_get(doc, "name"), NULL);
    if (!pName) {
        fprintf(stderr, "Enemy definition has no valid name: %s\n", file);
        nori_free(doc);
        exit(EXIT_FAILURE);
    }

    size_t nameLen = strlen(pName) + 1;
    loaded.name = malloc(nameLen);

    if (!loaded.name) {
        fprintf(stderr, "Unable to allocate enemy name!\n");
        nori_free(doc);
        exit(EXIT_FAILURE);
    }

    memcpy((char *) loaded.name, pName, nameLen);

    nori_node *commands = nori_doc_get(doc, "commands");
    if (!commands) {
        fprintf(stderr, "\"commands\" array not present in \"%s\", exiting!\n", path);
        nori_free(doc);
        exit(EXIT_FAILURE);
    }
    loaded.commandCount = (uint32_t) nori_count(commands);

    if (loaded.commandCount > 0) {
        loaded.commands = calloc(loaded.commandCount, sizeof(*loaded.commands));

        if (!loaded.commands) {
            fprintf(stderr, "Unable to allocate enemy commands!\n");
            nori_free(doc);
            exit(EXIT_FAILURE);
        }
    }

    for (uint32_t i = 0; i < loaded.commandCount; i++) {
        nori_node *at = nori_at(commands, i);
        EnemyCommand *cmd = &loaded.commands[i];

        const char *op = nori_get_string(nori_get(at, "op"), NULL);

        if (!op) {
            fprintf(stderr, "Enemy command has no operation!\n");
            nori_free(doc);
            exit(EXIT_FAILURE);
        }
        if (strcmp(op, "move_to") == 0) {
            cmd->op = OP_MOVE_TO;
            cmd->move.target = (Vec2){ nori_get_float(nori_at(nori_get(at, "target"), 0), 0.f),
                                       nori_get_float(nori_at(nori_get(at, "target"), 1), 0.f) };
            cmd->move.duration = (uint32_t) nori_get_long(nori_get(at, "duration"), 0);
        } else if (strcmp(op, "move_by") == 0) {
            cmd->op = OP_MOVE_BY;
            cmd->move.target = (Vec2){ nori_get_float(nori_at(nori_get(at, "target"), 0), 0.f),
                                       nori_get_float(nori_at(nori_get(at, "target"), 1), 0.f) };
            cmd->move.duration = (uint32_t) nori_get_long(nori_get(at, "duration"), 0);
        } else if (strcmp(op, "wait") == 0) {
            cmd->op = OP_WAIT;
            cmd->wait.ticks = (uint32_t) nori_get_long(nori_get(at, "ticks"), 0);
        } else if (strcmp(op, "fire") == 0) {
            cmd->op = OP_FIRE;
            cmd->fire.pattern = nori_get_ref(nori_get(at, "pattern"), NULL);
        } else {
            fprintf(stderr, "Unknown enemy operation: %s\n", op);
            nori_free(doc);
            exit(EXIT_FAILURE);
        }
    }
    loaded.type = (EnemyType) nori_get_long(nori_doc_get(doc, "type"), 0);

    nori_free(doc);

    enemyDefinitions[id] = loaded;
}

static void LoadStage(const char *file, StageId id) {
    char path[PATH_MAX];

    if (!Asset_GetPath(path, sizeof(path), "data/stage", file)) {
        fprintf(stderr, "Failed to locate stage definition: %s\n", file);
        exit(EXIT_FAILURE);
    }

    nori_doc *doc = nori_new();
    if (!doc) {
        fprintf(stderr, "Failed to create new nori doc!\n");
        exit(EXIT_FAILURE);
    }

    nori_parse_file(doc, path);

    Stage loaded = { 0 };
    const char *pName = nori_get_string(nori_doc_get(doc, "name"), NULL);
    if (!pName) {
        fprintf(stderr, "Stage definition has no valid name: %s\n", file);
        nori_free(doc);
        exit(EXIT_FAILURE);
    }

    size_t nameLen = strlen(pName) + 1;
    loaded.name = malloc(nameLen);

    if (!loaded.name) {
        fprintf(stderr, "Unable to allocate stage name!\n");
        nori_free(doc);
        exit(EXIT_FAILURE);
    }

    memcpy((char *) loaded.name, pName, nameLen);

    loaded.eventCount = nori_count(nori_doc_get(doc, "timeline"));
    loaded.events = malloc(sizeof(TimelineEvent) * loaded.eventCount);
    for (uint32_t i = 0; i < loaded.eventCount; i++) {
        nori_node *at = nori_at(nori_doc_get(doc, "timeline"), i);
        loaded.events[i].time = (uint32_t) nori_get_long(nori_get(at, "time"), 0);
        loaded.events[i].enemy = (EnemyId) nori_get_long(nori_get(at, "enemy"), 0);
        loaded.events[i].health = (uint32_t) nori_get_long(nori_get(at, "health"), 0);
        loaded.events[i].position =
            (Vec2){ nori_get_float(nori_at(nori_get(at, "position"), 0), 0.f),
                    nori_get_float(nori_at(nori_get(at, "position"), 1), 0.f) };
    }

    nori_free(doc);

    stageDefinitions[id] = loaded;
}

void Level_LoadEnemies(void) {
    LoadEnemyDef("test.nori", ENEMY_ID_TEST);
}

void Level_LoadStages(void) {
    LoadStage("test_stage.nori", STAGE_ID_TEST);
}

void Level_StartStage(Level *level, StageId id) {
    if (!level || (unsigned) id >= STAGE_ID_LAST)
        return;

    if (!World_Init(&level->world)) {
        level->levelRunning = false;
        return;
    }

    level->currentStage = id;
    level->levelRunning = true;
    stageDefinitions[level->currentStage].current = 0;
}

void Level_Tick(Level *level, Application *app, float dt) {
    if (!level || !app || !level->levelRunning)
        return;

    Stage *stage = &stageDefinitions[level->currentStage];
    while (stage->current < stage->eventCount &&
           stage->events[stage->current].time <= level->world.ticks) {
        if (!World_DoEvent(&level->world, &stage->events[stage->current])) {
            fprintf(stderr, "Failed to spawn scheduled enemy!\n");
            level->levelRunning = false;
            return;
        }

        stage->current++;
    }

    World_Tick(&level->world, dt);
    level->world.ticks++;
}

void Level_Draw(Level *level, Application *app, CommandBuffer *cmd) {
    World_Render(&level->world, app, cmd);

    { // Debug Draw
      // RDG_Line(
      //     &g->dbgBatch,
      //     PLAYFIELD_WIDTH / 2.f,
      //     0.f,
      //     PLAYFIELD_WIDTH / 2.f,
      //     PLAYFIELD_HEIGHT,
      //     2,
      //     (Vec3){ 1.f, 1.f, 1.f }
      // );
      // RDG_Line(
      //     &g->dbgBatch,
      //     0.f,
      //     PLAYFIELD_HEIGHT / 2.f,
      //     PLAYFIELD_WIDTH,
      //     PLAYFIELD_HEIGHT / 2.f,
      //     2,
      //     (Vec3){ 1.f, 1.f, 1.f }
      // );
      // EnemyController *e = &level->controller;
      //
      // for (uint32_t i = 0; i <= COLLIDER_GRID_COLUMNS; i++) {
      //     RDG_Line(
      //         &g->dbgBatch,
      //         i * COLLIDER_GRID_SIZE,
      //         0.f,
      //         i * COLLIDER_GRID_SIZE,
      //         PLAYFIELD_HEIGHT,
      //         1.f,
      //         (Vec3){ 0.5f, 0.5f, 0.5f }
      //     );
      // }
      //
      // for (uint32_t i = 0; i <= COLLIDER_GRID_ROWS; i++) {
      //     RDG_Line(
      //         &g->dbgBatch,
      //         0,
      //         i * COLLIDER_GRID_SIZE,
      //         PLAYFIELD_WIDTH,
      //         i * COLLIDER_GRID_SIZE,
      //         1.f,
      //         (Vec3){ 0.5f, 0.5f, 0.5f }
      //     );
      // }
      // uint32_t i = 0;
      // while (i < e->count) {
      //     RDG_Circle(&g->dbgBatch, e->positions[i].x, e->positions[i].y, 8.f, 1.f, RED_3);
      //     i++;
      // }
      // i = 0;
      // while (i < level->collisionGrid.entryCount) {
      //     CollisionGrid *gr = &level->collisionGrid;
      //     CollisionEntry *entry = &gr->entries[i];
      //     if (entry->collider->type == COLLIDER_CIRCLE)
      //         RDG_Circle(
      //             &g->dbgBatch,
      //             entry->position->x,
      //             entry->position->y,
      //             entry->collider->circle.radius,
      //             1.f,
      //             RED_3
      //         );
      //     if (entry->collider->type == COLLIDER_OBB)
      //         RDG_Rect(
      //             &g->dbgBatch,
      //             entry->position->x - entry->collider->obb.halfSize.x,
      //             entry->position->y - entry->collider->obb.halfSize.y,
      //             entry->collider->obb.halfSize.x * 2.f,
      //             entry->collider->obb.halfSize.y * 2.f,
      //             entry->collider->obb.rotation,
      //             1.f,
      //             RED_3
      //         );
      //     i++;
      // }
      //
      // RDG_Rect(
      //     &g->dbgBatch,
      //     level->player.position.x - PLAYER_WIDTH / 2.f,
      //     level->player.position.y - PLAYER_HEIGHT / 2.f,
      //     PLAYER_WIDTH,
      //     PLAYER_HEIGHT,
      //     0.f,
      //     1,
      //     GREEN_3
      // );
      //
      // RDG_Circle(
      //     &g->dbgBatch,
      //     level->player.position.x,
      //     level->player.position.y,
      //     level->player.collider.circle.radius,
      //     1,
      //     RED_3
      // );
      //
      // RDG_Circle(
      //     &g->dbgBatch,
      //     level->player.position.x,
      //     level->player.position.y,
      //     level->player.grazeCollider.circle.radius,
      //     1,
      //     BLUE_3
      // );
      //
      // RDG_Circle(
      //     &g->dbgBatch,
      //     level->player.position.x,
      //     level->player.position.y,
      //     level->player.collectCollider.circle.radius,
      //     1,
      //     GREEN_3
      // );
    }
}

void Level_Destroy(Level *level) {
    if (!level)
        return;

    if (level->levelRunning)
        World_Destroy(&level->world);

    for (uint32_t i = 0; i < ENEMY_ID_LAST; i++) {
        free(enemyDefinitions[i].name);
        free(enemyDefinitions[i].commands);
    }

    for (uint32_t i = 0; i < STAGE_ID_LAST; i++) {
        free(stageDefinitions[i].name);
        free(stageDefinitions[i].events);
    }

    *level = (Level){ 0 };
}
