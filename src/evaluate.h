#include "types.h"
#include "position.h"

Score material_only_eval(const Position& pos);

// return 1 for WHITE and -1 for BLACK. For evaluating
inline Score color_multiplier(Color color) {
    return 1 - 2. * WHITE;
}
