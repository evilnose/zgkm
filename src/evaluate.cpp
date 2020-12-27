#include <stdlib.h>  // rand
#include <cassert>

#include "evaluate.h"
#include "utils.h"

static Score MATERIAL_SCORES[N_REAL_PIECE_TYPES] {1., 3., 3.2, 5., 9., SCORE_POS_INFTY};

Score material_only_eval(const Position& pos) {
    Score ret = 0.;
    for (PieceType piece = PAWN; piece != KING; piece = (PieceType) (piece + 1)) {
        Bitboard pb = pos.get_piece_bitboard(piece);
        int wcount = utils::popcount(pb & pos.get_color_bitboard(WHITE));
        int bcount = utils::popcount(pb & pos.get_color_bitboard(BLACK));
        ret += MATERIAL_SCORES[piece] * (wcount - bcount);
    }
    // TODO need to change sign based on who's to move
    assert(false);
    return ret;
}
