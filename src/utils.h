#pragma once

#include "types.h"

#include <random>
#include <string>
#include <cassert>

static bool is_slider_table[]{0, 0, 0, 1, 1, 1, 0, 0};

namespace utils {
inline int sq_rank(Square sq) { return sq / 8; }
inline int sq_file(Square sq) { return sq % 8; }

inline Square to_square(uint8_t val) {
    return static_cast<Square>(val);
}

bool move_square(Square& sq, int d_rank, int d_file);

inline bool move_square(Square& sq, const Direction& dir) {
    return move_square(sq, dir.d_rank, dir.d_file);
}

inline int popcount(unsigned long long n) {
    return __builtin_popcount((unsigned int)(n)) +
           __builtin_popcount((unsigned int)(n >> 32));
}

inline bool is_slider(PieceType pt) {
    assert(pt < (int)N_PIECE_TYPES);
    return is_slider_table[(int)pt];
}

inline CastleState to_castle_state(Color c, BoardSide side) {
	return (CastleState)((1 << (2 * (int)c)) + (1 << (int)side));
}

// /* read from file and trim all whitespace; output to buf */
void read_and_trim(std::string fname, char buf[]);

// pseudorandom number generator
class PRNG {
    // std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_int_distribution<long long int> dist;
    U64 state;

   public:
    PRNG(U64 seed) : gen(seed), state(seed) {
    }

    U64 rand64() {
        // return dist(gen);
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1DLL;
    }

    U64 rand64_sparse() {
        return U64(rand64() & rand64() & rand64());
    }
};

inline Color opposite_color(Color c) {
    return (Color)((int)N_COLORS - 1 - (int)c);
}

std::string repr(Bitboard board);
}
