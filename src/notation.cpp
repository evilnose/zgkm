#include "notation.h"
#include "utils.h"
#include "logger.h"

#include <cassert>
#include <cctype>
#include <sstream>

namespace {
Square parse_square(const char* buf) {
    int file = buf[0] - 'a';
    int rank = buf[1] - '1';
    return utils::make_square(rank, file);
}
}

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
                                  const Position& pos, bool checking) {
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
        pos.get_piece(src_sq, color, piece);
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

        bool mating = checking && legal_moves.size() == 0;
        // Final suffix
        if (checking) {
            buf[bufidx++] = '+';
        } else if (mating) {
            buf[bufidx++] = '#';
        }
    } else {
        buf[2] = 'O';
        bool queenside = get_move_castle_side(mv) == QUEENSIDE;
        buf[3] = queenside * '-';
        buf[4] = queenside * 'O';
    }

    return std::string(buf);
}

std::string notation::dump_uci_move(Move mv) {
    MoveType type = get_move_type(mv);
    if (type == CASTLING_MOVE) {
        BoardSide side = get_move_castle_side(mv);
        Color color = get_move_castle_color(mv);
        Square king_src = color == WHITE ? SQ_E1 : SQ_E8;
        return square_str(king_src) + square_str(utils::king_castle_target(color, side));
    } else {
        std::string ret = square_str(get_move_source(mv)) + square_str(get_move_target(mv));
        if (get_move_type(mv) == PROMOTION) {
            ret += std::tolower(piece_char(get_move_promotion(mv)));
        }
        return ret;
    }
}

Move notation::parse_uci_move(const Position& pos, const std::string& mv_str) {
    // test for castling moves
    if (mv_str == "e1g1") {
        // white king-side castle
        assert(pos.has_castling_rights(WHITE_OO));
        return create_castling_move(WHITE, KINGSIDE);
    } else if (mv_str == "e1c1") {
        assert(pos.has_castling_rights(WHITE_OOO));
        return create_castling_move(WHITE, QUEENSIDE);
    } else if (mv_str == "e8g8") {
        assert(pos.has_castling_rights(BLACK_OO));
        return create_castling_move(BLACK, KINGSIDE);
    } else if (mv_str == "e8c8") {
        assert(pos.has_castling_rights(BLACK_OOO));
        return create_castling_move(BLACK, QUEENSIDE);
    }

    const char* mv_buf = mv_str.c_str();
    Square src = parse_square(mv_buf);
    Square dest = parse_square(mv_buf + 2);
    Color to_move = pos.get_side_to_move();

    if (bboard::mask_square(dest) == pos.get_enpassant()) {
        // assert(pos.get_piece(src, to_move, out_piece) == PAWN);
        return create_enpassant(src, dest);
    }

    if (mv_str.size() == 5) {
        // promotion
        PieceType ptype = notation::parse_piece_char(mv_str[4]);
        assert(ptype != NO_PIECE);
        return create_promotion_move(src, dest, ptype);
    }
    return create_normal_move(src, dest);
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
        if (rights & WHITE_OO) {
            postfix += "K";
        }
        if (rights & WHITE_OOO) {
            postfix += "Q";
        }
        if (rights & BLACK_OO) {
            postfix += "k";
        }
        if (rights & BLACK_OOO) {
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

PieceType notation::parse_piece_char(char piece_char) {
    switch (piece_char) {
        case 'p':
            return PAWN;
        case 'n':
            return KNIGHT;
        case 'b':
            return BISHOP;
        case 'r':
            return ROOK;
        case 'q':
            return QUEEN;
        case 'k':
            return KING;
        default:
            return NO_PIECE;
    }
}

void Position::load_fen(std::istream& fen_is) {
    clear();
    for (int row = 0; row < 8; row++) {
        int col = 0;
        while (col < 8) {
            // multiplied by 9 to skip the separator
            char piece_char;
            assert(fen_is && fen_is.peek() != EOF);
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
                PieceType ptype = notation::parse_piece_char(piece_char);
                if (ptype != NO_PIECE) {
                    add_piece(sq, c, ptype);
                } else {
                    assert(piece_char == '.');
                }
                col++;
            }
        }
        // ignore row delimiter
        fen_is.ignore();
    }
    char stom;  // side to move
    std::string castling_rights;
    std::string enpassant_square;
    int halfmove_clock;
    int fullmove_number;
    fen_is >> stom;
    fen_is >> castling_rights >> enpassant_square;
    if (fen_is >> halfmove_clock) {
        fen_is >> fullmove_number;
    } else {
        halfmove_clock = 0;
        fullmove_number = 1;
    }
    assert(stom == 'w' || stom == 'b');

    side_to_move = (Color)(stom == 'b');

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

    set_halfmove_clock(halfmove_clock);
    set_fullmove_number(fullmove_number);

    hash = compute_hash();
	pos_counts.clear();
    pos_counts[hash] = 1;
}
