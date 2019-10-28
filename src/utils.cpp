#include "utils.h"

using std::string;

string repr(Bitboard bitboard) {
    char ret[73];  // 8 * 9
    for (int i = 0; i < 8; i++) {
        ret[i * 9 + 8] = '\n';
    }

    for (int index = 0; index < 64; index++) {
        int rank = sq_rank(index);
        int file = sq_file(index);
        char ch = ((1ULL << index) & bitboard) ? '1' : '0';
        ret[(7 - rank) * 9 + file] = ch;
    }
    ret[72] = '\0';
    return string(ret);
}
