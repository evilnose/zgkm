#include "utils.h"

bool move_square(Square& sq, const Direction& dir) {
    int rank = sq_rank(sq) + dir.d_rank;
    int file = sq_file(sq) + dir.d_file;

    if (rank < 0 || rank >= 8 || file < 0 || file >= 8) {
        return false;
    }

    sq = rank * 8 + file;
    return true;
}

Bitboard mask_square(const Square& square) {
    return 1ULL << square;
}

int popcount(unsigned long long n) {
    return __builtin_popcount((unsigned int)(n)) + __builtin_popcount((unsigned int)(n >> 32));
}
