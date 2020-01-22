#include "movegen.h"
#include "bitboard.h"
#include "utils.h"

#include <cassert>
#include <queue>

// TODO remove; for debugging
#include <iostream>

using std::vector;

namespace {
Bitboard absolute_pins(const Position& pos, Color pinned_color,
                       Bitboard& pinner_out) {
    Color opp_color = utils::opposite_color(pinned_color);
    // Color opp_color = (Color) ((int) N_COLORS - 1 - (int) pinned_color);
    Bitboard pinned = 0ULL;
    Bitboard full_occ = pos.get_all_bitboard();
    Bitboard own_occ = pos.get_color_bitboard(pinned_color);
    Square king_sq = bboard::bitscan_fwd(pos.get_bitboard(pinned_color, KING));

    Bitboard queen_occ = pos.get_bitboard(opp_color, QUEEN);
    Bitboard pinner = bboard::rook_xray_attacks(king_sq, full_occ, own_occ) &
                      (pos.get_bitboard(opp_color, ROOK) | queen_occ);
    Bitboard pinner_acc = pinner;
    while (pinner) {
        Square sq = bboard::bitscan_fwd_remove(pinner);
        pinned |= bboard::blocker(sq, king_sq, full_occ) & own_occ;
    }
    pinner = bboard::bishop_xray_attacks(king_sq, full_occ, own_occ) &
             (pos.get_bitboard(opp_color, BISHOP) | queen_occ);
    pinner_acc |= pinner;

    while (pinner) {
        Square sq = bboard::bitscan_fwd_remove(pinner);
        pinned |= bboard::blocker(sq, king_sq, full_occ) & own_occ;
    }

    pinner_out = pinner_acc;
    return pinned;
}

inline void add_moves(vector<Move>& moves, Square src, Bitboard tgts) {
    while (tgts != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(tgts);
        Move tmp = create_normal_move(sq, src);
        moves.push_back(tmp);
    }
}

// add all possible promotion moves
inline void add_promotion_moves(vector<Move>& moves, Square src,
                                Bitboard tgts) {
    while (tgts != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(tgts);
        for (PieceType piece = KNIGHT; piece <= QUEEN;
             piece = (PieceType)(piece + 1)) {
            Move tmp = create_promotion_move(sq, src, piece);
            moves.push_back(tmp);
        }
    }
}

inline void add_pawn_moves(vector<Move>& moves, Square src, Bitboard tgts) {
    if (utils::sq_rank(bboard::bitscan_fwd(tgts)) % 7 == 0) {
        add_promotion_moves(moves, src, tgts);
    } else {
        add_moves(moves, src, tgts);
    }
}

inline void add_enpassant(vector<Move>& moves, Square src, Square tgt) {
    moves.push_back(create_enpassant(tgt, src));
}

inline void add_castling_move(vector<Move>& moves, Color c, BoardSide side) {
    moves.push_back(create_castling_move(c, side));
}

/*
inline void add_specific_promotion_move(vector<Move>& moves, Square src, Square
tgt, PieceType target_piece) { moves.push_back(create_promotion_move(tgt, src,
target_piece));
}
*/

// returns whether (squares are on same rank OR squares are on same file)
inline bool same_line(Square sq1, Square sq2) {
    return (utils::sq_rank(sq1) == utils::sq_rank(sq2)) ||
           (utils::sq_file(sq1) == utils::sq_file(sq2));
}
}  // namespace

