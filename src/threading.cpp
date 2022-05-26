#include "threading.h"
#include "movegen.h"
#include "notation.h"
#include "logger.h"

#include <iostream>
#include <cassert>
#include <algorithm>

std::vector<Move> reconstruct_pv(const Position& root_pos, const ht::Table& table) {
    Position pos = root_pos;
    std::vector<Move> pv;
    while (true) {
        ZobristKey key = pos.get_hash();
        if (!table.contains(key)) {
            break;
        }

        ht::Entry entry = table.get(key);
        if (entry.bestmove == NULL_MOVE) {
            break;
        }
        pv.push_back(entry.bestmove);
        pos.make_move(entry.bestmove);
    }

    return pv;
}

namespace uci {
void pv() {
    std::cerr << "uci::pv() not implemented.";
    assert(false);
}

void bestmove(Move move) {
    std::cout << "bestmove " << notation::dump_uci_move(move) << std::endl;
}

void info(const thread::SearchState& state) {
    std::cout << "info score cp " << state.best_eval << " nodes " << state.nodes \
        << " tt_hits " << state.tt_hits \
        << " tt_collisions " << state.tt_collisions;

    if (state.pv.size() != 0){
        std::cout << " pv";
        for (Move mv : state.pv) {
            std::cout << " " << notation::dump_uci_move(mv);
        }
    }

    std::cout << std::endl;
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

const Position& get_position() {
    return main_thread()->get_position();
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

const Position& Thread::get_position() const {
    return root_pos;
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
    // multiply by 0.5 as a heuristic to estimate how much time the next iteration will take
    bool stop = time_alloc != 0 && timer.elapsed_millis() > time_alloc;
    stop |= limit.fixed_time != 0 && timer.elapsed_millis() > limit.fixed_time;
    if (stop) {
        assert(state.best_move != NULL_MOVE);
        // if (am_main()) {
        //     uci::info(state);
        //     uci::bestmove(state.best_move);
        // }
        return true;
    }
    return false;
}

// check return based on a time control, if there is one
inline bool Thread::check_tc_return() {
    // TODO add fixed time control, etc.
    // multiply by 0.5 as a heuristic to estimate how much time the next iteration will take
    bool stop = time_alloc != 0 && timer.elapsed_millis() > 0.6 * time_alloc;
    stop |= limit.fixed_time != 0 && timer.elapsed_millis() > 0.6 * limit.fixed_time;
    if (stop) {
        assert(state.best_move != NULL_MOVE);
        // if (am_main()) {
        //     uci::info(state);
        //     uci::bestmove(state.best_move);
        // }
        return true;
    }
    return false;
}

// TODO OPTIMIZE return directly if there is a single legal move
void Thread::search() {
    time_alloc = 0;
    // set time limits
    if (limit.tc.wtime != 0) {
        // algorithm adapted from Cray Blitz by Robert Hyatt
        // documented https://www.chessprogramming.org/Time_Management#Extra_Time
        int time_left = root_pos.get_side_to_move() == Color::BLACK ? limit.tc.btime : limit.tc.wtime;

        int n_moves = std::min(root_pos.get_fullmove_number(), 10);  // tune this number
        float factor = 2 - n_moves / 10.f;

        int moves_left = std::max(45 - n_moves, 5);
        float target = time_left / moves_left;
        time_alloc = target * factor;
    }

    timer.zero();
    std::vector<Move> moves;
    gen_legal_moves(root_pos, moves);

    // initialize state to garbage values, in case we don't get to search at all.
    state.best_eval = 0;
    state.best_move = moves[0];
    LOG(logDEBUG) << "Starting search";

    Move best_move = moves[0];
    int best_move_idx = 0;
    // set to 4 if there is no depth limit; otherwise set to min(4, target_depth) to avoid having
    // a loop like (4..3), e.g. if target depth is 3
    int start_depth = limit.depth == 0 ? 4 : std::min(4, limit.depth);
    for (int depth = start_depth; ; depth++) {
        if (limit.depth != 0 && depth > limit.depth) {
            break;
        }

        Score alpha = SCORE_NEG_INFTY;

        // search best move first
        std::swap(moves[0], moves[best_move_idx]);
        best_move_idx = 0;

        // iterate over moves
        for (unsigned i = 0; i < moves.size(); i++) {
            Move move = moves[i];

            if (stop_flag.load()) {
                // early stopping
                return;
            }

            root_pos.make_move(move);
            state.nodes++;
            state.cur_depth = 0;
            state.cur_depth++;

            if (ht::global_table().contains(root_pos.get_hash())) {
                state.tt_hits++;
            } else if (ht::global_table().has_collision(root_pos.get_hash())) {
                state.tt_collisions++;
            }

            Score val = -depth_search(SCORE_NEG_INFTY, -alpha, depth);

            state.cur_depth--;
            root_pos.unmake_move(move);

            if (val > alpha) {
                alpha = val;
                best_move = move;
                best_move_idx = i;
            }

            if (check_return()) break;
        }

        ht::global_table().put(ht::Entry{
            root_pos.get_hash(),  // key
            (unsigned) depth,  // depth
            alpha,  // score
            best_move,  // best_move
            1,  // node type
        });

        // TODO later, update within the moves loop. But need to implement pv first.
        state.best_eval = alpha;
        state.best_move = best_move;

        if (check_tc_return()) break;
    }
    LOG(logDEBUG) << "Finished search";
    // reconstruct PV
    auto pv = reconstruct_pv(root_pos, ht::global_table());
    state.pv = pv;
    uci::info(state);
    uci::bestmove(state.best_move);
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

    if (root_pos.is_drawn_by_threefold()) {
        return SCORE_DRAW;
    }

    short node_type = 3;
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

        Move best_move = NULL_MOVE;
        for (Move move : moves) {
            root_pos.make_move(move);
            assert(root_pos.position_good());
            state.nodes++;
            state.cur_depth++;
            Score s = -depth_search(-beta, -alpha, depth);
            state.cur_depth--;
            root_pos.unmake_move(move);
            if (s >= beta) {
                // opponent's upper bound broken. So curr pos will not be
                // allowed (fail-high)
                alpha = beta;
                node_type = 2;
                best_move = move;
                break;
            }
            if (s > alpha) {
                // update my guarantee
                alpha = s;
                node_type = 1;
                best_move = move;
            }
        }

        ht::global_table().put(ht::Entry{
            root_pos.get_hash(),  // key
            (unsigned int) (depth - state.cur_depth),  // depth
            alpha,  // score
            best_move,  // best_move
            node_type,  // node type
        });
    }

    return alpha;
}

void Thread::reset() {
    state = {};
}

MainThread::MainThread() {
}
}  // namespace threading
