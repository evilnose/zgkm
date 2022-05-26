#include "position.h"

#include "movegen.h"
#include "utils.h"
#include "logger.h"
#include "hash.h"

#include <cassert>
#include <cctype>
#include <istream>
#include <sstream>
#include <iostream>

using std::string;
using std::vector;

namespace {}  // namespace

Position::Position() {
    // initialization of members is done in load_fen
    std::istringstream iss(STARTING_FEN);
    load_fen(iss);
}

Position::Position(std::istream& fen_is)
    : piece_bitboards{}, color_bitboards{} {
    // initialization of members is done in load_fen
    load_fen(fen_is);
}

Position::Position(const Position& other)
    : side_to_move{other.side_to_move},
      castling_rights{other.castling_rights},
      piece_bitboards{other.piece_bitboards},
      color_bitboards{other.color_bitboards},
      enpassant_mask{other.enpassant_mask},
      halfmove_clock{other.halfmove_clock},
      fullmove_number{other.fullmove_number},
      hash{other.hash} {}

Position& Position::operator=(const Position& other) {
    side_to_move = other.side_to_move;
    castling_rights = other.castling_rights;
    piece_bitboards = other.piece_bitboards;
    color_bitboards = other.color_bitboards;
    enpassant_mask = other.enpassant_mask;
    halfmove_clock = other.halfmove_clock;
    fullmove_number = other.fullmove_number;
    hash = other.hash;
    return *this;
}

bool Position::operator==(const Position& other) const {
    if (side_to_move != other.side_to_move) {
        return false;
    }

    if (castling_rights != other.castling_rights) {
        return false;
    }

    if (fullmove_number != other.fullmove_number) {
        return false;
    }

    if (enpassant_mask != other.enpassant_mask) {
        return false;
    }

    if (piece_bitboards != other.piece_bitboards) {
        return false;
    }

    if (color_bitboards != other.color_bitboards) {
        return false;
    }

    return true;
}

bool Position::operator!=(const Position& other) const {
    return !(*this == other);
}

void Position::make_move(Move move) {
    MoveType type = get_move_type(move);
    PosState cur_state{NO_PIECE, castling_rights, enpassant_mask,
                       halfmove_clock};
    enpassant_mask = 0ULL;
    halfmove_clock++;  // increment halfmove_clock by default

    if (type == CASTLING_MOVE) {
        Color color = get_move_castle_color(move);
        fullmove_number += (int)color;  // add if white
        BoardSide side = get_move_castle_side(move);
        // TODO maybe hardcoding in / constexpr'ing mask_square makes this
        // faster?

        // remove rook
        remove_piece(utils::rook_castle_source(color, side), color, ROOK);
        // remove king
        remove_piece(utils::KING_INIT_SQUARES[color], color, KING);
        // add rook
        add_piece(utils::rook_castle_target(color, side), color, ROOK);
        // add king
        add_piece(utils::king_castle_target(color, side), color, KING);

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
        bool is_capture = false;  // used later for 50-move rule

        // remove captured piece if move is capture
        // NOTE: does NOT account for EP
        if (get_all_bitboard() & tgt_mask) {
            is_capture = true;
            get_piece(tgt, tgt_color, tgt_piece);
            remove_piece(tgt, tgt_color, tgt_piece);

            cur_state.captured_piece = tgt_piece;

            // remove castling rights for opponent if rook captured
            if (tgt_piece == ROOK && (tgt_mask & ROOK_FILES &
                                      (tgt_color == WHITE ? RANK_A : RANK_H))) {
                BoardSide side = (BoardSide)(!utils::sq_file(tgt));
                castling_rights &= ~utils::to_castling_rights(tgt_color, side);
            }
        }

        // remove src piece from its src location
        // do this for all move types here, except castling
        get_piece(src, src_color, src_piece);
        remove_piece(src, src_color, src_piece);
        fullmove_number += (int)src_color;

        // update src piece type if promotion
        if (type == PROMOTION) {
            src_piece = get_move_promotion(move);
        } else if (type == ENPASSANT) {
            // if en-passant, remove captured piece at square
            Color rmv_color = utils::opposite_color(src_color);
            Square rmv = tgt;
            utils::move_square(rmv, utils::pawn_direction(rmv_color), 0);
            // remove EP captured pawn
            remove_piece(rmv, rmv_color, PAWN);
            // don't need the following line b/c enpassant implies pawn
            // cur_state.captured_piece = tgt_piece;
        } else if (src_piece == PAWN && abs((int)tgt - (int)src) == 16) {
            // double pawn push, so update en-passant mask
            // TODO no-branch-if in condition?
            enpassant_mask = bboard::mask_square((Square)(((int)tgt + (int)src) / 2));
        }

        // place src piece at its new location
        add_piece(tgt, src_color, src_piece);

        if (src_piece == KING) {
            castling_rights &= ~utils::to_castling_rights(src_color);
        } else if ((src_piece == ROOK) && (src_mask & ROOK_FILES)) {
            // if file is 0, then boardSide is !0 = 1 (queenside) and etc.
            BoardSide side = (BoardSide)(!utils::sq_file(src));
            castling_rights &= ~utils::to_castling_rights(src_color, side);
        }

        // for 50-move rule. If is capture or piece is pawn, reset halfmove_clock
        halfmove_clock *= !(is_capture || src_piece == PAWN);
    }

    history.push(cur_state);
    side_to_move = utils::opposite_color(side_to_move);
    hash ^= zobrist::get_black_to_move_key();

	// increment hash count
    auto it = pos_counts.find(hash);
    if (it == pos_counts.end()) {
        // std::cout << "first " << hash << std::endl;
        pos_counts[hash] = 1;
    } else {
        // std::cout << "second " << hash << std::endl;
        it->second++;
    }

    assert(compute_hash() == hash);
}

