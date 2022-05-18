#include "types.h"

Direction NORTH{0, 1};
Direction SOUTH{0, -1};
Direction WEST{-1, 0};
Direction EAST{1, 0};
Direction NORTHWEST{-1, 1};
Direction NORTHEAST{1, 1};
Direction SOUTHWEST{-1, -1};
Direction SOUTHEAST{1, -1};

CastlingRights WHITE_OO = 1 << 0;
CastlingRights WHITE_OOO = 1 << 1;
CastlingRights BLACK_OO = 1 << 2;
CastlingRights BLACK_OOO = 1 << 3;
CastlingRights NO_CASTLING_RIGHTS = 0;
CastlingRights ALL_CASTLING_RIGHTS =
    WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO;

Square& operator++(Square& square) {
    square = static_cast<Square>(square + 1);
    return square;
}

Square operator++(Square& square, int) {
    Square ret = square;
    ++square;
    return ret;
}
