#include <stdlib.h>  // rand
#include <cassert>

#include "evaluate.h"
#include "utils.h"

static Score MATERIAL_SCORES[N_REAL_PIECE_TYPES] {1., 3., 3.2, 5., 9., SCORE_POS_INFTY};

Score evaluate(const Position& pos) {
    Score ret = 0.;
    Score color_multiplier = utils::color_multiplier(pos.get_side_to_move());
    
    // Add material
    for (PieceType piece = PAWN; piece != KING; piece = (PieceType) (piece + 1)) {
        Bitboard pb = pos.get_piece_bitboard(piece);
        int wcount = utils::popcount(pb & pos.get_color_bitboard(WHITE));
        int bcount = utils::popcount(pb & pos.get_color_bitboard(BLACK));
        ret += MATERIAL_SCORES[piece] * (wcount - bcount);
    }

    return ret * color_multiplier;
}
