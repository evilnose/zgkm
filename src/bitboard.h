#pragma once

#include <string>

#include "types.h"

namespace bboard {
// called once at the start of the program; populate move tables
void init_magic();
std::string repr(Bitboard bitboard);

struct MagicInfo {
    Bitboard magic;           // magic that multiplies the key to get the index
    Bitboard occupancy_mask;  // relevant occupancy masks
    Bitboard shift;           // number of shifts applied to index
    Bitboard *table;          // pointer to the move/atk table for this square
    unsigned int get_index(Bitboard occupancy);
};
}  // namespace bboard

void test_deleteme();
