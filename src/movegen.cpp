#include "movegen.h"
#include "bitboard.h"
#include "utils.h"

#include <iostream>

using std::vector;

namespace {
Bitboard absolute_pins(const Position& pos, Color pinned_color,
                       Bitboard& pinner_out) {
    Color opp_color = opposite_color(pinned_color);
    // Color opp_color = (Color) ((int) N_COLORS - 1 - (int) pinned_color);
    Bitboard pinned = 0ULL;
    Bitboard full_occ = pos.get_all_bitboard();
    Bitboard own_occ = pos.get_color_bitboard(pinned_color);
    Square king_sq = bb::bitscan_fwd(pos.get_bitboard(pinned_color, KING));

    Bitboard queen_occ = pos.get_bitboard(opp_color, QUEEN);
    Bitboard pinner = bb::rook_xray_attacks(king_sq, full_occ, own_occ) &
                      (pos.get_bitboard(opp_color, ROOK) | queen_occ);
    Bitboard pinner_acc = pinner;
    while (pinner) {
        Square sq = bb::bitscan_fwd(pinner);
        pinned |= bb::blocker(sq, king_sq, full_occ) & own_occ;
        pinner &= pinner - 1;
    }
    pinner = bb::bishop_xray_attacks(king_sq, full_occ, own_occ) &
             (pos.get_bitboard(opp_color, BISHOP) | queen_occ);
    pinner_acc |= pinner;

    while (pinner) {
        Square sq = bb::bitscan_fwd(pinner);
        pinned |= bb::blocker(sq, king_sq, full_occ) & own_occ;
        pinner &= pinner - 1;
    }

    pinner_out = pinner_acc;
    return pinned;
}

void add_moves(vector<Move>& moves, Square src, Bitboard tgts) {
    while (tgts != 0ULL) {
        Square sq = bb::bitscan_fwd(tgts);
        moves.push_back(create_move(sq, src, NORMAL_MOVE, KING));
        tgts &= ~mask_square(sq);
    }
}

// returns whether (squares are on same rank OR squares are on same file)
inline bool same_line(Square sq1, Square sq2) {
    return (sq_rank(sq1) == sq_rank(sq2)) || (sq_file(sq1) == sq_file(sq2));
}
}  // namespace

void test_absolute_pins(Position& position) {
    Bitboard pinner;
    Bitboard pinned = absolute_pins(position, BLACK, pinner);
    std::string pinned_repr = repr(pinned);
    std::string pinner_repr = repr(pinner);
    std::cout << "Pinned:\n"
              << pinned_repr << std::endl;
    std::cout << "Pinner:\n"
              << pinner_repr << std::endl;
}

