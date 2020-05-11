#include <algorithm>
#include <cassert>
#include <iostream>

#include "bitboard.h"
#include "utils.h"
#include "logger.h"

using std::string;

const Bitboard REL_OCC_MASK = 0x7E7E7E7E7E7E00ULL;

// magic bitboard database for bishops
bboard::MagicInfo bboard::b_magics[64];
bboard::MagicInfo bboard::r_magics[64];
Bitboard bboard::b_attack_table[bboard::B_TABLE_SZ];  // the big attack table
Bitboard bboard::r_attack_table[bboard::R_TABLE_SZ];
Direction b_directions[]{NORTHWEST, NORTHEAST, SOUTHWEST, SOUTHEAST};
Direction r_directions[]{NORTH, SOUTH, WEST, EAST};

Bitboard bboard::p_attack_table[N_COLORS][64];
Bitboard bboard::n_attack_table[64];
Bitboard bboard::k_attack_table[64];

// for bitscan
const uint8_t bboard::debruijn_table[64] =
    {
        0, 1, 2, 53, 3, 7, 54, 27,
        4, 38, 41, 8, 34, 55, 48, 28,
        62, 5, 39, 46, 44, 42, 22, 9,
        24, 35, 59, 56, 49, 18, 29, 11,
        63, 52, 6, 26, 37, 40, 33, 47,
        61, 45, 43, 21, 23, 58, 17, 10,
        51, 25, 36, 32, 60, 20, 57, 16,
        50, 31, 19, 15, 30, 14, 13, 12};

const Bitboard bboard::DEBRUIJN = 0x022fdd63cc95386d;

/*
Indexing:
a1 corresponds to the LSB and h8 to the MSB. a2 is the second-LSB.
*/

namespace {

// compute attacks for an occupancy the slow way
Bitboard sliding_attacks(const Square& origin, const Bitboard& occ,
                                   Direction directions[]) {
    Bitboard attacks = 0x0ULL;
    Bitboard last_and = 0x0ULL;
    for (int i = 0; i < 4; i++) {
        Direction dir = directions[i];
        Square sq = origin;

        while (utils::move_square(sq, dir)) {
            attacks |= (1ULL << sq);
            if ((attacks & occ) != last_and) {
                // cannot move further in this direction
                last_and = attacks & occ;
                break;
            }
        }
    }
    return attacks;
}

void b_init_occupancies() {
    // initialize occupancies
    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        // mask is the sliding attacks of sq on an empty board
        Bitboard mask = sliding_attacks(sq, 0x0ULL, b_directions);

        // get relevant occupancy (without edge)
        Bitboard rel_occupancy = mask & REL_OCC_MASK;
        assert(rel_occupancy);

        bboard::b_magics[sq].occupancy_mask = rel_occupancy;

        // NOTE assuming 64 bit
        bboard::b_magics[sq].shift = 64 - utils::popcount(rel_occupancy);
    }
}

void r_init_occupancies() {
    // initialize occupancies
    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        // mask is the sliding attacks of sq on an empty board
        Bitboard mask = 0x0ULL;

        // get relevant occupancy (without edge)
        int rank = utils::sq_rank(sq);
        int file = utils::sq_file(sq);
        for (int r = rank + 1; r < 7; r++) mask |= (1ULL << (file + r * 8));
        for (int r = rank - 1; r > 0; r--) mask |= (1ULL << (file + r * 8));
        for (int f = file + 1; f < 7; f++) mask |= (1ULL << (f + rank * 8));
        for (int f = file - 1; f > 0; f--) mask |= (1ULL << (f + rank * 8));

        assert(mask);
        bboard::r_magics[sq].occupancy_mask = mask;

        // NOTE assuming 64 bit
        bboard::r_magics[sq].shift = 64 - utils::popcount(mask);
    }
}

/*
Generate magics
 */
