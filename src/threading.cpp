#include "threading.h"
#include "movegen.h"
#include "notation.h"
#include "logger.h"

#include <iostream>
#include <cassert>
#include <algorithm>

namespace {

std::vector<Move> reconstruct_pv(const Position& position, const ht::Table& table) {
    Position pos = position;
    std::vector<Move> pv;
    while (true) {
        if (pos.is_drawn_by_50() || pos.is_drawn_by_threefold()) {
            break;
        }

        ZobristKey key = pos.get_hash();
        if (!table.contains(key)) {
            break;
        }

        ht::Entry entry = table.get(key);
        if (entry.bestmove == NULL_MOVE) {
            break;
        }
        std::vector<Move> legal_moves;
        gen_legal_moves(pos, legal_moves);
        if (find(legal_moves.begin(), legal_moves.end(), entry.bestmove) == legal_moves.end()) {
            break;
        }
        pv.push_back(entry.bestmove);
        pos.make_move(entry.bestmove);
    }

    return pv;
}

// pv move stands for both the hash table move and the last PV move in the depth 0 search function
std::vector<int> score_moves(const Position& pos, const std::vector<Move>& moves, Move pv_move) {
    // Most Valuable Victim, Least Valuable Attacker array, adapted from https://rustic-chess.org/search/ordering/mvv_lva.html
    // We need "Any" in here due to the unfortunate ordering of ANY_PIECE before NO_PIECE
    // It shouldn't be indexed in any case, just a padding.
    static int MVV_LVA[8][8] = {
        {15, 14, 13, 12, 11, 10, 0, 0}, // victim P, attacker P, N, B, R, Q, K, Any, None
        {25, 24, 23, 22, 21, 20, 0, 0}, // victim N, attacker P, N, B, R, Q, K, Any, None
        {35, 34, 33, 32, 31, 30, 0, 0}, // victim B, attacker P, N, B, R, Q, K, Any, None
        {45, 44, 43, 42, 41, 40, 0, 0}, // victim R, attacker P, N, B, R, Q, K, Any, None
        {55, 54, 53, 52, 51, 50, 0, 0}, // victim Q, attacker P, N, B, R, Q, K, Any, None
        {0, 0, 0, 0, 0, 0, 0, 0},       // victim K, attacker P, N, B, R, Q, K, Any, None
        {0, 0, 0, 0, 0, 0, 0, 0},       // victim Any, attacker P, N, B, R, Q, K, Any, None
        {0, 0, 0, 0, 0, 0, 0, 0},       // victim None, attacker P, N, B, R, Q, K, Any, None
    };

    std::vector<int> scores;
    for (Move move : moves) {
        if (move == pv_move) {
            scores.push_back(100);  // always start with the last best move/pv move
        }
        #if USE_MOVE_ORDERING
        Square src = get_move_source(move);
        Square tgt = get_move_target(move);
        SquareInfo src_i = pos.get_piece(src);
        SquareInfo tgt_i = pos.get_piece(tgt);
        scores.push_back(MVV_LVA[tgt_i.ptype][src_i.ptype]);
        #else
        scores.push_back(0);
        #endif
    }

    return scores;
}

Move pick_move(std::vector<Move>& moves, std::vector<int> scores, int s_index) {
    for (size_t i = s_index; i < moves.size(); i++) {
        if (scores[i] > scores[s_index]) {
            std::swap(scores[i], scores[s_index]);
            std::swap(moves[i], moves[s_index]);
        }
    }
    return moves[s_index];
}
}  // namespace

namespace uci {
void pv() {
    std::cerr << "uci::pv() not implemented.";
    assert(false);
}

void bestmove(Move move) {
    std::cout << "bestmove " << notation::dump_uci_move(move) << std::endl;
}

void info(const thread::SearchState& state, int depth, const utils::Timer& timer) {
    #if USE_PESTO
    static float multi = 1.f;
    #else
    static float multi = 100.f;
    #endif
    std::cout << "info score cp " << ((float) state.best_eval) * multi \
    	<< " depth " << depth \
        << " nodes " << state.nodes \
        << " tt_hits " << state.tt_hits \
        << " tt_collisions " << state.tt_collisions;

    if (state.pv.size() != 0){
        std::cout << " pv";
        for (Move mv : state.pv) {
            std::cout << " " << notation::dump_uci_move(mv);
        }

        std::cout << " time " << timer.elapsed_millis();
    }

    std::cout << std::endl;
}

}  // namespace uci

namespace thread {

// global pool variables
std::atomic<bool> stop_flag(false);
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

    stop_flag = false;

    for (auto pth : threads) {
        pth->reset();
        pth->set_search_limit(limit);
    }

