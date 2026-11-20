#include "level.h"

#include "bullets.h"
#include "engine/input.h"
#include "engine/maths.h"
#include "engine/util.h"
#include "game.h"
#include "graphics.h"
#include "patterns.h"
#include "playfield.h"

#include <nori.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// i mean, thats one way to do it
static const bool CollisionMatrix[COLLIDER_LAYER_LAST][COLLIDER_LAYER_LAST] = {
    { false, false, false, false, true,  true,  true  },
    { false, false, false, false, true,  false, false },
    { false, false, false, false, false, true,  false },
    { false, false, false, false, false, false, true  },
    { true,  true,  false, false, false, false, false },
    { true,  false, true,  false, false, false, false },
    { true,  false, false, true,  false, false, false },
};

static uint32_t CollisionGrid_CellCount(const CollisionGrid *grid) {
    return grid->columns * grid->rows;
}

static bool CollisionGrid_CellBounds(
    const CollisionGrid *grid,
    const Collier *collider,
    Vec2 position,
    uint32_t *minX,
    uint32_t *minY,
    uint32_t *maxX,
    uint32_t *maxY
) {
    if (!grid || !collider || !minX || !minY || !maxX || !maxY || grid->cellSize <= 0.f ||
        grid->columns == 0 || grid->rows == 0 || collider->type == COLLIDER_NONE)
        return false;

    float radius = 0.f;
    Vec2 extent = { 0.f, 0.f };

    switch (collider->type) {
        case COLLIDER_CIRCLE:
            radius = collider->circle.radius;
            extent = (Vec2){ radius, radius };
            break;
        case COLLIDER_OBB:
            {
                float c = fabsf(cosf(collider->obb.rotation));
                float s = fabsf(sinf(collider->obb.rotation));
                extent.x = c * collider->obb.halfSize.x + s * collider->obb.halfSize.y;
                extent.y = s * collider->obb.halfSize.x + c * collider->obb.halfSize.y;
            }
            break;
        default:
            return false;
    }

    float left = position.x - extent.x;
    float top = position.y - extent.y;
    float right = position.x + extent.x;
    float bottom = position.y + extent.y;
    float width = grid->columns * grid->cellSize;
    float height = grid->rows * grid->cellSize;

    if (right < 0.f || bottom < 0.f || left >= width || top >= height)
        return false;

    int32_t x0 = (int32_t) floorf(left / grid->cellSize);
    int32_t y0 = (int32_t) floorf(top / grid->cellSize);
    int32_t x1 = (int32_t) floorf(right / grid->cellSize);
    int32_t y1 = (int32_t) floorf(bottom / grid->cellSize);

    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 >= (int32_t) grid->columns)
        x1 = (int32_t) grid->columns - 1;
    if (y1 >= (int32_t) grid->rows)
        y1 = (int32_t) grid->rows - 1;

    if (x0 > x1 || y0 > y1)
        return false;

    *minX = (uint32_t) x0;
    *minY = (uint32_t) y0;
    *maxX = (uint32_t) x1;
    *maxY = (uint32_t) y1;
    return true;
}

static bool CollisionGrid_ReserveIndices(CollisionGrid *grid, uint32_t count) {
    if (count <= grid->indexCapacity)
        return true;

    uint32_t capacity = grid->indexCapacity ? grid->indexCapacity : 1;
    while (capacity < count) {
        if (capacity > UINT32_MAX / 2)
            return false;
        capacity *= 2;
    }

    uint32_t *indices = realloc(grid->indices, sizeof(*indices) * capacity);
    if (!indices)
        return false;

    grid->indices = indices;
    grid->indexCapacity = capacity;
    return true;
}

static bool CollisionGrid_ReserveEntries(CollisionGrid *grid, uint32_t count) {
    if (grid->entries && count <= grid->capacity)
        return true;

    uint32_t capacity = grid->capacity ? grid->capacity : 1;
    while (capacity < count) {
        if (capacity > UINT32_MAX / 2)
            return false;
        capacity *= 2;
    }

    CollisionEntry *entries = realloc(grid->entries, sizeof(*entries) * capacity);
    if (!entries)
        return false;

    grid->entries = entries;
    grid->capacity = capacity;
    return true;
}

static uint64_t CollisionGrid_HashPair(uint64_t key) {
    key ^= key >> 30;
    key *= UINT64_C(0xbf58476d1ce4e5b9);
    key ^= key >> 27;
    key *= UINT64_C(0x94d049bb133111eb);
    return key ^ (key >> 31);
}

