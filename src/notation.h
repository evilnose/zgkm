#pragma once
/* Parsing/generating strings as representation of positions/move history */
#include "types.h"
#include "position.h"

#include <string>
#include <vector>
#include <istream>

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

    inline std::string square_str(Square sq) {
        return std::to_string(utils::sq_rank(sq) + 1) + file_char(sq);
    }

    /*
    Return move string in Standard Algebraic Notation.
    legal_moves:    All legal moves
    move_idx:       Index of the move that should be printed
    */
    std::string pretty_move(Move mv, const std::vector<Move>& legal_moves,
			const Position& pos, bool checking, bool mating);

	// void load_fen(Position& pos, std::istream& ins);

    std::string to_fen(const Position& pos);

    // same as to_fen but with '/' replaced with '\n'
    // and all empty squares represented by '.'
    std::string to_aligned_fen(const Position& pos);
}
