#include <cassert>

#include "bitboard.h"
#include "movegen.h"

int main(int argc, char* argv[]) {
    bb::initialize();
    test_magics();
    printf("Done.\n");

    printf("Testing absolute pins...\n");
    const char* board =
        ".q......"
        "..K....."
        "..P....."
        "..r.B..."
        ".....b.."
        "........"
        "....k..."
        "........";
    CastleState cstate;
    Position pos(board, BLACK, cstate);
    test_absolute_pins(pos);
    printf("Done.\n");

    printf("Testing get_attackers...\n");
    const char* board1 =
        "K......."
        ".....Q.."
        ".NP.PP.."
        "...k...."
        "....P..."
        "..nRN..."
        "........"
        "........";
    Position pos1(board1, BLACK, cstate);
    Square king_sq = bb::bitscan_fwd(pos1.get_bitboard(WHITE, KING));
    test_get_attackers(pos1, king_sq, BLACK);

    // test create move
    Square target = 33;
    Square source = 1;
    MoveType mt = CASTLING_MOVE;
    PieceType pt = QUEEN;
    Move mv = create_move(target, source, mt, pt);
    printf("Testing create move...\n");
    assert(move_source(mv) == source);
    assert(move_target(mv) == target);
    assert(move_type(mv) == mt);
    assert(move_promotion(mv) == pt);
    printf("Done.\n");

    printf("Testing movegen - 0 checks...\n");
    const char* board2 = 
        "........"
        "...K..P."
        "........"
        "........"
        "........"
        "........"
        "R....nk."
        "........";
    Position pos2(board2, WHITE, cstate);
    std::vector<Move> moves = gen_legal_moves(pos2);
    printf("Found %zd legal moves\n", moves.size());
    printf("Done.\n");
    
    printf("All done.\n");
    return 0;
}
