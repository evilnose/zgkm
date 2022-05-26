#pragma once

#include "types.h"
#include "evaluate.h"
#include "position.h"

#include <functional>

// TODO add PV
struct SearchResult {
    Score eval;
    Move best_move;
};

struct TimeControlParams {
    int wtime;
    int btime;
    int winc;
    int binc;
};

struct SearchLimit {
    TimeControlParams tc;
    int fixed_time;
    int nodes;
    int depth;
};

struct SearchMetrics {
    int depth;
    int nodes;
    int time;  // time elapsed for the search in millis
};

// using ResultCallback = std::function<void(Move best, Score eval)>;

// search on pos until depth. e.g. depth_search(pos, 1) evaluates all positions after one move
// should not be called after game is over
// SearchResult depth_search(const Position &pos, int depth);

// basic parallel search using one thread
// void timed_search(const Position &pos, int btime, int wtime, int binc, int cinc,
//                   ResultCallback callback);
