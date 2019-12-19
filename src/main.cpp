#include "notation.h"
#include "bitboard.h"
#include "movegen.h"
#include "utils.h"

#include <cassert>
// TODO for debug; remove this
#include <iostream>
#include <sstream>
#include <fstream>

int main(int argc, char* argv[]) {
    bboard::initialize();
    test_magics();
    printf("Done.\n");

    printf("Testing absolute pins...\n");
    const char* board =
        ".q....../"
        "..K...../"
        "..P...../"
        "..r.B.../"
        ".....b../"
        "......../"
        "....k.../"
        "........ "
        "b - - 0 0";
    Position pos;
    // pos.load_inline_ascii(board, BLACK, c_rights);
    std::stringstream ss(board);
    notation::load_fen(pos, ss);
    test_absolute_pins(pos);
    printf("Done.\n");

    printf("Testing get_attackers...\n");
    const char* board1 =
        "K......./"
        ".....Q../"
        ".NP.PP../"
        "...k..../"
        "....P.../"
        "..nRN.../"
        "......../"
        "........ "
        "w - - 0 0";
    std::istringstream stream1(board1);
    Position pos1;
    notation::load_fen(pos1, stream1);
    Square king_sq = bboard::bitscan_fwd(pos1.get_bitboard(WHITE, KING));
    test_get_attackers(pos1, king_sq, BLACK);

    // test create move
    Square target = utils::to_square(33);
    Square source = utils::to_square(1);
    // MoveType mt = NORMAL_MOVE;
    // PieceType pt = QUEEN;
    Move mv = create_normal_move(target, source);
    printf("Testing create move...\n");
    assert(move_source(mv) == source);
    assert(move_target(mv) == target);
    assert(move_type(mv) == NORMAL_MOVE);
    // assert(move_promotion(mv) == pt);
    printf("Done.\n");

    printf("Testing movegen(0 checks)...\n");
    Position pos2;
    const char* fname2 = "./fixtures/promotions.fen"; 
    std::ifstream infile;
    infile.open(fname2);
    printf("Reading from %s...\n", fname2);
    assert(infile.good());
    notation::load_fen(pos2, infile);
    std::vector<Move> moves = gen_legal_moves(pos2);
    printf("Found %zd legal moves:\n", moves.size());
    for (Move mv : moves) {
        // assuming no check or mate
        std::cout << notation::pretty_move(mv, moves, pos2, false, false)
                  << std::endl;
    }
    printf("Done.\n");

    printf("All done.\n");
    return 0;
}
