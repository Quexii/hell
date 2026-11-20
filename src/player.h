#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

#include "collider.h"

typedef struct Player Player;
typedef struct World World;

struct Player {
    Vec2 position;
    uint32_t hearts, bombs;
    float speed;
    Collier collider, grazeCollider, collectCollider;
    float shootTimer, shootInterval;
    // TODO: Add characters with their own abilities...
    // possibly some game mechanics, roguelike/skilltree?
};

void Player_Damage(World *world);

void Player_OnDeath(World *world);

#endif // GAME_PLAYER_H
