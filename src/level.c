#include "level.h"

#include "engine/maths.h"
#include "engine/util.h"
#include "game.h"
#include "patterns.h"

#include <nori.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void LoadEnemyDef(Level *level, const char *file, EnemyId id) {
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
    loaded.commands = malloc(sizeof(EnemyCommand) * loaded.commandCount);
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

    level->enemies[id] = loaded;
}

static void LoadStage(Level *level, const char *file, StageId id) {
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

    level->stages[id] = loaded;
}

static void Enemy_Tick(EnemyController *e, BulletSim *bullets, uint32_t i, float dt) {
    const Enemy *enemy = e->enemies[i];

    while (e->commandIndices[i] < enemy->commandCount) {
        const EnemyCommand *cmd = &enemy->commands[e->commandIndices[i]];

        switch (cmd->op) {
            case OP_MOVE_TO:
            case OP_MOVE_BY:
                {
                    uint32_t duration = cmd->move.duration;
                    if (duration == 0) {
                        if (cmd->op == OP_MOVE_TO) {
                            e->positions[i] = cmd->move.target;
                        } else {
                            e->positions[i].x += cmd->move.target.x;
                            e->positions[i].y += cmd->move.target.y;
                        }

                        e->commandIndices[i]++;
                        e->commandTicks[i] = 0;
                        continue;
                    }

                    if (e->commandTicks[i] == 0)
                        e->moveStarts[i] = e->positions[i];

                    Vec2 start = e->moveStarts[i];
                    Vec2 target = cmd->move.target;

                    if (cmd->op == OP_MOVE_BY) {
                        target.x += start.x;
                        target.y += start.y;
                    }

                    uint32_t elapsed = ++e->commandTicks[i];

                    float t = (float) elapsed / (float) duration;
                    if (t > 1.0f)
                        t = 1.0f;

                    e->positions[i] = (Vec2){ start.x + (target.x - start.x) * t,
                                              start.y + (target.y - start.y) * t };

                    if (elapsed >= duration) {
                        e->commandIndices[i]++;
                        e->commandTicks[i] = 0;
                        continue;
                    }

                    return;
                }

            case OP_WAIT:
                {
                    if (cmd->wait.ticks == 0) {
                        e->commandIndices[i]++;
                        e->commandTicks[i] = 0;
                        continue;
                    }

                    if (++e->commandTicks[i] >= cmd->wait.ticks) {
                        e->commandIndices[i]++;
                        e->commandTicks[i] = 0;
                        continue;
                    }
                    return;
                }

            case OP_FIRE:
                {
                    Pattern *pattern = &e->patterns[i];
                    if (!pattern->timers) {
                        *pattern = Pattern_Create(cmd->fire.pattern, e->positions[i]);

                        if (!pattern->timers) {
                            fprintf(stderr, "Failed to instantiate enemy pattern!\n");

                            e->commandIndices[i]++;
                            e->commandTicks[i] = 0;
                            continue;
                        }
                    }

                    pattern->origin = e->positions[i];

                    if (Pattern_Update(pattern, bullets, dt)) {
                        Pattern_Destroy(pattern);

                        e->commandIndices[i]++;
                        e->commandTicks[i] = 0;
                        continue;
                    }
                    return;
                }

            default:
                fprintf(stderr, "Invalid enemy command: %d\n", cmd->op);

                e->commandIndices[i]++;
                e->commandTicks[i] = 0;
                continue;
        }
    }
}

void Level_LoadEnemies(Level *level) {
    LoadEnemyDef(level, "test.nori", ENEMY_ID_TEST);
}

void Level_LoadStages(Level *level) {
    LoadStage(level, "test_stage.nori", STAGE_ID_TEST);
}

void Level_StartStage(Level *level, StageId id) {
    if (!level || (unsigned) id >= STAGE_ID_LAST)
        return;

    Stage *stage = &level->stages[id];

    EnemyController_Destroy(&level->controller);

    if (!EnemyController_Init(&level->controller, stage->eventCount)) {
        fprintf(stderr, "Failed to initialize enemy controller!\n");
        level->levelRunning = false;
        return;
    }

    stage->current = 0;

    level->currentStage = id;
    level->levelRunning = true;
    level->ticks = 0;
}

