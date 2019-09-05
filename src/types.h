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
    CAPTURE_MOVE,
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

struct Move {
    MoveType type;
    Color side;
    // could use a union here but barely saves any space
    Square src_square;
    Square dst_square;
    BoardSide castle_side;
};

enum CastleState {
    WHITE_O_O = 1 << 0,
    WHITE_O_O_O = 1 << 1,
    BLACK_O_O = 1 << 2,
    BLACK_O_O_O = 1 << 3
};
