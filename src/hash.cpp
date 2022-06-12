#include "hash.h"

#include <array>
#include "utils.h"
#include "logger.h"

/// zobrist stuff
constexpr unsigned N_ZOBRIST_PIECES = 12;
ZobristKey table[64][N_ZOBRIST_PIECES];
ZobristKey black_to_move;


/// hashtable stuff
// std::array<hashtable::Entry> table;


void zobrist::initialize() {
	utils::PRNG prng(123);  // NOTE: this seed was arbitrarily chosen
	for (unsigned i = 0; i < 64; i++) {
		for (unsigned j = 0; j < N_ZOBRIST_PIECES; j++) {
			table[i][j] = prng.rand64();
		}
	}
	black_to_move = prng.rand64();
}

ZobristKey zobrist::get_key(Square sq, PieceType type, Color c) {
	// if black, then the index += 6
	unsigned pindex = ((unsigned) type) + 6 * c;

	assert(pindex < N_ZOBRIST_PIECES);

	return table[sq][pindex];
}

ZobristKey zobrist::get_black_to_move_key() {
	return black_to_move;
}

ht::Table::Table(size_t sz) : sz(sz), entries(sz, ht::Entry{}) {
}

ht::Entry ht::Table::get(ZobristKey key) const {
	size_t index = key % sz;
	ht::Entry entry = entries[index];
	return entry;
}

bool ht::Table::contains(ZobristKey key) const {
	size_t index = key % sz;
	ht::Entry entry = entries[index];
	return entry.key == key;
}

bool ht::Table::has_collision(ZobristKey key) const {
	size_t index = key % sz;
	ht::Entry entry = entries[index];
	bool ret = entry.key != key && entry.key != 0;
	// if (ret)
	// 	LOG(logERROR) << entry.key << " != " << key;
	return ret;
}

// TODO for now the replacement strategy favors preserving PV
void ht::Table::put(ht::Entry entry) {
	size_t index = entry.key % sz;

	// assuming that the only case a key would be 0 is when the entry's uninitialized
	if (entries[index].key != 0) {
		// there is a collision!
		// for now we replace only if not pv-node
		if (entries[index].node_type == 1 && entry.node_type != 1) {
			return;
		}

		// depth-preferred
		if (entry.depth > entries[index].depth) {
			entries[index] = entry;
		}
	} else {
		entries[index] = entry;
	}
}

static ht::Table g_table(8192 * 8192);  // default value

ht::Table& ht::global_table() {
	return g_table;
}

void ht::set_global_table(size_t sz) {
	g_table = ht::Table(sz);
}
