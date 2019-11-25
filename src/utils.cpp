#include "utils.h"

#include <cwctype>

#include <fstream>

using std::string;

string repr(Bitboard bitboard) {
    char ret[73];  // 8 * 9
    for (int i = 0; i < 8; i++) {
        ret[i * 9 + 8] = '\n';
    }

    for (Square sq = SQ_A1; sq <= SQ_H8; sq++) {
        int rank = sq_rank(sq);
        int file = sq_file(sq);
        char ch = ((1ULL << sq) & bitboard) ? '1' : '0';
        ret[(7 - rank) * 9 + file] = ch;
    }
    ret[72] = '\0';
    return string(ret);
}

bool move_square(Square& sq, int d_rank, int d_file) {
    int rank = sq_rank(sq) + d_rank;
    int file = sq_file(sq) + d_file;

    if (rank < 0 || rank >= 8 || file < 0 || file >= 8) {
        return false;
    }

    sq = to_square(rank * 8 + file);
    return true;
}

void read_and_trim(string fname, char buf[]) {
    std::ifstream in;
    int i = 0;
    char ch;
    in.open(fname);
    assert(in.good());
    while (i < 255 && in.get(ch)) {
        if (!iswspace(ch))
            buf[i++] = ch;
    }
    in.close();
    buf[i] = '\0';
}