static bool CollisionGrid_PairWasAdded(uint64_t *table, size_t capacity, uint64_t key) {
    size_t slot = (size_t) CollisionGrid_HashPair(key) & (capacity - 1);
    while (table[slot] != UINT64_MAX) {
        if (table[slot] == key)
            return true;
        slot = (slot + 1) & (capacity - 1);
    }

    table[slot] = key;
    return false;
}

static void Level_OnPlayerDeath(Level *level) {
    (void) level;
}

static void Level_DamagePlayer(Level *level) {
    if (level->player.hearts == 0)
        return;

    level->player.hearts--;
    if (level->player.hearts == 0)
        Level_OnPlayerDeath(level);
}

static void
Level_OnEntryCollision(
    const CollisionEntry *a,
    const CollisionEntry *b,
    void *ownerA,
    void *ownerB,
    Level *level
) {
    const CollisionEntry *player = NULL;
    const CollisionEntry *other = NULL;
    void *otherOwner = NULL;

    if (a->collider->layer == COLLIDER_LAYER_PLAYER) {
        player = a;
        other = b;
        otherOwner = ownerB;
    } else if (b->collider->layer == COLLIDER_LAYER_PLAYER) {
        player = b;
        other = a;
        otherOwner = ownerA;
    }

    if (player && other) {
        if (other->collider->layer == COLLIDER_LAYER_ENEMY) {
            Level_DamagePlayer(level);
            return;
        }

        if (other->collider->layer == COLLIDER_LAYER_ENEMY_BULLET) {
            BulletSim *bullets = otherOwner;
            Level_DamagePlayer(level);
            if (bullets && other->index < bullets->count)
                bullets->life[other->index] = -1.f;
            return;
        }
    }

    const CollisionEntry *bullet = NULL;
    const CollisionEntry *enemy = NULL;
    BulletSim *bullets = NULL;

    if (a->collider->layer == COLLIDER_LAYER_PLAYER_BULLET &&
        b->collider->layer == COLLIDER_LAYER_ENEMY) {
        bullet = a;
        enemy = b;
        bullets = ownerA;
    } else if (a->collider->layer == COLLIDER_LAYER_ENEMY &&
               b->collider->layer == COLLIDER_LAYER_PLAYER_BULLET) {
        bullet = b;
        enemy = a;
        bullets = ownerB;
    }

    if (!bullet || !enemy || enemy->owner != &level->controller || !bullets)
        return;

    if (enemy->index < level->controller.count && level->controller.healths[enemy->index] > 0)
        level->controller.healths[enemy->index]--;

    if (bullet->index < bullets->count)
        bullets->life[bullet->index] = -1.f;
}

static void Level_RemoveDeadEnemies(Level *level) {
    EnemyController *e = &level->controller;

    uint32_t i = 0;
    while (i < e->count) {
        if (e->healths[i] == 0) {
            Level_Remove(level, i);
            continue;
        }
        i++;
    }
}

static void Level_CheckCollisions(Level *level, BulletSim *bullets) {
    EnemyController *e = &level->controller;
    static const Collier enemyCollider = {
        .type = COLLIDER_CIRCLE,
        .layer = COLLIDER_LAYER_ENEMY,
        .circle.radius = 8.f,
    };
    CollisionGrid_Begin(&level->collisionGrid);
    CollisionGrid_Add(
        &level->collisionGrid,
        &level->player.collider,
        &level->player.position,
        &level->player,
        0
    );

    for (uint32_t i = 0; i < e->count; i++) {
        CollisionGrid_Add(&level->collisionGrid, &enemyCollider, &e->positions[i], e, i);
    }

    for (uint32_t i = 0; i < bullets->count; i++) {
        CollisionGrid_Add(
            &level->collisionGrid,
            &bullets->colliders[i],
            &bullets->positions[i],
            bullets,
            i
        );
    }

    if (CollisionGrid_End(&level->collisionGrid))
        CollisionGrid_CheckEntries(&level->collisionGrid, Level_OnEntryCollision, level);

    Level_RemoveDeadEnemies(level);
}

