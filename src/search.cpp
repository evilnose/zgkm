#include "search.h"

#include <cassert>
#include <chrono>
#include <thread>

#include "movegen.h"
#include "logger.h"

// TODO refactor delete this file
#if 0
namespace {
// DFS
Score alpha_beta_search(Position &pos, int depth, Score alpha, Score beta) {
    assert(depth >= 0);
    assert(pos.position_good());

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

        if (depth == 0) {
            return evaluate(pos);
        }

        for (Move move : moves) {
            pos.make_move(move);
            assert(pos.position_good());
            Score s = -alpha_beta_search(pos, depth - 1, -beta, -alpha);
            pos.unmake_move(move);
            if (s >= beta) {
                // opponent's upper bound broken. So curr pos will not be
                // allowed
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

// search with the given remaining X and O times, allocating how much time to
// spend automatically time all in millis
void search(const Position &pos, int btime, int wtime, int binc, int cinc,
            ResultCallback callback) {
    assert(!pos.is_over_slow());
    int my_time = pos.get_side_to_move() == Color::BLACK ? btime : wtime;
    // allocate time; very basic for now: assumes 40 moves in total, and make it
    // at least 50 ms
    int time_alloc = my_time / (40 - pos.get_fullmove_number());
    time_alloc = std::max(time_alloc, 50);

    std::vector<Move> moves;
    gen_legal_moves(pos, moves);
    // iterative deepening loop
    int depth = 2;
    Position localpos = pos;
    Score eval = SCORE_NEG_INFTY;
    utils::Timer timer;
    while (depth <= 20) {
        Score alpha = SCORE_NEG_INFTY;
        Move best = NULL_MOVE;
        for (Move move : moves) {
            localpos.make_move(move);
            SearchResult result = depth_search(localpos, depth - 1);
            auto elapsed = timer.elapsed_millis();
            // time's up
            if (time_alloc - elapsed < 100) {
                return;
            }
            localpos.make_move(move);
            Score res =
                -alpha_beta_search(localpos, depth - 1, alpha, SCORE_POS_INFTY);
            localpos.unmake_move(move);
            if (res > alpha) {
                alpha = res;
                best = move;
            }
            localpos.unmake_move(move);
        }
        assert(best != NULL_MOVE);

        // reinsert best move as the first move in the vector, so that it is
        // explored first in the next iteration
        moves.erase(find(moves.begin(), moves.end(), best));
        moves.insert(moves.begin(), best);
        eval = alpha * color_multiplier(pos.get_side_to_move());
        depth++;
    }

    assert(eval != SCORE_NEG_INFTY);
    callback(moves[0], eval);
}
}  // namespace

SearchResult depth_search(const Position &orig_pos, int depth) {
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
    for (Move move : moves) {
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

    return SearchResult{
        alpha * color_multiplier(pos.get_side_to_move()),
        best_move,
    };
}

void timed_search(const Position &pos, int btime, int wtime, int binc, int cinc,
                  ResultCallback callback) {
    std::thread thd(search, pos, btime, wtime, binc, cinc, callback);
}

#endif