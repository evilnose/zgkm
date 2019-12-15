#pragma once

#include <cinttypes>

using Bitboard = uint64_t;
using I8 = int8_t;
using U64 = uint64_t;
using Move = unsigned short;
using CastlingRights = unsigned short;

constexpr Bitboard RANK_A = 0xFF;
constexpr Bitboard RANK_B = RANK_A << 8;
constexpr Bitboard RANK_C = RANK_B << 8;
constexpr Bitboard RANK_D = RANK_C << 8;
constexpr Bitboard RANK_E = RANK_D << 8;
constexpr Bitboard RANK_F = RANK_E << 8;
constexpr Bitboard RANK_G = RANK_F << 8;
constexpr Bitboard RANK_H = RANK_G << 8;

constexpr Bitboard PROMOTION_RANKS = RANK_A | RANK_H;

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

enum PieceType {
    NO_PIECE,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    ANY_PIECE,
    N_PIECE_TYPES
};

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
struct Move {
    MoveType type;
    Color side;
    // could use a union here but barely saves any space
    Square src_square;
    Square dst_square;
    BoardSide castle_side;
};
*/
/*
Move representation
bits 0-5:   target square (64) / color if castling move
bits 6-11:  source square (64) / castling side
bits 12-13: move type (4)
bits 14-15: promotion piece (4; 0->KNIGHT, ... 3->QUEEN)
*/
constexpr int MOVE_LEN = 16;
constexpr Move MOVE_TARGET_MASK = 0xFC00;
constexpr Move MOVE_SOURCE_MASK = 0x3F0;
constexpr Move MOVE_PROMOTION_MASK = 0x3;

inline Square move_target(Move mv) {
    return (Square)(mv >> (MOVE_LEN - 6));
}

inline Square move_source(Move mv) {
    return (Square)((mv & ~MOVE_TARGET_MASK) >> (MOVE_LEN - 12));
}

inline Color move_castle_color(Move mv) {
	return (Color)(mv >> (MOVE_LEN - 6));
}

inline BoardSide move_castle_side(Move mv) {
    return (BoardSide)((mv & ~MOVE_TARGET_MASK) >> (MOVE_LEN - 12));
}

inline MoveType move_type(Move mv) {
    return (MoveType)((mv & 0xf) >> (MOVE_LEN - 14));
}

inline PieceType move_promotion(Move mv) {
    return (PieceType)((mv & MOVE_PROMOTION_MASK) + 2);
}

inline Move create_enpassant(Square tar, Square src, MoveType type) {
    return (((Move)ENPASSANT) << 2) | ((Move)src << 4) |
           ((Move)tar << 10);
}

inline Move create_normal_move(Square tar, Square src) {
    return (((Move)NORMAL_MOVE) << 2) | ((Move)src << 4) | ((Move)tar << 10);
}

inline Move create_castling_move(Color c, BoardSide side) {
    return (((Move)CASTLING_MOVE) << 2) | ((Move)side << 4) | ((Move)c << 10);
}

extern CastlingRights WHITE_O_O;
extern CastlingRights WHITE_O_O_O;
extern CastlingRights BLACK_O_O;
extern CastlingRights BLACK_O_O_O;
extern CastlingRights NO_CASTLING_RIGHTS;
extern CastlingRights ALL_CASTLING_RIGHTS;
