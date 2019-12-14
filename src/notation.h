#pragma once
/* Parsing/generating strings as representation of positions/move history */

#include <string>
#include <vector>
#include "types.h"
#include "position.h"

using std::string;
using std::vector;

/*
 * Custom verbose FEN format for debugging (.vfen)
 *
 * The same as FEN, except in the board representation part, empty
 * squares are not "merged" and represented by the number of it. Instead
 * empty squares are represented by ".". Also "/" are replaced by newline
 * characters. The rest that follows after the board representation 
 * starts on a new line.
 */

namespace notation {
    inline char file_char(int f) {
        return 'a' + f;
    }

    /*
    Return move string in Standard Algebraic Notation.
    legal_moves:    All legal moves
    move_idx:       Index of the move that should be printed
    */
    string pretty_move(Move mv, const std::vector<Move>& legal_moves,
			const Position& pos, bool checking, bool mating);

	/*
	 * load verbose FEN format string str into Position pos
	 * NOTE: does not check validity
	 */
	bool load_vfen(Position& pos, const char* str);
}
