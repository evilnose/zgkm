#include "threading.h"
#include "movegen.h"
#include "notation.h"
#include "logger.h"

#include <iostream>
#include <cassert>
#include <algorithm>

namespace uci {
void pv() {
    std::cerr << "uci::pv() not implemented.";
    assert(false);
}

void bestmove(Move move) {
    std::cout << "bestmove " << notation::dump_uci_move(move) << std::endl;
}
}  // namespace uci

namespace thread {

// global pool variables
std::atomic<bool> stop_flag;
int MAX_SEARCH_DEPTH = 80;
std::vector<Thread *> threads;

// return the main thread, assuming set_num_threads() has been done.
MainThread* main_thread() {
    assert(threads.size() != 0);
    return ((MainThread*) threads[0]);
}

void set_num_threads(int n_threads) {
    assert(n_threads > 0);
    if (threads.size() != 0) {
        LOG(logERROR) << "Cannot call set_num_threads() more than once yet.";
    }

    // create main thread
    threads.push_back(new MainThread());

    for (int i = 1; i < n_threads; i++) {
        threads.push_back(new Thread());
    }
}

void set_position(const Position& pos) {
    for (auto pth : threads) {
        pth->set_position(pos);
    }
}

void start_search(SearchLimit limit) {
    if (main_thread()->is_searching()) {
        // TODO change to blocking wait
        std::cerr << "Already searching, so this command is ignored." << std::endl;
        return;
    }

    for (auto pth : threads) {
        pth->reset();
        pth->set_search_limit(limit);
    }

    main_thread()->start_search();
    // TODO need to tell other threads to start searching too. before or after main?
}

void stop_search() {
    stop_flag = true;
}

void cleanup() {
    for (auto pth : threads) {
        delete pth;
    }
}

Thread::Thread() : inner_thread(&Thread::thread_func, this), start_flag(false) {
}

void Thread::set_position(const Position& pos) {
    root_pos = pos;
}

void Thread::set_search_limit(SearchLimit limit) {
    this->limit = limit;
}

void Thread::start_search() {
    {
        std::lock_guard<std::mutex> lk(start_m);
        start_flag = true;
    }
    start_cv.notify_one();
}

bool Thread::is_searching() {
    std::lock_guard<std::mutex> lk(start_m);
    return start_flag;
}

bool Thread::am_main() {
    return this == main_thread();
}

void Thread::thread_func() {
    while (true) {
        {
            // wait for a start signal
            std::unique_lock<std::mutex> lk(start_m);
            start_cv.wait(lk, [this]{ return start_flag; });
        }

        // starting search
        search();

        {
            // reset start_flag
            std::lock_guard<std::mutex> lk(start_m);
            start_flag = false;
        }
    }
}

inline bool Thread::check_return() {
    // TODO add fixed time control, etc.
    if (time_alloc != 0 && timer.elapsed_millis() > time_alloc) {
        assert(state.best_move != NULL_MOVE);
        if (am_main()) {
            uci::bestmove(state.best_move);
        }
        return true;
    }
    return false;
}

// TODO OPTIMIZE return directly if there is a single legal move
void Thread::search() {

    time_alloc = 0;
    // set time limits
    if (limit.tc.wtime != 0) {
        int my_time = root_pos.get_side_to_move() == Color::BLACK ? limit.tc.btime : limit.tc.wtime;
        // allocate time; very basic and bad for now
        // at least 50ms and at most 3000ms
        time_alloc = my_time / std::abs(40 - root_pos.get_fullmove_number());
        time_alloc = std::max(time_alloc, 50.f);
        time_alloc = std::min(time_alloc, 3000.f);
    }

    timer.zero();
    std::vector<Move> moves;
    gen_legal_moves(root_pos, moves);

    // initialize state to garbage values, in case we don't get to search at all.
    state.best_eval = 0;
    state.best_move = moves[0];
    LOG(logDEBUG) << "Starting search";

    Move best_move = moves[0];
    // TODO impose depth limit, if there is one
    for (int depth = 4; depth < MAX_SEARCH_DEPTH; depth++) {
        Score alpha = SCORE_NEG_INFTY;

        // search best move first
        // TODO define macro
        root_pos.make_move(best_move);
        state.cur_depth = 0;
        state.cur_depth++;
        LOG(logDEBUG) << depth << ": starting depth search";
        Score val = -depth_search(SCORE_NEG_INFTY, -alpha, depth);
        LOG(logDEBUG) << depth << ": finishing depth search";
        state.cur_depth--;
        root_pos.unmake_move(best_move);

        if (check_return()) return;

        for (Move move : moves) {
            if (move == best_move) {
                // already searched best move
                continue;
            }

            if (stop_flag.load()) {
                return;
            }

            root_pos.make_move(move);
            state.cur_depth = 0;
            state.cur_depth++;
            LOG(logDEBUG) << depth << ": starting depth search";
            Score val = -depth_search(SCORE_NEG_INFTY, -alpha, depth);
            state.cur_depth--;
            LOG(logDEBUG) << depth << ": finishing depth search";
            root_pos.unmake_move(move);

            if (val > alpha) {
                alpha = val;
                best_move = move;
            }

            if (check_return()) return;
        }

        // TODO later, update within the moves loop. But need to implement pv first.
        state.best_eval = alpha;
        state.best_move = best_move;
    }
    LOG(logDEBUG) << "Finished search";
        // // reinsert best move as the first move in the vector, so that it is explored first in the
        // // next iteration
        // moves.erase(find(moves.begin(), moves.end(), best));
        // moves.insert(moves.begin(), best);
        // depth++;
}

Score Thread::depth_search(Score alpha, Score beta, int depth) {
    std::vector<Move> moves;
    bool checking = gen_legal_moves(root_pos, moves);

    if (root_pos.is_drawn_by_50()) {
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
		// this would never be true if depth == 0, i.e. never initialized
        if (state.cur_depth == depth) {
            return evaluate(root_pos);
        }

        for (Move move : moves) {
            root_pos.make_move(move);
            assert(root_pos.position_good());
            state.cur_depth++;
            Score s = -depth_search(-beta, -alpha, depth);
            state.cur_depth--;
            root_pos.unmake_move(move);
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

void Thread::reset() {
    state = {};
}

MainThread::MainThread() {
}
}  // namespace threading
