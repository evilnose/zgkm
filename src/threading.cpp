#include "threading.h"
#include "movegen.h"

#include <iostream>  // TODO 


namespace threading {

std::atomic<bool> stop_search;
int MAX_SEARCH_DEPTH = 80;

ThreadPool::ThreadPool(int n_workers) {
    assert(n_workers > 0);

    // create main thread
    threads.push_back(new MainThread());

    for (int i = 1; i < n_workers; i++) {
        threads.push_back(new Thread());
    }
}

ThreadPool::~ThreadPool() {
    for (auto pth : threads) {
        delete pth;
    }
}

void ThreadPool::set_position(const Position& pos) {
    for (auto pth : threads) {
        pth->set_position(pos);
    }
}

void ThreadPool::start_search(SearchLimitType limit_type, SearchLimit limit) {
    if (main_thread()->is_searching()) {
        std::cerr << "Already searching. Aborted." << std::endl;
    }

    for (auto pth : threads) {
        pth->set_search_limit(limit_type, limit);
    }

    main_thread()->start_search();
    // TODO need to tell other threads to start searching too
}

MainThread* ThreadPool::main_thread() {
    return ((MainThread*) threads[0]);
}

Thread::Thread() : inner_thread(&Thread::thread_func, this), start_flag(false), stop(false) {
}

void Thread::set_position(const Position& pos) {
    root_pos = pos;
}

void Thread::set_search_limit(SearchLimitType limit_type, SearchLimit limit) {
    this->limit_type = limit_type;
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

void Thread::search() {
    std::cout << "hi" << std::endl;
    #if 0
    if (depth == 0)
    {
        return material_only_eval(position);
    }
    Score alpha = alpha;
    Score beta = beta;
    Move best = NULL_MOVE;
    std::vector<Move> moves;
    gen_legal_moves(pos, moves);
    for (Move move : moves)
    {
        pos.make_move(move);
        Score res = -search(pos, depth - 1, alpha, beta);
        pos.unmake_move(move);
        if (res > alpha)
        {
            alpha = res;
            best = move;
        }
    }
    assert(best != NULL_MOVE);
    #endif

        // // reinsert best move as the first move in the vector, so that it is explored first in the
        // // next iteration
        // moves.erase(find(moves.begin(), moves.end(), best));
        // moves.insert(moves.begin(), best);
        // depth++;
}


MainThread::MainThread() {
}
}
