#pragma once

#include "types.h"

#include <cassert>
#include <random>
#include <string>


namespace utils {

static bool is_slider_table[]{0, 0, 0, 1, 1, 1, 0, 0};
extern Square KING_INIT_SQUARES[2];

inline int sq_rank(Square sq) { return sq / 8; }
inline int sq_file(Square sq) { return sq % 8; }
inline Square make_square(int rank, int file) {
    return Square(rank * 8 + file);
}

inline Square to_square(uint8_t val) { return static_cast<Square>(val); }

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

inline CastlingRights to_castling_rights(Color c, BoardSide side) {
    return (CastlingRights)((1 << (2 * (int)c)) + (1 << (int)side));
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
    PRNG(U64 seed) : gen(seed), state(seed) {}

    U64 rand64() {
        // return dist(gen);
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1DLL;
    }

    U64 rand64_sparse() { return U64(rand64() & rand64() & rand64()); }
};

inline Color opposite_color(Color c) {
    return (Color)((int)N_COLORS - 1 - (int)c);
}

std::string repr(Bitboard board);

inline int pawn_direction(Color c) { return 1 - static_cast<int>(c) * 2; }

// KING_CASTLING_TARGET[i][j] returns the king target square for color i
// and boardside j.
static Square KING_CASTLING_TARGET[2][2] { { SQ_G1, SQ_C1 }, { SQ_G8, SQ_C8 } };
static Square ROOK_CASTLING_TARGET[2][2] { { SQ_F1, SQ_D1 }, { SQ_F8, SQ_D8 } };
static Square ROOK_CASTLING_SOURCE[2][2] { { SQ_H1, SQ_A1 }, { SQ_H8, SQ_A8 } };

inline Square king_castle_target(Color c, BoardSide side) {
    return KING_CASTLING_TARGET[c][side];
}

inline Square rook_castle_target(Color c, BoardSide side) {
    return ROOK_CASTLING_TARGET[c][side];
}

inline Square rook_castle_source(Color c, BoardSide side) {
    return ROOK_CASTLING_SOURCE[c][side];
}

}  // namespace utils
