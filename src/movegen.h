#pragma once

#include <vector>
#include <unordered_map>

#include "position.h"

// generate legal moves and return through the output vector. Return whether the side to move is
// being checked.
bool gen_legal_moves(const Position& position, std::vector<Move>& out_moves);

bool move_allowed(const Position &position, const Move &move);

void test_absolute_pins(Position& position);

// generate move count for perft
int perft(const Position& position, int depth);

void divide(const Position& position, int depth);