void test_absolute_pins(Position& position) {
    Bitboard pinner;
    Bitboard pinned = absolute_pins(position, BLACK, pinner);
    std::string pinned_repr = utils::repr(pinned);
    std::string pinner_repr = utils::repr(pinner);
    std::cout << "Pinned:\n" << pinned_repr << std::endl;
    std::cout << "Pinner:\n" << pinner_repr << std::endl;
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
    // TODO weird enpassant pin
    Color atk_c = pos.get_side_to_move();
    Color def_c = utils::opposite_color(atk_c);
    Bitboard atk_occ = pos.get_color_bitboard(atk_c);
    Bitboard def_occ = pos.get_color_bitboard(def_c);
    Bitboard all_occ = atk_occ | def_occ;

    Square king_sq = bboard::bitscan_fwd(pos.get_bitboard(atk_c, KING));
    Bitboard checkers = pos.get_attackers(king_sq, def_c);
    int n_checks = utils::popcount(checkers);

    Bitboard def_attacks = pos.get_attack_mask(def_c);
    Bitboard king_attacks =
        bboard::king_attacks(king_sq) & ~def_attacks & ~atk_occ;

    Bitboard enpassant = pos.get_enpassant();
    Square enpassant_sq = bboard::bitscan_fwd(enpassant);

    vector<Move> moves;
    add_moves(moves, king_sq, king_attacks);  // add king moves regardless

    if (n_checks == 0) {
        Bitboard pinner = 0ULL;
        Bitboard pinned = absolute_pins(pos, atk_c, pinner);

        // pawns
        Bitboard pawns = pos.get_bitboard(atk_c, PAWN);
        Bitboard free_pawns = pawns & ~pinned;
        Bitboard pinned_pawns = pawns & pinned;
        Bitboard special_rank = atk_c == WHITE ? RANK_C : RANK_F;
        while (free_pawns != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_pawns);

            Bitboard pawn_mask = bboard::pawn_pushes(sq, atk_c) & ~all_occ;

            // add moves for starting pushes (2 squares). Can only push 2 ahead
            // if pawn can push to rank C/F first (determined by
            // pawn_mask & special_rank).
            // left shift by 8 if attacker is white, otherwise right shift by 8
            pawn_mask |= (pawn_mask & special_rank) << ((atk_c == WHITE) * 8);
            pawn_mask |= (pawn_mask & special_rank) >> ((atk_c == BLACK) * 8);

            // Note need to (& ~all_occ) again since 2 square pushes
            // can be blocked by pieces on the D/E rank.
            pawn_mask &= ~all_occ;

            add_pawn_moves(moves, sq,
                      pawn_mask | (bboard::pawn_attacks(sq, atk_c) & def_occ));

            if (bboard::pawn_attacks(sq, atk_c) & enpassant) {
                add_enpassant(moves, sq, enpassant_sq);
            }
        }

        while (pinned_pawns != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(pinned_pawns);
            int d_rank = utils::sq_rank(sq) - utils::sq_rank(king_sq);
            if (d_rank != 0) {
                int d_file = utils::sq_file(sq) - utils::sq_file(king_sq);
                if (d_file == 0) {
                    // pawn pushes
                    Bitboard pawn_mask =
                        bboard::pawn_pushes(sq, atk_c) & ~all_occ;

                    pawn_mask |=
                        ((pawn_mask & special_rank) << ((atk_c == WHITE) * 8));
                    pawn_mask |=
                        ((pawn_mask & special_rank) >> ((atk_c == BLACK) * 8));

                    add_pawn_moves(moves, sq, pawn_mask & ~all_occ);
                } else {
                    if (d_rank > 0) {
                        // one potential pawn capture
                        Square t_sq = sq;
                        utils::move_square(t_sq, d_rank, d_file);
                        Bitboard tar_mask = bboard::mask_square(t_sq);
                        if (tar_mask & enpassant) {
                            add_enpassant(moves, sq, enpassant_sq);
                        } else {
                            add_pawn_moves(moves, sq, tar_mask & def_occ);
                        }
                    }
                }
            }  // else if on same rank, pawn can't move
        }

        // knights
        Bitboard free_knights = pos.get_bitboard(atk_c, KNIGHT) & ~pinned;
        while (free_knights != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_knights);
            add_moves(moves, sq, bboard::knight_attacks(sq) & ~atk_occ);
        }

        // bishops
        Bitboard bishops = pos.get_bitboard(atk_c, BISHOP);
        Bitboard free_bishops = bishops & ~pinned;
        Bitboard pinned_bishops = bishops & pinned;
        while (free_bishops != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_bishops);
            add_moves(moves, sq,
                      bboard::bishop_attacks(sq, all_occ) & ~atk_occ);
        }

        Bitboard kb_atk = bboard::bishop_attacks(king_sq, 0ULL);
        while (pinned_bishops != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(pinned_bishops);
            if (!same_line(sq, king_sq)) {
                // on same diagonal. Bishop can move along common diagonal
                // between self and king
                add_moves(moves, sq,
                          bboard::bishop_attacks(sq, all_occ) & kb_atk);
            }
        }

        // rooks
        Bitboard rooks = pos.get_bitboard(atk_c, ROOK);
        Bitboard free_rooks = rooks & ~pinned;
        Bitboard pinned_rooks = rooks & pinned;
        while (free_rooks != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_rooks);
            add_moves(moves, sq, bboard::rook_attacks(sq, all_occ) & ~atk_occ);
        }

        Bitboard kr_atk = bboard::rook_attacks(king_sq, 0ULL);
        while (pinned_rooks != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(pinned_rooks);
            if (same_line(sq, king_sq)) {
                // on same line. Rooks can move along common line
                // between self and king
                add_moves(moves, sq,
                          bboard::rook_attacks(sq, all_occ) & kr_atk);
            }
        }

        Bitboard queens = pos.get_bitboard(atk_c, QUEEN);
        Bitboard free_queens = queens & ~pinned;
        Bitboard pinned_queens = queens & pinned;
        while (free_queens != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_queens);
            add_moves(moves, sq, bboard::queen_attacks(sq, all_occ) & ~atk_occ);
        }

        while (pinned_queens != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(pinned_queens);
            if (same_line(sq, king_sq)) {
                // pinned like a rook
                add_moves(moves, sq,
                          bboard::rook_attacks(sq, all_occ) & kr_atk);
            } else {
                // pinned like a bishop
                add_moves(moves, sq,
                          bboard::bishop_attacks(sq, all_occ) & kb_atk);
            }
        }

        // printf("CASTLING RIGHTS: %c\n", utils::to_castling_rights(atk_c,
        // KINGSIDE)); castling
        if (pos.has_castling_rights(
                utils::to_castling_rights(atk_c, KINGSIDE)) &&
            !(bboard::castle_king_occ(atk_c, KINGSIDE) & def_attacks) &&
            !(bboard::castle_between_occ(atk_c, KINGSIDE) & all_occ)) {
            add_castling_move(moves, atk_c, KINGSIDE);
        }

        if (pos.has_castling_rights(
                utils::to_castling_rights(atk_c, QUEENSIDE)) &&
            !(bboard::castle_king_occ(atk_c, QUEENSIDE) & def_attacks) &&
            !(bboard::castle_between_occ(atk_c, QUEENSIDE) & all_occ)) {
            add_castling_move(moves, atk_c, QUEENSIDE);
        }
    } else if (n_checks == 1) {
        Color dummy_c;
        PieceType ptype;
        Square checker_sq = bboard::bitscan_fwd(checkers);
        pos.get_piece(checker_sq, dummy_c, ptype);
        assert(dummy_c == def_c);
        // squares that can be captured for check evasion
        Bitboard capture_mask = checkers;
        // squares that can be blocked for check evasion
        // Note that this distinction is required b/c pawn has
        // different capture and push (block) squares
        Bitboard block_mask = 0ULL;
        if (utils::is_slider(ptype)) {
            switch (ptype) {
                case BISHOP:
                    block_mask = bboard::bishop_attacks(king_sq, all_occ) &
                                 bboard::bishop_attacks(checker_sq, all_occ);
                    break;
                case ROOK:
                    block_mask = bboard::rook_attacks(king_sq, all_occ) &
                                 bboard::rook_attacks(checker_sq, all_occ);
                    break;
                case QUEEN:  // use no-branch-if?
                    if (same_line(king_sq, checker_sq)) {
                        block_mask = bboard::rook_attacks(king_sq, all_occ) &
                                     bboard::rook_attacks(checker_sq, all_occ);
                    } else {
                        block_mask =
                            bboard::bishop_attacks(king_sq, all_occ) &
                            bboard::bishop_attacks(checker_sq, all_occ);
                    }
                    break;
                default:
                    assert(false);
                    break;
            }
        }
        Bitboard check_mask = capture_mask | block_mask;

        Bitboard pinner = 0ULL;
        Bitboard pinned = absolute_pins(pos, atk_c, pinner);

        // pawns
        Bitboard free_pawns = pos.get_bitboard(atk_c, PAWN) & ~pinned;
        Bitboard special_rank = atk_c == WHITE ? RANK_C : RANK_F;
        while (free_pawns != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_pawns);

            Bitboard pawn_mask = bboard::pawn_pushes(sq, atk_c) & ~all_occ;

            // add moves for starting pushes (2 squares). Can only push 2 ahead
            // if pawn can push to rank C/F first (determined by
            // pawn_mask & special_rank).
            // left shift by 8 if attacker is white, otherwise right shift by 8
            pawn_mask |= (pawn_mask & special_rank) << ((atk_c == WHITE) * 8);
            pawn_mask |= (pawn_mask & special_rank) >> ((atk_c == BLACK) * 8);

            // Note need to (& ~all_occ) again since 2 square pushes
            // can be blocked by pieces on the D/E rank.
            pawn_mask &= ~all_occ;

            // add capture moves
            Bitboard pawn_attacks = bboard::pawn_attacks(sq, atk_c);

            add_pawn_moves(moves, sq,
                      (pawn_mask & block_mask) | (pawn_attacks & capture_mask));

            // if can EP capture AND (EP pawn is the checker OR
            // enpassant mask blocks the attacker)
            if ((pawn_attacks & enpassant) &&
                (checkers & pos.get_bitboard(def_c, PAWN)) |
                    (enpassant & block_mask)) {
                add_enpassant(moves, sq, enpassant_sq);
            }
        }

        // pinned pawns cannot possibly help evade check

        // knights
        Bitboard free_knights = pos.get_bitboard(atk_c, KNIGHT) & ~pinned;
        while (free_knights != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_knights);
            add_moves(moves, sq, bboard::knight_attacks(sq) & check_mask);
        }

        // bishops
        Bitboard free_bishops = pos.get_bitboard(atk_c, BISHOP) & ~pinned;
        while (free_bishops != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_bishops);
            add_moves(moves, sq,
                      bboard::bishop_attacks(sq, all_occ) & check_mask);
        }

        // rooks
        Bitboard free_rooks = pos.get_bitboard(atk_c, ROOK) & ~pinned;
        while (free_rooks != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_rooks);
            add_moves(moves, sq,
                      bboard::rook_attacks(sq, all_occ) & check_mask);
        }

        Bitboard free_queens = pos.get_bitboard(atk_c, QUEEN) & ~pinned;
        while (free_queens != 0ULL) {
            Square sq = bboard::bitscan_fwd_remove(free_queens);
            add_moves(moves, sq,
                      bboard::queen_attacks(sq, all_occ) & check_mask);
        }
    }  // else only king moves are legal, and nothing more needs to be done
    return moves;
}

