#include <cassert>
#include <queue>

#include "movegen.h"
#include "bitboard.h"
#include "utils.h"
#include "notation.h"
#include "logger.h"

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
        Move tmp = create_normal_move(src, sq);
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
            Move tmp = create_promotion_move(src, sq, piece);
            moves.push_back(tmp);
        }
    }
}

inline void add_pawn_moves(vector<Move>& moves, Square src, Bitboard tgts) {
    // TODO no-branch if?
    if (utils::sq_rank(bboard::bitscan_fwd(tgts)) % 7 == 0) {
        add_promotion_moves(moves, src, tgts);
    } else {
        add_moves(moves, src, tgts);
    }
}

inline void add_enpassant(vector<Move>& moves, Square src, Square tgt) {
    moves.push_back(create_enpassant(src, tgt));
}

inline void add_castling_move(vector<Move>& moves, Color c, BoardSide side) {
    moves.push_back(create_castling_move(c, side));
}

/*
inline void add_specific_promotion_move(vector<Move>& moves, Square src, Square
tgt, PieceType target_piece) { moves.push_back(create_promotion_move(src, tgt,
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
    LOG(logINFO) << "Pinned:\n" << pinned_repr;
    LOG(logINFO) << "Pinner:\n" << pinner_repr;
}

bool gen_legal_moves(const Position& pos, vector<Move>& moves) {
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

            add_pawn_moves(
                moves, sq,
                pawn_mask | (bboard::pawn_attacks(sq, atk_c) & def_occ));

            if (bboard::pawn_attacks(sq, atk_c) & enpassant) {
                Bitboard xray = (all_occ & ~bboard::mask_square(sq) &
                    ~bboard::mask_square(utils::enpassant_actual(enpassant_sq, def_c))) | enpassant;
                if (!(bboard::rook_attacks(king_sq, xray) & (pos.get_bitboard(def_c, ROOK) | pos.get_bitboard(def_c, QUEEN))))
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
                    // pawn captures
                    if (d_rank * utils::pawn_direction(atk_c) > 0) {
                        // one potential pawn capture
                        Square t_sq = sq;
                        bool good = utils::move_square(t_sq, utils::sgn(d_rank), utils::sgn(d_file));
                        assert(good);
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

            add_pawn_moves(
                moves, sq,
                (pawn_mask & block_mask) | (pawn_attacks & capture_mask));

            // if can EP capture AND (EP pawn is the checker OR
            // enpassant mask blocks the attacker)
            if ((pawn_attacks & enpassant) &&
                (checkers & pos.get_bitboard(def_c, PAWN)) |
                    (enpassant & block_mask)) {
                add_enpassant(moves, sq, enpassant_sq);
            }
        }

        // pinned pieces cannot possibly help evade check

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
    return n_checks != 0;
}

int perft(const Position& pos, int depth) {
    Position position = pos; // make copy here
    assert(pos.position_good());
    int count = 0;
    std::vector<Move> legal_moves;
    gen_legal_moves(position, legal_moves);
    if (depth == 1) {
        return legal_moves.size();
    }
    for (Move move : legal_moves) {
        position.make_move(move);
        assert(position.position_good());
        count += perft(position, depth - 1);
        position.unmake_move(move);
        assert(position.position_good());
    }
    return count;
}

void divide(Position& position, int depth) {
    long total = 0;
    vector<Move> legal_moves;
    gen_legal_moves(position, legal_moves);
    if (depth == 1) {
        total = legal_moves.size();
        for (Move move : legal_moves) {
            LOG(logINFO) << notation::dump_uci_move(move) << ": 1";
        }
    } else {
        for (Move move : legal_moves) {
            position.make_move(move);
            long count = perft(position, depth - 1);
            position.unmake_move(move);
            total += count;
            LOG(logINFO) << notation::dump_uci_move(move) << ": " << count;
        }
    }
    LOG(logINFO) << "Moves: " << legal_moves.size();
    LOG(logINFO) << "Nodes: " << total;
}