bool CollisionGrid_Init(
    CollisionGrid *grid, uint32_t columns, uint32_t rows, float cellSize, uint32_t capacity
) {
    if (!grid || columns == 0 || rows == 0 || cellSize <= 0.f)
        return false;

    *grid = (CollisionGrid){ 0 };
    grid->columns = columns;
    grid->rows = rows;
    grid->cellSize = cellSize;
    grid->capacity = capacity;

    uint32_t cellCount = CollisionGrid_CellCount(grid);
    grid->cells = calloc(cellCount, sizeof(*grid->cells));
    grid->counts = calloc(cellCount, sizeof(*grid->counts));
    grid->cursor = calloc(cellCount, sizeof(*grid->cursor));
    grid->entries = capacity ? calloc(capacity, sizeof(*grid->entries)) : NULL;

    if (!grid->cells || !grid->counts || !grid->cursor || (capacity && !grid->entries) ||
        !CollisionGrid_ReserveIndices(grid, capacity)) {
        CollisionGrid_Destroy(grid);
        return false;
    }

    return true;
}

void CollisionGrid_Clear(CollisionGrid *grid) {
    if (!grid)
        return;

    uint32_t cellCount = CollisionGrid_CellCount(grid);
    memset(grid->cells, 0, sizeof(*grid->cells) * cellCount);
    memset(grid->counts, 0, sizeof(*grid->counts) * cellCount);
    memset(grid->cursor, 0, sizeof(*grid->cursor) * cellCount);
}

void CollisionGrid_Destroy(CollisionGrid *grid) {
    if (!grid)
        return;

    free(grid->cells);
    free(grid->indices);
    free(grid->counts);
    free(grid->cursor);
    free(grid->entries);
    *grid = (CollisionGrid){ 0 };
}

void CollisionGrid_Begin(CollisionGrid *grid) {
    if (!grid)
        return;

    CollisionGrid_Clear(grid);
    grid->entryCount = 0;
}

bool CollisionGrid_Add(
    CollisionGrid *grid,
    const Collier *collider,
    const Vec2 *position,
    void *owner,
    uint32_t index
) {
    if (!grid || !collider || !position || collider->type == COLLIDER_NONE)
        return false;

    if (!CollisionGrid_ReserveEntries(grid, grid->entryCount + 1))
        return false;

    grid->entries[grid->entryCount++] = (CollisionEntry){ collider, position, owner, index };
    return true;
}

bool CollisionGrid_End(CollisionGrid *grid) {
    if (!grid)
        return false;

    uint32_t cellCount = CollisionGrid_CellCount(grid);

    for (uint32_t i = 0; i < grid->entryCount; i++) {
        const CollisionEntry *entry = &grid->entries[i];
        uint32_t minX, minY, maxX, maxY;
        if (!CollisionGrid_CellBounds(
                grid, entry->collider, *entry->position, &minX, &minY, &maxX, &maxY
            ))
            continue;

        for (uint32_t y = minY; y <= maxY; y++) {
            for (uint32_t x = minX; x <= maxX; x++)
                grid->counts[y * grid->columns + x]++;
        }
    }

    uint32_t total = 0;
    for (uint32_t i = 0; i < cellCount; i++) {
        grid->cells[i].offset = total;
        grid->cells[i].count = grid->counts[i];
        grid->cursor[i] = total;
        total += grid->counts[i];
    }

    if (!CollisionGrid_ReserveIndices(grid, total))
        return false;

    for (uint32_t i = 0; i < grid->entryCount; i++) {
        const CollisionEntry *entry = &grid->entries[i];
        uint32_t minX, minY, maxX, maxY;
        if (!CollisionGrid_CellBounds(
                grid, entry->collider, *entry->position, &minX, &minY, &maxX, &maxY
            ))
            continue;

        for (uint32_t y = minY; y <= maxY; y++) {
            for (uint32_t x = minX; x <= maxX; x++) {
                uint32_t cell = y * grid->columns + x;
                grid->indices[grid->cursor[cell]++] = i;
            }
        }
    }

    return true;
}

