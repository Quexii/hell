#include "world.h"

#include "bullets.h"
#include "collider.h"
#include "engine/application.h"
#include "engine/input.h"
#include "game.h"
#include "level.h"
#include "pattern.h"
#include "player.h"
#include "playfield.h"
#include "stage.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void World_OnEntryCollision(
    const CollisionEntry *a, const CollisionEntry *b, void *ownerA, void *ownerB, World *world
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
            Player_Damage(world);
            return;
        }

        if (other->collider->layer == COLLIDER_LAYER_ENEMY_BULLET) {
            BulletSim *bullets = otherOwner;
            Player_Damage(world);
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
    } else if (
        a->collider->layer == COLLIDER_LAYER_ENEMY &&
        b->collider->layer == COLLIDER_LAYER_PLAYER_BULLET
    ) {
        bullet = b;
        enemy = a;
        bullets = ownerB;
    }

    if (!bullet || !enemy || enemy->owner != &world->controller || !bullets)
        return;

    if (enemy->index < world->controller.count && world->controller.healths[enemy->index] > 0)
        world->controller.healths[enemy->index]--;

    if (bullet->index < bullets->count)
        bullets->life[bullet->index] = -1.f;
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

bool EnemyController_Add(EnemyController *e, Vec2 position, uint32_t health, EnemyId enemyId) {
    if (!e || enemyId >= ENEMY_ID_LAST)
        return false;

    if (e->count >= e->capacity)
        return false;

    uint32_t i = e->count++;

    e->positions[i] = position;
    e->healths[i] = health;
    e->enemies[i] = &enemyDefinitions[enemyId];

    e->commandIndices[i] = 0;
    e->commandTicks[i] = 0;
    e->moveStarts[i] = position;
    e->patterns[i] = (Pattern){ 0 };
    return true;
}

bool EnemyController_Remove(EnemyController *e, uint32_t i) {
    if (!e || i >= e->count)
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

bool World_Init(World *world) {
    EnemyController_Destroy(&world->controller);
    CollisionGrid_Destroy(&world->collisionGrid);

    if (!CollisionGrid_Init(
            &world->collisionGrid,
            COLLIDER_GRID_COLUMNS,
            COLLIDER_GRID_ROWS,
            COLLIDER_GRID_SIZE,
            10000
        )) {
        fprintf(stderr, "Failed to initialize collision grid!\n");
        return false;
    }

    if (!EnemyController_Init(&world->controller, 11000)) {
        fprintf(stderr, "Failed to initialize enemy controller!\n");
        return false;
    }

    if (!BulletSim_Create(&world->bullets, MAX_BULLETS)) {
        fprintf(stderr, "Failed to create the bullet system\n");
        return false;
    }

    world->ticks = 0;

    // temporarly set static player properties
    world->player.position = (Vec2){ PLAYFIELD_WIDTH / 2.f, PLAYFIELD_HEIGHT - PLAYER_HEIGHT };
    world->player.speed = 180.f;
    world->player.hearts = 10.f;
    world->player.bombs = 2.f;
    world->player.collider =
        (Collier){ COLLIDER_CIRCLE, COLLIDER_LAYER_PLAYER, .circle.radius = 6.f };
    world->player.grazeCollider =
        (Collier){ COLLIDER_CIRCLE, COLLIDER_LAYER_PLAYER_GRAZE, .circle.radius = 10.f };
    world->player.collectCollider =
        (Collier){ COLLIDER_CIRCLE, COLLIDER_LAYER_PLAYER_PICKUP, .circle.radius = 18.f };
    world->player.shootInterval = 0.04f;
    return true;
}

void World_Destroy(World *world) {
    EnemyController_Destroy(&world->controller);
    CollisionGrid_Destroy(&world->collisionGrid);
    BulletSim_Destory(&world->bullets);
}

bool World_DoEvent(World *world, TimelineEvent *e) {
    // eventually events will do more than just spawn entities
    return EnemyController_Add(&world->controller, e->position, e->health, e->enemy);
}

void World_Tick(World *world, float dt) {
    if (!world)
        return;

    BulletSim_Update(&world->bullets, dt, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT);

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

    world->player.position.x += movement.x * speed * dt;
    world->player.position.y += movement.y * speed * dt;
    world->player.position.x =
        Clampf(world->player.position.x, PLAYER_WIDTH / 2.f, PLAYFIELD_WIDTH - PLAYER_WIDTH / 2.f);
    world->player.position.y = Clampf(
        world->player.position.y, PLAYER_HEIGHT / 2.f, PLAYFIELD_HEIGHT - PLAYER_HEIGHT / 2.f
    );

    world->player.shootTimer += dt;
    if (Input_KeyDown(SDL_SCANCODE_K)) {
        if (world->player.shootTimer >= world->player.shootInterval) {
            world->player.shootTimer = 0.f;

            BulletSim_Spawn(
                &world->bullets,
                &(BulletDesc){
                    .position = world->player.position,
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

    EnemyController *e = &world->controller;

    uint32_t i = 0;
    while (i < e->count) {
        if (e->healths[i] <= 0) {
            EnemyController_Remove(e, i);
            continue;
        }

        Enemy_Tick(e, &world->bullets, i, dt);

        if (e->healths[i] <= 0) {
            EnemyController_Remove(e, i);
            continue;
        }

        i++;
    }

    { // collisions
        static const Collier enemyCollider = {
            .type = COLLIDER_CIRCLE,
            .layer = COLLIDER_LAYER_ENEMY,
            .circle.radius = 8.f,
        };
        CollisionGrid_Begin(&world->collisionGrid);
        CollisionGrid_Add(
            &world->collisionGrid,
            &world->player.collider,
            &world->player.position,
            &world->player,
            0
        );

        for (uint32_t i = 0; i < e->count; i++) {
            CollisionGrid_Add(&world->collisionGrid, &enemyCollider, &e->positions[i], e, i);
        }

        for (uint32_t i = 0; i < world->bullets.count; i++) {
            CollisionGrid_Add(
                &world->collisionGrid,
                &world->bullets.colliders[i],
                &world->bullets.positions[i],
                &world->bullets,
                i
            );
        }

        if (CollisionGrid_End(&world->collisionGrid))
            CollisionGrid_CheckEntries(
                &world->collisionGrid,
                (CollisionEntryHandler) World_OnEntryCollision,
                (void *) world
            );
    }

    i = 0;
    while (i < e->count) {
        if (e->healths[i] == 0) {
            EnemyController_Remove(e, i);
            continue;
        }
        i++;
    }

    world->ticks++;
}

void World_Render(World *world, Application *app, CommandBuffer *cmd) {
    (void) cmd;

    Game *g = (Game*) app->userdata;

    if (world->bullets.count > 0) {
        BulletData *out = Batch_Reserve(&g->bulletBatch, world->bullets.count);

        if (out) {
            for (uint32_t i = 0; i < world->bullets.count; i++) {
                out[i] = (BulletData){
                    .position = world->bullets.positions[i],
                    .rotation = world->bullets.rotations[i],
                    .size = world->bullets.sizes[i],
                    .spriteID = world->bullets.spriteIDs[i],
                    .tint = world->bullets.tints[i], // MutCol_A(sim->tints[i], (uint8_t) Clamp(255 *
                                           // sim->life[i], 0, 255))
                };
            }
        } else {
            SDL_Log("Bullet batch full: count=%u capacity=%u", world->bullets.count, g->bulletBatch.capacity);
        }
    }
}