    main_thread()->start_search();
    // TODO need to tell other threads to start searching too. before or after main?
}

void stop_search() {
    main_thread()->diagnostics();
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
    position = pos;
}

const Position& Thread::get_position() const {
    return position;
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
        int time_left = position.get_side_to_move() == Color::BLACK ? limit.tc.btime : limit.tc.wtime;

        int n_moves = std::min(position.get_fullmove_number(), 10);  // tune this number
        float factor = 2 - n_moves / 10.f;

        int moves_left = std::max(45 - n_moves, 5);
        float target = time_left / moves_left;
        time_alloc = target * factor;
    }

    timer.zero();

    state.best_eval = 0;
    state.best_move = NULL_MOVE;
    LOG(logDEBUG) << "Starting search";

    std::vector<Move> moves;
    gen_legal_moves(position, moves);

    // initialize state to garbage values, in case we don't get to search at all.

    // set to 4 if there is no depth limit; otherwise set to min(4, target_depth) to avoid having
    // a loop like (4..3), e.g. if target depth is 3
    int start_depth = limit.depth == 0 ? 4 : std::min(4, limit.depth);
    for (int depth = start_depth; ; depth++) {
        if (limit.depth != 0 && depth > limit.depth) {
            break;
        }

        // need to reorder scores since best_move might have changed
        std::vector<int> move_scores = score_moves(position, moves, state.best_move);

        Score alpha = SCORE_NEG_INFTY;

        // iterate over moves
        for (unsigned i = 0; i < moves.size(); i++) {
            Move move = pick_move(moves, move_scores, i);

            if (stop_flag) {
                // early stopping
                break;
            }

            position.make_move(move);
            state.nodes++;
            state.cur_depth = 0;
            state.max_depth_searched = 0;
            state.cur_depth++;
            state.max_depth_searched = std::max(state.cur_depth, state.max_depth_searched);

            if (ht::global_table().contains(position.get_hash())) {
                // state.tt_hits++;
            } else if (ht::global_table().has_collision(position.get_hash())) {
                state.tt_collisions++;
            }

            Score val = -depth_search(SCORE_NEG_INFTY, -alpha, depth);
            // LOG(logERROR) << "finished depth search";

            state.cur_depth--;
            position.unmake_move(move);

            if (stop_flag) {
                // early stop; can't use the value for this move
                break;
            }

            if (val > alpha) {
                alpha = val;

                state.best_eval = alpha * color_multiplier(position.get_side_to_move());
                state.best_move = move;
            }
        }

        if (state.best_move == NULL_MOVE) {
            // in the off chance that this somehow happens
            state.best_move = moves[0];
        }

        ht::global_table().put(ht::Entry{
            position.get_hash(),  // key
            (unsigned) depth,  // depth
            alpha,  // score
            state.best_move,  // best_move
            1,  // node type
        });

        auto pv = reconstruct_pv(position, ht::global_table());
        state.pv = pv;
        uci::info(state, depth, timer);

		if (stop_flag) break;
        if (check_tc_return()) break;
    }
    // reconstruct PV
    // uci::info(state);
    uci::bestmove(state.best_move);
    // std::cout << notation::to_aligned_fen(position) << std::endl;
        // // reinsert best move as the first move in the vector, so that it is explored first in the
        // // next iteration
        // moves.erase(find(moves.begin(), moves.end(), best));
        // moves.insert(moves.begin(), best);
        // depth++;
}

bool Thread::probe_tt(Score& alpha, Score& beta, int depth, const std::vector<Move>& moves, Move& pv_move, Score& out_eval) {
    ZobristKey hash_key = position.get_hash();
    ht::Entry entry = ht::global_table().get(hash_key);
    if (entry.key == hash_key) {
        // HACK the second condition tests if entry.depth == 0 && depth == 1000, since depth is
        // either << 1000 or == 1000, as in the case of qsearch. If entry and the caller of probe_tt
        // are both qsearch, then we still want to use this entry.
        if ((int) entry.depth >= depth - state.cur_depth || entry.depth + depth == 1000) {
            state.tt_hits++;

            // return directly or update alpha-beta bounds?
            if (entry.node_type == 1) {
                if (entry.score < 0) {
                    for (Move move : moves) {
                        position.make_move(move);
                        if (position.is_drawn_by_threefold()) {
                            position.unmake_move(move);
                            out_eval = 0;
                            return true;
                        }
                        position.unmake_move(move);
                    }
                }

                // exact
                out_eval = entry.score;
                return true;
            } else if (entry.node_type == 2) {
                // lower bound
                alpha = std::max(alpha, entry.score);
                if (alpha >= beta) {
                    out_eval = alpha;
                    return true;
                }
            } else {
                // upper bound
                beta = std::min(beta, entry.score);
            }
        }

        // update PV move?
        if (entry.bestmove != NULL_MOVE) {
            pv_move = entry.bestmove;
        }
    } else if (ht::global_table().has_collision(position.get_hash())) {
        state.tt_collisions++;
    }

    return false;
}

