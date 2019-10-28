#pragma once

#include <string>

#include "types.h"

namespace bb {
// called once at the start of the program; populate move tables
void initialize();

std::string repr(Bitboard bitboard);

Square bitscan_fwd(Bitboard bitboard);

Bitboard bishop_attacks(Square sq, Bitboard occ);

Bitboard rook_attacks(Square sq, Bitboard occ);

Bitboard queen_attacks(Square sq, Bitboard occ);

Bitboard pawn_attacks(Square sq, Color attacker_color);

Bitboard knight_attacks(Square sq);

Bitboard king_attacks(Square sq);

// Find the first occupied square from s1 to s2.
Bitboard blocker(Square s1, Square s2, Bitboard occ);

/*
sq        square of the bishop
occ         board occupancy
blockers    occupancy of the bishop's side's pieces
Returns     squares shielded by blockers from the bishop
*/
Bitboard bishop_xray_attacks(Square sq, Bitboard occ, Bitboard blockers);

/* same as above except for a rook */
Bitboard rook_xray_attacks(Square sq, Bitboard occ, Bitboard blockers);

struct MagicInfo {
    Bitboard magic;           // magic that multiplies the key to get the index
    Bitboard occupancy_mask;  // relevant occupancy masks
    Bitboard shift;           // number of shifts applied to index
    Bitboard *table;          // pointer to the move/atk table for this square
    unsigned int get_index(Bitboard occupancy);
};
}  // namespace bb

void test_magics();
