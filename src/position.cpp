
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
      fullmove_number{1} {}

Position::Position(std::istream& fen_is)
    : piece_bitboards{}, color_bitboards{} {
    load_fen(fen_is);
}

void Position::load_fen(std::istream& fen_is) {
    for (int row = 0; row < 8; row++) {
        int col = 0;
        while (col < 8) {
            // multiplied by 9 to skip the separator
            char piece_char;
            fen_is >> piece_char;
            if (std::isdigit(piece_char)) {
                col += (piece_char - '0');
                assert(col <= 8);
            } else {
                Color c;
                if (std::isupper(piece_char)) {
                    c = WHITE;
                    piece_char = std::tolower(piece_char);
                } else {
                    c = BLACK;
                }
                Square sq = utils::make_square(7 - row, col);
                switch (piece_char) {
                    case 'p':
                        set_piece(sq, c, PAWN);
                        break;
                    case 'b':
                        set_piece(sq, c, BISHOP);
                        break;
                    case 'n':
                        set_piece(sq, c, KNIGHT);
                        break;
                    case 'r':
                        set_piece(sq, c, ROOK);
                        break;
                    case 'q':
                        set_piece(sq, c, QUEEN);
                        break;
                    case 'k':
                        set_piece(sq, c, KING);
                        break;
                    default:
                        if (piece_char != '.') {
                            printf("%c\n", piece_char);
                        }
                        assert(piece_char == '.');
                        break;
                }
                col++;
            }
        }
        // ignore row delimiter
        fen_is.ignore();
    }
    char side_to_move;
    std::string castling_rights;
    std::string enpassant_square;
    std::string halfmove_clock;
    std::string fullmove_number;
    fen_is >> side_to_move;
    fen_is >> castling_rights >> enpassant_square;
    fen_is >> halfmove_clock >> fullmove_number;
    assert(side_to_move == 'w' || side_to_move == 'b');

    set_side_to_move((Color)(side_to_move == 'b'));

    CastlingRights c_rights = NO_CASTLING_RIGHTS;
    if (castling_rights != "-") {
        assert(castling_rights.length() <= 4);
        for (auto it = castling_rights.begin(); it != castling_rights.end();
             it++) {
            switch (*it) {
                case 'K':
                    c_rights |= WHITE_OO;
                    break;
                case 'Q':
                    c_rights |= WHITE_OOO;
                    break;
                case 'k':
                    c_rights |= BLACK_OO;
                    break;
                case 'q':
                    c_rights |= BLACK_OOO;
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }
    set_castling_rights(c_rights);

    if (enpassant_square != "-") {
        assert(enpassant_square.length() == 2);
        set_enpassant(utils::make_square(enpassant_square[1] - '1',
                                         enpassant_square[0] - 'a'));
    } else {
        // unset en-passant
        set_enpassant(N_SQUARES);
    }

    set_halfmove_clock(std::stoi(halfmove_clock));
    set_fullmove_number(std::stoi(fullmove_number));
}

void Position::make_move(Move move) {
    MoveType type = get_move_type(move);
    enpassant_mask = 0ULL;
    PosState cur_state{NO_PIECE, castling_rights, enpassant_mask,
                       halfmove_clock};

    if (type == CASTLING_MOVE) {
        Color color = get_move_castle_color(move);
        fullmove_number += (int)color;  // add if white
        BoardSide side = get_move_castle_side(move);
        // TODO maybe hardcoding in / constexpr'ing mask_square makes this
        // faster?
        Bitboard king_target =
            bboard::mask_square(utils::king_castle_target(color, side));
        Bitboard rook_target =
            bboard::mask_square(utils::rook_castle_target(color, side));
        Bitboard king_source =
            bboard::mask_square(utils::KING_INIT_SQUARES[color]);
        Bitboard rook_source =
            bboard::mask_square(utils::rook_castle_source(color, side));

        // remove rook
        piece_bitboards[ROOK] &= ~rook_source;
        piece_bitboards[ANY_PIECE] &= ~rook_source;
        color_bitboards[color] &= ~rook_source;
        // remove king
        piece_bitboards[KING] &= ~king_source;
        piece_bitboards[ANY_PIECE] &= ~king_source;
        color_bitboards[color] &= ~king_source;
        // add rook
        piece_bitboards[ROOK] |= rook_target;
        piece_bitboards[ANY_PIECE] |= rook_target;
        color_bitboards[color] |= rook_target;
        // add king
        piece_bitboards[KING] |= king_target;
        piece_bitboards[ANY_PIECE] |= king_target;
        color_bitboards[color] |= king_target;

        // remove castling rights
        castling_rights &= ~utils::to_castling_rights(color);
    } else {
        Square src = get_move_source(move);
        Square tgt = get_move_target(move);
        Bitboard src_mask = bboard::mask_square(src);
        Bitboard tgt_mask = bboard::mask_square(tgt);
        Color src_color;
        Color tgt_color;
        PieceType src_piece;
        PieceType tgt_piece;

        // remove captured piece if move is capture
        // NOTE: does NOT account for EP
        if (get_all_bitboard() & tgt_mask) {
            bool good = get_piece(tgt, tgt_color, tgt_piece);
            cur_state.captured_piece = tgt_piece;
            assert(good);  // must have piece there
            // only remove if piece exists
            piece_bitboards[tgt_piece] &= ~tgt_mask;
            color_bitboards[tgt_color] &= ~tgt_mask;
            // do not apply mask to ANY_PIECE because
            // new piece replaces captured piece

            // remove castling rights for opponent if rook captured
            if (tgt_piece == ROOK && (tgt_mask & ROOK_FILES)) {
                BoardSide side = (BoardSide)(!utils::sq_file(tgt));
                castling_rights &= ~utils::to_castling_rights(tgt_color, side);
            }
        }

        // remove src piece from its src location
        // do this for all move types here
        bool good = get_piece(src, src_color, src_piece);
        assert(good);
        fullmove_number += (int)src_color;
        piece_bitboards[src_piece] &= ~src_mask;
        piece_bitboards[ANY_PIECE] &= ~src_mask;
        color_bitboards[src_color] &= ~src_mask;

        // update src piece type if promotion
        if (type == PROMOTION) {
            src_piece = get_move_promotion(move);
        } else if (type == ENPASSANT) {
            // if en-passant, remove captured piece at square
            Color rmv_color = utils::opposite_color(src_color);
            Square rmv = tgt;
            utils::move_square(rmv, utils::pawn_direction(rmv_color), 0);
            Bitboard rmv_mask = bboard::mask_square(rmv);
            // remove EP captured pawn
            piece_bitboards[PAWN] &= ~rmv_mask;
            color_bitboards[rmv_color] &= ~rmv_mask;
            piece_bitboards[ANY_PIECE] &= ~rmv_mask;
        } else if (src_piece == PAWN && abs((int)tgt - (int)src) == 16) {
            // TODO no-branch-if in condition?
            enpassant_mask =
                bboard::mask_square((Square)(((int)tgt + (int)src) / 2));
        }

        // place src piece at its new location
        piece_bitboards[src_piece] |= tgt_mask;
        piece_bitboards[ANY_PIECE] |= tgt_mask;
        color_bitboards[src_color] |= tgt_mask;

        if (src_piece == KING) {
            castling_rights &= ~utils::to_castling_rights(src_color);
        } else if ((src_piece == ROOK) && (src_mask & ROOK_FILES)) {
            // if file is 0, then boardSide is !0 = 1 (queenside) and etc.
            BoardSide side = (BoardSide)(!utils::sq_file(src));
            castling_rights &= ~utils::to_castling_rights(src_color, side);
        }
    }

    history.push(cur_state);
    side_to_move = utils::opposite_color(side_to_move);
}

void Position::unmake_move(Move move) {
    MoveType type = get_move_type(move);
    enpassant_mask = 0ULL;
    PosState last_state = history.top();
    history.pop();
    enpassant_mask = last_state.enpassant_mask;
    halfmove_clock = last_state.halfmove_clock;

    if (type == CASTLING_MOVE) {
        Color color = get_move_castle_color(move);
        fullmove_number -= (int)color;  // sub if white
        BoardSide side = get_move_castle_side(move);
        // TODO maybe hardcoding in / constexpr'ing mask_square makes this
        // faster?
        Bitboard king_target =
            bboard::mask_square(utils::king_castle_target(color, side));
        Bitboard rook_target =
            bboard::mask_square(utils::rook_castle_target(color, side));
        Bitboard king_source =
            bboard::mask_square(utils::KING_INIT_SQUARES[color]);
        Bitboard rook_source =
            bboard::mask_square(utils::rook_castle_source(color, side));

        // remove rook
        piece_bitboards[ROOK] &= ~rook_target;
        piece_bitboards[ANY_PIECE] &= ~rook_target;
        color_bitboards[color] &= ~rook_target;
        // remove king
        piece_bitboards[KING] &= ~king_target;
        piece_bitboards[ANY_PIECE] &= ~king_target;
        color_bitboards[color] &= ~king_target;
        // add rook
        piece_bitboards[ROOK] |= rook_source;
        piece_bitboards[ANY_PIECE] |= rook_source;
        color_bitboards[color] |= rook_source;
        // add king
        piece_bitboards[KING] |= king_source;
        piece_bitboards[ANY_PIECE] |= king_source;
        color_bitboards[color] |= king_source;
    } else {
        Square src = get_move_source(move);
        Square tgt = get_move_target(move);
        Bitboard src_mask = bboard::mask_square(src);
        Bitboard tgt_mask = bboard::mask_square(tgt);
        Color src_color;
        // Color tgt_color;
        PieceType src_piece;
        // PieceType tgt_piece;

        // remove src piece from tgt location
        // do this for all move types here
        bool good = get_piece(tgt, src_color, src_piece);
        assert(good);
        fullmove_number -= (int)src_color;
        piece_bitboards[src_piece] &= ~src_mask;
        piece_bitboards[ANY_PIECE] &= ~src_mask;
        color_bitboards[src_color] &= ~src_mask;

        if (type == ENPASSANT) {
            // if en-passant, update tgt_square
            Color rmv_color = utils::opposite_color(src_color);
            Square rmv = tgt;
            utils::move_square(rmv, utils::pawn_direction(rmv_color), 0);
            Bitboard rmv_mask = bboard::mask_square(rmv);
            // restore captured pawn
            color_bitboards[utils::opposite_color(src_color)] |= rmv_mask;
            piece_bitboards[last_state.captured_piece] |= rmv_mask;
            piece_bitboards[ANY_PIECE] |= rmv_mask;
        } else {
            if (type == PROMOTION) {
                // restore src piece type if promotion
                src_piece = PAWN;
            } else if (src_piece == PAWN && abs((int)tgt - (int)src) == 16) {
                // TODO no-branch-if in condition?
                enpassant_mask =
                    bboard::mask_square((Square)(((int)tgt + (int)src) / 2));
            }
            // restore captured piece
            if (last_state.captured_piece != NO_PIECE) {
                color_bitboards[utils::opposite_color(src_color)] |= tgt_mask;
                piece_bitboards[last_state.captured_piece] |= tgt_mask;
                piece_bitboards[ANY_PIECE] |= tgt_mask;
            }
        }

        // place src piece back
        piece_bitboards[src_piece] |= src_mask;
        piece_bitboards[ANY_PIECE] |= src_mask;
        color_bitboards[src_color] |= src_mask;
    }

    side_to_move = utils::opposite_color(side_to_move);
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
bool Position::get_piece(Square sq, Color& c_out, PieceType& p_out) const {
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
        Square sq = bboard::bitscan_fwd_remove(pawns);
        mask |= bboard::pawn_attacks(sq, col);
    }

    // knights
    Bitboard knights = get_bitboard(col, KNIGHT);
    while (knights != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(knights);
        mask |= bboard::knight_attacks(sq);
    }

    // bishops
    Bitboard bishops = get_bitboard(col, BISHOP);
    while (bishops != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(bishops);
        mask |= bboard::bishop_attacks(sq, occ);
    }

    // rooks
    Bitboard rooks = get_bitboard(col, ROOK);
    while (rooks != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(rooks);
        mask |= bboard::rook_attacks(sq, occ);
    }

    // queens
    Bitboard queens = get_bitboard(col, QUEEN);
    while (queens != 0ULL) {
        Square sq = bboard::bitscan_fwd_remove(queens);
        mask |= bboard::rook_attacks(sq, occ);
        mask |= bboard::bishop_attacks(sq, occ);
    }

    // king
    Square king_sq = bboard::bitscan_fwd(get_bitboard(col, KING));
    mask |= bboard::king_attacks(king_sq);

    return mask;
}

void Position::clear() {}

void Position::set_piece(Square sq, Color c, PieceType piece) {
    Bitboard mask = bboard::mask_square(sq);
    piece_bitboards[(int)piece] |= mask;
    piece_bitboards[ANY_PIECE] |= mask;
    color_bitboards[(int)c] |= mask;
}

bool Position::is_checking() {
    Color atk_c = get_side_to_move();
    Square king_sq = bboard::bitscan_fwd(get_bitboard(atk_c, KING));
    return get_attackers(king_sq, utils::opposite_color(atk_c)) != 0ULL;
}

// TODO this is actually a big deal
bool Position::is_game_over() {
    // TODO once finished, update pretty_move
    return false;
}

// void Position::remove_piece(Square sq, Color c, PieceType piece) {
//     Bitboard mask = ~bboard::mask_square(sq);
//     piece_bitboards[(int)piece] &= mask;
//     piece_bitboards[ANY_PIECE] &= mask;
//     color_bitboards[(int)c] &= mask;
// }

#include <iostream>
void test_get_attackers(Position& pos, Square sq, Color atk_color) {
    Bitboard attackers = pos.get_attackers(sq, atk_color);
    std::cout << "attackers mask:\n" << utils::repr(attackers) << std::endl;
}
