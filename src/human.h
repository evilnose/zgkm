#pragma once
/* Converting things to human-readable format/notation */

#include <string>
#include <vector>
#include "types.h"
#include "position.h"

using std::string;
using std::vector;

namespace human {
    inline char file_char(int f) {
        return 'a' + f;
    }

    /*
    Return move string in Standard Algebraic Notation.
    legal_moves:    All legal moves
    move_idx:       Index of the move that should be printed
    */
    string pretty_move(Move mv, const std::vector<Move>& legal_moves, const Position& pos, bool checking, bool mating);
}
