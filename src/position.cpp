#include <cassert>

#include "movegen.h"
#include "position.h"
#include "utils.h"

using std::string;
using std::vector;

namespace {
}  // namespace

Position::Position(string ascii, Color side2move, const CastleState& cstate)
    : piece_bitboards{}, color_bitboards{}, side_to_move(side2move), castle_state(cstate) {
    assert(ascii.length() == 64);
    for (int i = 0; i < 64; i++) {
        char lower = std::tolower(ascii[i]);
        Color color = lower == ascii[i] ? WHITE : BLACK;
        PieceType pt;

        switch (lower) {
            case 'p':
                pt = PAWN;
                break;
            case 'n':
                pt = KNIGHT;
                break;
            case 'b':
                pt = BISHOP;
                break;
            case 'r':
                pt = ROOK;
                break;
            case 'q':
                pt = QUEEN;
                break;
            case 'k':
                pt = KING;
                break;
            case '.':
                continue;
            default:
                printf("WARNING: unrecognized char in from_ascii() string");
                return;
        }
        Square rank = 7 - i / 8;
        Square file = i % 8;
        Bitboard mask = 1ULL << (rank * 8 + file);
        piece_bitboards[pt] |= mask;
        piece_bitboards[ANY_PIECE] |= mask;
        color_bitboards[color] |= mask;
    }
}

// TODO don't forget to zero-initialize array members in any new constructor

Bitboard Position::get_attackers(Square target_sq, Color atk_color) const {
    Color own_color = opposite_color(atk_color);
    Bitboard mask = 0ULL;

    // pawns
    mask |= bb::pawn_attacks(target_sq, own_color) &
           get_bitboard(atk_color, PAWN);

    // knights
    mask |= bb::knight_attacks(target_sq) & get_bitboard(atk_color, KNIGHT);

    // bishops/queens
    Bitboard queen_mask = get_bitboard(atk_color, QUEEN);
    Bitboard attacks = bb::bishop_attacks(target_sq, get_all_bitboard());
    mask |= attacks & get_bitboard(atk_color, BISHOP);
    mask |= attacks & queen_mask;

    // rooks/queens
    attacks = bb::rook_attacks(target_sq, get_all_bitboard());
    mask |= attacks & get_bitboard(atk_color, ROOK);
    mask |= attacks & queen_mask;

    // king
    mask |= bb::king_attacks(target_sq) & this->get_bitboard(atk_color, KING);

    return mask;
}

Bitboard Position::get_attack_mask(Color col) const {
    Bitboard mask = 0ULL;
    // pawns
    Bitboard pawns = get_bitboard(col, PAWN);
    while (pawns != 0ULL) {
        Square sq = bb::bitscan_fwd(pawns);
        mask |= bb::pawn_attacks(sq, col);
        pawns &= ~mask_square(sq);        
    }

    // knights
    Bitboard knights = get_bitboard(col, KNIGHT);
    while (knights != 0ULL) {
        Square sq = bb::bitscan_fwd(knights);
        mask |= bb::knight_attacks(sq);
        knights &= ~mask_square(sq);        
    }

    Bitboard occ = get_all_bitboard();

    // bishops
    Bitboard bishops = get_bitboard(col, BISHOP);
    while (bishops != 0ULL) {
        Square sq = bb::bitscan_fwd(bishops);
        mask |= bb::bishop_attacks(sq, occ);
        bishops &= ~mask_square(sq);        
    }

    // rooks
    Bitboard rooks = get_bitboard(col, ROOK);
    while (rooks != 0ULL) {
        Square sq = bb::bitscan_fwd(rooks);
        mask |= bb::rook_attacks(sq, occ);
        rooks &= ~mask_square(sq);        
    }

    // queens
    Bitboard queens = get_bitboard(col, QUEEN);
    while (queens != 0ULL) {
        Square sq = bb::bitscan_fwd(queens);
        mask |= bb::rook_attacks(sq, occ);
        mask |= bb::bishop_attacks(sq, occ);
        queens &= ~mask_square(sq);
    }

    // king
    Square king_sq = bb::bitscan_fwd(get_bitboard(col, KING));
    mask |= bb::king_attacks(king_sq);

    return mask;
}

#include <iostream>
void test_get_attackers(Position& pos, Square sq, Color atk_color) {
    Bitboard attackers = pos.get_attackers(sq, atk_color);
    std::cout << "attackers mask:\n" << repr(attackers) << std::endl;
}
