#pragma once

#include <string>

#include "moves.h"
#include "bitboard.h"

using std::string;

struct Position
{
public:
    Position();
    Position(string serial);
    bool castle_allowed(CastleState cs);

    // Note: does not modify this Position object and instead returns a new one
    Position make_move(const Move &) const;
    string to_ascii() const;
    string serialize() const;

    inline Bitboard get_piece_bitboard(PieceType p) const
    {
        return piece_bitboards[p];
    }

    inline Bitboard get_color_bitboard(Color c) const
    {
        return color_bitboards[c];
    }

private:
    Bitboard piece_bitboards[N_PIECE_TYPES];
    Bitboard color_bitboards[N_COLORS];
    Bitboard all_bitboard; // bitboard of all pieces of both sides
    Color side_to_move;
    CastleState castle_state;
};
