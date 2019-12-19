
#include "position.h"
#include "movegen.h"
#include "utils.h"

#include <cassert>
#include <cctype>

#include <istream>

using std::string;
using std::vector;

namespace {}  // namespace

Position::Position()
    : side_to_move(WHITE),
      castling_rights(ALL_CASTLING_RIGHTS),
      piece_bitboards{},
      color_bitboards{},
      enpassant_mask{},
	  halfmove_clock{0},
	  fullmove_number{1}
	  {}

void Position::apply_move(const Move& move) {
    // TODO

}

Bitboard Position::get_attackers(Square target_sq, Color atk_color) const {
    Color own_color = utils::opposite_color(atk_color);
    Bitboard mask = 0ULL;

    // pawns
    mask |= bboard::pawn_attacks(target_sq, own_color) &
            get_bitboard(atk_color, PAWN);

    // knights
    mask |= bboard::knight_attacks(target_sq) & get_bitboard(atk_color, KNIGHT);

    // bishops/queens
    Bitboard queen_mask = get_bitboard(atk_color, QUEEN);
    Bitboard attacks = bboard::bishop_attacks(target_sq, get_all_bitboard());
    mask |= attacks & get_bitboard(atk_color, BISHOP);
    mask |= attacks & queen_mask;

    // rooks/queens
    attacks = bboard::rook_attacks(target_sq, get_all_bitboard());
    mask |= attacks & get_bitboard(atk_color, ROOK);
    mask |= attacks & queen_mask;

    // king
    mask |=
        bboard::king_attacks(target_sq) & this->get_bitboard(atk_color, KING);

    return mask;
}

// TODO add more members in Position to make this more optimized
bool Position::get_piece_at(Square sq, Color& c_out, PieceType& p_out) const {
    Bitboard mask = bboard::mask_square(sq);
    Color c;
    if (mask & get_color_bitboard(WHITE)) {
        c = WHITE;
    } else if (mask & get_color_bitboard(BLACK)) {
        c = BLACK;
    } else {
        return false;
    }

    PieceType p;
    if (mask & get_piece_bitboard(PAWN)) {
        p = PAWN;
    } else if (mask & get_piece_bitboard(KNIGHT)) {
        p = KNIGHT;
    } else if (mask & get_piece_bitboard(BISHOP)) {
        p = BISHOP;
    } else if (mask & get_piece_bitboard(ROOK)) {
        p = ROOK;
    } else if (mask & get_piece_bitboard(QUEEN)) {
        p = QUEEN;
    } else if (mask & get_piece_bitboard(KING)) {
        p = KING;
    } else {
        return false;
    }

    c_out = c;
    p_out = p;
    return true;
}

Bitboard Position::get_attack_mask(Color col) const {
    Bitboard mask = 0ULL;
    // remove king from occ so he doesn't block anything
    Bitboard occ =
        get_all_bitboard() & ~get_bitboard(utils::opposite_color(col), KING);

    // pawns
    Bitboard pawns = get_bitboard(col, PAWN);
    while (pawns != 0ULL) {
        Square sq = bboard::bitscan_fwd(pawns);
        mask |= bboard::pawn_attacks(sq, col);
        pawns &= ~bboard::mask_square(sq);
    }

    // knights
    Bitboard knights = get_bitboard(col, KNIGHT);
    while (knights != 0ULL) {
        Square sq = bboard::bitscan_fwd(knights);
        mask |= bboard::knight_attacks(sq);
        knights &= ~bboard::mask_square(sq);
    }

    // bishops
    Bitboard bishops = get_bitboard(col, BISHOP);
    while (bishops != 0ULL) {
        Square sq = bboard::bitscan_fwd(bishops);
        mask |= bboard::bishop_attacks(sq, occ);
        bishops &= ~bboard::mask_square(sq);
    }

    // rooks
    Bitboard rooks = get_bitboard(col, ROOK);
    while (rooks != 0ULL) {
        Square sq = bboard::bitscan_fwd(rooks);
        mask |= bboard::rook_attacks(sq, occ);
        rooks &= ~bboard::mask_square(sq);
    }

    // queens
    Bitboard queens = get_bitboard(col, QUEEN);
    while (queens != 0ULL) {
        Square sq = bboard::bitscan_fwd(queens);
        mask |= bboard::rook_attacks(sq, occ);
        mask |= bboard::bishop_attacks(sq, occ);
        queens &= ~bboard::mask_square(sq);
    }

    // king
    Square king_sq = bboard::bitscan_fwd(get_bitboard(col, KING));
    mask |= bboard::king_attacks(king_sq);

    return mask;
}

void Position::clear() {

}

void Position::place_piece(Color c, PieceType piece, Square sq) {
	Bitboard mask = bboard::mask_square(sq);
	piece_bitboards[(int)piece] |= mask;
    piece_bitboards[ANY_PIECE] |= mask;
	color_bitboards[(int)c] |= mask;
}

#include <iostream>
void test_get_attackers(Position& pos, Square sq, Color atk_color) {
    Bitboard attackers = pos.get_attackers(sq, atk_color);
    std::cout << "attackers mask:\n" << utils::repr(attackers) << std::endl;
}