void gen_magics(bboard::MagicInfo magics[], Bitboard table[],
                Direction directions[]) {
    Bitboard reference[4096];  // 2^12, largest size of occ set of any square
    Bitboard occupancy[4096];
    unsigned long long seed = 322;  // soft TODO find good seeds
    int ord = 0;                    // ordinality; used as index for reference[]
    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        Bitboard rel_occupancy = magics[sq].occupancy_mask;
        // stores the current subset of occupancy
        Bitboard n = 0ULL;
        // iterate over subsets using the carry-rippler
        // https://www.chessprogramming.org/Traversing_Subsets_of_a_Set

        // ord is the size of the table for the previous square
        magics[sq].table = (sq == SQ_A1) ? table : (magics[sq - 1].table + ord);
        ord = 0;
        do {
            // TODO attack reference in as a temp local var
            reference[ord] = sliding_attacks(sq, n, directions);
            occupancy[ord] = n;
            ord++;
            n = (n - rel_occupancy) & rel_occupancy;
        } while (n != 0ULL);
        utils::PRNG prng(seed);
        bool good = true;
        // try 100 million times
        for (int k = 0; k < 100000000; k++) {
            magics[sq].magic = (Bitboard)prng.rand64_sparse();
            // need certain bit density
            if (utils::popcount((magics[sq].magic * rel_occupancy) >> 56) < 6)
                continue;

            int collisions = 0;  // count of acceptable (good) collisions
            /*
            TODO implement later
            there is a speed-up trick here by sacrificing space:
            here we are filling the table with -1's each epoch to denote that
            the index is unused. we can avoid that by keeping an array of 
            integers, where array[index] denotes the last epoch number $index
            is used. if array[index] < current epoch, then index is free to use
            and array[index] is updated; else fail this current epoch if 
            reference != table[index].

            Note that the size of this array could be statically allocated to be
            4096, the largest possible table size for a square, or dynamically
            allocated to have size = ord

            Also TODO optimize for PEXT simd instructions where
            index = pext(occupancy_mask, occupancy)
            */
            std::fill_n(magics[sq].table, ord, 0ULL);
            good = true;
            for (int i = 0; i < ord; i++) {
                // note index <= ord because ord = 2 ** (64 - shifts) and
                // index is shifted to have at most (64 - shifts) nonzero bits
                int index = magics[sq].get_index(occupancy[i]);
                if (magics[sq].table[index] == 0ULL) {
                    // unoccupied
                    magics[sq].table[index] = reference[i];
                } else {
                    if (magics[sq].table[index] != reference[i]) {
                        // bad collision
                        good = false;
                        break;
                    } else {  // otherwise, good collision
                        collisions++;
                    }
                }
            }
            if (good) {
                break;
            }
        }
        if (!good) {
            LOG(logERROR) << "magic not found";
        }
    }
}

#if 0
// temporary function for faster debugging
Bitboard from_str(string repr) {
    Bitboard ret = 0x0ULL;
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            if (repr[(7 - rank) * 8 + file] == '1')
                ret |= 1ULL << (rank * 8 + file);
        }
    }

    return ret;
}
#endif

void init_magic(void) {
    b_init_occupancies();
    r_init_occupancies();

    gen_magics(bboard::b_magics, bboard::b_attack_table, b_directions);
    gen_magics(bboard::r_magics, bboard::r_attack_table, r_directions);
}

