#pragma once

#include <string>
#include <vector>

#include "bitboard.h"

class Position {
   public:

    Position();

    Position(std::string serial);
    
    /*
    Initialize board from ASCII representation.
    The ASCII string is 64 characters long. The first ASCII char corresponds to
    A8, and the last to H1. The chess piece at each position is denoted by:
    
    . if empty,
    p if a pawn,
    n if a knight,
    b if a bishop,
    r if a rook,
    q if a queen, and
    k if a king.

    If the piece is black, the letter is capitalized, i.e. a black pawn is 
    denoted by 'P'.

    NOTE: This does not check for validity of the board, so make sure of that 
    before calling it.
    */
    Position(std::string ascii, Color side2move, const CastleState& cstate);

    inline bool castle_allowed(CastleState cs) const {
        return cs & ~castle_state;
    }

    void apply_move(const Move&);
    
    // std::string to_ascii() const;

    // std::string serialize() const;

    inline Bitboard get_piece_bitboard(PieceType p) const {
        return piece_bitboards[p];
    }

    inline Bitboard get_color_bitboard(Color c) const {
        return color_bitboards[c];
    }

    inline Bitboard get_bitboard(Color c, PieceType p) const {
        return piece_bitboards[p] & color_bitboards[c];
    }

    inline Bitboard get_all_bitboard() const {
        return piece_bitboards[ANY_PIECE];
    }

    bool get_piece_at(Square sq, Color& c_out, PieceType& p_out) const;

    Bitboard get_attackers(Square target_sq, Color atk_color) const;

    // return a bitboard mask of all the squares that c is attacking
    Bitboard get_attack_mask(Color c) const;

    inline Color get_side_to_move() const {
        return side_to_move;
    }

    inline bool get_enpassant() const {
        return enpassant_mask;
    };

   private:
    Bitboard piece_bitboards[N_PIECE_TYPES];
    Bitboard color_bitboards[N_COLORS];
    Color side_to_move;
    CastleState castle_state;
    // zero if no en-passant last ply, else the capture mask of en-passant
    // e.g. last ply a2-a4, enpassant would hold occupancy of a3.
    Bitboard enpassant_mask;
};

void test_get_attackers(Position& pos, Square sq, Color atk_color);
