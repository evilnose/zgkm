#pragma once

#include <vector>
#include <unordered_map>

#include "position.h"

std::vector<Move> gen_legal_moves(const Position& position);

bool move_allowed(const Position &position, const Move &move);

void test_absolute_pins(Position& position);

// generate move count for perft
int perft(Position& position, int depth);

void divide(Position& position, int depth);
