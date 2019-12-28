#include "bitboard.h"
#include "movegen.h"
#include "notation.h"
#include "utils.h"

#include<cassert>

void basic_tests();
void test_perft();

int main(int argc, char* argv[]) {
    bboard::initialize();
    // basic_tests();
    test_perft();
    return 0;
}

// temporary test functions before the code is developed enough
// for expect scripts to be used
#include <fstream>
#include <iostream>
#include <sstream>

void basic_tests() {
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
    pos.load_fen(ss);
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
    pos1.load_fen(stream1);
    Square king_sq = bboard::bitscan_fwd(pos1.get_bitboard(WHITE, KING));
    test_get_attackers(pos1, king_sq, BLACK);

    // test create move
    Square target = utils::to_square(33);
    Square source = utils::to_square(1);
    // MoveType mt = NORMAL_MOVE;
    // PieceType pt = QUEEN;
    Move mv = create_normal_move(target, source);
    printf("Testing create move...\n");
    assert(get_move_source(mv) == source);
    assert(get_move_target(mv) == target);
    assert(get_move_type(mv) == NORMAL_MOVE);
    // assert(move_promotion(mv) == pt);
    printf("Done.\n");

    printf("Testing movegen(0 checks)...\n");
    Position pos2;
    const char* fname2 = "./fixtures/promotions.fen";
    std::ifstream infile;
    infile.open(fname2);
    printf("Reading from %s...\n", fname2);
    assert(infile.good());
    pos2.load_fen(infile);
    std::vector<Move> moves = gen_legal_moves(pos2);
    printf("Found %zd legal moves:\n", moves.size());
    for (Move mv : moves) {
        // assuming no check or mate
        std::cout << notation::pretty_move(mv, moves, pos2, false, false)
                  << std::endl;
    }
    printf("Done.\n");

    printf("Testing apply_move...(continuing from movegen)\n");
    for (Move mv : moves) {
        std::cout << "Applying move: "
                  << notation::pretty_move(mv, moves, pos2, false, false)
                  << std::endl;
        Position copy = pos2;
        copy.apply_move(mv);
        std::cout << notation::to_aligned_fen(copy) << "\n\n";
    }
}

void test_perft() {
    std::istringstream iss = std::istringstream(STARTING_FEN);
    Position pos(iss);
    int depth = 5;
    std::vector<int> counts = perft(pos, depth);
    // std::cout << counts << std::endl;
    std::cout << "perft for position:" << "\n\n";
    std::cout << notation::to_aligned_fen(pos) << "\n\n";
    for (int d = 1; d <= depth; d++) {
        std::cout << "depth " << d << ": " << counts[d-1] << " moves" << std::endl;
    }
}
