#include <cassert>

#include "bitboard.h"
#include "movegen.h"
#include "notation.h"
#include "uci.h"
#include "utils.h"
#include "logger.h"

void basic_tests();
void print_moves(const char*);
void test_perft();
void test_unmake();
void run_divide();

int main(int argc, char* argv[]) {


    /* Actual main code */
    bboard::initialize();
    // UCI::initialize(argc, argv);

    // UCI::loop();

    /* END Actual main code */

    basic_tests();
    // print_moves("./fixtures/temp.fen");
    test_perft();
    // run_divide();
    // test_unmake();
    return 0;
}

// temporary test functions before the code is developed enough
// for expect scripts to be used
#include <fstream>
#include <iostream>
#include <sstream>

void basic_tests() {
    test_magics();

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
    const char* fname2 = "./fixtures/castling.fen";
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

    printf("Testing make_move...(continuing from movegen)\n");
    for (Move mv : moves) {
        std::cout << "Applying move: "
                  << notation::pretty_move(mv, moves, pos2, false, false)
                  << std::endl;
        Position copy = pos2;
        copy.make_move(mv);
        std::cout << notation::to_aligned_fen(copy) << "\n\n";
    }
}

void print_moves(const char* fname) {
    std::ifstream infile;
    infile.open(fname);
    printf("Reading from %s...\n", fname);
    assert(infile.good());
    Position pos(infile);
    std::vector<Move> moves = gen_legal_moves(pos);
    printf("Found %zd legal moves:\n", moves.size());
    for (Move mv : moves) {
        // assuming no check or mate
        std::cout << notation::pretty_move(mv, moves, pos, false, false)
                  << std::endl;
    }
    printf("Done.\n");
}

void test_perft() {
    std::istringstream iss = std::istringstream(KIWIPETE_FEN);
    Position pos(iss);
    int depth = 5;
    int count = perft(pos, depth);
    // std::cout << counts << std::endl;
    std::cout << "perft for position:"
              << "\n\n";
    std::cout << notation::to_aligned_fen(pos) << "\n\n";
    std::cout << "DEPTH: " << depth << "; COUNT: " << count << std::endl;
}

void test_unmake() {
    const char* fname = "./fixtures/kiwipete_weird.fen";
    std::ifstream infile;
    infile.open(fname);
    printf("Reading from %s...\n", fname);
    assert(infile.good());
    // std::istringstream iss = std::istringstream(KIWIPETE_FEN);
    Position pos(infile);
    auto moves = gen_legal_moves(pos);
    for (Move mv : moves) {
        Position temp = pos;
        temp.make_move(mv);
        temp.unmake_move(mv);
        // if (pos != temp) {
        //     std::cout << (notation::pretty_move(mv, moves, pos, false,
        //     false)) << std::endl;
        // }
        assert(pos == temp);
    }
    printf("Done testing unmake.\n");
}

void run_divide() {
    std::string my_str = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    // std::string my_str =
    // "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/P2P2PP/r2Q1R1K w kq - 0 1";
    std::istringstream iss = std::istringstream(my_str);
    Position pos(iss);

    divide(pos, 6);
}