bool EnemyController_Init(EnemyController *e, uint32_t capacity) {
    if (!e)
        return false;

    *e = (EnemyController){ 0 };
    e->capacity = capacity;

    if (capacity == 0)
        return true;

    e->positions = calloc(capacity, sizeof(*e->positions));
    e->healths = calloc(capacity, sizeof(*e->healths));
    e->enemies = calloc(capacity, sizeof(*e->enemies));

    e->commandIndices = calloc(capacity, sizeof(*e->commandIndices));
    e->commandTicks = calloc(capacity, sizeof(*e->commandTicks));
    e->moveStarts = calloc(capacity, sizeof(*e->moveStarts));
    e->patterns = calloc(capacity, sizeof(*e->patterns));

    if (!e->positions || !e->healths || !e->enemies || !e->commandIndices || !e->commandTicks ||
        !e->moveStarts || !e->patterns) {
        EnemyController_Destroy(e);
        return false;
    }

    return true;
}

void EnemyController_Destroy(EnemyController *e) {
    if (!e)
        return;

    if (e->patterns) {
        for (uint32_t i = 0; i < e->capacity; i++)
            Pattern_Destroy(&e->patterns[i]);
    }

    free(e->positions);
    free(e->healths);
    free(e->enemies);

    free(e->commandIndices);
    free(e->commandTicks);
    free(e->moveStarts);
    free(e->patterns);

    *e = (EnemyController){ 0 };
}

bool Level_Spawn(Level *level, TimelineEvent *t) {
    if (!level || !t || (unsigned) t->enemy >= ENEMY_ID_LAST)
        return false;

    EnemyController *e = &level->controller;

    if (e->count >= e->capacity)
        return false;

    uint32_t i = e->count++;

    e->positions[i] = t->position;
    e->healths[i] = t->health;
    e->enemies[i] = &level->enemies[t->enemy];

    e->commandIndices[i] = 0;
    e->commandTicks[i] = 0;
    e->moveStarts[i] = t->position;
    e->patterns[i] = (Pattern){ 0 };

    return true;
}

bool Level_Remove(Level *level, uint32_t i) {
    if (!level)
        return false;

    EnemyController *e = &level->controller;

    if (i >= e->count)
        return false;

    uint32_t last = --e->count;

    Pattern_Destroy(&e->patterns[i]);

    if (i == last)
        return true;

    e->positions[i] = e->positions[last];
    e->healths[i] = e->healths[last];
    e->enemies[i] = e->enemies[last];
    e->commandIndices[i] = e->commandIndices[last];
    e->commandTicks[i] = e->commandTicks[last];
    e->moveStarts[i] = e->moveStarts[last];
    e->patterns[i] = e->patterns[last];
    e->patterns[last] = (Pattern){ 0 };

    return true;
}

void Level_Tick(Level *level, Application *app, float dt) {
    if (!level || !app || !level->levelRunning)
        return;

    Game *game = (Game *) app->userdata;

    Stage *stage = &level->stages[level->currentStage];
    EnemyController *e = &level->controller;

    while (stage->current < stage->eventCount &&
           stage->events[stage->current].time <= level->ticks) {
        if (!Level_Spawn(level, &stage->events[stage->current])) {
            fprintf(stderr, "Failed to spawn scheduled enemy!\n");
            level->levelRunning = false;
            return;
        }

        stage->current++;
    }

    uint32_t i = 0;
    while (i < e->count) {
        if (e->healths[i] == 0) {
            Level_Remove(level, i);
            continue;
        }

        Enemy_Tick(e, &game->bullets, i, dt);

        if (e->healths[i] == 0) {
            Level_Remove(level, i);
            continue;
        }

        i++;
    }

    level->ticks++;

    if (stage->current >= stage->eventCount && e->count == 0)
        level->levelRunning = false;
}

void Level_Draw(Level *level, Application *app, CommandBuffer *cmd) {
    Game *game = (Game *) app->userdata;
}

void Level_Destroy(Level *level) {
    if (!level)
        return;

    EnemyController_Destroy(&level->controller);

    for (uint32_t i = 0; i < ENEMY_ID_LAST; i++) {
        free(level->enemies[i].name);
        free(level->enemies[i].commands);
    }

    for (uint32_t i = 0; i < STAGE_ID_LAST; i++) {
        free(level->stages[i].name);
        free(level->stages[i].events);
    }

    *level = (Level){ 0 };
}
