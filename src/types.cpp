#include "types.h"

Direction NORTH{0, 1};
Direction SOUTH{0, -1};
Direction WEST{-1, 0};
Direction EAST{1, 0};
Direction NORTHWEST{-1, 1};
Direction NORTHEAST{1, 1};
Direction SOUTHWEST{-1, -1};
Direction SOUTHEAST{1, -1};

CastleState init_cstate = (CastleState)15;

Square& operator++(Square& square) {
    square = static_cast<Square>(square + 1);
    return square;
}

Square operator++(Square& square, int) {
    Square ret = square;
    ++square;
    return ret;
}

