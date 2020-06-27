#include <vector>
#include <thread>
#include <memory>

//using ThrPtr = std::unique_ptr<std::thread>;

// class that performs parallel search in the background.
// currently only supports one thread to test the search/eval function
class ParallelSearch {
   public:
    //ParallelSearch(int num_threads);
    ParallelSearch();

    // run a task if a task is not already running
    // returns whether succesful
    bool run();

    // TODO add func to change num of threads
   private:
    //int num_threads;
    //std::vector<std::thread> threads;
    std::thread thread;
};
