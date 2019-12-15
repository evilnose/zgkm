#include "types.h"

Direction NORTH{0, 1};
Direction SOUTH{0, -1};
Direction WEST{-1, 0};
Direction EAST{1, 0};
Direction NORTHWEST{-1, 1};
Direction NORTHEAST{1, 1};
Direction SOUTHWEST{-1, -1};
Direction SOUTHEAST{1, -1};

CastlingRights WHITE_O_O_O = 1 << 0;
CastlingRights WHITE_O_O = 1 << 1;
CastlingRights BLACK_O_O = 1 << 2;
CastlingRights BLACK_O_O_O = 1 << 3;
CastlingRights NO_CASTLING_RIGHTS = 0;
CastlingRights ALL_CASTLING_RIGHTS =
    WHITE_O_O | WHITE_O_O_O | BLACK_O_O | BLACK_O_O_O;

Square& operator++(Square& square) {
    square = static_cast<Square>(square + 1);
    return square;
}

Square operator++(Square& square, int) {
    Square ret = square;
    ++square;
    return ret;
}
