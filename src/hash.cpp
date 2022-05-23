#include "hash.h"

#include <array>
#include "utils.h"

/// zobrist stuff
constexpr unsigned N_ZOBRIST_PIECES = 12;
ZobristKey table[64][N_ZOBRIST_PIECES];
ZobristKey black_to_move;


/// hashtable stuff
// std::array<hashtable::Entry> table;


void zobrist::initialize() {
	utils::PRNG prng(38520315250);  // NOTE: this seed was arbitrarily chosen
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < N_ZOBRIST_PIECES; j++) {
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

void hashtable::alloc(unsigned n) {

}
