#include "collider.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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
    CollisionGrid *grid, const Collier *collider, const Vec2 *position, void *owner, uint32_t index
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
    const CollisionGrid *grid, CollisionEntryHandler handler, void *userdata
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
                    handler(entryA, entryB, entryA->owner, entryB->owner, userdata);
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
