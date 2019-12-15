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

string notation::pretty_move(Move mv, const std::vector<Move>& legal_moves,
                             const Position& pos, bool checking, bool mating) {
    char buf[8] = "O-\0\0\0\0\0";  // kingside castle by default
    int bufidx = 0;
    MoveType mt = move_type(mv);
    Square tgt_sq = move_target(mv);
    if (mt == NORMAL_MOVE || mt == ENPASSANT || mt == PROMOTION) {
        bool is_capture =
            (pos.get_all_bitboard() & bboard::mask_square(tgt_sq)) ||
            mt == ENPASSANT;
        Color color;
        PieceType piece;
        Square src_sq = move_source(mv);
        bool got = pos.get_piece_at(src_sq, color, piece);
        assert(got);
        if (piece == PAWN) {
            if (is_capture) {
                buf[bufidx++] = file_char(utils::sq_file(src_sq));
            }
        } else {
            buf[bufidx++] = piece_char(piece);
        }

        Bitboard occ = pos.get_bitboard(color, piece);
        if (!bboard::one_bit(occ)) {
            char ambiguity = 0;  // 1 if some other piece is on the same rank, 2
                                 // if same file, 3 if both
            for (const Move& other_mv : legal_moves) {
                if (other_mv == mv) continue;

                Square other_src = move_source(other_mv);
                if (pos.get_bitboard(color, piece) &
                    bboard::mask_square(other_src)) {
                    ambiguity |=
                        utils::sq_rank(src_sq) == utils::sq_rank(other_src);
                    ambiguity |=
                        (utils::sq_file(src_sq) == utils::sq_file(other_src))
                        << 1;
                }
            }

            switch (ambiguity) {
                case 0:
                case 2:
                    buf[bufidx++] = file_char(utils::sq_file(src_sq));
                    break;
                case 3:
                    buf[bufidx++] = file_char(utils::sq_file(src_sq));
                case 1:
                    buf[bufidx++] = '1' + utils::sq_rank(src_sq);
                    break;
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
            buf[bufidx++] = piece_char(move_promotion(mv));
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

    return string(buf);
}

/*
 * load FEN format string str into Position pos
 * NOTE: does not check validity
 */
void notation::load_fen(Position& pos, std::istream& ins) {
    for (int row = 0; row < 8; row++) {
        int col = 0;
        while (col < 8) {
            // multiplied by 9 to skip the separator
            char piece_char;
            ins >> piece_char;
            if (std::isdigit(piece_char)) {
                col += (piece_char - '0');
                assert(col <= 8);
            } else {
                Color c;
                if (std::isupper(piece_char)) {
                    c = BLACK;
                    piece_char = std::tolower(piece_char);
                } else {
                    c = WHITE;
                }
                Square sq = utils::make_square(7 - row, col);
                switch (piece_char) {
                    case 'p':
                        pos.place_piece(c, PAWN, sq);
                        break;
                    case 'b':
                        pos.place_piece(c, BISHOP, sq);
                        break;
                    case 'n':
                        pos.place_piece(c, KNIGHT, sq);
                        break;
                    case 'r':
                        pos.place_piece(c, ROOK, sq);
                        break;
                    case 'q':
                        pos.place_piece(c, QUEEN, sq);
                        break;
                    case 'k':
                        pos.place_piece(c, KING, sq);
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
        ins.ignore();
    }
    char side_to_move;
    std::string castling_rights;
    std::string enpassant_square;
    std::string halfmove_clock;
    std::string fullmove_number;
    ins >> side_to_move;
    ins >> castling_rights >> enpassant_square;
    ins >> halfmove_clock >> fullmove_number;
    assert(side_to_move == 'w' || side_to_move == 'b');

    pos.set_side_to_move((Color)(side_to_move == 'b'));
    
    if (castling_rights != "-") {
        assert(castling_rights.length() <= 4);
        CastlingRights c_rights = NO_CASTLING_RIGHTS;
        for (auto it = castling_rights.begin(); it != castling_rights.end(); it++) {
            switch (*it) {
                case 'k':
                    c_rights |= WHITE_O_O;
                    break;
                case 'K':
                    c_rights |= BLACK_O_O;
                    break;
                case 'q':
                    c_rights |= WHITE_O_O_O;
                    break;
                case 'Q':
                    c_rights |= BLACK_O_O_O;
                    break;
                default:
                    assert(false);
                    break;
            }
        }
        pos.set_castling_rights(c_rights);
    }

    if (enpassant_square != "-") {
        assert(enpassant_square.length() == 2);
        pos.set_enpassant(utils::make_square(enpassant_square[1] - '1', enpassant_square[0] - 'a'));
    } else {
        // unset en-passant
        pos.set_enpassant(N_SQUARES);
    }

    pos.set_halfmove_clock(std::stoi(halfmove_clock));
    pos.set_fullmove_number(std::stoi(fullmove_number));
}