void CollisionGrid_CheckEntries(
    const CollisionGrid *grid,
    CollisionEntryHandler handler,
    Level *level
) {
    if (!grid || !handler)
        return;

    uint64_t pairCount = 0;
    for (uint32_t cell = 0; cell < CollisionGrid_CellCount(grid); cell++) {
        uint32_t n = grid->cells[cell].count;
        if (n > 1)
            pairCount += (uint64_t) n * (n - 1) / 2;
    }

    if (pairCount > (uint64_t) SIZE_MAX / sizeof(uint64_t) || pairCount > SIZE_MAX / 2)
        return;

    size_t pairCapacity = 1;
    while (pairCapacity < (size_t) pairCount * 2)
        pairCapacity *= 2;

    uint64_t *pairs = pairCount ? malloc(sizeof(*pairs) * pairCapacity) : NULL;
    if (pairCount && !pairs)
        return;
    if (pairs)
        for (size_t i = 0; i < pairCapacity; i++)
            pairs[i] = UINT64_MAX;

    for (uint32_t cell = 0; cell < CollisionGrid_CellCount(grid); cell++) {
        const CollisionCell *at = &grid->cells[cell];
        for (uint32_t a = 0; a < at->count; a++) {
            uint32_t first = grid->indices[at->offset + a];
            for (uint32_t b = a + 1; b < at->count; b++) {
                uint32_t second = grid->indices[at->offset + b];
                uint32_t low = first < second ? first : second;
                uint32_t high = first < second ? second : first;
                uint64_t key = ((uint64_t) low << 32) | high;
                if (CollisionGrid_PairWasAdded(pairs, pairCapacity, key))
                    continue;

                const CollisionEntry *entryA = &grid->entries[low];
                const CollisionEntry *entryB = &grid->entries[high];
                if (!ColliderLayer_Intersects(entryA->collider->layer, entryB->collider->layer))
                    continue;
                if (Collider_Intersects(
                        (Collier *) entryA->collider,
                        (Collier *) entryB->collider,
                        (Vec2 *) entryA->position,
                        (Vec2 *) entryB->position
                    ))
                    handler(entryA, entryB, entryA->owner, entryB->owner, level);
            }
        }
    }

    free(pairs);
}

bool CollisionGrid_Build(
    CollisionGrid *grid, const Collier *colliders, const Vec2 *positions, uint32_t count
) {
    if (!grid || !colliders || !positions || count > grid->capacity)
        return false;

    CollisionGrid_Clear(grid);
    uint32_t cellCount = CollisionGrid_CellCount(grid);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t minX, minY, maxX, maxY;
        if (!CollisionGrid_CellBounds(
                grid, &colliders[i], positions[i], &minX, &minY, &maxX, &maxY
            ))
            continue;

        for (uint32_t y = minY; y <= maxY; y++) {
            for (uint32_t x = minX; x <= maxX; x++)
                grid->counts[y * grid->columns + x]++;
        }
    }

    uint32_t total = 0;
    for (uint32_t i = 0; i < cellCount; i++) {
        grid->cells[i].offset = total;
        grid->cells[i].count = grid->counts[i];
        grid->cursor[i] = total;
        total += grid->counts[i];
    }

    if (!CollisionGrid_ReserveIndices(grid, total))
        return false;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t minX, minY, maxX, maxY;
        if (!CollisionGrid_CellBounds(
                grid, &colliders[i], positions[i], &minX, &minY, &maxX, &maxY
            ))
            continue;

        for (uint32_t y = minY; y <= maxY; y++) {
            for (uint32_t x = minX; x <= maxX; x++) {
                uint32_t cell = y * grid->columns + x;
                grid->indices[grid->cursor[cell]++] = i;
            }
        }
    }

    return true;
}

