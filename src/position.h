#pragma once

#include <vector>
#include <stack>
#include <array>
#include <iostream>
#include <unordered_map>

#include "bitboard.h"

struct PosState {
    PieceType captured_piece;
    CastlingRights castling_rights;
    Bitboard enpassant_mask;
    int halfmove_clock; 
};

class Position {
   public:
    // initialize to starting position
    Position();

    /*
     * load FEN format string str into Position pos
     * NOTE: does not check validity
     * NOTE: this is less strict than standard FEN - 
     * the separator between rows, while conventionally
     * the character '/', does not matter here. And
     * one can use '.' to represent one empty square
     * for visual purposes.
     */
    Position(std::istream& fen_is);

    ~Position() = default;

    // NOTE history is NOT copied
    Position(const Position& other);

    // NOTE history is NOT copied
    Position& operator=(const Position& other);

    // NOTE does not compare halfmove_clock
    bool operator==(const Position& other) const;

    bool operator!=(const Position& other) const;

    // Note: implemented in notation.cpp
    void load_fen(std::istream& fen_is);

    // Position(std::string ascii, Color side2move, const CastlingRights& cstate);

    /*
    0 0 1
    0 1 1
    1 0 0
    1 1 1
    */
    inline bool has_castling_rights(CastlingRights cr) const {
        return !(cr & ~castling_rights);
    }

    inline CastlingRights get_castling_rights() const {
        return castling_rights;
    }

    void make_move(Move);

    void unmake_move(Move);

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

    inline bool has_piece(Square sq) const {
        return piece_bitboards[ANY_PIECE] & bboard::mask_square(sq);
    };

    // get color and piece type at the given square, if there is a piece there.
    // returns whether a piece was there.
    bool get_piece(Square sq, Color& c_out, PieceType& p_out) const;

    Bitboard get_attackers(Square target_sq, Color atk_color) const;

    /*
    Return a bitboard maskset of all the squares that c is attacking
    NOTE this ignores the king for sliding pieces occupancy
    */
    Bitboard get_attack_mask(Color c) const;

    inline Color get_side_to_move() const { return side_to_move; }

    inline Bitboard get_enpassant() const { return enpassant_mask; };

    void add_piece(Square sq, Color c, PieceType piece);

    void remove_piece(Square sq, Color c, PieceType piece);

    inline void set_castling_rights(CastlingRights c_rights) {
        castling_rights = c_rights;
    }

    // Note: sq is the TARGET CAPTURE square of the en-passant pawn
    // i.e. behind it. If sq == N_SQUARES, unset enpassant_mask
    inline void set_enpassant(Square sq) {
        if (sq != N_SQUARES) {
            enpassant_mask = bboard::mask_square(sq);
        } else {
            enpassant_mask = 0ULL;
        }
    }

    inline void set_halfmove_clock(int clock) {
        halfmove_clock = clock;
    }

    inline int get_halfmove_clock() const {
        return halfmove_clock;
    }

    inline void set_fullmove_number(int number) {
        fullmove_number = number;
    }

    inline int get_fullmove_number() const {
        return fullmove_number;
    }

    // returns whether king of the side to move is in check
    bool is_checking() const;

    /*
    These functions are used for convenience only. For efficiency, the engine should use 
    the result from gen_legal_moves().
    */
    // Returns whether the last side moved has won
    bool is_won_slow() const;

    bool is_stalemate_slow() const;

    inline bool is_over_slow() const {
        return is_won_slow() || is_stalemate_slow() || is_drawn_by_50();
    }

    // is drawn by fifty-move rule
    inline bool is_drawn_by_50() const {
        return halfmove_clock >= 100;
    }

    inline bool is_drawn_by_threefold() const {
        for (auto it = pos_counts.begin(); it != pos_counts.end(); it++) {
            if (it->second >= 3) {
                return true;
            }
        }

        return false;
    }

    inline bool is_twofold_repeated() const {
        for (auto it = pos_counts.begin(); it != pos_counts.end(); it++) {
            if (it->second >= 2) {
                return true;
            }
        }

        return false;
    }

    // basic assertions about the integrity of data fields
    bool position_good() const;

    inline ZobristKey get_hash() const { return hash; }

   private:
    Color side_to_move;
    CastlingRights castling_rights;
    std::array<Bitboard, N_PIECE_TYPES - 1> piece_bitboards; // excludes NO_PIECE
    std::array<Bitboard, N_COLORS> color_bitboards;
    //zero if no en-passant last ply, else the capture mask of en-passant
    //e.g. last ply a2-a4, enpassant would hold occupancy of a3.
    //
    Bitboard enpassant_mask;

    // number of halfmoves since the last capture or pawn advance
    // TODO game is drawn after this reaches 100
    int halfmove_clock; 
    int fullmove_number;

    std::stack<PosState> history;

    std::unordered_map<ZobristKey, unsigned> pos_counts;

	// incrementally updated zobrist hash
    ZobristKey hash;

    // clear all pieces and state
    void clear();

	// re-calculate the hash
    ZobristKey compute_hash();
};

void test_get_attackers(Position& pos, Square sq, Color atk_color);
