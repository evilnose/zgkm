#pragma once

using Bitboard = unsigned long long;
using I8 = signed char;
using Square = unsigned char;

using U64 = unsigned long long;

constexpr Square SQ_A1 = 0;
constexpr Square SQ_H8 = 63;

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

enum Color {
    WHITE,
    BLACK,
    N_COLORS
};

enum MoveType {
    NORMAL_MOVE,
    CASTLING_MOVE,
    PROMOTION,
    EN_PASSANT,
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
bits 0-5:   target square (64)
bits 6-11:  source square (64)
bits 12-13: move type (4)
bits 14-15: promotion piece (4; 0->KNIGHT, ... 3->QUEEN)
*/
using Move = unsigned short;
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

inline MoveType move_type(Move mv) {
    return (MoveType)((mv & 0xf) >> (MOVE_LEN - 14));
}

inline PieceType move_promotion(Move mv) {
    return (PieceType)((mv & MOVE_PROMOTION_MASK) + 2);
}

/*
inline Move create_move(Square tar, Square src, MoveType type,
                           PieceType promotion) {
    return ((Move)piece - 2) | (((Move)promotion) << 2) | ((Move)src << 4) |
           ((Move)tar << 10);
}
*/

inline Move create_normal_move(Square tar, Square src) {
    return (((Move)NORMAL_MOVE) << 2) | ((Move)src << 4) | ((Move)tar << 10);
}

enum CastleState {
    WHITE_O_O = 1 << 0,
    WHITE_O_O_O = 1 << 1,
    BLACK_O_O = 1 << 2,
    BLACK_O_O_O = 1 << 3
};

extern CastleState init_cstate;
