#include <cassert>

#include "search.h"
#include "movegen.h"

namespace {
    // DFS
    Score alpha_beta_search(Position& pos, int depth, Score alpha, Score beta) {
        assert(depth >= 0);
        assert(pos.position_good());

        if (depth == 0) {
            return evaluate(pos);
        }

        std::vector<Move> moves;
        bool checking = gen_legal_moves(pos, moves);

        if (pos.is_drawn_by_50()) {
            return SCORE_DRAW;
        }

        if (moves.size() == 0) {
            if (checking) {
                // I lose
                return SCORE_NEG_INFTY;
            } else {
                return SCORE_DRAW;
            }
        } else {
            for (Move move: moves) {
                pos.make_move(move);
                assert(pos.position_good());
                Score s = -alpha_beta_search(pos, depth - 1, -beta, -alpha);
                pos.unmake_move(move);
                if (s >= beta) {
                    // opponent's upper bound broken. So curr pos will not be allowed
                    return beta;
                }
                if (s > alpha) {
                    // update my guarantee
                    alpha = s;
                }
            }
        }
        return alpha;
    }
}  // namespace

SearchResult depth_search(const Position& orig_pos, int depth) {
    assert(depth >= 1);

    Position pos = orig_pos;
    assert(pos.position_good());

    Score alpha = SCORE_NEG_INFTY;
    Score beta = SCORE_POS_INFTY;

    std::vector<Move> moves;
    bool checking = gen_legal_moves(pos, moves);

    // this should not be called after the game is over
    assert(moves.size() != 0 && !pos.is_drawn_by_50());

    Move best_move = NULL_MOVE;
    for (Move move: moves) {
        pos.make_move(move);
        assert(pos.position_good());
        Score s = -alpha_beta_search(pos, depth - 1, -beta, -alpha);
        pos.unmake_move(move);
        assert(pos.position_good());
        if (s > alpha) {
            best_move = move;
            alpha = s;
        }
    }

    assert(best_move != NULL_MOVE);

    return SearchResult {
        best_move,
        alpha * color_multiplier(pos.get_side_to_move()),
    };
}
