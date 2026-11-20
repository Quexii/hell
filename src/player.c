#include "player.h"

#include "world.h"

void Player_Damage(World *world) {
    if (world->player.hearts-- <= 0)
        Player_OnDeath(world);
}

void Player_OnDeath(World *world) {
    (void) world;
}
