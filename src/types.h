#pragma once

#include <cinttypes>
#include <cassert>
#include <string>

// #define MAT_ONLY
#define USE_PESTO 0

using Bitboard = uint64_t;
using ZobristKey = uint64_t;
using I8 = int8_t;
using U64 = uint64_t;
using Move = unsigned short;
using CastlingRights = unsigned char;

#if USE_PESTO
using Score = int;  // centipawns
#else
using Score = float;
#endif

constexpr Bitboard RANK_A = 0xFF;
constexpr Bitboard RANK_B = RANK_A << 8;
constexpr Bitboard RANK_C = RANK_B << 8;
constexpr Bitboard RANK_D = RANK_C << 8;
constexpr Bitboard RANK_E = RANK_D << 8;
constexpr Bitboard RANK_F = RANK_E << 8;
constexpr Bitboard RANK_G = RANK_F << 8;
constexpr Bitboard RANK_H = RANK_G << 8;
constexpr Bitboard PROMOTION_RANKS = RANK_A | RANK_H;
constexpr Bitboard WHITE_HALF = RANK_A | RANK_B | RANK_C | RANK_D;
constexpr Bitboard BLACK_HALF = ~WHITE_HALF;

constexpr Bitboard ROOK_FILES = 0x8181818181818181;

struct Direction {
    I8 d_rank;
    I8 d_file;
};

extern Direction NORTH;
extern Direction SOUTH;
extern Direction WEST;
extern Direction EAST;
extern Direction NORTHWEST;
extern Direction NORTHEAST;
extern Direction SOUTHWEST;
extern Direction SOUTHEAST;

/* 
NOTE if this were changed, some other things need to be changed too:
piece offset for promotion move;
is_slider_table
*/
enum PieceType {
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    ANY_PIECE,
    NO_PIECE,
    N_PIECE_TYPES
};

// i.e. PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
constexpr int N_REAL_PIECE_TYPES = (int) KING + 1;

enum Square: uint8_t {
    SQ_A1 = 0, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    N_SQUARES
};

constexpr char STARTING_FEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr char KIWIPETE_FEN[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

Square& operator++(Square& square);
Square operator++(Square& square, int);

enum Color {
    WHITE,
    BLACK,
    N_COLORS
};

enum MoveType {
    NORMAL_MOVE,
    CASTLING_MOVE,
    PROMOTION,
    ENPASSANT,
    N_MOVE_TYPES
};

enum BoardSide {
    KINGSIDE,
    QUEENSIDE,
    N_BOARD_SIDES
};

/*
Move representation
bits 0-1: promotion piece (4; 0->KNIGHT, ... 3->QUEEN)
bits 2-3: move type (4)
bits 4-9:  source square (64) / castling side
bits 10-15:   target square (64) / color if castling move
*/
constexpr int MOVE_LEN = 16;
constexpr Move MOVE_TARGET_MASK = 0xFC00;
constexpr Move MOVE_SOURCE_MASK = 0x3F0;
constexpr Move MOVE_PROMOTION_MASK = 0x3;
constexpr Move NULL_MOVE = 0;

// Move helper functions
inline Square get_move_target(Move mv) {
    return (Square)(mv >> (MOVE_LEN - 6));
}

inline Square get_move_source(Move mv) {
    return (Square)((mv & ~MOVE_TARGET_MASK) >> (MOVE_LEN - 12));
}

inline Color get_move_castle_color(Move mv) {
    return (Color)(mv >> (MOVE_LEN - 6));
}

inline BoardSide get_move_castle_side(Move mv) {
    return (BoardSide)((mv & ~MOVE_TARGET_MASK) >> (MOVE_LEN - 12));
}

inline MoveType get_move_type(Move mv) {
    return (MoveType)((mv & 0xf) >> (MOVE_LEN - 14));
}

inline PieceType get_move_promotion(Move mv) {
    return (PieceType)((mv & MOVE_PROMOTION_MASK) + 1);
}

// BEGIN move helper functions
// The 'tgt' argument should specify the square that the capturing pawn would move to.
inline Move create_enpassant(Square src, Square tgt) {
    return (((Move)ENPASSANT) << 2) | ((Move)src << 4) |
           ((Move)tgt << 10);
}

inline Move create_normal_move(Square src, Square tgt) {
    return (((Move)NORMAL_MOVE) << 2) | ((Move)src << 4) | ((Move)tgt << 10);
}

inline Move create_castling_move(Color c, BoardSide side) {
    return (((Move)CASTLING_MOVE) << 2) | ((Move)side << 4) | ((Move)c << 10);
}

inline Move create_promotion_move(Square src, Square tgt, PieceType target_piece) {
    assert(target_piece == QUEEN || target_piece == ROOK || target_piece == KNIGHT ||
            target_piece == BISHOP);
    return ((Move)target_piece - 1) | ((Move)PROMOTION << 2) | ((Move)src << 4)
        | ((Move)tgt << 10);
}
// END Move helper functions

extern CastlingRights WHITE_OO;
extern CastlingRights WHITE_OOO;
extern CastlingRights BLACK_OO;
extern CastlingRights BLACK_OOO;
extern CastlingRights NO_CASTLING_RIGHTS;
extern CastlingRights ALL_CASTLING_RIGHTS;

constexpr Score SCORE_NEG_INFTY = -1000000;
constexpr Score SCORE_POS_INFTY = 1000000;
// Score for drawing the opponent. Might want to make this adjustable in the future
constexpr Score SCORE_DRAW = 0;
