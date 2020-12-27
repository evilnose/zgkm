// Main thread -- an idea taken from Stockfish but is indeed simple. All threads
// perform search.. tasks, but only one thread is needed for a couple of
// administrative things such as UCI output,.. time management, etc. Hence the
// main thread does that as well as search.
#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

#include "position.h"
#include "search.h"

namespace threading {

// One thread represents one search task with one root node.
class Thread {
   public:
    Thread();
    virtual ~Thread() {}

    void set_position(const Position& pos);
    void set_search_limit(SearchLimitType limit_type, SearchLimit limit);
    virtual void start_search();
    bool is_searching();

   private:

    void search();
    void thread_func();

    std::thread inner_thread;
    std::condition_variable start_cv;
    std::mutex start_m;
    bool start_flag;
    bool stop;

    Position root_pos;
    SearchLimitType limit_type;
    SearchLimit limit;
};  // class Thread

// A single master thread is launched for each program. Normally it does
// nothing, but when a search command is issued it launches one or more worker
// threads to do the search.
class MainThread : public Thread {
   public:
    MainThread();
    virtual ~MainThread() {}

   private:
};  // class MainThread

class ThreadPool {
   public:
    ThreadPool(int n_threads);
    ~ThreadPool();

    void set_position(const Position& position);

    // start searching
    void start_search(SearchLimitType limit_type, SearchLimit limit);

   private:
    // list of threads. The one at 0th index is the main thread.
    std::vector<Thread *> threads;

    MainThread* main_thread();
};
}  // namespace threading
