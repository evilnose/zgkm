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
#include "utils.h"
#include "hash.h"

namespace thread {

struct SearchState {
   unsigned long nodes;
   Move best_move;
   Score best_eval;
   std::vector<Move> pv;
   int cur_depth;
   int max_depth_searched;
   int tt_hits;  // transposition table hits
   int tt_collisions;
};

// One thread represents one search task with one root node.
class Thread {
   public:
    Thread();
    virtual ~Thread() {}

    // set the root position
    void set_position(const Position& pos);
    // set the root position
    const Position& get_position() const;
    // set the limit of search
    void set_search_limit(SearchLimit limit);
    // reset temporary states such as SearchState
    void reset();
    // start searching; does not check if a searc is already in place.
    virtual void start_search();
    // whether a search is already in place
    bool is_searching();
    void diagnostics() {
      //  std::cout << "Printing Diagnostics" << std::endl;
      //  std::cout << state.cur_depth << std::endl;
    }

   private:

	 // Am I the main thread?
    bool am_main();

    void thread_func();

    // helper search function using members such as SearchLimit.
    void search();

    bool probe_tt(Score& alpha, Score& beta, int depth, const std::vector<Move>& moves, Move& pv_move, Score &out_eval);

    // search for a fixed number of plies from position, based on cur_depth and 
    Score depth_search(Score alpha, Score beta, int depth);

    // quiescence search
    Score qsearch(Score alpha, Score beta);

    bool check_return();

    bool check_tc_return();


    std::thread inner_thread;
    std::condition_variable start_cv;
    std::mutex start_m;
    bool start_flag;

    Position position;
    SearchLimit limit;
    SearchState state;
    // zeroed at the start of search
    utils::Timer timer;

    float time_alloc;
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

void set_num_threads(int n_threads);
void start_search(SearchLimit limit);
void stop_search();
void set_position(const Position& pos);
const Position& get_position();
void cleanup();

}  // namespace thread
