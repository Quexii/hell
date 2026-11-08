#include "pattern.h"

void Pattern_Update(Pattern *p, BulletSim *bullets, float dt) {
    if (!p || !p->emit || !bullets || p->interval <= 0.f)
        return;

    p->time += dt;
    while (p->time >= p->nextEmit) {
        p->emit(p, bullets);
        p->emission++;
        p->nextEmit += p->interval;
    }
}
