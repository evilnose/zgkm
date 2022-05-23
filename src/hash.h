#pragma once

#include "types.h"

namespace zobrist {
void initialize(void);
ZobristKey get_key(Square, PieceType, Color);
ZobristKey get_black_to_move_key(void);
}  // namespace zobrist

namespace hashtable {

struct Entry {
	ZobristKey key;
};

void alloc(unsigned n);  // allocate new, empty table with n slots
}  // namespace hashtable
