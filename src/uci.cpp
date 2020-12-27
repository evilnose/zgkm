#include "uci.h"
#include "movegen.h"
#include "utils.h"
#include "search.h"
#include "threading.h"
#include "logger.h"

#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <atomic>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::istringstream;
using std::string;
using threading::ThreadPool;

static ThreadPool* pool;

void run_position(istringstream &iss)
{
    string fen;
    iss >> fen;
    if (fen == "startpos")
    {
        fen = STARTING_FEN;
    }
    else
    {
        assert(fen == "fen");
        std::getline(iss, fen);
        utils::trim(fen);
    }
    istringstream fen_ss(fen);
    Position pos(fen_ss);
    pool->set_position(pos);
}

void run_go_perft(int depth)
{
    cerr << "Not implemented" << endl;
    //int result = perft(position, depth);
    //cout << result << endl;
}

std::unordered_map<string, string> parse_keyvalue(istringstream &iss)
{
    std::unordered_map<string, string> ret;
    auto it = std::istream_iterator<string>(iss);

    while (it != std::istream_iterator<string>()) {
        ret[*it] = *(++it);
        it++;
    }

    return ret;
}

void delegate_command(const string &command, istringstream &liness)
{
}

void uci::initialize(int argc, char *argv[])
{
    bboard::initialize();
    pool = new ThreadPool(1);
}

void uci::loop()
{
    string line;
    istringstream liness;
    string command;
    while (cin)
    {
        getline(cin, line);
        liness = istringstream{line};
        if (liness >> command)
        {
            if (command == "uci")
            {
                cout << "id name ZGKM 0.0\n";
                cout << "id author Gary Geng" << endl;
                cout << "uciok" << endl;
            }
            else if (command == "debug")
            {
                cout << "not implemented" << endl;
            }
            else if (command == "isready")
            {
                cout << "readyok" << endl;
            }
            else if (command == "setoption")
            {
                cout << "not implemented" << endl;
            }
            else if (command == "ucinewgame")
            {
                cout << "not implemented" << endl;
            }
            else if (command == "position")
            {
                run_position(liness);
            }
            else if (command == "go")
            {
                std::unordered_map<string, string> args = parse_keyvalue(liness);

                std::vector<string> constraints{"wtime", "depth", "nodes", "mate", "movetime", "infinite"};
                bool constraint_found = false;
                std::string constraint;
                std::string value;
                for (const string& cons : constraints) {
                    auto it = args.find(cons);
                    if (it != args.end()) {
                        // assert that the number of constraints is exactly one
                        constraint_found = true;
                        constraint = it->first;
                        value = it->second;
                        break;
                    }
                }
                if (!constraint_found) {
                    std::cerr << "search constraint not found!";
                    continue;
                }

                SearchLimitType slimit_type;
                SearchLimit slimit;
                if (constraint == "wtime") {
                    int wtime = std::stoi(value);
                    int btime = std::stoi(args["btime"]);
                    int winc = std::stoi(utils::get_or_default(args, string("winc"), string("0")));
                    int binc = std::stoi(utils::get_or_default(args, string("binc"), string("0")));
                    slimit_type = TIME_CONTROL;
                    slimit.time_control = TimeControlParams{wtime, btime, winc, binc};
                    // TODO search in a new thread
                    pool->start_search(slimit_type, slimit);

                } else if (constraint == "infinite") {
                    LOG(logWARNING) << "infinite constraint not actually implemented." << endl;
                    pool->start_search(slimit_type, slimit);
                } else {
                    LOG(logERROR) << "Constraint not implemented: " << constraint << endl;
                }
            }
        }
    }
}

void uci::cleanup() {
    delete pool;
}
