#include <stdlib.h>  // rand
#include <cassert>

#include "evaluate.h"
#include "movegen.h"
#include "utils.h"

static Score MATERIAL_SCORES[N_REAL_PIECE_TYPES] {1., 3., 3.2, 5., 9., SCORE_POS_INFTY};

Score mobility(Color c, const Position& pos) {
    Color def_c = utils::opposite_color(c);
    Bitboard def_occ = pos.get_color_bitboard(def_c);
    Bitboard def_half = c == WHITE ? WHITE_HALF : BLACK_HALF;
    Score mob = 0;

    Bitboard pinner = 0ULL;
    Bitboard pinned = absolute_pins(pos, c, pinner);

    Bitboard free_knights = pos.get_bitboard(c, KNIGHT) & ~pinned;
    while (free_knights != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(free_knights);
        Bitboard attacks = bboard::knight_attacks(sq);
        Bitboard attacks_in_their_side = attacks & def_half;
        Bitboard capture_attacks = attacks & def_occ;

        mob += utils::popcount(attacks) + 0.5f * utils::popcount(capture_attacks) + 0.5f * utils::popcount(attacks_in_their_side);
    }

    // NOTE not counting pinned pieces
    Bitboard free_bishops = pos.get_bitboard(c, BISHOP) & ~pinned;
    while (free_bishops != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(free_bishops);
        Bitboard attacks = bboard::bishop_attacks(sq, def_occ);
        Bitboard attacks_in_their_side = attacks & def_half;
        Bitboard capture_attacks = attacks & def_occ;

        mob += utils::popcount(attacks) + 0.5f * utils::popcount(capture_attacks) + 0.5f * utils::popcount(attacks_in_their_side);
    }

    Bitboard free_rooks = pos.get_bitboard(c, ROOK) & ~pinned;
    while (free_rooks != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(free_rooks);
        Bitboard attacks = bboard::rook_attacks(sq, def_occ);
        Bitboard attacks_in_their_side = attacks & def_half;

        mob += 0.5f * utils::popcount(attacks) + utils::popcount(attacks_in_their_side);
    }

    Bitboard free_queens = pos.get_bitboard(c, ROOK) & ~pinned;
    while (free_rooks != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(free_rooks);
        Bitboard attacks = bboard::queen_attacks(sq, def_occ);
        // Bitboard attacks_in_their_side = attacks & def_half;

        mob += utils::popcount(attacks);
    }

    return mob;
}

Score evaluate(const Position& pos) {
    Score ret = 0.;
    Score color_multiplier = utils::color_multiplier(pos.get_side_to_move());

    // tunable parameters
    Score mobility_weight = 0.1f;
    Score checked_penalty = 1.f;
    
    // Add material
    for (PieceType piece = PAWN; piece != KING; piece = (PieceType) (piece + 1)) {
        Bitboard pb = pos.get_piece_bitboard(piece);
        int wcount = utils::popcount(pb & pos.get_color_bitboard(WHITE));
        int bcount = utils::popcount(pb & pos.get_color_bitboard(BLACK));
        ret += MATERIAL_SCORES[piece] * (wcount - bcount);
    }

#ifndef MAT_ONLY
    // Add mobility
    ret += mobility_weight * (mobility(WHITE, pos) - mobility(BLACK, pos));
#endif

    Score value_score = ret * color_multiplier;

#ifndef MAT_ONLY
    // Add small penalty if in check
    if (pos.is_checking()) {
        value_score -= checked_penalty;
    }
#endif

    return value_score;
}