void Position::unmake_move(Move move) {
    auto it = pos_counts.find(hash);
    assert(it != pos_counts.end());
    it->second--;
    if (it->second == 0) {
        pos_counts.erase(it);
    }

    assert(history.size() != 0);
    MoveType type = get_move_type(move);
    PosState last_state = history.top();
    history.pop();
    enpassant_mask = last_state.enpassant_mask;
    halfmove_clock = last_state.halfmove_clock;
    castling_rights = last_state.castling_rights;

    if (type == CASTLING_MOVE) {
        Color color = get_move_castle_color(move);
        fullmove_number -= (int)color;  // sub if white
        BoardSide side = get_move_castle_side(move);
        // TODO maybe hardcoding in / constexpr'ing mask_square makes this
        // faster?

        // remove rook
        remove_piece(utils::rook_castle_target(color, side), color, ROOK);
        // remove king
        remove_piece(utils::king_castle_target(color, side), color, KING);
        // add rook
        add_piece(utils::rook_castle_source(color, side), color, ROOK);
        // add king
        add_piece(utils::KING_INIT_SQUARES[color], color, KING);
    } else {
        Square src = get_move_source(move);
        Square tgt = get_move_target(move);
        Color src_color;
        // Color tgt_color;
        PieceType src_piece;
        // PieceType tgt_piece;

        // remove src piece from tgt location
        // do this for all move types here
        get_piece(tgt, src_color, src_piece);
        fullmove_number -= (int)src_color;
        remove_piece(tgt, src_color, src_piece);

        if (type == ENPASSANT) {
            // if en-passant, update tgt_square
            Color rmv_color = utils::opposite_color(src_color);
            Square rmv = tgt;
            utils::move_square(rmv, utils::pawn_direction(rmv_color), 0);
            // restore captured pawn
            add_piece(rmv, rmv_color, PAWN);
        } else {
            if (type == PROMOTION) {
                // restore src piece type if promotion
                src_piece = PAWN;
            }

            // restore captured piece
            if (last_state.captured_piece != NO_PIECE) {
                add_piece(tgt, utils::opposite_color(src_color), last_state.captured_piece);
            }
        }

        // place src piece back
        add_piece(src, src_color, src_piece);
    }

    side_to_move = utils::opposite_color(side_to_move);
    hash ^= zobrist::get_black_to_move_key();

    assert(compute_hash() == hash);
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
void Position::get_piece(Square sq, Color& c_out, PieceType& p_out) const {
    Bitboard mask = bboard::mask_square(sq);
    if (mask & get_color_bitboard(WHITE)) {
        c_out = WHITE;
    } else if (mask & get_color_bitboard(BLACK)) {
        c_out = BLACK;
    } else {
        LOG(logERROR) << "there is no piece at the given square!";
        assert(false);
        exit(EXIT_FAILURE);
    }

    if (mask & get_piece_bitboard(PAWN)) {
        p_out = PAWN;
    } else if (mask & get_piece_bitboard(KNIGHT)) {
        p_out = KNIGHT;
    } else if (mask & get_piece_bitboard(BISHOP)) {
        p_out = BISHOP;
    } else if (mask & get_piece_bitboard(ROOK)) {
        p_out = ROOK;
    } else if (mask & get_piece_bitboard(QUEEN)) {
        p_out = QUEEN;
    } else if (mask & get_piece_bitboard(KING)) {
        p_out = KING;
    } else {
        LOG(logERROR) << "there is no piece at the given square!";
        assert(false);
        exit(EXIT_FAILURE);
    }
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

void Position::add_piece(Square sq, Color c, PieceType piece) {
    Bitboard mask = bboard::mask_square(sq);
    piece_bitboards[(int)piece] |= mask;
    piece_bitboards[ANY_PIECE] |= mask;
    color_bitboards[(int)c] |= mask;
    
    hash ^= zobrist::get_key(sq, piece, c);
}

void Position::remove_piece(Square sq, Color c, PieceType piece) {
    Bitboard mask = ~bboard::mask_square(sq);
    piece_bitboards[(int)piece] &= mask;
    piece_bitboards[ANY_PIECE] &= mask;
    color_bitboards[(int)c] &= mask;

    hash ^= zobrist::get_key(sq, piece, c);
}

bool Position::is_checking() const {
    Color atk_c = get_side_to_move();
    Square king_sq = bboard::bitscan_fwd(get_bitboard(atk_c, KING));
    return get_attackers(king_sq, utils::opposite_color(atk_c)) != 0ULL;
}

bool Position::is_won_slow() const {
    std::vector<Move> moves;
    // checking and no moves
    return gen_legal_moves(*this, moves) && moves.size() == 0;
}

bool Position::is_stalemate_slow() const {
    std::vector<Move> moves;
    // checking and no moves
    return !gen_legal_moves(*this, moves) && moves.size() == 0;
}

bool Position::position_good() const {
    Bitboard piece_mask = 0ULL;
    for (PieceType pt = PAWN; pt != ANY_PIECE; pt = (PieceType)(pt + 1)) {
        if (piece_mask & piece_bitboards[pt]) {
            return false;
        }
        piece_mask |= piece_bitboards[pt];
    }
    for (PieceType pt = PAWN; pt != ANY_PIECE; pt = (PieceType)(pt + 1)) {
        if ((piece_bitboards[pt] | piece_bitboards[ANY_PIECE]) !=
               piece_bitboards[ANY_PIECE]) {
                   return false;
        }
    }
    if (piece_mask != piece_bitboards[ANY_PIECE]) {
        return false;
    }

    if (color_bitboards[WHITE] & color_bitboards[BLACK]) {
        return false;
    }
    if ((color_bitboards[WHITE] | color_bitboards[BLACK]) !=
           piece_bitboards[ANY_PIECE]) {
               return false;
    }
    return true;
}

void Position::clear() {
    side_to_move = WHITE;
    castling_rights = ALL_CASTLING_RIGHTS;
    piece_bitboards = {};
    color_bitboards = {};
    enpassant_mask = {};
    halfmove_clock = 0;
    fullmove_number = 1;
    history = {};
    hash = 0;
}

ZobristKey Position::compute_hash() {
    // from https://en.wikipedia.org/wiki/Zobrist_hashing
    ZobristKey hs{};

    if (side_to_move == BLACK) {
        hs ^= zobrist::get_black_to_move_key();
    }

    for (Color c : {WHITE, BLACK}) {
        for (PieceType pt = PAWN; pt != ANY_PIECE; pt = (PieceType)(pt + 1)) {
            Bitboard bb = get_bitboard(c, pt);
            while (bb) {
                Square sq = bboard::bitscan_fwd_remove(bb);
                hs ^= zobrist::get_key(sq, pt, c);
            }
        }
    }
    return hs;
}

#include <iostream>
void test_get_attackers(Position& pos, Square sq, Color atk_color) {
    Bitboard attackers = pos.get_attackers(sq, atk_color);
    std::cout << "attackers mask:\n" << utils::repr(attackers) << std::endl;
}
