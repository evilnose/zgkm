#pragma once

#include <string>
#include <vector>

#include "bitboard.h"

class Position {
   public:
    Position();

    Position(std::string serial);

	// TODO remove
    void load_inline_ascii(std::string ascii, Color side2move,
                           const CastleState& cstate, int enpassant_file = -1);

    // Position(std::string ascii, Color side2move, const CastleState& cstate);

    inline bool has_castling_rights(CastleState cs) const {
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

    /*
    Return a bitboard mask of all the squares that c is attacking
    NOTE this ignores the king for sliding pieces occupancy
    */
    Bitboard get_attack_mask(Color c) const;

	inline void set_side_to_move(Color c) { side_to_move = c; }

    inline Color get_side_to_move() const { return side_to_move; }

    inline Bitboard get_enpassant() const { return enpassant_mask; };

	// clears everything and resets all states. i.e. empty board
	// and starting castling rights
	void clear();

	void place_piece(Color c, PieceType piece, Square sq);

	inline void set_castle_state(CastleState cstate) {
		castle_state = cstate;
	}

	// Note: sq is the TARGET CAPTURE square of the en-passant pawn
	// i.e. behind it
	inline void set_enpassant(Square sq) {
		enpassant_mask = bboard::mask_square(sq);
	}

	inline void set_halfmove_clock(int clock) {
		halfmove_clock = clock;
	}

	inline void set_fullmove_number(int number) {
		fullmove_number = number;
	}

   private:
    Color side_to_move;
    CastleState castle_state;
    Bitboard piece_bitboards[N_PIECE_TYPES];
    Bitboard color_bitboards[N_COLORS];
    /* zero if no en-passant last ply, else the capture mask of en-passant
     * e.g. last ply a2-a4, enpassant would hold occupancy of a3.
     */
    Bitboard enpassant_mask;

	// number of halfmoves since the last capture or pawn advance
	// TODO game is drawn after this reaches 100
	int halfmove_clock; 
	int fullmove_number;
};

void test_get_attackers(Position& pos, Square sq, Color atk_color);