Score Thread::depth_search(Score alpha, Score beta, int depth) {
    if (state.nodes % 2048 == 2047) {
        if (check_return()) {
            stop_flag = true;
            return alpha;
        }
    }

    std::vector<Move> moves;
    bool checking = gen_legal_moves(position, moves);

    if (position.is_drawn_by_50()) {
        return SCORE_DRAW;
    }

    if (position.is_drawn_by_threefold()) {
        return SCORE_DRAW;
    }

    Move pv_move = NULL_MOVE;

    #if USE_TT
    Score tt_eval;
    // probe_tt tells us whether tt_eval is populated and we should return now
    if (probe_tt(alpha, beta, depth, moves, pv_move, tt_eval)) {
        return tt_eval;
    }
    #endif

    short node_type = 3;
    if (moves.size() == 0) {
        if (checking) {
            // I lose
            return SCORE_NEG_INFTY + position.get_halfmove_clock();
        } else {
            return SCORE_DRAW;
        }
    }
    // this would never be true if depth == 0, i.e. never initialized
    if (state.cur_depth == depth) {
        #if USE_QSEARCH
        return qsearch(alpha, beta);
        #else
        return evaluate(position);
        #endif
    }

    std::vector<int> move_scores = score_moves(position, moves, pv_move);

    Move best_move = NULL_MOVE;
    for (size_t i = 0; i < moves.size(); i++) {

        Move move = pick_move(moves, move_scores, i);

        position.make_move(move);
        assert(position.position_good());
        state.nodes++;
        state.cur_depth++;
        state.max_depth_searched = std::max(state.cur_depth, state.max_depth_searched);
        Score s = -depth_search(-beta, -alpha, depth);
        state.cur_depth--;
        position.unmake_move(move);
        if (stop_flag) {
            return alpha;  // TODO break?
        }
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
        position.get_hash(),  // key
        (unsigned int) (depth - state.cur_depth),  // depth
        alpha,  // score
        best_move,  // best_move
        node_type,  // node type
    });

    return alpha;
}

Score Thread::qsearch(Score alpha, Score beta) {
    if (state.nodes % 2048 == 2047) {
        if (check_return()) {
            stop_flag = true;
            return alpha;
        }
    }

    std::vector<Move> moves;
    bool checking = gen_legal_moves(position, moves);

    if (position.is_drawn_by_50()) {
        return SCORE_DRAW;
    }

    if (position.is_drawn_by_threefold()) {
        return SCORE_DRAW;
    }

    short node_type = 3;
    if (moves.size() == 0) {
        if (checking) {
            // I lose
            return SCORE_NEG_INFTY + position.get_halfmove_clock();
        } else {
            return SCORE_DRAW;
        }
    }

    // standing pat
    Score eval = evaluate(position);
    if (eval >= beta) {
        return beta;
    }
    if (eval > alpha) {
        return eval;
    }

    // obtain capture moves
    std::vector<Move> capture_moves;
    gen_legal_moves(position, moves);
    for (Move mv : moves) {
        if (has_piece(position.get_piece(get_move_target(mv)))) {
            capture_moves.push_back(mv);
        }
    }

	Move pv_move = NULL_MOVE;
    #if USE_TT
    Score tt_eval;
    // probe_tt tells us whether tt_eval is populated and we should return now
    // give a large value for depth so that we don't return early (see probe_tt for the condition check)
    if (probe_tt(alpha, beta, 1000, capture_moves, pv_move, tt_eval)) {
        return tt_eval;
    }
    #endif

    std::vector<int> move_scores = score_moves(position, capture_moves, pv_move);

    Move best_move = NULL_MOVE;
    for (size_t i = 0; i < capture_moves.size(); i++) {

        Move move = pick_move(capture_moves, move_scores, i);

        position.make_move(move);
        assert(position.position_good());
        state.nodes++;
        state.cur_depth++;
        state.max_depth_searched = std::max(state.cur_depth, state.max_depth_searched);
        Score s = -qsearch(-beta, -alpha);
        state.cur_depth--;
        position.unmake_move(move);

        if (stop_flag) {
            return alpha;
        }
        if (s >= beta) {
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
        position.get_hash(),  // key
        (unsigned int) 0,  // special depth for qsearch
        alpha,  // score
        best_move,  // best_move
        node_type,  // node type
    });

    return alpha;
}

void Thread::reset() {
    state = {};
}

MainThread::MainThread() {
}
}  // namespace threading
