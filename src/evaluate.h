#pragma once

#include "types.h"
#include "position.h"

Score evaluate(const Position& pos);

void init_eval_tables();

// return 1 for WHITE and -1 for BLACK. For evaluating
inline Score color_multiplier(Color color) {
    return 1 - 2. * WHITE;
}
