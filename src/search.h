#include "types.h"
#include "evaluate.h"
#include "position.h"

// TODO replace best_move by PV
struct SearchResult {
    Move best_move;
    Score eval;
};

// search on pos until depth. e.g. depth_search(pos, 1) evaluates all positions after one move
// should not be called after game is over
SearchResult depth_search(const Position& pos, int depth);
