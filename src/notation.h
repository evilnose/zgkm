#pragma once
/* Parsing/generating strings as representation of positions/move history */
#include "position.h"
#include "types.h"

#include <istream>
#include <string>
#include <vector>

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
inline char file_char(int f) { return 'a' + f; }

inline std::string square_str(Square sq) {
    char* ret = new char[3];
    ret[0] = file_char(utils::sq_file(sq));
    ret[1] = '1' + utils::sq_rank(sq);
    ret[2] = 0;
    return ret;
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
}  // namespace notation