/*
Main movegen function pseudocode

function main_movegen:
    checks = get_attacks_at(king_square)
    pinned, pinner = get_my_pins(...)
    move_list = []

    if checks == 0:

    for each my_knight:
        if pinned & my_knight.mask != 0:
            continue
        else:
            knight_mask = knight_mask_db[my_knight.square]
            knight_mask &= ~my_occupancy    # can't jump on own pieces
            extract_move_mask(knight_mask, my_knight.square, KNIGHT, move_list)

    for each my_bishop:
        bishop_mask = bishop_mask(my_bishop) & ~my_occupancy
        if pinned & my_bishop.mask != 0:
            if my_bishop and my_king on same diagonal (not rank/file):
                # restrict bishop movement to the pinned diagonal
                bishop_mask &= get_diagonal(my_king.square, my_bishop.square)
            else:
                # bishop can't move
                continue
        
        extract_move_mask(bishop_mask, ...)
        

    for each my_rook:
        rook_mask = ...
        if pinned & my_rook.mask != 0:
            if my_rook and my_king on same rank/file:
                rook_mask &= get_rook_rank_mask(...)
            else:
                # rook can't move
                continue

        extract_move_mask(rook_mask, ...)
            

    for each my_queen:
        queen_mask = ...
        if pinned & my_queen.mask != 0:
            if my_queen and my_king on same diagonal:
                queen_mask &= get_rook_rank_mask(...)
            else: # must be on same diagonal
                queen_mask &= get_diagonal(...)

        extract_move_mask(queen_mask, ...)

    # TODO pawns

    else if checks == 1:

    else if checks == 2:

endfunc

function extract_move_mask(move_mask, origin_square, piece_type, move_list):
    # extracts bits from move_mask as target squares for the piece at origin
    # square, creating a Move object and appending it to move_list.
    ...
endfunc

*/
vector<Move> gen_legal_moves(const Position& pos) {
    // TODO pawns
    // TODO castling
    Color atk_c = pos.get_side_to_move();
    Color def_c = opposite_color(atk_c);
    Bitboard atk_occ = pos.get_color_bitboard(atk_c);
    Bitboard def_occ = pos.get_color_bitboard(def_c);
    Bitboard all_occ = atk_occ | def_occ;

    Square king_sq = bb::bitscan_fwd(pos.get_bitboard(atk_c, KING));
    Bitboard checks = pos.get_attackers(king_sq, def_c);
    int n_checks = popcount(checks);

    Bitboard def_attacks = pos.get_attack_mask(def_c);
    Bitboard king_attacks = bb::king_attacks(king_sq) & ~def_attacks & ~atk_occ;

    vector<Move> moves;
    add_moves(moves, king_sq, king_attacks);  // add king moves regardless

    if (n_checks == 0) {
        Bitboard pinner = 0ULL;
        Bitboard pinned = absolute_pins(pos, atk_c, pinner);

        // pawns

        Bitboard free_knights = pos.get_bitboard(atk_c, KNIGHT) & ~pinned;
        // knights
        while (free_knights != 0ULL) {
            Square sq = bb::bitscan_fwd(free_knights);
            add_moves(moves, sq, bb::knight_attacks(sq) & ~atk_occ);
            free_knights &= ~mask_square(sq);
        }

        // bishops
        Bitboard bishops = pos.get_bitboard(atk_c, BISHOP);
        Bitboard free_bishops = bishops & ~pinned;
        Bitboard pinned_bishops = bishops & pinned;
        while (free_bishops != 0ULL) {
            Square sq = bb::bitscan_fwd(free_bishops);
            add_moves(moves, sq, bb::bishop_attacks(sq, all_occ));
            free_bishops &= ~mask_square(sq);
        }

        Bitboard kb_atk = bb::bishop_attacks(king_sq, 0ULL);
        while (pinned_bishops != 0ULL) {
            Square sq = bb::bitscan_fwd(pinned_bishops);
            if (!same_line(sq, king_sq)) {
                // on same diagonal. Bishop can move along common diagonal
                // between self and king
                add_moves(moves, sq, bb::bishop_attacks(sq, all_occ) & kb_atk);
            }
            pinned_bishops &= ~mask_square(sq);
        }

        // rooks
        Bitboard rooks = pos.get_bitboard(atk_c, ROOK);
        Bitboard free_rooks = rooks & ~pinned;
        Bitboard pinned_rooks = rooks & pinned;
        while (free_rooks != 0ULL) {
            Square sq = bb::bitscan_fwd(free_rooks);
            add_moves(moves, sq, bb::rook_attacks(sq, all_occ));
            free_rooks &= ~mask_square(sq);
        }

        Bitboard kr_atk = bb::rook_attacks(king_sq, 0ULL);
        while (pinned_rooks != 0ULL) {
            Square sq = bb::bitscan_fwd(pinned_rooks);
            if (same_line(sq, king_sq)) {
                // on same line. Rooks can move along common line
                // between self and king
                add_moves(moves, sq, bb::rook_attacks(sq, all_occ) & kr_atk);
            }
            pinned_rooks &= ~mask_square(sq);
        }

        Bitboard queens = pos.get_bitboard(atk_c, QUEEN);
        Bitboard free_queens = queens & ~pinned;
        Bitboard pinned_queens = queens & pinned;
        while (free_queens != 0ULL) {
            Square sq = bb::bitscan_fwd(free_queens);
            add_moves(moves, sq, bb::queen_attacks(sq, all_occ));
            free_queens &= ~mask_square(sq);
        }

        while (pinned_queens != 0ULL) {
            Square sq = bb::bitscan_fwd(pinned_queens);
            if (same_line(sq, king_sq)) {
                // pinned like a rook
                add_moves(moves, sq, bb::rook_attacks(sq, all_occ) & kr_atk);
            } else {
                // pinned like a bishop
                add_moves(moves, sq, bb::bishop_attacks(sq, all_occ) & kb_atk);
            }
            pinned_queens &= ~mask_square(sq);
        }
    } else if (n_checks == 2) {
        // only king moves are legal
        // TODO remove this case if redundant
    }
	return moves;
    // TODO add n_checks == 1
}

// vector<Move> gen_king_moves(const Position& position) {
//     /*
//     Notes

//     steps:
//     1. generate move masks for all opposing pieces. For sliding pieces,
//     remember to exclude this king as a blocker.
//     2. check all available squares for king with masks

//     */

//     //TODO
// }

// vector<Move> gen_bishop_moves(const Position& position) {
//     const Bitboard key = position.get_piece_bitboard(ANY_PIECE);
//     // TODO
// }

// vector<Move> gen_rook_moves(const Position& position) {
//     // TODO
// }