void CollisionGrid_Check(
    const CollisionGrid *grid,
    const Collier *colliders,
    const Vec2 *positions,
    uint32_t count,
    CollisionHandler handler,
    void *userdata
) {
    if (!grid || !colliders || !positions || !handler || count == 0)
        return;

    uint64_t pairCount = 0;
    for (uint32_t cell = 0; cell < CollisionGrid_CellCount(grid); cell++) {
        uint32_t n = grid->cells[cell].count;
        if (n > 1)
            pairCount += (uint64_t) n * (n - 1) / 2;
    }

    if (pairCount > SIZE_MAX / sizeof(uint64_t))
        return;

    uint64_t *pairs = pairCount ? malloc(sizeof(*pairs) * (size_t) pairCount) : NULL;
    if (pairCount && !pairs)
        return;
    uint64_t pairUsed = 0;

    for (uint32_t cell = 0; cell < CollisionGrid_CellCount(grid); cell++) {
        const CollisionCell *at = &grid->cells[cell];
        for (uint32_t a = 0; a < at->count; a++) {
            uint32_t first = grid->indices[at->offset + a];
            for (uint32_t b = a + 1; b < at->count; b++) {
                uint32_t second = grid->indices[at->offset + b];
                uint32_t low = first < second ? first : second;
                uint32_t high = first < second ? second : first;
                uint64_t key = ((uint64_t) low << 32) | high;
                bool seen = false;

                for (uint64_t p = 0; p < pairUsed; p++) {
                    if (pairs[p] == key) {
                        seen = true;
                        break;
                    }
                }
                if (seen)
                    continue;
                pairs[pairUsed++] = key;

                if (!ColliderLayer_Intersects(colliders[low].layer, colliders[high].layer))
                    continue;
                if (Collider_Intersects(
                        (Collier *) &colliders[low],
                        (Collier *) &colliders[high],
                        (Vec2 *) &positions[low],
                        (Vec2 *) &positions[high]
                    ))
                    handler(low, high, userdata);
            }
        }
    }

    free(pairs);
}

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
            case OP_DIE:
                {
                    e->healths[i] = 0;
                }
                return;

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
    CollisionGrid_Destroy(&level->collisionGrid);

    if (!CollisionGrid_Init(
            &level->collisionGrid,
            COLLIDER_GRID_COLUMNS,
            COLLIDER_GRID_ROWS,
            COLLIDER_GRID_SIZE,
            stage->eventCount + 1
        )) {
        fprintf(stderr, "Failed to initialize collision grid!\n");
        level->levelRunning = false;
        return;
    }

    if (!EnemyController_Init(&level->controller, stage->eventCount + 1000)) {
        fprintf(stderr, "Failed to initialize enemy controller!\n");
        level->levelRunning = false;
        return;
    }

    stage->current = 0;

    level->currentStage = id;
    level->levelRunning = true;
    level->ticks = 0;

    // temporarly set static player properties
    level->player.position = (Vec2){ PLAYFIELD_WIDTH / 2.f, PLAYFIELD_HEIGHT - PLAYER_HEIGHT };
    level->player.speed = 180.f;
    level->player.hearts = 10.f;
    level->player.bombs = 2.f;
    level->player.collider =
        (Collier){ COLLIDER_CIRCLE, COLLIDER_LAYER_PLAYER, .circle.radius = 6.f };
    level->player.grazeCollider =
        (Collier){ COLLIDER_CIRCLE, COLLIDER_LAYER_PLAYER_GRAZE, .circle.radius = 10.f };
    level->player.collectCollider =
        (Collier){ COLLIDER_CIRCLE, COLLIDER_LAYER_PLAYER_PICKUP, .circle.radius = 18.f };
    level->player.shootInterval = 0.04f;
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

    float speed = 240.f;

    if (Input_KeyDown(SDL_SCANCODE_LSHIFT))
        speed *= 0.4f;

    Vec2 movement = { (float) Input_KeyDown(SDL_SCANCODE_D) - (float) Input_KeyDown(SDL_SCANCODE_A),

                      (float) Input_KeyDown(SDL_SCANCODE_S) -
                          (float) Input_KeyDown(SDL_SCANCODE_W) };

    float len = sqrtf(movement.x * movement.x + movement.y * movement.y);

    if (len > 0.f) {
        movement.x /= len;
        movement.y /= len;
    }

    level->player.position.x += movement.x * speed * dt;
    level->player.position.y += movement.y * speed * dt;
    level->player.position.x =
        Clampf(level->player.position.x, PLAYER_WIDTH / 2.f, PLAYFIELD_WIDTH - PLAYER_WIDTH / 2.f);
    level->player.position.y = Clampf(
        level->player.position.y, PLAYER_HEIGHT / 2.f, PLAYFIELD_HEIGHT - PLAYER_HEIGHT / 2.f
    );

    level->player.shootTimer += dt;
    if (Input_KeyDown(SDL_SCANCODE_K)) {
        if (level->player.shootTimer >= level->player.shootInterval) {
            level->player.shootTimer = 0.f;

            BulletSim_Spawn(
                &game->bullets,
                &(BulletDesc){
                    .position = level->player.position,
                    .velocity = (Vec2){ 0.f,  -280.f  },
                    .spriteID = BULLETTYPE_DARK_ICE_HEX,
                    .size = (Vec2){ 14.f, 14.f },
                    .rotationMode = BULLET_ROTATION_VELOCITY,
                    .rotationOffset = ToRadians(-90.f),
                    .tint = RGBA8(255, 255, 255, 255),
                    .collider = {
                        .type = COLLIDER_CIRCLE,
                        .layer = COLLIDER_LAYER_PLAYER_BULLET,
                        .circle.radius = 7.f,
                    },
            }
            );
        }
    }

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

    Level_CheckCollisions(level, &game->bullets);

    level->ticks++;

}

