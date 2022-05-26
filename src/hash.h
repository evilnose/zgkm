#pragma once

#include "types.h"

#include <cstddef>
#include <vector>

namespace zobrist {
void initialize(void);
ZobristKey get_key(Square, PieceType, Color);
ZobristKey get_black_to_move_key(void);
}  // namespace zobrist

namespace ht {

struct Entry {
	ZobristKey key;  // 8B
	unsigned depth;  // 4B
	Score score;  // 4B; integrated bound and value score
	Move bestmove;  // 2B; this is NULL_MOVE if node is terminal or node_type == 3, i.e. fail-low
	short node_type;  // 2B; Knuth's type 1, 2, or 3 node
	// TODO age?
};

class Table {
public:
 Table(size_t);
 Entry get(ZobristKey) const;
 bool contains(ZobristKey) const;
 bool has_collision(ZobristKey) const;
 void put(Entry);

private:
 size_t sz;
 std::vector<Entry> entries;  // initialized to 0's
};


Table& global_table();
void set_global_table(size_t);

}  // namespace ht