// initialize attack tables for pawns, knights, and kings
void init_attack_tables(void) {
    Bitboard mask;
    Square t_sq;
    Color colors[]{WHITE, BLACK};

    // pawns
    for (Color c : colors) {
        int d_rank = c == WHITE ? 1 : -1;
        // actually we can omit the last row for each color since a pawn cannot
        // possibly be there, but that doesn't make a huge difference
        for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
            mask = 0ULL;

            t_sq = sq;
            if (utils::move_square(t_sq, d_rank, -1)) {
                mask |= bboard::mask_square(t_sq);
            }

            t_sq = sq;
            if (utils::move_square(t_sq, d_rank, 1)) {
                mask |= bboard::mask_square(t_sq);
            }

            bboard::p_attack_table[(int)c][sq] = mask;
        }
    }

    // // pawn first pushes (2 squares)
    // for (Square sq = SQ_A2; sq <= SQ_H2; sq++) {
    //     // move two ranks up
    //     bboard::p_push_table[(int)WHITE][sq] |= bboard::mask_square(to_square(sq + 16));
    // }

    // for (Square sq = SQ_A7; sq <= SQ_H7; sq++) {
    //     // move two ranks down
    //     bboard::p_push_table[(int)BLACK][sq] |= bboard::mask_square(to_square(sq - 16));
    // }

    // knights
    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        mask = 0ULL;
        for (int d_rank = -1; d_rank <= 1; d_rank += 2) {
            for (int d_file = -2; d_file <= 2; d_file += 4) {
                t_sq = sq;
                if (utils::move_square(t_sq, d_rank, d_file)) {
                    mask |= bboard::mask_square(t_sq);
                }
            }
        }

        for (int d_file = -1; d_file <= 1; d_file += 2) {
            for (int d_rank = -2; d_rank <= 2; d_rank += 4) {
                t_sq = sq;
                if (utils::move_square(t_sq, d_rank, d_file)) {
                    mask |= bboard::mask_square(t_sq);
                }
            }
        }

        bboard::n_attack_table[sq] = mask;
    }

    // kings
    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        mask = 0ULL;
        for (int d_rank = -1; d_rank <= 1; d_rank++) {
            for (int d_file = -1; d_file <= 1; d_file++) {
                t_sq = sq;
                if (utils::move_square(t_sq, d_rank, d_file)) {
                    mask |= bboard::mask_square(t_sq);
                }
            }
        }

        mask &= ~bboard::mask_square(sq);  // unset bit at offset (0, 0)

        bboard::k_attack_table[sq] = mask;
    }
}
}  // namespace

void bboard::initialize() {
    LOG(logDEBUG) << "Initializing magics...";
    init_magic();
    LOG(logDEBUG) << "Done.";
    LOG(logDEBUG) << "Initializing other attacks...\n";
    init_attack_tables();
    LOG(logDEBUG) << "Done.";
}

unsigned int bboard::MagicInfo::get_index(Bitboard occupancy) {
    return (unsigned int)(((occupancy & occupancy_mask) * magic) >> shift);
}

void test_magics() {
    LOG(logINFO) << "Magic initialized. Testing...";

    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        bboard::MagicInfo m = bboard::b_magics[sq];
        Bitboard occ = m.occupancy_mask;
        Bitboard n = 0ULL;
        do {
            Bitboard att = m.table[m.get_index(n)];
            assert(sliding_attacks(sq, n, b_directions) == att);
            n = (n - occ) & occ;
        } while (n != 0ULL);
    }
    LOG(logINFO) << "Magic tested for bishops";

    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        bboard::MagicInfo m = bboard::r_magics[sq];
        Bitboard occ = m.occupancy_mask;
        Bitboard n = 0ULL;
        do {
            Bitboard att = m.table[m.get_index(n)];
            assert(sliding_attacks(sq, n, r_directions) == att);
            n = (n - occ) & occ;
        } while (n != 0ULL);
    }
    LOG(logINFO) << "Magic tested for rooks";
    LOG(logINFO) << "Done.";
}

Bitboard bboard::blocker(Square s1, Square s2, Bitboard occ) {
    I8 d_rank = utils::sq_rank(s2) - utils::sq_rank(s1);
    I8 d_file = utils::sq_file(s2) - utils::sq_file(s1);

    Direction dir{(I8)((0 < d_rank) - (d_rank < 0)),
                  (I8)((0 < d_file) - (d_file < 0))};

    for (utils::move_square(s1, dir); s1 != s2; utils::move_square(s1, dir)) {
        if ((occ & bboard::mask_square(s1)) != 0ULL) {
            return bboard::mask_square(s1);
        }
    }
    assert(false);
    return 0ULL;
}

Bitboard bboard::bishop_xray_attacks(Square sq, Bitboard occ, Bitboard blockers) {
    Bitboard attacks = bishop_attacks(sq, occ);  // attacks b4 removing blockers
    blockers &= attacks;                         // remove only blockers of bishop's ray
    // get symmetric difference between attacks before removing blockers and
    // attacks after removing blockers (i.e. occ ^ blockers).
    return attacks ^ bishop_attacks(sq, occ ^ blockers);
}

Bitboard bboard::rook_xray_attacks(Square sq, Bitboard occ, Bitboard blockers) {
    Bitboard attacks = rook_attacks(sq, occ);
    blockers &= attacks;
    return attacks ^ rook_attacks(sq, occ ^ blockers);
}
