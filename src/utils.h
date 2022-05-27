#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <locale>
#include <random>
#include <string>

using namespace std::chrono;

#include "types.h"

namespace utils {

static bool is_slider_table[]{0, 0, 1, 1, 1, 0, 0, 0};
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

inline Square flip(Square sq) {
    return (Square) ((int) sq ^ 56);
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
    return (CastlingRights)(1 << (2 * (int)c + (int)side));
}

// TODO optimize this if necessary
inline CastlingRights to_castling_rights(Color c) {
    return utils::to_castling_rights(c, KINGSIDE) |
           utils::to_castling_rights(c, QUEENSIDE);
}

// /* read from file and trim all whitespace; output to buf */
void read_and_trim(std::string fname, char buf[]);

// pseudorandom number generator using xorshift*
// (https://en.wikipedia.org/wiki/Xorshift)
class PRNG {
    U64 state;

   public:
    PRNG(U64 seed) : state(seed) {}

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
static Square KING_CASTLING_TARGET[2][2]{{SQ_G1, SQ_C1}, {SQ_G8, SQ_C8}};
static Square ROOK_CASTLING_TARGET[2][2]{{SQ_F1, SQ_D1}, {SQ_F8, SQ_D8}};
static Square ROOK_CASTLING_SOURCE[2][2]{{SQ_H1, SQ_A1}, {SQ_H8, SQ_A8}};

inline Square king_castle_target(Color c, BoardSide side) {
    return KING_CASTLING_TARGET[c][side];
}

inline Square rook_castle_target(Color c, BoardSide side) {
    return ROOK_CASTLING_TARGET[c][side];
}

inline Square rook_castle_source(Color c, BoardSide side) {
    return ROOK_CASTLING_SOURCE[c][side];
}

// return the actual square the enpassant pawn is located, not the captured square
inline Square enpassant_actual(Bitboard enp_sq, Color color) {
    return (Square)(enp_sq + utils::pawn_direction(color) * 8);
}

// returns (int) 1 if val > 0, 0 if == 0, -1 if < 0.
template <typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

// returns 1 if color is WHITE and -1 if BLACK.
inline float color_multiplier(Color color) {
    return 1.0 - ((float) color) * 2;
}

/* String functions */

// The below trim functions (ltrim, rtrim, trim) are from Stack Overflow "jotik"
// https://stackoverflow.com/a/217605
// trim from start (in place)
inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](int ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

// trim from both ends (in place)
inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

template <template <class, class, class...> class C, typename K, typename V,
          typename... Args>
inline V get_or_default(const C<K, V, Args...>& m, K const& key,
                        const V& defval) {
    typename C<K, V, Args...>::const_iterator it = m.find(key);
    if (it == m.end()) return defval;
    return it->second;
}

class Timer {
   public:
    Timer() { zero(); }

    inline void zero() {
        start = system_clock::now();
    }

    inline double elapsed_secs() {
        auto cur = system_clock::now();
        std::chrono::duration<double> elapsed_seconds = cur - start;
        return elapsed_seconds.count();
    }

    inline double elapsed_millis() {
        return elapsed_secs() * 1000;
    }

   private:
    system_clock::time_point start;
};  // class Timer
}  // namespace utils