#include "notation.h"  // TODO remove this
// TODO make this recursive and add undo_move
// but before you do THAT, first make this work up to whatever
// depth this unoptimized junk can handle in a reasonable amount of time
// TODO also, output captures, ep captures, castles, promotions, checks, etc.
int perft(Position& position, int depth) {
    std::vector<long> node_count;
    std::queue<Position> last_q;
    std::queue<Position> cur_q;
    last_q.push(position);
    for (int d = 1; d <= depth; d++) {
        long count = 0;
        long ncastles = 0;
        long nenpassant = 0;
        long nchecks = 0;
        long ncaptures = 0;
        long npromotions = 0;
        std::cout << "------DEPTH " << d << "------"
                  << "\n\n";
        while (!last_q.empty()) {
            Position pos = last_q.front();
            last_q.pop();

            std::vector<Move> legal_moves = gen_legal_moves(pos);
            for (Move move : gen_legal_moves(pos)) {
                Position next_pos = pos;
                next_pos.make_move(move);
                // std::cout << "\n"
                //           << notation::pretty_move(move, legal_moves, pos,
                //                                    pos.is_checking(), false)
                //           << '\n'
                //           << notation::to_aligned_fen(next_pos) << "\n\n";
                cur_q.push(next_pos);
                count++;
                if (next_pos.is_checking()) {
                    // std::cout << notation::to_aligned_fen(next_pos) <<
                    // "\n\n";
                    nchecks++;
                }
                Color defc = utils::opposite_color(pos.get_side_to_move());
                if (pos.get_color_bitboard(defc) !=
                    next_pos.get_color_bitboard(defc)) {
                    ncaptures++;
                }
                switch (get_move_type(move)) {
                    case CASTLING_MOVE:
                        ncastles++;
                        break;
                    case ENPASSANT:
                        nenpassant++;
                        break;
                    case PROMOTION:
                        npromotions++;
                        break;
                    default:
                        break;
                }
            }
        }
        std::cout << "castles: " << ncastles << std::endl;
        std::cout << "enpassants: " << nenpassant << std::endl;
        std::cout << "checks: " << nchecks << std::endl;
        std::cout << "captures: " << ncaptures << std::endl;
        std::cout << "promotions: " << npromotions << std::endl;
        // clear cur_q and assign cur_q to last_q
        std::queue<Position> empty;
        empty.swap(cur_q);
        last_q.swap(empty);
        node_count.push_back(count);
    }
    return node_count;
}
