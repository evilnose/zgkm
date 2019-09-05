#include <algorithm>
#include <cassert>
#include <iostream>

#include "bitboard.h"
#include "utils.h"

constexpr int B_TABLE_SZ = 5248;
constexpr int R_TABLE_SZ = 102400;
constexpr Bitboard REL_OCC_MASK = 0x7E7E7E7E7E7E00ULL;
using std::string;
using namespace bboard;

// magic bitboard database for bishops
MagicInfo b_magics[64];
MagicInfo r_magics[64];
Bitboard b_move_table[B_TABLE_SZ];  // the big move table
Bitboard r_move_table[R_TABLE_SZ];
Direction b_directions[]{NORTHWEST, NORTHEAST, SOUTHWEST, SOUTHEAST};
Direction r_directions[]{NORTH, SOUTH, WEST, EAST};

// for bitscan
static constexpr Square debruijn_table[64] =
    {
        0, 1, 2, 53, 3, 7, 54, 27,
        4, 38, 41, 8, 34, 55, 48, 28,
        62, 5, 39, 46, 44, 42, 22, 9,
        24, 35, 59, 56, 49, 18, 29, 11,
        63, 52, 6, 26, 37, 40, 33, 47,
        61, 45, 43, 21, 23, 58, 17, 10,
        51, 25, 36, 32, 60, 20, 57, 16,
        50, 31, 19, 15, 30, 14, 13, 12};

constexpr Bitboard DEBRUIJN = 0x022fdd63cc95386d;

/*
Indexing:
a1 corresponds to the LSB and h8 to the MSB. a2 is the second-LSB.
*/

namespace {

// compute moves for an occupancy the slow way
inline Bitboard sliding_moves(const Square& origin, const Bitboard& occ,
                              Direction directions[]) {
    Bitboard moves = 0x0ULL;
    Bitboard last_and = 0x0ULL;
    for (int i = 0; i < 4; i++) {
        Direction dir = directions[i];
        Square sq = origin;

        while (move_square(sq, dir)) {
            moves |= (1ULL << sq);
            if ((moves & occ) != last_and) {
                // cannot move further in this direction
                last_and = moves & occ;
                break;
            }
        }
    }
    return moves;
}

inline void b_init_occupancies() {
    // initialize occupancies
    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        // mask is the sliding moves of sq on an empty board
        Bitboard mask = sliding_moves(sq, 0x0ULL, b_directions);

        // get relevant occupancy (without edge)
        Bitboard rel_occupancy = mask & REL_OCC_MASK;
        assert(rel_occupancy);

        b_magics[sq].occupancy_mask = rel_occupancy;

        // soft TODO: optimize for 32 bit
        // https://www.chessprogramming.org/Magic_Bitboards#32-bit_Magics
        b_magics[sq].shift = 64 - popcount(rel_occupancy);
    }
}

inline void r_init_occupancies() {
    // initialize occupancies
    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        // mask is the sliding moves of sq on an empty board
        Bitboard mask = 0x0ULL;

        // get relevant occupancy (without edge)
        int rank = sq_rank(sq);
        int file = sq_file(sq);
        for (int r = rank + 1; r < 7; r++) mask |= (1ULL << (file + r * 8));
        for (int r = rank - 1; r > 0; r--) mask |= (1ULL << (file + r * 8));
        for (int f = file + 1; f < 7; f++) mask |= (1ULL << (f + rank * 8));
        for (int f = file - 1; f > 0; f--) mask |= (1ULL << (f + rank * 8));

        assert(mask);
        r_magics[sq].occupancy_mask = mask;

        // soft TODO: optimize for 32 bit
        // https://www.chessprogramming.org/Magic_Bitboards#32-bit_Magics
        r_magics[sq].shift = 64 - popcount(mask);
    }
}

/*
Generate magics
 */
void gen_magics(MagicInfo magics[], Bitboard table[], Direction directions[]) {
    Bitboard reference[4096];  // 2^12, largest size of occ set of any square
    Bitboard occupancy[4096];
    unsigned long long seed = 322;  // TODO find good seeds
    int ordsum = 0;
    int ord = 0;  // ordinality; used as index for reference[]
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
            // TODO move reference in as a temp local var
            reference[ord] = sliding_moves(sq, n, directions);
            occupancy[ord] = n;
            ord++;
            n = (n - rel_occupancy) & rel_occupancy;
        } while (n != 0ULL);
        PRNG prng(seed);
        bool good = true;
        // try 100 million times
        for (int k = 0; k < 100000000; k++) {
            // TODO generate magics in a loop until a magic is found to be correct
            // ALSO magic needs to have a certain number of nonzero bits
            magics[sq].magic = (Bitboard)prng.rand64_sparse();
            // need certain bit density
            if (popcount((magics[sq].magic * rel_occupancy) >> 56) < 6)
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
            /*
            Also possible here to exchange space for time: instead of using
            the bit subset trick again here, simply iterate over the reference
            table. But how would I get the occupancies? Answer: in the 
            previous subset iteration loop, store the occupancies in a temp
            array.
            */
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
                // printf("Magic found. Collisions: %d; size: %d, trials: %d\n",
                //        collisions, ord, k);
                break;
            }
        }
        if (!good) {
            printf("ERROR: magic not found.");
        }
    }
}

// return the index of the least significant set bit
inline Square bitscan(Bitboard board) {
    return debruijn_table[((board & -board) * DEBRUIJN) >> 58];
}

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
}  // namespace

// TODO try constexpr this whole file :)
void bboard::init_magic(void) {
    b_init_occupancies();
    r_init_occupancies();

    gen_magics(b_magics, b_move_table, b_directions);
    gen_magics(r_magics, r_move_table, r_directions);
}

string bboard::repr(Bitboard bitboard) {
    char ret[73];  // 8 * 9
    for (int i = 0; i < 8; i++) {
        ret[i * 9 + 8] = '\n';
    }

    for (int index = 0; index < 64; index++) {
        int rank = sq_rank(index);
        int file = sq_file(index);
        char ch = ((1ULL << index) & bitboard) ? '1' : '0';
        ret[(7 - rank) * 9 + file] = ch;
    }
    ret[72] = '\0';
    return string(ret);
}

unsigned int bboard::MagicInfo::get_index(Bitboard occupancy) {
    return (unsigned int)(((occupancy & occupancy_mask) * magic) >> shift);
}

void test_deleteme() {
    init_magic();

    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        MagicInfo m = b_magics[sq];
        Bitboard occ = m.occupancy_mask;
        Bitboard n = 0ULL;
        do {
            Bitboard att = m.table[m.get_index(n)];
            if (sliding_moves(sq, n, b_directions) != att) {
                printf("assertion failed\n");
                return;
            }
            n = (n - occ) & occ;
        } while (n != 0ULL);
    }
    std::cout << "Magics tested for bishop" << std::endl;

    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        MagicInfo m = r_magics[sq];
        Bitboard occ = m.occupancy_mask;
        Bitboard n = 0ULL;
        do {
            Bitboard att = m.table[m.get_index(n)];
            if (sliding_moves(sq, n, r_directions) != att) {
                printf("assertion failed\n");
                return;
            }
            n = (n - occ) & occ;
        } while (n != 0ULL);
    }
    std::cout << "Magics tested for rook" << std::endl;
}
