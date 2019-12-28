#pragma once

#include <vector>

#include "position.h"

std::vector<Move> gen_legal_moves(const Position& position);

bool move_allowed(const Position &position, const Move &move);

void test_absolute_pins(Position& position);

// generate move count for perft
std::vector<int> perft(const Position& position, int depth);