void Level_Draw(Level *level, Application *app, CommandBuffer *cmd) {
    (void) level;
    (void) app;
    (void) cmd;

    { // Debug Draw
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

    EnemyController_Destroy(&level->controller);
    CollisionGrid_Destroy(&level->collisionGrid);

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

bool Collider_Intersects_Circle_Circle(Collier *a, Collier *b, Vec2 *pos1, Vec2 *pos2) {
    if (!a || !b || !pos1 || !pos2 || a->type != COLLIDER_CIRCLE || b->type != COLLIDER_CIRCLE)
        return false;

    float dx = pos1->x - pos2->x;
    float dy = pos1->y - pos2->y;
    float radius = a->circle.radius + b->circle.radius;

    return dx * dx + dy * dy <= radius * radius;
}

bool Collider_Intersects_Circle_OBB(Collier *a, Collier *b, Vec2 *pos1, Vec2 *pos2) {
    if (!a || !b || !pos1 || !pos2 || a->type == b->type)
        return false;

    Collier *circle;
    Collier *obb;
    Vec2 circlePos, obbPos;

    if (a->type == COLLIDER_CIRCLE && b->type == COLLIDER_OBB) {
        circle = a;
        obb = b;
        circlePos = *pos1;
        obbPos = *pos2;
    } else if (b->type == COLLIDER_CIRCLE && a->type == COLLIDER_OBB) {
        circle = b;
        obb = a;
        circlePos = *pos2;
        obbPos = *pos1;
    } else {
        return false;
    }

    float c = cosf(obb->obb.rotation);
    float s = sinf(obb->obb.rotation);
    float dx = circlePos.x - obbPos.x;
    float dy = circlePos.y - obbPos.y;
    float lx = dx * c + dy * s;
    float ly = -dx * s + dy * c;
    float cx = fmaxf(-obb->obb.halfSize.x, fminf(lx, obb->obb.halfSize.x));
    float cy = fmaxf(-obb->obb.halfSize.y, fminf(ly, obb->obb.halfSize.y));
    float ex = lx - cx;
    float ey = ly - cy;

    return ex * ex + ey * ey <= circle->circle.radius * circle->circle.radius;
}

bool Collider_Intersects_OBB_OBB(Collier *a, Collier *b, Vec2 *pos1, Vec2 *pos2) {
    if (!a || !b || !pos1 || !pos2 || a->type != COLLIDER_OBB || b->type != COLLIDER_OBB)
        return false;

    Vec2 axes[] = { OBB_AxisX(a->obb.rotation),
                    OBB_AxisY(a->obb.rotation),
                    OBB_AxisX(b->obb.rotation),
                    OBB_AxisY(b->obb.rotation) };

    Vec2 delta = { pos2->x - pos1->x, pos2->y - pos1->y };

    for (uint32_t i = 0; i < 4; i++) {
        Vec2 axis = axes[i];

        float distance = fabsf(Vec2_Dot(delta, axis));
        float ra = OBB_ProjectRadius(axis, a->obb.halfSize, a->obb.rotation);
        float rb = OBB_ProjectRadius(axis, b->obb.halfSize, b->obb.rotation);

        if (distance > ra + rb)
            return false;
    }

    return true;
}

bool Collider_Intersects(Collier *a, Collier *b, Vec2 *pos1, Vec2 *pos2) {
    if (!a || !b || !pos1 || !pos2)
        return false;

    switch (a->type) {
        case COLLIDER_CIRCLE:
            switch (b->type) {
                case COLLIDER_CIRCLE:
                    return Collider_Intersects_Circle_Circle(a, b, pos1, pos2);

                case COLLIDER_OBB:
                    return Collider_Intersects_Circle_OBB(a, b, pos1, pos2);

                default:
                    return false;
            }

        case COLLIDER_OBB:
            switch (b->type) {
                case COLLIDER_CIRCLE:
                    return Collider_Intersects_Circle_OBB(a, b, pos1, pos2);

                case COLLIDER_OBB:
                    return Collider_Intersects_OBB_OBB(a, b, pos1, pos2);

                default:
                    return false;
            }

        default:
            return false;
    }
}

bool ColliderLayer_Intersects(ColliderLayer a, ColliderLayer b) {
    if (a >= COLLIDER_LAYER_LAST || b >= COLLIDER_LAYER_LAST)
        return false;

    return CollisionMatrix[a][b];
}
