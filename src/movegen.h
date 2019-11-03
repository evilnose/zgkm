#pragma once

#include <vector>

#include "position.h"

std::vector<Move> gen_legal_moves(const Position& position);
bool move_allowed(const Position &position, const Move &move);
void test_absolute_pins(Position& position);
