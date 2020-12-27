#pragma once

#include "types.h"
#include "evaluate.h"
#include "position.h"

#include <functional>

// TODO replace best_move by PV
struct SearchResult
{
    Move best_move;
    Score eval;
};

enum SearchLimitType
{
    UNLIMITED,  // no limit
    TIME_CONTROL,  // limited by remaining black & white time and inc
    FIXED_TIME,  // fixed amount of time
    NODES,  // number of nodes searched
};

struct TimeControlParams {
    int wtime;
    int btime;
    int winc;
    int binc;
};

// Conditions for stopping a search
union SearchLimit
{
    TimeControlParams time_control;
    int fixed_time;
    int nodes;
};

using ResultCallback = std::function<void(Move best, Score eval)>;

// search on pos until depth. e.g. depth_search(pos, 1) evaluates all positions after one move
// should not be called after game is over
SearchResult depth_search(const Position &pos, int depth);

// basic parallel search using one thread
void timed_search(const Position &pos, int btime, int wtime, int binc, int cinc,
                  ResultCallback callback);
