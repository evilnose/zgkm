#pragma once

#include <string>

#include "types.h"
#include "utils.h"

namespace bboard {

struct MagicInfo {
    Bitboard magic;           // magic that multiplies the key to get the index
    Bitboard occupancy_mask;  // relevant occupancy masks
    Bitboard shift;           // number of shifts applied to index
    Bitboard *table;          // pointer to the move/atk table for this square
    unsigned int get_index(Bitboard occupancy);
};

constexpr int B_TABLE_SZ = 5248;
constexpr int R_TABLE_SZ = 102400;

// for bitscan
const extern uint8_t debruijn_table[64];

const extern Bitboard DEBRUIJN;
// magic bitboard database for bishops
extern MagicInfo b_magics[64];
extern MagicInfo r_magics[64];
extern Bitboard b_attack_table[B_TABLE_SZ];  // the big attack table
extern Bitboard r_attack_table[R_TABLE_SZ];
extern Bitboard p_attack_table[N_COLORS][64];
extern Bitboard n_attack_table[64];
extern Bitboard k_attack_table[64];

// occupancy that must not be attacked for castling to be possible.
// NOTE does not include king's own square.
// Indexing: white kingside, white queenside, black kingside, black queenside.
static Bitboard castle_king_occ_table[]{0x60ULL, 0xcULL, 0x6000000000000000ULL,
                                   0xc00000000000000ULL};

// occupancy that must not be occupied for castling to be possible (i.e.
// squares between king and rook
static Bitboard castle_between_occ_table[]{0x60ULL, 0xeULL, 0x6000000000000000ULL,
                                   0xe00000000000000ULL};

// called once at the start of the program; populate move tables
void initialize();

std::string repr(Bitboard bitboard);

inline Bitboard mask_square(const Square& square) {
    return 1ULL << square;
}

// return the index of the least significant set bit
inline Square bitscan_fwd(Bitboard board) {
    return utils::to_square(
        debruijn_table[((board & -board) * DEBRUIJN) >> 58]);
}

// same as bitscan_fwd but unsets the found bit
inline Square bitscan_fwd_remove(Bitboard& board) {
    Square sq = bitscan_fwd(board);
    board &= ~bboard::mask_square(sq);
    return sq;
}

// return whether there is exactly one bit set
inline bool one_bit(Bitboard board) {
    return board && !(board & (board - 1));  // && board for edge case 0
}

inline Bitboard bishop_attacks(Square sq, Bitboard occ) {
    return b_magics[sq].table[b_magics[sq].get_index(occ)];
}

inline Bitboard rook_attacks(Square sq, Bitboard occ) {
    return r_magics[sq].table[r_magics[sq].get_index(occ)];
}

inline Bitboard queen_attacks(Square sq, Bitboard occ) {
    return bishop_attacks(sq, occ) | rook_attacks(sq, occ);
}

inline Bitboard pawn_attacks(Square sq, Color atk_color) {
    return p_attack_table[atk_color][sq];
}

inline Bitboard pawn_pushes(Square sq, Color atk_color) {
    int d_rank = utils::pawn_direction(atk_color);
    // return 0 if promotion
    return bboard::mask_square(utils::to_square(sq + d_rank * 8));
}

inline Bitboard knight_attacks(Square sq) { return n_attack_table[sq]; }

inline Bitboard king_attacks(Square sq) { return k_attack_table[sq]; }

inline Bitboard castle_king_occ(Color c, BoardSide side) {
    return castle_king_occ_table[static_cast<int>(c) * 2 + static_cast<int>(side)];
}

inline Bitboard castle_between_occ(Color c, BoardSide side) {
    return castle_between_occ_table[static_cast<int>(c) * 2 + static_cast<int>(side)];
}

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
}  // namespace bboard

void test_magics();
