#include "movegen.h"
#include "bitboard.h"

using std::vector;

vector<Move> gen_king_moves(const Position& position) {
    /*
    Notes

    steps:
    1. generate move masks for all opposing pieces. For sliding pieces,
    remember to exclude this king as a blocker.
    2. check all available squares for king with masks

    */

   //TODO
}

vector<Move> gen_bishop_moves(const Position& position) {
    const Bitboard key = position.get_piece_bitboard(ANY_PIECE);
    // TODO
}

vector<Move> gen_rook_moves(const Position& position) {
    // TODO
}
