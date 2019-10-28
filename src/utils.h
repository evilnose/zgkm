#pragma once

#include "types.h"

#include <random>
#include <string>

constexpr int sq_rank(Square sq) { return sq / 8; }
constexpr int sq_file(Square sq) { return sq % 8; }

constexpr bool move_square(Square& sq, int d_rank, int d_file) {
    int rank = sq_rank(sq) + d_rank;
    int file = sq_file(sq) + d_file;

    if (rank < 0 || rank >= 8 || file < 0 || file >= 8) {
        return false;
    }

    sq = rank * 8 + file;
    return true;
}

constexpr bool move_square(Square& sq, const Direction& dir) {
    return move_square(sq, dir.d_rank, dir.d_file);
}

constexpr Bitboard mask_square(const Square& square) {
    return 1ULL << square;
}

constexpr int popcount(unsigned long long n) {
    return __builtin_popcount((unsigned int)(n)) +
           __builtin_popcount((unsigned int)(n >> 32));
}

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

constexpr Color opposite_color(Color c) {
    return (Color)((int)N_COLORS - 1 - (int)c);
}

std::string repr(Bitboard board);
