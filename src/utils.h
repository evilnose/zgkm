#pragma once

#include "types.h"

#include <random>

bool move_square(Square& sq, const Direction& dir);
Bitboard mask_square(const Square& sq);
int popcount(unsigned long long);
inline int sq_rank(Square sq) { return sq / 8; }
inline int sq_file(Square sq) { return sq % 8; }

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
