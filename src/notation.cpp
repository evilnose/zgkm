#include "notation.h"
#include "utils.h"

#include <cassert>
#include <cctype>
#include <sstream>

// Given a target square for castling, return whether it is kingside.
inline bool is_kingside_castle(Square target_sq) { return target_sq & 4; }

inline char piece_char(PieceType piece) {
    switch (piece) {
        case PAWN:
            return 'P';
        case KNIGHT:
            return 'N';
        case BISHOP:
            return 'B';
        case ROOK:
            return 'R';
        case KING:
            return 'K';
        default: /* queen */
            return 'Q';
    }
}

std::string notation::pretty_move(Move mv, const std::vector<Move>& legal_moves,
                                  const Position& pos, bool checking,
                                  bool mating) {
    char buf[8] = "O-\0\0\0\0\0";  // kingside castle by default
    int bufidx = 0;
    MoveType mt = get_move_type(mv);
    Square tgt_sq = get_move_target(mv);
    if (mt == NORMAL_MOVE || mt == ENPASSANT || mt == PROMOTION) {
        bool is_capture =
            (pos.get_all_bitboard() & bboard::mask_square(tgt_sq)) ||
            mt == ENPASSANT;
        Color color;
        PieceType piece;
        Square src_sq = get_move_source(mv);
        bool got = pos.get_piece(src_sq, color, piece);
        assert(got);
        if (piece == PAWN) {
            if (is_capture) {
                buf[bufidx++] = file_char(utils::sq_file(src_sq));
            }
        } else {
            buf[bufidx++] = piece_char(piece);
        }

        Bitboard occ = pos.get_bitboard(color, piece);
        if (!bboard::one_bit(occ) && piece != PAWN) {
            char ambiguity = 0;  // 1 if some other piece is on the same rank, 2
                                 // if same file, 3 if both
            for (const Move& other_mv : legal_moves) {
                if (other_mv == mv) continue;

                Square other_src = get_move_source(other_mv);
                // if there is another move with same target sq as this move and
                // other move source has same piece and color as this piece,
                // consider ambiguity
                if (get_move_target(other_mv) == tgt_sq &&
                    (pos.get_bitboard(color, piece) &
                     bboard::mask_square(other_src))) {
                    ambiguity |=
                        utils::sq_rank(src_sq) == utils::sq_rank(other_src);
                    ambiguity |=
                        (utils::sq_file(src_sq) == utils::sq_file(other_src))
                        << 1;
                }
            }

            if (ambiguity & 2) {
                buf[bufidx++] = file_char(utils::sq_file(src_sq));
            }
            if (ambiguity & 1) {
                buf[bufidx++] = '1' + utils::sq_rank(src_sq);
            }
        }

        // Add 'x' if is a capture
        buf[bufidx] = 'x';
        bufidx += is_capture;

        // Target square
        buf[bufidx++] = file_char(utils::sq_file(tgt_sq));
        buf[bufidx++] = '1' + utils::sq_rank(tgt_sq);

        // Promotion
        if (mt == PROMOTION) {
            buf[bufidx++] = '=';
            buf[bufidx++] = piece_char(get_move_promotion(mv));
        }

        // Final suffix
        if (checking) {
            buf[bufidx++] = '+';
        } else if (mating) {
            buf[bufidx++] = '#';
        }
    } else {
        buf[2] = 'O';
        bool queenside = !is_kingside_castle(tgt_sq);
        buf[3] = queenside * '-';
        buf[4] = queenside * 'O';
    }

    return std::string(buf);
}

std::string notation::to_fen(const Position& pos) {
    std::string ret;
    ret.reserve(128);
    // TODO
    return "to_fen() NOT FINISHED\n";
}

std::string notation::to_aligned_fen(const Position& pos) {
    std::string board(72, '.');
    for (int rank = 0; rank < 7; rank++) {
        board[rank * 9 + 8] = '\n';
    }
    // last line is followed by a space instead of a newline
    board[71] = ' ';
    for (Color color = WHITE; color != N_COLORS; color = (Color)(color + 1)) {
        for (PieceType piece = PAWN; piece != ANY_PIECE;
             piece = (PieceType)(piece + 1)) {
            Bitboard mask = pos.get_bitboard(color, piece);
            while (mask != 0ULL) {
                Square sq = bboard::bitscan_fwd_remove(mask);
                char c;
                switch (piece) {
                    case PAWN:
                        c = 'p';
                        break;
                    case KNIGHT:
                        c = 'n';
                        break;
                    case BISHOP:
                        c = 'b';
                        break;
                    case ROOK:
                        c = 'r';
                        break;
                    case QUEEN:
                        c = 'q';
                        break;
                    case KING:
                        c = 'k';
                        break;
                    default:
                        assert(false);
                        break;
                }
                if (color == WHITE) {
                    c = std::toupper(c);
                }
                board[(7 - utils::sq_rank(sq)) * 9 + utils::sq_file(sq)] = c;
            }
        }
    }

    std::string postfix = pos.get_side_to_move() == WHITE ? "w " : "b ";
    CastlingRights rights = pos.get_castling_rights();
    if (rights == NO_CASTLING_RIGHTS) {
        postfix += "-";
    } else {
        if (rights & BLACK_OO) {
            postfix += "K";
        }
        if (rights & BLACK_OOO) {
            postfix += "Q";
        }
        if (rights & WHITE_OO) {
            postfix += "k";
        }
        if (rights & WHITE_OOO) {
            postfix += "q";
        }
    }
    postfix += " ";

    Bitboard enpassant = pos.get_enpassant();
    if (enpassant) {
        Square enp_sq = bboard::bitscan_fwd(enpassant);
        postfix += square_str(enp_sq) + ' ';
    } else {
        postfix += "- ";
    }

    postfix += std::to_string(pos.get_halfmove_clock()) + " ";
    postfix += std::to_string(pos.get_fullmove_number());
    return board + postfix;
}
